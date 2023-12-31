/* linux/drivers/iommu/exynos_iommu.c
 *
 * Copyright (c) 2011-2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef CONFIG_EXYNOS_IOMMU_DEBUG
#define DEBUG
#endif

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/memblock.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/string.h>

#include <plat/cpu.h>

#include <asm/cacheflush.h>

#include "exynos-iommu.h"

#define MODULE_NAME "exynos-sysmmu"

#define SECT_MASK (~(SECT_SIZE - 1))
#define LPAGE_MASK (~(LPAGE_SIZE - 1))
#define SPAGE_MASK (~(SPAGE_SIZE - 1))

#define lv1ent_fault_link(sent) (*(sent) == ZERO_LV2LINK)
#define lv1ent_fault(sent) ((*(sent) == ZERO_LV2LINK) ||     \
		((*(sent) & 3) == 0) || ((*(sent) & 3) == 3))
#define lv1ent_page(sent) ((*(sent) != ZERO_LV2LINK) && ((*(sent) & 3) == 1))
#define lv1ent_section(sent) ((*(sent) & 3) == 2)

#define lv2ent_fault(pent) ((*(pent) & 3) == 0 || (*(pent) & SPAGE_MASK) == fault_page)
#define lv2ent_small(pent) ((*(pent) & 2) == 2)
#define lv2ent_large(pent) ((*(pent) & 3) == 1)

#define section_phys(sent) (*(sent) & SECT_MASK)
#define section_offs(iova) ((iova) & 0xFFFFF)
#define lpage_phys(pent) (*(pent) & LPAGE_MASK)
#define lpage_offs(iova) ((iova) & 0xFFFF)
#define spage_phys(pent) (*(pent) & SPAGE_MASK)
#define spage_offs(iova) ((iova) & 0xFFF)

#define lv1ent_offset(iova) ((iova) >> SECT_ORDER)
#define lv2ent_offset(iova) (((iova) & 0xFF000) >> SPAGE_ORDER)

#define NUM_LV1ENTRIES 4096
#define NUM_LV2ENTRIES 256

#define LV2TABLE_SIZE (NUM_LV2ENTRIES * sizeof(long))

#define SPAGES_PER_LPAGE (LPAGE_SIZE / SPAGE_SIZE)

#define lv2table_base(sent) (*(sent) & 0xFFFFFC00)

#define mk_lv1ent_sect(pa) ((pa) | 2)
#define mk_lv1ent_page(pa) ((pa) | 1)
#define mk_lv2ent_lpage(pa) ((pa) | 1)
#define mk_lv2ent_spage(pa) ((pa) | 2)

#define CTRL_ENABLE	0x5
#define CTRL_BLOCK	0x7
#define CTRL_DISABLE	0x0

#define CFG_LRU		0x1
#define CFG_QOS(n)	((n & 0xF) << 7)
#define CFG_MASK	0x0150FFFF /* Selecting bit 0-15, 20, 22 and 24 */
#define CFG_ACGEN	(1 << 24) /* System MMU 3.3 only */
#define CFG_SYSSEL	(1 << 22) /* System MMU 3.2 only */
#define CFG_FLPDCACHE	(1 << 20) /* System MMU 3.2+ only */
#define CFG_SHAREABLE	(1 << 12) /* System MMU 3.x only */
#define CFG_QOS_OVRRIDE (1 << 11) /* System MMU 3.3 only */

#define REG_MMU_CTRL		0x000
#define REG_MMU_CFG		0x004
#define REG_MMU_STATUS		0x008
#define REG_MMU_FLUSH		0x00C
#define REG_MMU_FLUSH_ENTRY	0x010
#define REG_PT_BASE_ADDR	0x014
#define REG_INT_STATUS		0x018
#define REG_INT_CLEAR		0x01C
/* System MMU 3.3 only */
#define REG_PB_INFO		0x400
#define REG_PB_LMM		0x404
#define REG_PB_INDICATE		0x408
#define REG_PB_CFG		0x40C
#define REG_PB_START_VA		0x410
#define REG_PB_END_VA		0x414

/* Debugging information */
#define REG_L1TLB_READ_VPN	0x38
#define REG_L1TLB_DATA_VPN	0x3C
/* System MMU 2.0 and higher only */
#define REG_L1TLB_READ_IDX	0x40
#define REG_L1TLB_DATA0_IDX	0x44
#define REG_L1TLB_DATA1_IDX	0x48
/* System MMU 2.x only */
#define REG_L2TLB_READ		0x4C
#define REG_L2TLB_DATA0		0x50
#define REG_L2TLB_DATA1		0x54
/* Syste MMU 3.0 ~ 3.2 only */
#define REG_PB_SADDR(min, idx)	(((min == 2) ? 0x70 : 0x4C) + idx * 8)
#define REG_PB_EADDR(min, idx)	(((min == 2) ? 0x74 : 0x50) + idx * 8)
#define REG_PB_BASE_VA(min, idx) (((min == 2) ? 0x88 : 0x5C) + idx * 4)
#define REG_PB_READ(min, idx)	(((min == 2) ? 0x98 : 0x68) + idx * 8)
#define REG_PB_DATA(min, idx)	(((min == 2) ? 0x9C : 0x70) + idx * 8)
/* System MMU 3.3 only */
#define REG_SPB_BASE_VPN	0x418

#define REG_PAGE_FAULT_ADDR	0x024
#define REG_AW_FAULT_ADDR	0x028
#define REG_AR_FAULT_ADDR	0x02C
#define REG_DEFAULT_SLAVE_ADDR	0x030

#define REG_MMU_VERSION		0x034

#define MMU_MAJ_VER(reg)	(reg >> 28)
#define MMU_MIN_VER(reg)	((reg >> 21) & 0x7F)

#define MAX_NUM_PBUF		6

#define NUM_MINOR_OF_SYSMMU_V3	4

static void *sysmmu_placeholder; /* Inidcate if a device is System MMU */

#define is_sysmmu(sysmmu) (sysmmu->archdata.iommu == &sysmmu_placeholder)
#define has_sysmmu(dev)							\
	(dev->parent && dev->archdata.iommu && is_sysmmu(dev->parent))
#define for_each_sysmmu(dev, sysmmu)					\
	for (sysmmu = dev->parent; sysmmu && is_sysmmu(sysmmu);		\
			sysmmu = sysmmu->parent)
#define for_each_sysmmu_until(dev, sysmmu, until)			\
	for (sysmmu = dev->parent; sysmmu != until; sysmmu = sysmmu->parent)

static struct kmem_cache *lv2table_kmem_cache;
static unsigned long fault_page;
static unsigned long *zero_lv2_table;
#define ZERO_LV2LINK mk_lv1ent_page(__pa(zero_lv2_table))

static int recover_fault_handler (struct iommu_domain *domain,
				struct device *dev, unsigned long fault_addr,
				int itype);

static unsigned long *section_entry(unsigned long *pgtable, unsigned long iova)
{
	return pgtable + lv1ent_offset(iova);
}

static unsigned long *page_entry(unsigned long *sent, unsigned long iova)
{
	return (unsigned long *)__va(lv2table_base(sent)) + lv2ent_offset(iova);
}

static unsigned short fault_reg_offset[SYSMMU_FAULTS_NUM] = {
	REG_PAGE_FAULT_ADDR,
	REG_AR_FAULT_ADDR,
	REG_AW_FAULT_ADDR,
	REG_PAGE_FAULT_ADDR,
	REG_AR_FAULT_ADDR,
	REG_AR_FAULT_ADDR,
	REG_AW_FAULT_ADDR,
	REG_AW_FAULT_ADDR
};

static char *sysmmu_fault_name[SYSMMU_FAULTS_NUM] = {
	"PAGE FAULT",
	"AR MULTI-HIT FAULT",
	"AW MULTI-HIT FAULT",
	"BUS ERROR",
	"AR SECURITY PROTECTION FAULT",
	"AR ACCESS PROTECTION FAULT",
	"AW SECURITY PROTECTION FAULT",
	"AW ACCESS PROTECTION FAULT",
	"UNKNOWN FAULT"
};

/*
 * Metadata attached to each System MMU devices.
 */
struct exynos_iommu_data {
	struct exynos_iommu_owner *owner;
};

struct sysmmu_drvdata {
	struct device *sysmmu;	/* System MMU's device descriptor */
	struct device *master;	/* Client device that needs System MMU */
	char *dbgname;
	int nsfrs;
	void __iomem **sfrbases;
	struct clk *clk;
	int activations;
	struct iommu_domain *domain; /* domain given to iommu_attach_device() */
	sysmmu_fault_handler_t fault_handler;
	unsigned long pgtable;
	struct sysmmu_version ver; /* mach/sysmmu.h */
	short qos;
	spinlock_t lock;
	struct sysmmu_prefbuf pbufs[MAX_NUM_PBUF];
	int num_pbufs;
	struct dentry *debugfs_root;
	bool runtime_active;
	enum sysmmu_property prop; /* mach/sysmmu.h */
	bool tlbinv_entry;
};

struct exynos_iommu_domain {
	struct list_head clients; /* list of sysmmu_drvdata.node */
	unsigned long *pgtable; /* lv1 page table, 16KB */
	short *lv2entcnt; /* free lv2 entry counter for each section */
	spinlock_t lock; /* lock for this structure */
	spinlock_t pgtablelock; /* lock for modifying page table @ pgtable */
};

static inline void pgtable_flush(void *vastart, void *vaend)
{
	dmac_flush_range(vastart, vaend);
	outer_flush_range(virt_to_phys(vastart),
				virt_to_phys(vaend));
}

static bool set_sysmmu_active(struct sysmmu_drvdata *data)
{
	/* return true if the System MMU was not active previously
	   and it needs to be initialized */
	return ++data->activations == 1;
}

static bool set_sysmmu_inactive(struct sysmmu_drvdata *data)
{
	/* return true if the System MMU is needed to be disabled */
	BUG_ON(data->activations < 1);
	return --data->activations == 0;
}

static bool is_sysmmu_active(struct sysmmu_drvdata *data)
{
	return data->activations > 0;
}

static unsigned int __sysmmu_version(struct sysmmu_drvdata *drvdata,
					int idx, unsigned int *minor)
{
	unsigned int major;

	major = readl(drvdata->sfrbases[idx] + REG_MMU_VERSION);

	if ((MMU_MAJ_VER(major) == 0) || (MMU_MAJ_VER(major) > 3)) {
		/* register MMU_VERSION is used for special purpose */
		if (drvdata->ver.major == 0) {
			/* min ver. is not important for System MMU 1 and 2 */
			major = 1;
		} else {
			if (minor)
				*minor = drvdata->ver.minor;
			major = drvdata->ver.major;
		}

		return major;
	}

	if (minor)
		*minor = MMU_MIN_VER(major);

	major = MMU_MAJ_VER(major);

	return major;
}

static bool has_sysmmu_capable_pbuf(struct sysmmu_drvdata *drvdata,
			int idx, struct sysmmu_prefbuf pbuf[], int *min)
{
	if (__sysmmu_version(drvdata, idx, min) != 3)
		return false;

	if (*min == 3)
		return false;

	if ((pbuf[0].config & SYSMMU_PBUFCFG_WRITE) &&
		(drvdata->prop & SYSMMU_PROP_WRITE))
		return true;

	if (!(pbuf[0].config & SYSMMU_PBUFCFG_WRITE) &&
		(drvdata->prop & SYSMMU_PROP_READ))
		return true;

	return false;
}

static void sysmmu_unblock(void __iomem *sfrbase)
{
	__raw_writel(CTRL_ENABLE, sfrbase + REG_MMU_CTRL);
}

static bool sysmmu_block(void __iomem *sfrbase)
{
	int i = 120;

	__raw_writel(CTRL_BLOCK, sfrbase + REG_MMU_CTRL);
	while ((i > 0) && !(__raw_readl(sfrbase + REG_MMU_STATUS) & 1))
		--i;

	if (!(__raw_readl(sfrbase + REG_MMU_STATUS) & 1)) {
		sysmmu_unblock(sfrbase);
		return false;
	}

	return true;
}

static void __sysmmu_tlb_invalidate(void __iomem *sfrbase)
{
	__raw_writel(0x1, sfrbase + REG_MMU_FLUSH);
}

static void __sysmmu_tlb_invalidate_entry(void __iomem *sfrbase,
					  dma_addr_t iova)
{
	__raw_writel(iova | 0x1, sfrbase + REG_MMU_FLUSH_ENTRY);
}

static void __sysmmu_set_ptbase(void __iomem *sfrbase,
				       unsigned long pgd)
{
	__raw_writel(pgd, sfrbase + REG_PT_BASE_ADDR);

	__sysmmu_tlb_invalidate(sfrbase);
}

static void __sysmmu_set_prefbuf(void __iomem *pbufbase, unsigned long base,
					unsigned long size, int idx)
{
	__raw_writel(base, pbufbase + idx * 8);
	__raw_writel(size - 1 + base,  pbufbase + 4 + idx * 8);
}

/*
 * Offset of prefetch buffer setting registers are different
 * between SysMMU 3.1 and 3.2. 3.3 has a single prefetch buffer setting.
 */
static unsigned short
	pbuf_offset[NUM_MINOR_OF_SYSMMU_V3] = {0x04C, 0x04C, 0x070, 0x410};

#define SYSMMU_PBUFCFG_DEFAULT (		\
		SYSMMU_PBUFCFG_TLB_UPDATE |	\
		SYSMMU_PBUFCFG_ASCENDING |	\
		SYSMMU_PBUFCFG_PREFETCH		\
	)

static int __prepare_prefetch_buffers(struct sysmmu_drvdata *drvdata, int idx,
				struct sysmmu_prefbuf prefbuf[], int num_pb)
{
	int ret_num_pb = 0;
	int i;
	struct exynos_iovmm *vmm;

	if (!drvdata->master || !drvdata->master->archdata.iommu) {
		dev_err(drvdata->sysmmu, "%s: No master device is specified\n",
					__func__);
		return 0;
	}

	vmm = ((struct exynos_iommu_owner *)
			(drvdata->master->archdata.iommu))->vmm_data;
	if (!vmm || (drvdata->num_pbufs > 0)) {
		if (drvdata->num_pbufs > num_pb)
			drvdata->num_pbufs = num_pb;

		memcpy(prefbuf, drvdata->pbufs,
				drvdata->num_pbufs * sizeof(prefbuf[0]));

		return drvdata->num_pbufs;
	}

	if (drvdata->prop & SYSMMU_PROP_READ) {
		ret_num_pb = min(vmm->inplanes, num_pb);
		for (i = 0; i < ret_num_pb; i++) {
			prefbuf[i].base = vmm->iova_start[i];
			prefbuf[i].size = vmm->iovm_size[i];
			prefbuf[i].config =
				SYSMMU_PBUFCFG_DEFAULT | SYSMMU_PBUFCFG_READ;
		}
	}

	if ((drvdata->prop & SYSMMU_PROP_WRITE) &&
				(ret_num_pb < num_pb) && (vmm->onplanes > 0)) {
		for (i = 0; i < min(num_pb - ret_num_pb, vmm->onplanes); i++) {
			prefbuf[ret_num_pb + i].base =
					vmm->iova_start[vmm->inplanes + i];
			prefbuf[ret_num_pb + i].size =
					vmm->iovm_size[vmm->inplanes + i];
			prefbuf[ret_num_pb + i].config =
				SYSMMU_PBUFCFG_DEFAULT | SYSMMU_PBUFCFG_WRITE;
		}

		ret_num_pb += i;
	}

	if (drvdata->prop & SYSMMU_PROP_WINDOW_MASK) {
		unsigned long prop = (drvdata->prop & SYSMMU_PROP_WINDOW_MASK)
						>> SYSMMU_PROP_WINDOW_SHIFT;
		BUG_ON(ret_num_pb != 0);
		for (i = 0; (i < (vmm->inplanes + vmm->onplanes)) &&
						(ret_num_pb < num_pb); i++) {
			if (prop & 1) {
				prefbuf[ret_num_pb].base = vmm->iova_start[i];
				prefbuf[ret_num_pb].size = vmm->iovm_size[i];
				prefbuf[ret_num_pb].config =
					SYSMMU_PBUFCFG_DEFAULT |
					SYSMMU_PBUFCFG_READ;
				ret_num_pb++;
			}
			prop >>= 1;
			if (prop == 0)
				break;
		}
	}

	return ret_num_pb;
}

static void __exynos_sysmmu_set_pbuf_ver31(struct sysmmu_drvdata *drvdata,
					   int idx)
{
	unsigned long cfg =
		__raw_readl(drvdata->sfrbases[idx] + REG_MMU_CFG) & CFG_MASK;
	int num_bufs;
	struct sysmmu_prefbuf prefbuf[2];

	num_bufs = __prepare_prefetch_buffers(drvdata, idx, prefbuf, 2);
	if (num_bufs == 0)
		return;

	BUG_ON(num_bufs > 2);

	if (num_bufs == 2) {
		/* Separate PB mode */
		cfg |= 2 << 28;

		if (prefbuf[1].size == 0)
			prefbuf[1].size = 1;
		__sysmmu_set_prefbuf(drvdata->sfrbases[idx] + pbuf_offset[1],
					prefbuf[1].base, prefbuf[1].size, 1);
	} else {
		/* Combined PB mode */
		cfg |= 3 << 28;
		drvdata->num_pbufs = 1;
		drvdata->pbufs[0] = prefbuf[0];
	}

	__raw_writel(cfg, drvdata->sfrbases[idx] + REG_MMU_CFG);

	if (prefbuf[0].size == 0)
		prefbuf[0].size = 1;
	__sysmmu_set_prefbuf(drvdata->sfrbases[idx] + pbuf_offset[1],
				prefbuf[0].base, prefbuf[0].size, 0);
}

static void __exynos_sysmmu_set_pbuf_ver32(struct sysmmu_drvdata *drvdata,
					   int idx)
{
	int i;
	unsigned long cfg =
		__raw_readl(drvdata->sfrbases[idx] + REG_MMU_CFG) & CFG_MASK;
	int num_bufs;
	struct sysmmu_prefbuf prefbuf[3];

	num_bufs = __prepare_prefetch_buffers(drvdata, idx, prefbuf, 3);
	if (num_bufs == 0)
		return;

	cfg |= 7 << 16; /* enabling PB0 ~ PB2 */

	switch (num_bufs) {
	case 1:
		/* Combined PB mode (0 ~ 2) */
		cfg |= 1 << 19;
		break;
	case 2:
		/* Combined PB mode (0 ~ 1) */
		cfg |= 1 << 21;
		break;
	case 3:
		break;
	default:
		BUG();
	}

	for (i = 0; i < num_bufs; i++) {
		if (prefbuf[i].size == 0) {
			dev_err(drvdata->sysmmu,
				"%s: Trying to init PB[%d/%d]with zero-size\n",
				__func__, idx, num_bufs);
			prefbuf[i].size = 1;
		}
		__sysmmu_set_prefbuf(drvdata->sfrbases[idx] + pbuf_offset[2],
			prefbuf[i].base, prefbuf[i].size, i);
	}

	__raw_writel(cfg, drvdata->sfrbases[idx] + REG_MMU_CFG);
}

static unsigned int find_lmm_preset(unsigned int num_pb, unsigned int num_bufs)
{
	static char lmm_preset[4][6] = {  /* [num of PB][num of buffers] */
	/*	  1,  2,  3,  4,  5,  6 */
		{ 1,  1,  0, -1, -1, -1}, /* num of pb: 3 */
		{ 3,  2,  1,  0, -1, -1}, /* num of pb: 4 */
		{-1, -1, -1, -1, -1, -1},
		{ 5,  5,  4,  2,  1,  0}, /* num of pb: 6 */
		};
	unsigned int lmm;

	BUG_ON(num_bufs > 6);
	lmm = lmm_preset[num_pb - 3][num_bufs - 1];
	BUG_ON(lmm == -1);
	return lmm;
}

static unsigned int find_num_pb(unsigned int num_pb, unsigned int lmm)
{
	static char lmm_preset[6][6] = { /* [pb_num - 1][pb_lmm] */
		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0},
		{3, 2, 0, 0, 0, 0},
		{4, 3, 2, 1, 0, 0},
		{0, 0, 0, 0, 0, 0},
		{6, 5, 4, 3, 3, 2},
	};

	num_pb = lmm_preset[num_pb - 1][lmm];
	BUG_ON(num_pb == 0);
	return num_pb;
}
static void __exynos_sysmmu_set_pbuf_ver33(struct sysmmu_drvdata *drvdata,
					   int idx)
{
	unsigned int i, num_pb, lmm;
	int num_bufs;
	struct sysmmu_prefbuf prefbuf[6];

	num_pb = __raw_readl(drvdata->sfrbases[idx] + REG_PB_INFO) & 0xFF;
	if ((num_pb != 3) && (num_pb != 4) && (num_pb != 6)) {
		dev_err(drvdata->master,
			"%s: Read invalid PB information from %s\n",
			__func__, drvdata->dbgname);
		return;
	}

	num_bufs = __prepare_prefetch_buffers(drvdata, idx, prefbuf, num_pb);
	if ((num_bufs == 0) || (num_bufs > 6)) {
		dev_dbg(drvdata->master,
			"%s: Requested an invalid number of buffers of %s - NUM_PB %d\n",
			__func__, drvdata->dbgname, num_pb);
		return;
	}

	lmm = find_lmm_preset(num_pb, (unsigned int)num_bufs);
	num_pb = find_num_pb(num_pb, lmm);

	__raw_writel(lmm, drvdata->sfrbases[idx] + REG_PB_LMM);

	for (i = 0; i < num_pb; i++) {
		__raw_writel(i, drvdata->sfrbases[idx] + REG_PB_INDICATE);
		__raw_writel(0, drvdata->sfrbases[idx] + REG_PB_CFG);
		if (num_bufs <= i)
			continue; /* unused PB */
		if (prefbuf[i].size == 0) {
			dev_err(drvdata->sysmmu,
				"%s: Trying to init PB[%d/%d]with zero-size\n",
				__func__, i, num_bufs);
			continue;
		}
		__sysmmu_set_prefbuf(drvdata->sfrbases[idx] + pbuf_offset[3],
					prefbuf[i].base, prefbuf[i].size, 0);
		__raw_writel(prefbuf[i].config | 1,
					drvdata->sfrbases[idx] + REG_PB_CFG);
	}
}

static void (*func_set_pbuf[NUM_MINOR_OF_SYSMMU_V3])
			(struct sysmmu_drvdata *, int) = {
		__exynos_sysmmu_set_pbuf_ver31,
		__exynos_sysmmu_set_pbuf_ver31,
		__exynos_sysmmu_set_pbuf_ver32,
		__exynos_sysmmu_set_pbuf_ver33,
};

static void __sysmmu_check_pbuf_region(struct sysmmu_prefbuf pbuf[], int nbufs)
{
	unsigned long pbuf_end;
	int i;

	for (i = 0; i < nbufs - 1; i++) {
		if (pbuf[i].base > pbuf[i + 1].base) {
			struct sysmmu_prefbuf tmp_pbuf;
			memcpy(&tmp_pbuf, &pbuf[i], sizeof(pbuf[0]));
			memcpy(&pbuf[i], &pbuf[i + 1], sizeof(pbuf[0]));
			memcpy(&pbuf[i + 1], &tmp_pbuf, sizeof(pbuf[0]));
		} else if (pbuf[i].base == pbuf[i + 1].base) {
			memset(&pbuf[i + 1], 0, sizeof(pbuf[0]));
		}
	} /* arrange prefetch regions according to base address */

	for (i = 0; i < nbufs - 1; i++) {
		pbuf_end = PAGE_ALIGN(pbuf[i].base + pbuf[i].size);
		if (pbuf_end > pbuf[i + 1].base) {
			if ((pbuf[i + 1].base + pbuf[i + 1].size) > pbuf_end) {
				pbuf[i + 1].base = pbuf_end;
				pbuf[i + 1].size = pbuf[i + 1].base +
					pbuf[i + 1].size - pbuf_end;
			} else {
				int j;
				for (j = i + 1; j < nbufs - 1; j++)
					memcpy(&pbuf[j], &pbuf[j + 1], sizeof(pbuf[0]));
				--nbufs;
			}
		}
	}
}

void exynos_sysmmu_set_pbuf(struct device *dev, int nbufs,
				struct sysmmu_prefbuf prefbuf[])
{
	struct device *sysmmu;
	int nsfrs;

	if (WARN_ON(nbufs < 1))
		return;

	for_each_sysmmu(dev, sysmmu) {
		unsigned long flags;
		struct sysmmu_drvdata *drvdata;

		drvdata = dev_get_drvdata(sysmmu);

		spin_lock_irqsave(&drvdata->lock, flags);

		drvdata->num_pbufs = min(6, nbufs);

		if ((drvdata->num_pbufs == 0) || !is_sysmmu_active(drvdata)) {
			spin_unlock_irqrestore(&drvdata->lock, flags);
			continue;
		}

		__sysmmu_check_pbuf_region(prefbuf, drvdata->num_pbufs);
		memcpy(drvdata->pbufs, prefbuf,
				sizeof(prefbuf[0]) * drvdata->num_pbufs);

		for (nsfrs = 0; nsfrs < drvdata->nsfrs; nsfrs++) {
			int min;

			if (!has_sysmmu_capable_pbuf(
					drvdata, nsfrs, prefbuf, &min))
				continue;

			if (sysmmu_block(drvdata->sfrbases[nsfrs])) {
				func_set_pbuf[min](drvdata, nsfrs);
				sysmmu_unblock(drvdata->sfrbases[nsfrs]);
			}
		} /* while (nsfrs < drvdata->nsfrs) */
		spin_unlock_irqrestore(&drvdata->lock, flags);
	}
}

void exynos_sysmmu_set_prefbuf(struct device *dev,
				unsigned long base0, unsigned long size0,
				unsigned long base1, unsigned long size1)
{
	struct sysmmu_prefbuf pbuf[2];
	int nbufs = 1;

	pbuf[0].base = base0;
	pbuf[0].size = size0;
	if (base1) {
		pbuf[1].base = base1;
		pbuf[1].size = size1;
		nbufs = 2;
	}

	exynos_sysmmu_set_pbuf(dev, nbufs, pbuf);
}

#ifdef CONFIG_PM_SLEEP
static void __sysmmu_restore_state(struct sysmmu_drvdata *drvdata)
{
	int i, min;

	for (i = 0; i < drvdata->nsfrs; i++) {
		if (__sysmmu_version(drvdata, i, &min) == 3) {
			if (sysmmu_block(drvdata->sfrbases[i])) {
				func_set_pbuf[min](drvdata, i);
				sysmmu_unblock(drvdata->sfrbases[i]);
			}
		}
	}
}
#endif

static void __set_fault_handler(struct sysmmu_drvdata *data,
					sysmmu_fault_handler_t handler)
{
	data->fault_handler = handler;
}

void exynos_sysmmu_set_fault_handler(struct device *dev,
					sysmmu_fault_handler_t handler)
{
	struct exynos_iommu_owner *owner = dev->archdata.iommu;
	struct device *sysmmu;
	unsigned long flags;

	spin_lock_irqsave(&owner->lock, flags);

	for_each_sysmmu(dev, sysmmu)
		__set_fault_handler(dev_get_drvdata(sysmmu), handler);

	spin_unlock_irqrestore(&owner->lock, flags);
}

static unsigned int pgsizes[4]  = {SZ_64K, SZ_4K, SZ_1M, SZ_16M};
static unsigned int v3pb_num[6][6] = { /* [pb num - 1][lmm] */
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{3, 2, 0, 0, 0, 0},
	{4, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{6, 5, 4, 3, 3, 2},
};

static void sysmmu_dump_tlb(void __iomem *sfrbase,
			    unsigned int maj, unsigned int min)
{
	unsigned int idx;
	unsigned int data0, data1;
	unsigned int num_tlb = __raw_readl(sfrbase + REG_MMU_VERSION) & 0x7F;

	pr_crit("MMU_CTRL: %#010x, MMU_CFG: %#010x, MMU_STATUS: %#010x\n",
		__raw_readl(sfrbase + REG_MMU_CTRL),
		__raw_readl(sfrbase + REG_MMU_CFG),
		__raw_readl(sfrbase + REG_MMU_STATUS));

	pr_crit("Dumping TLB of System MMU %d.%d (%d entries)\n",
		maj, min, num_tlb);
	if (maj == 1) {
		unsigned int data;
		unsigned int size;
		unsigned int cur_va = 0;
		for (idx = 0; idx < (~0 >> SPAGE_ORDER); idx++) {
			unsigned int va = idx * SPAGE_SIZE;
			__raw_writel(va | 1, sfrbase + REG_L1TLB_READ_VPN);
			data = __raw_readl(sfrbase + REG_L1TLB_DATA_VPN);
			if (data & 1) {
				size = pgsizes[(data >> 5) & 3];
				if ((va & ~(size - 1)) != cur_va) {
					cur_va = va;
					pr_crit("TLB[%#010x] = %#x\n",
						cur_va, data);
				}
			}
		}

		return;
	}

	/* System MMU 2.0 and higher */

	for (idx = 0; idx < num_tlb; idx++) {
		__raw_writel((idx << 4) | 1, sfrbase + REG_L1TLB_READ_IDX);
		data0 = __raw_readl(sfrbase + REG_L1TLB_DATA0_IDX);
		data1 = __raw_readl(sfrbase + REG_L1TLB_DATA1_IDX);
		pr_crit("TLB[%02d] = %#010x %#010x\n", idx, data0, data1);
	}

	if (maj == 2) {
		unsigned int way;

		for (way = 0; way < 8; way++) {
			for (idx = 0; idx < 64; idx++) {
				__raw_writel((idx << 8) | (way << 4) | 1,
						sfrbase + REG_L2TLB_READ);
				data0 = __raw_readl(sfrbase + REG_L2TLB_DATA0);
				data1 = __raw_readl(sfrbase + REG_L2TLB_DATA1);
				if (data0 & 1) {
					pr_crit(
					"L2TLB[%02d][%02d] = %#010x %#010x\n",
					way, idx, data0, data1);
				}
			}
		}

		return;
	}

	/* System MMU 3.x */
	if (min < 2) {
		unsigned int cfg;
		unsigned int idx;
		unsigned int num_pb_ent = 16;

		cfg = (__raw_readl(sfrbase + REG_MMU_CFG) >> 28) & 3;
		if (cfg == 0) {
			pr_crit("PB disabled!\n");
			return;
		}

		if (cfg == 3) {
			cfg = 1;
			num_pb_ent = 32;
		}

		for (idx = 0; idx < cfg; idx++) {
			unsigned int jdx;
			pr_crit(
			"PB%d: SADDR %#010x, EADDR %#010x, BASE_VA %#010x\n",
				idx,
				__raw_readl(sfrbase + REG_PB_SADDR(1, idx)),
				__raw_readl(sfrbase + REG_PB_EADDR(1, idx)),
				__raw_readl(sfrbase + REG_PB_BASE_VA(1, idx)));
			for (jdx = 0; jdx < num_pb_ent; jdx++) {
				__raw_writel((jdx << 4) | 1,
						sfrbase + REG_PB_READ(1, idx));
				pr_crit("PB%d[%d]: DATA %#010x\n", idx, jdx,
				__raw_readl(sfrbase + REG_PB_DATA(1, idx)));
			}
		}
		return;
	}

	if (min == 2) {
		unsigned int cfg;
		unsigned int idx;
		unsigned int en_map;
		unsigned int num_pb_ent = 16;

		cfg = __raw_readl(sfrbase + REG_MMU_CFG);
		if (((cfg >> 16) & 0x2F) == 0) {
			pr_crit("PB disabled!\n");
			return;
		}

		en_map = (cfg >> 16) & 7;
		if (cfg & (1 << 19)) {
			en_map = 1;
			num_pb_ent = 64;
		} else if (cfg & (1 << 21)) {
			en_map = (en_map >> 1) | 1;
			num_pb_ent = 32;
		}

		for (idx = 0; idx < 3; idx++) {
			unsigned int jdx;
			if ((en_map & (1 << idx)) == 0)
				continue;
			pr_crit(
			"PB%d: SADDR %#010x, EADDR %#010x, BASE_VA %#010x\n",
				idx,
				__raw_readl(sfrbase + REG_PB_SADDR(2, idx)),
				__raw_readl(sfrbase + REG_PB_EADDR(2, idx)),
				__raw_readl(sfrbase + REG_PB_BASE_VA(2, idx)));

			if (idx == 2)
				num_pb_ent = 32;

			for (jdx = 0; jdx < num_pb_ent; jdx++) {
				__raw_writel((jdx << 4) | 1,
						sfrbase + REG_PB_READ(2, idx));
				pr_crit("PB%d[%d]: DATA %#010x\n", idx, jdx,
				__raw_readl(sfrbase + REG_PB_DATA(2, idx)));
			}
		}
		return;
	}

	if (min == 3) {
		unsigned int idx;
		unsigned int pbnum;
		unsigned int pblmm;

		pbnum = __raw_readl(sfrbase + REG_PB_INFO);
		pblmm = __raw_readl(sfrbase + REG_PB_LMM);
		pr_crit("PB_INFO: %#010x, PB_LMM: %#010x\n", pbnum, pblmm);
		pbnum = v3pb_num[(pbnum & 0xFF) - 1][pblmm];

		for (idx = 0; idx < pbnum; idx++) {
			__raw_writel(idx, sfrbase + REG_PB_INDICATE);
			pr_crit(
			"PB[%d] = CFG: %#010x, START: %#010x, END: %#010x\n",
					idx, __raw_readl(sfrbase + REG_PB_CFG),
					__raw_readl(sfrbase + REG_PB_START_VA),
					__raw_readl(sfrbase + REG_PB_END_VA));
			pblmm = __raw_readl(sfrbase + REG_SPB_BASE_VPN);
			__raw_writel((1 << 8) | idx, sfrbase + REG_PB_INDICATE);
			pr_crit("PB[%d] = BASE0: %#010x, BASE1: %#010x\n", idx,
				pblmm, __raw_readl(sfrbase + REG_SPB_BASE_VPN));

		}
	}
}


static enum exynos_sysmmu_inttype find_fault_information(
			struct sysmmu_drvdata *drvdata, int idx,
			unsigned long *fault_addr)
{
	unsigned int maj, min;
	unsigned long base = 0;
	unsigned int itype;
	unsigned int info = 0;
	unsigned long *ent;

	pr_crit("\n-----------[ SYSTEM MMU FAULT INFORMATION ]-------------\n");
	itype = __ffs(__raw_readl(drvdata->sfrbases[idx] + REG_INT_STATUS));
	if (WARN_ON(!((itype >= 0) && (itype < SYSMMU_FAULT_UNKNOWN)))) {
		pr_crit("Fault is not occurred by this System MMU!\n");
		pr_crit("Please check if IRQ or register base address\n");
		return SYSMMU_FAULT_UNKNOWN;
	}

	maj = __sysmmu_version(drvdata, idx, &min);
	if ((maj == 3) && (min == 3)) {
		*fault_addr = __raw_readl(
				drvdata->sfrbases[idx] + REG_PAGE_FAULT_ADDR);
		info = __raw_readl(drvdata->sfrbases[idx] + 0x4C);
	} else {
		*fault_addr = __raw_readl(drvdata->sfrbases[idx] +
					  fault_reg_offset[itype]);
	}

	base = __raw_readl(drvdata->sfrbases[idx] + REG_PT_BASE_ADDR);
	if (base != drvdata->pgtable) {
		pr_crit("Page table must be %#010lx but %#010lx is specified\n",
			drvdata->pgtable, base);
		/* page table base specified in H/W may invalid */
		if ((base < __pa(PAGE_OFFSET)) || (base >= __pa(high_memory)))
			base = drvdata->pgtable;
	}

	pr_crit("%s occured at 0x%lx by '%s'(Page table base: 0x%lx)\n",
		sysmmu_fault_name[itype], *fault_addr,
		drvdata->dbgname, base);

	ent = section_entry(__va(base), *fault_addr);
	pr_crit("\tLv1 entry: 0x%lx\n", *ent);

	if (lv1ent_page(ent)) {
		ent = page_entry(ent, *fault_addr);
		pr_crit("\t Lv2 entry: 0x%lx\n", *ent);
	}
	pr_crit("\t(reserved fault lv2 page table: %#010lx)\n",
		__pa(zero_lv2_table));

	sysmmu_dump_tlb(drvdata->sfrbases[idx], maj, min);

	pr_crit("-------[ END OF SYSTEM MMU FAULT INFORMATION ]----------\n\n");
	return (enum exynos_sysmmu_inttype)itype;
}

static irqreturn_t exynos_sysmmu_irq(int irq, void *dev_id)
{
	/* SYSMMU is in blocked when interrupt occurred. */
	struct sysmmu_drvdata *drvdata = dev_id;
	struct exynos_iommu_owner *owner = NULL;
	enum exynos_sysmmu_inttype itype;
	unsigned long addr = -1;
	const char *mmuname = NULL;
	int i, ret = -ENOSYS;

	if (drvdata->master)
		owner = drvdata->master->archdata.iommu;

	if (owner)
		spin_lock(&owner->lock);

	WARN_ON(!is_sysmmu_active(drvdata));

	for (i = 0; i < drvdata->nsfrs; i++) {
		struct resource *irqres;
		irqres = platform_get_resource(
				to_platform_device(drvdata->sysmmu),
				IORESOURCE_IRQ, i);
		if (irqres && ((int)irqres->start == irq)) {
			mmuname = irqres->name;
			break;
		}
	}

	if (WARN_ON(i == drvdata->nsfrs)) {
		itype = SYSMMU_FAULT_UNKNOWN;
	} else {
		itype = find_fault_information(drvdata, i, &addr);
	}

	if (drvdata->domain) /* owner is always set if drvdata->domain exists */
		ret = report_iommu_fault(drvdata->domain,
					owner->dev, addr, itype);

	if ((ret == -ENOSYS) && drvdata->fault_handler) {
		ret = drvdata->fault_handler(
					owner ? owner->dev : drvdata->sysmmu,
					mmuname ? mmuname : drvdata->dbgname,
					itype, drvdata->pgtable, addr);
	}

	panic("Halted by System MMU Fault from %s!", drvdata->dbgname);

	if (!ret && (itype != SYSMMU_FAULT_UNKNOWN))
		__raw_writel(1 << itype, drvdata->sfrbases[i] + REG_INT_CLEAR);
	else
		dev_dbg(owner ? owner->dev : drvdata->sysmmu,
				"%s is not handled by %s\n",
				sysmmu_fault_name[itype], drvdata->dbgname);

	sysmmu_unblock(drvdata->sfrbases[i]);

	if (owner)
		spin_unlock(&owner->lock);

	return IRQ_HANDLED;
}

static void __sysmmu_disable_nocount(struct sysmmu_drvdata *drvdata)
{
		int i;

		for (i = 0; i < drvdata->nsfrs; i++) {
			__raw_writel(0, drvdata->sfrbases[i] + REG_MMU_CFG);
			__raw_writel(CTRL_DISABLE,
				drvdata->sfrbases[i] + REG_MMU_CTRL);
		}

		dev_dbg(drvdata->sysmmu, "clk_disable\n");
		clk_disable(drvdata->clk);
}

static bool __sysmmu_disable(struct sysmmu_drvdata *drvdata)
{
	bool disabled;
	unsigned long flags;

	spin_lock_irqsave(&drvdata->lock, flags);

	disabled = set_sysmmu_inactive(drvdata);

	if (disabled) {
		drvdata->pgtable = 0;
		drvdata->domain = NULL;

		if (drvdata->runtime_active)
			__sysmmu_disable_nocount(drvdata);

		dev_dbg(drvdata->sysmmu, "Disabled %s\n", drvdata->dbgname);
	} else  {
		dev_dbg(drvdata->sysmmu, "%d times left to disable %s\n",
					drvdata->activations, drvdata->dbgname);
	}

	spin_unlock_irqrestore(&drvdata->lock, flags);

	return disabled;
}

static void __sysmmu_init_config(struct sysmmu_drvdata *drvdata, int idx)
{
	unsigned long cfg = CFG_LRU | CFG_QOS(drvdata->qos);
	int maj, min = 0;

	maj = __sysmmu_version(drvdata, idx, &min);
	if ((maj == 1) || (maj == 2))
		goto set_cfg;

	BUG_ON(maj != 3);

	if (min < 2)
		goto set_cfg;

	BUG_ON(min > 3);

	cfg |= CFG_FLPDCACHE;
	cfg |= (min == 2) ? CFG_SYSSEL : (CFG_ACGEN | CFG_QOS_OVRRIDE);

	func_set_pbuf[min](drvdata, idx);
set_cfg:
	cfg |= __raw_readl(drvdata->sfrbases[idx] + REG_MMU_CFG) & ~CFG_MASK;
	__raw_writel(cfg, drvdata->sfrbases[idx] + REG_MMU_CFG);
}

static void __sysmmu_enable_nocount(struct sysmmu_drvdata *drvdata)
{
	int i;

	clk_enable(drvdata->clk);
	dev_dbg(drvdata->sysmmu, "clk_enable\n");

	for (i = 0; i < drvdata->nsfrs; i++) {
		if (soc_is_exynos5420() || soc_is_exynos5260())
			__raw_writel(0, drvdata->sfrbases[i] + REG_MMU_CTRL);

		BUG_ON(__raw_readl(drvdata->sfrbases[i] + REG_MMU_CTRL)
								& CTRL_ENABLE);

		__sysmmu_init_config(drvdata, i);

		__sysmmu_set_ptbase(drvdata->sfrbases[i], drvdata->pgtable);

		__raw_writel(CTRL_ENABLE, drvdata->sfrbases[i] + REG_MMU_CTRL);
	}
}

static int __sysmmu_enable(struct sysmmu_drvdata *drvdata,
			unsigned long pgtable, struct iommu_domain *domain)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&drvdata->lock, flags);
	if (set_sysmmu_active(drvdata)) {
		drvdata->pgtable = pgtable;
		drvdata->domain = domain;

		if (drvdata->runtime_active)
			__sysmmu_enable_nocount(drvdata);

		dev_dbg(drvdata->sysmmu, "Enabled %s\n", drvdata->dbgname);
	} else {
		ret = (pgtable == drvdata->pgtable) ? 1 : -EBUSY;

		dev_dbg(drvdata->sysmmu, "%s is already enabled\n",
							drvdata->dbgname);
	}

	if (WARN_ON(ret < 0))
		set_sysmmu_inactive(drvdata); /* decrement count */

	spin_unlock_irqrestore(&drvdata->lock, flags);

	return ret;
}

/* __exynos_sysmmu_enable: Enables System MMU
 *
 * returns -error if an error occurred and System MMU is not enabled,
 * 0 if the System MMU has been just enabled and 1 if System MMU was already
 * enabled before.
 */
static int __exynos_sysmmu_enable(struct device *dev, unsigned long pgtable,
				struct iommu_domain *domain)
{
	int ret = 0;
	unsigned long flags;
	struct exynos_iommu_owner *owner = dev->archdata.iommu;
	struct device *sysmmu;

	BUG_ON(!has_sysmmu(dev));

	spin_lock_irqsave(&owner->lock, flags);

	for_each_sysmmu(dev, sysmmu) {
		struct sysmmu_drvdata *drvdata = dev_get_drvdata(sysmmu);
		drvdata->master = dev;
		ret = __sysmmu_enable(drvdata, pgtable, domain);
		if (ret < 0) {
			struct device *iter;
			for_each_sysmmu_until(dev, iter, sysmmu) {
				drvdata = dev_get_drvdata(iter);
				__sysmmu_disable(drvdata);
				drvdata->master = NULL;
			}
		}
	}

	spin_unlock_irqrestore(&owner->lock, flags);

	return ret;
}

int exynos_sysmmu_enable(struct device *dev, unsigned long pgtable)
{
	int ret;

	BUG_ON(!memblock_is_memory(pgtable));

	ret = __exynos_sysmmu_enable(dev, pgtable, NULL);

	return ret;
}

bool exynos_sysmmu_disable(struct device *dev)
{
	unsigned long flags;
	bool disabled = true;
	struct exynos_iommu_owner *owner = dev->archdata.iommu;
	struct device *sysmmu;

	BUG_ON(!has_sysmmu(dev));

	spin_lock_irqsave(&owner->lock, flags);

	/* Every call to __sysmmu_disable() must return same result */
	for_each_sysmmu(dev, sysmmu) {
		struct sysmmu_drvdata *drvdata = dev_get_drvdata(sysmmu);
		disabled = __sysmmu_disable(drvdata);
		if (disabled)
			drvdata->master = NULL;
	}

	spin_unlock_irqrestore(&owner->lock, flags);

	return disabled;
}

void exynos_sysmmu_tlb_invalidate(struct device *dev)
{
	struct device *sysmmu;

	for_each_sysmmu(dev, sysmmu) {
		unsigned long flags;
		struct sysmmu_drvdata *drvdata;

		drvdata = dev_get_drvdata(sysmmu);
		if (drvdata->tlbinv_entry)
			continue;

		spin_lock_irqsave(&drvdata->lock, flags);
		if (is_sysmmu_active(drvdata) &&
				drvdata->runtime_active) {
			int i;
			for (i = 0; i < drvdata->nsfrs; i++) {
				if (sysmmu_block(drvdata->sfrbases[i])) {
					__sysmmu_tlb_invalidate(
							drvdata->sfrbases[i]);
					sysmmu_unblock(drvdata->sfrbases[i]);
				}
			}
		} else {
			dev_dbg(dev,
				"%s is disabled. Skipping TLB invalidation\n",
				drvdata->dbgname);
		}
		spin_unlock_irqrestore(&drvdata->lock, flags);
	}
}

static void sysmmu_tlb_invalidate_flpdcache(struct device *dev, dma_addr_t iova)
{
	struct device *sysmmu;

	for_each_sysmmu(dev, sysmmu) {
		unsigned long flags;
		struct sysmmu_drvdata *drvdata;

		drvdata = dev_get_drvdata(sysmmu);
		if (!drvdata->tlbinv_entry)
			continue;

		spin_lock_irqsave(&drvdata->lock, flags);
		if (is_sysmmu_active(drvdata) &&
				drvdata->runtime_active) {
			int i;
			for (i = 0; i < drvdata->nsfrs; i++) {
				unsigned int maj, min;
				maj = __sysmmu_version(drvdata, i, &min);
				if ((maj == 3) && (min == 3))
					__sysmmu_tlb_invalidate_entry(
						drvdata->sfrbases[i], iova);
			}
		} else {
			dev_dbg(dev,
			"%s is disabled. Skipping TLB invalidation @ %#x\n",
			drvdata->dbgname, iova);
		}
		spin_unlock_irqrestore(&drvdata->lock, flags);
	}
}

static void sysmmu_tlb_invalidate_entry(struct device *dev, dma_addr_t iova)
{
	struct device *sysmmu;

	for_each_sysmmu(dev, sysmmu) {
		unsigned long flags;
		struct sysmmu_drvdata *drvdata;

		drvdata = dev_get_drvdata(sysmmu);
		if (!drvdata->tlbinv_entry)
			continue;

		spin_lock_irqsave(&drvdata->lock, flags);
		if (is_sysmmu_active(drvdata) &&
				drvdata->runtime_active) {
			int i;
			for (i = 0; i < drvdata->nsfrs; i++)
				__sysmmu_tlb_invalidate_entry(
						drvdata->sfrbases[i], iova);
		} else {
			dev_dbg(dev,
			"%s is disabled. Skipping TLB invalidation @ %#x\n",
			drvdata->dbgname, iova);
		}
		spin_unlock_irqrestore(&drvdata->lock, flags);
	}
}

#ifdef CONFIG_EXYNOS_IOMMU_RECOVER_FAULT_HANDLER
static int recover_fault_handler (struct iommu_domain *domain,
				struct device *dev, unsigned long fault_addr,
				int itype)
{
	struct exynos_iommu_domain *priv = domain->priv;
	struct exynos_iommu_owner *owner;
	unsigned long flags;

	if (itype == SYSMMU_PAGEFAULT) {
		struct exynos_iovmm *vmm_data;
		unsigned long *sent;
		unsigned long *pent;

		BUG_ON(priv->pgtable == NULL);

		spin_lock_irqsave(&priv->pgtablelock, flags);

		sent = section_entry(priv->pgtable, fault_addr);
		if (!lv1ent_page(sent)) {
			pent = kmem_cache_zalloc(lv2table_kmem_cache, GFP_ATOMIC);
			if (!pent)
				return -ENOMEM;

			*sent = mk_lv1ent_page(__pa(pent));
			pgtable_flush(sent, sent + 1);
		}
		pent = page_entry(sent, fault_addr);
		if (lv2ent_fault(pent)) {
			*pent = mk_lv2ent_spage(__pa(fault_page));
			pgtable_flush(pent, pent + 1);
		} else {
			pr_err("[%s] 0x%lx by '%s' is already mapped\n",
				sysmmu_fault_name[itype], fault_addr, dev_name(dev));
		}

		spin_unlock_irqrestore(&priv->pgtablelock, flags);

		owner = dev->archdata.iommu;
		vmm_data = (struct exynos_iovmm *)owner->vmm_data;
		if (find_iovm_region(vmm_data, fault_addr)) {
			pr_err("[%s] 0x%lx by '%s' is remapped\n", sysmmu_fault_name[itype],
				fault_addr, dev_name(dev));
		} else {
			pr_err("[%s] '%s' accessed unmapped address(0x%lx)\n",
				sysmmu_fault_name[itype], dev_name(dev), fault_addr);
		}
	} else if (itype == SYSMMU_AR_MULTIHIT || itype == SYSMMU_AW_MULTIHIT) {
		spin_lock_irqsave(&priv->lock, flags);
		list_for_each_entry(owner, &priv->clients, client)
			sysmmu_tlb_invalidate_entry(owner->dev, fault_addr);
		spin_unlock_irqrestore(&priv->lock, flags);

		pr_err("[%s] occured at 0x%lx by '%s'\n", sysmmu_fault_name[itype],
			fault_addr, dev_name(dev));
	} else {
		return -ENOSYS;
	}

	return 0;
}
#else
static int recover_fault_handler (struct iommu_domain *domain,
				struct device *dev, unsigned long fault_addr,
				int itype)
{
	return -ENOSYS;
}
#endif

static int __init __sysmmu_init_clock(struct device *sysmmu,
					struct sysmmu_drvdata *drvdata,
					struct device *master)
{
	struct sysmmu_platform_data *platdata = dev_get_platdata(sysmmu);
	char *conid;
	struct clk *parent_clk;
	int ret;

	drvdata->clk = clk_get(sysmmu, "sysmmu");
	if (IS_ERR(drvdata->clk)) {
		dev_dbg(sysmmu, "No gating clock found.\n");
		drvdata->clk = NULL;
		return 0;
	}

	if (!master)
		return 0;

	conid = platdata->clockname;
	if (!conid) {
		dev_dbg(sysmmu, "No parent clock specified.\n");
		return 0;
	}

	parent_clk = clk_get(master, conid);
	if (IS_ERR(parent_clk)) {
		parent_clk = clk_get(NULL, conid);
		if (IS_ERR(parent_clk)) {
			clk_put(drvdata->clk);
			dev_err(sysmmu, "No parent clock '%s,%s' found\n",
				dev_name(master), conid);
			return PTR_ERR(parent_clk);
		}
	}

	ret = clk_set_parent(drvdata->clk, parent_clk);
	if (ret) {
		clk_put(drvdata->clk);
		dev_err(sysmmu, "Failed to set parent clock '%s,%s'\n",
				dev_name(master), conid);
	}

	clk_put(parent_clk);

	return ret;
}

#define has_more_master(dev) ((unsigned long)dev->archdata.iommu & 1)
#define master_initialized(dev) (!((unsigned long)dev->archdata.iommu & 1) \
				&& ((unsigned long)dev->archdata.iommu & ~1))

static struct device * __init __sysmmu_init_master(
				struct device *sysmmu, struct device *dev) {
	struct exynos_iommu_owner *owner;
	struct device *master = (struct device *)((unsigned long)dev & ~1);
	int ret;

	if (!master)
		return NULL;

	/*
	 * has_more_master() call to the main master device returns false while
	 * the same call to the other master devices (shared master devices)
	 * return true.
	 * Shared master devices are moved after 'sysmmu' in the DPM list while
	 * 'sysmmu' is moved before the master device not to break the order of
	 * suspend/resume.
	 */
	if (has_more_master(master)) {
		void *pret;
		pret = __sysmmu_init_master(sysmmu, master->archdata.iommu);
		if (IS_ERR(pret))
			return pret;

		ret = device_move(master, sysmmu, DPM_ORDER_DEV_AFTER_PARENT);
		if (ret)
			return ERR_PTR(ret);
	} else {
		struct device *child = master;
		/* Finding the topmost System MMU in the hierarchy of master. */
		while (child && child->parent && is_sysmmu(child->parent))
			child = child->parent;

		ret = device_move(child, sysmmu, DPM_ORDER_PARENT_BEFORE_DEV);
		if (ret)
			return ERR_PTR(ret);

		if (master_initialized(master)) {
			dev_dbg(sysmmu,
				"Assigned initialized master device %s.\n",
							dev_name(master));
			return master;
		}
	}

	/*
	 * There must not be a master device which is initialized and
	 * has a link to another master device.
	 */
	BUG_ON(master_initialized(master));

	owner = devm_kzalloc(sysmmu, sizeof(*owner), GFP_KERNEL);
	if (!owner) {
		dev_err(sysmmu, "Failed to allcoate iommu data.\n");
		return ERR_PTR(-ENOMEM);
	}

	INIT_LIST_HEAD(&owner->client);
	owner->dev = master;
	spin_lock_init(&owner->lock);

	master->archdata.iommu = owner;

	if (soc_is_exynos5410()) {
		ret = exynos_create_iovmm(master, 1, 0);
		if (ret) {
			dev_err(sysmmu, "Failed create IOVMM context.\n");
			return ERR_PTR(ret);
		}
	}

#ifdef CONFIG_DISPLAY_EARLY_DPMS
	device_set_early_complete(sysmmu, EARLY_COMP_MASTER);
#endif

	dev_dbg(sysmmu, "Assigned master device %s.\n", dev_name(master));

	return master;
}

static int __init __sysmmu_setup(struct device *sysmmu,
				struct sysmmu_drvdata *drvdata)
{
	struct sysmmu_platform_data *platdata = dev_get_platdata(sysmmu);
	struct device *master;
	int ret;

	drvdata->ver = platdata->ver;
	drvdata->dbgname = platdata->dbgname;
	drvdata->qos = platdata->qos;
	drvdata->prop = platdata->prop;
	if ((drvdata->qos < 0) || (drvdata->qos > 15))
		drvdata->qos = 15;
	drvdata->num_pbufs = 0;

	drvdata->tlbinv_entry = platdata->tlbinv_entry;

	master = __sysmmu_init_master(sysmmu, sysmmu->archdata.iommu);
	if (!master) {
		dev_dbg(sysmmu, "No master device is assigned\n");
	} else if (IS_ERR(master)) {
		dev_err(sysmmu, "Failed to initialize master device.\n");
		return PTR_ERR(master);
	}

	ret = __sysmmu_init_clock(sysmmu, drvdata, master);
	if (ret)
		dev_err(sysmmu, "Failed to initialize gating clocks\n");

	return ret;
}

static void __create_debugfs_entry(struct sysmmu_drvdata *drvdata);

static int __init exynos_sysmmu_probe(struct platform_device *pdev)
{
	int i, ret;
	struct device *dev = &pdev->dev;
	struct sysmmu_drvdata *data;

	data = devm_kzalloc(dev,
			sizeof(*data) + sizeof(*data->sfrbases) *
				(pdev->num_resources / 2),
			GFP_KERNEL);
	if (!data) {
		dev_err(dev, "Not enough memory\n");
		return -ENOMEM;
	}

	data->nsfrs = pdev->num_resources / 2;
	data->sfrbases = (void __iomem **)(data + 1);

	for (i = 0; i < data->nsfrs; i++) {
		struct resource *res;
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(dev, "Unable to find IOMEM region\n");
			return -ENOENT;
		}

		data->sfrbases[i] = devm_request_and_ioremap(dev, res);
		if (!data->sfrbases[i]) {
			dev_err(dev, "Unable to map IOMEM @ PA:%#x\n",
							res->start);
			return -EBUSY;
		}
	}

	for (i = 0; i < data->nsfrs; i++) {
		ret = platform_get_irq(pdev, i);
		if (ret <= 0) {
			dev_err(dev, "Unable to find IRQ resource\n");
			return ret;
		}

		ret = devm_request_irq(dev, ret, exynos_sysmmu_irq, 0,
					dev_name(dev), data);
		if (ret) {
			dev_err(dev, "Unabled to register interrupt handler\n");
			return ret;
		}
	}

	pm_runtime_enable(dev);

	ret = __sysmmu_setup(dev, data);
	if (!ret) {
		data->runtime_active = !pm_runtime_enabled(dev);
		data->sysmmu = dev;
		spin_lock_init(&data->lock);

		__create_debugfs_entry(data);

		platform_set_drvdata(pdev, data);

		dev->archdata.iommu = &sysmmu_placeholder;
		dev_dbg(dev, "(%s) Initialized\n", data->dbgname);
	}

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int sysmmu_suspend(struct device *dev)
{
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);
	unsigned long flags;
	spin_lock_irqsave(&drvdata->lock, flags);
	if (is_sysmmu_active(drvdata) &&
		(!pm_runtime_enabled(dev) || drvdata->runtime_active))
		__sysmmu_disable_nocount(drvdata);
	spin_unlock_irqrestore(&drvdata->lock, flags);
	return 0;
}

static int sysmmu_resume(struct device *dev)
{
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);
	unsigned long flags;
	spin_lock_irqsave(&drvdata->lock, flags);
	if (is_sysmmu_active(drvdata) &&
		(!pm_runtime_enabled(dev) || drvdata->runtime_active)) {
		__sysmmu_enable_nocount(drvdata);
		__sysmmu_restore_state(drvdata);
	}
	spin_unlock_irqrestore(&drvdata->lock, flags);
	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int sysmmu_runtime_suspend(struct device *dev)
{
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);
	unsigned long flags;
	spin_lock_irqsave(&drvdata->lock, flags);
	if (is_sysmmu_active(drvdata))
		__sysmmu_disable_nocount(drvdata);
	drvdata->runtime_active = false;
	spin_unlock_irqrestore(&drvdata->lock, flags);
	return 0;
}

static int sysmmu_runtime_resume(struct device *dev)
{
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);
	unsigned long flags;
	spin_lock_irqsave(&drvdata->lock, flags);
	drvdata->runtime_active = true;
	if (is_sysmmu_active(drvdata))
		__sysmmu_enable_nocount(drvdata);
	spin_unlock_irqrestore(&drvdata->lock, flags);
	return 0;
}
#endif

static const struct dev_pm_ops __pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sysmmu_suspend, sysmmu_resume)
	SET_RUNTIME_PM_OPS(sysmmu_runtime_suspend, sysmmu_runtime_resume, NULL)
};

static struct platform_driver exynos_sysmmu_driver __refdata = {
	.probe		= exynos_sysmmu_probe,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= MODULE_NAME,
		.pm		= &__pm_ops,
	}
};

static int exynos_iommu_domain_init(struct iommu_domain *domain)
{
	struct exynos_iommu_domain *priv;
	int i;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->pgtable = (unsigned long *)__get_free_pages(
						GFP_KERNEL | __GFP_ZERO, 2);
	if (!priv->pgtable)
		goto err_pgtable;

	priv->lv2entcnt = (short *)__get_free_pages(
						GFP_KERNEL | __GFP_ZERO, 1);
	if (!priv->lv2entcnt)
		goto err_counter;

	for (i = 0; i < NUM_LV1ENTRIES; i += 8) {
		priv->pgtable[i + 0] = ZERO_LV2LINK;
		priv->pgtable[i + 1] = ZERO_LV2LINK;
		priv->pgtable[i + 2] = ZERO_LV2LINK;
		priv->pgtable[i + 3] = ZERO_LV2LINK;
		priv->pgtable[i + 4] = ZERO_LV2LINK;
		priv->pgtable[i + 5] = ZERO_LV2LINK;
		priv->pgtable[i + 6] = ZERO_LV2LINK;
		priv->pgtable[i + 7] = ZERO_LV2LINK;
	}

	pgtable_flush(priv->pgtable, priv->pgtable + NUM_LV1ENTRIES);

	spin_lock_init(&priv->lock);
	spin_lock_init(&priv->pgtablelock);
	INIT_LIST_HEAD(&priv->clients);

	domain->priv = priv;
	domain->handler = recover_fault_handler;
	return 0;

err_counter:
	free_pages((unsigned long)priv->pgtable, 2);
err_pgtable:
	kfree(priv);
	return -ENOMEM;
}

static void exynos_iommu_domain_destroy(struct iommu_domain *domain)
{
	struct exynos_iommu_domain *priv = domain->priv;
	struct exynos_iommu_owner *owner, *n;
	unsigned long flags;
	int i;

	WARN_ON(!list_empty(&priv->clients));

	spin_lock_irqsave(&priv->lock, flags);

	list_for_each_entry_safe(owner, n, &priv->clients, client) {
		while (!exynos_sysmmu_disable(owner->dev))
			; /* until System MMU is actually disabled */
		list_del_init(&owner->client);
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	for (i = 0; i < NUM_LV1ENTRIES; i++)
		if (lv1ent_page(priv->pgtable + i))
			kmem_cache_free(lv2table_kmem_cache,
					__va(lv2table_base(priv->pgtable + i)));

	free_pages((unsigned long)priv->pgtable, 2);
	free_pages((unsigned long)priv->lv2entcnt, 1);
	kfree(domain->priv);
	domain->priv = NULL;
}

static int exynos_iommu_attach_device(struct iommu_domain *domain,
				   struct device *dev)
{
	struct exynos_iommu_owner *owner = dev->archdata.iommu;
	struct exynos_iommu_domain *priv = domain->priv;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&priv->lock, flags);

	ret = __exynos_sysmmu_enable(dev, __pa(priv->pgtable), domain);

	if (ret == 0)
		list_add_tail(&owner->client, &priv->clients);

	spin_unlock_irqrestore(&priv->lock, flags);

	if (ret < 0)
		dev_err(dev, "%s: Failed to attach IOMMU with pgtable %#lx\n",
				__func__, __pa(priv->pgtable));
	else
		dev_info(dev, "%s: Attached new IOMMU with pgtable 0x%lx%s\n",
					__func__, __pa(priv->pgtable),
					(ret == 0) ? "" : ", again");

	return ret;
}

static void exynos_iommu_detach_device(struct iommu_domain *domain,
				    struct device *dev)
{
	struct exynos_iommu_owner *owner, *n;
	struct exynos_iommu_domain *priv = domain->priv;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	list_for_each_entry_safe(owner, n, &priv->clients, client) {
		if (owner == dev->archdata.iommu) {
			if (exynos_sysmmu_disable(dev))
				list_del_init(&owner->client);
			break;
		}
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	if (owner == dev->archdata.iommu)
		dev_info(dev, "%s: Detached IOMMU with pgtable %#lx\n",
					__func__, __pa(priv->pgtable));
	else
		dev_info(dev, "%s: No IOMMU is attached\n", __func__);
}

static unsigned long *alloc_lv2entry(struct exynos_iommu_domain *priv,
		unsigned long *sent, unsigned long iova, short *pgcounter)
{
	if (lv1ent_fault(sent)) {
		unsigned long *pent;
		struct exynos_iommu_owner *owner;
		unsigned long flags;

		pent = kmem_cache_zalloc(lv2table_kmem_cache, GFP_ATOMIC);
		BUG_ON((unsigned long)pent & (LV2TABLE_SIZE - 1));
		if (!pent)
			return ERR_PTR(-ENOMEM);

		*sent = mk_lv1ent_page(__pa(pent));
		kmemleak_ignore(pent);
		*pgcounter = NUM_LV2ENTRIES;
		pgtable_flush(pent, pent + NUM_LV2ENTRIES);
		pgtable_flush(sent, sent + 1);

		/*
		 * If pretched SLPD is a fault SLPD in zero_l2_table, FLPD cache
		 * caches the address of zero_l2_table. This function replaces
		 * the zero_l2_table with new L2 page table to write valid
		 * mappings.
		 * Accessing the valid area may cause page fault since FLPD
		 * cache may still caches zero_l2_table for the valid area
		 * instead of new L2 page table that have the mapping
		 * information of the valid area
		 * Thus any replacement of zero_l2_table with other valid L2
		 * page table must involve FLPD cache invalidation if the System
		 * MMU have prefetch feature and FLPD cache (version 3.3).
		 * FLPD cache invalidation is performed with TLB invalidation
		 * by VPN without blocking. It is safe to invalidate TLB without
		 * blocking because the target address of TLB invalidation is not
		 * currently mapped.
		 */
		spin_lock_irqsave(&priv->lock, flags);
		list_for_each_entry(owner, &priv->clients, client)
			sysmmu_tlb_invalidate_flpdcache(owner->dev, iova);
		spin_unlock_irqrestore(&priv->lock, flags);
	} else if (lv1ent_section(sent)) {
		return ERR_PTR(-EADDRINUSE);
	}

	return page_entry(sent, iova);
}

static int lv1set_section(unsigned long *sent, phys_addr_t paddr, short *pgcnt)
{
	if (lv1ent_section(sent))
		return -EADDRINUSE;

	if (lv1ent_page(sent)) {
		if (*pgcnt != NUM_LV2ENTRIES)
			return -EADDRINUSE;

		kmem_cache_free(lv2table_kmem_cache, page_entry(sent, 0));

		*pgcnt = 0;
	}

	*sent = mk_lv1ent_sect(paddr);

	pgtable_flush(sent, sent + 1);

	return 0;
}

static int lv2set_page(unsigned long *pent, phys_addr_t paddr, size_t size,
								short *pgcnt)
{
	if (size == SPAGE_SIZE) {
		if (!lv2ent_fault(pent))
			return -EADDRINUSE;

		*pent = mk_lv2ent_spage(paddr);
		pgtable_flush(pent, pent + 1);
		*pgcnt -= 1;
	} else { /* size == LPAGE_SIZE */
		int i;
		for (i = 0; i < SPAGES_PER_LPAGE; i++, pent++) {
			if (!lv2ent_fault(pent)) {
				memset(pent, 0, sizeof(*pent) * i);
				return -EADDRINUSE;
			}

			*pent = mk_lv2ent_lpage(paddr);
		}
		pgtable_flush(pent - SPAGES_PER_LPAGE, pent);
		*pgcnt -= SPAGES_PER_LPAGE;
	}

	return 0;
}

static int exynos_iommu_map(struct iommu_domain *domain, unsigned long iova,
			 phys_addr_t paddr, size_t size, int prot)
{
	struct exynos_iommu_domain *priv = domain->priv;
	unsigned long *entry;
	unsigned long flags;
	int ret = -ENOMEM;

	BUG_ON(priv->pgtable == NULL);

	spin_lock_irqsave(&priv->pgtablelock, flags);

	entry = section_entry(priv->pgtable, iova);

	if (size == SECT_SIZE) {
		if (lv1ent_fault_link(entry)) {
			unsigned long flags;
			struct exynos_iommu_owner *owner;
			spin_lock_irqsave(&priv->lock, flags);
			list_for_each_entry(owner, &priv->clients, client)
				sysmmu_tlb_invalidate_flpdcache(
							owner->dev, iova);
			spin_unlock_irqrestore(&priv->lock, flags);
		}

		ret = lv1set_section(entry, paddr,
					&priv->lv2entcnt[lv1ent_offset(iova)]);
	} else {
		unsigned long *pent;

		pent = alloc_lv2entry(priv, entry, iova,
					&priv->lv2entcnt[lv1ent_offset(iova)]);

		if (IS_ERR(pent))
			ret = PTR_ERR(pent);
		else
			ret = lv2set_page(pent, paddr, size,
					&priv->lv2entcnt[lv1ent_offset(iova)]);
	}

	if (ret)
		pr_err("%s: Failed(%d) to map %#x bytes @ %#lx\n",
			__func__, ret, size, iova);

	spin_unlock_irqrestore(&priv->pgtablelock, flags);

	return ret;
}

static size_t exynos_iommu_unmap(struct iommu_domain *domain,
					       unsigned long iova, size_t size)
{
	struct exynos_iommu_domain *priv = domain->priv;
	struct exynos_iommu_owner *owner;
	unsigned long flags;
	unsigned long *ent;
	size_t err_page;

	BUG_ON(priv->pgtable == NULL);

	spin_lock_irqsave(&priv->pgtablelock, flags);

	ent = section_entry(priv->pgtable, iova);

	if (lv1ent_section(ent)) {
		if (WARN_ON(size < SECT_SIZE))
			goto err;

		*ent = ZERO_LV2LINK;
		pgtable_flush(ent, ent + 1);
		size = SECT_SIZE;
		goto done;
	}

	if (unlikely(lv1ent_fault(ent))) {
		if (size > SECT_SIZE)
			size = SECT_SIZE;
		goto done;
	}

	/* lv1ent_page(sent) == true here */

	ent = page_entry(ent, iova);

	if (unlikely(lv2ent_fault(ent))) {
		size = SPAGE_SIZE;
		goto done;
	}

	if (lv2ent_small(ent)) {
		*ent = 0;
		size = SPAGE_SIZE;
		pgtable_flush(ent, ent + 1);
		priv->lv2entcnt[lv1ent_offset(iova)] += 1;
		goto done;
	}

	/* lv1ent_large(ent) == true here */
	if (WARN_ON(size < LPAGE_SIZE))
		goto err;

	memset(ent, 0, sizeof(*ent) * SPAGES_PER_LPAGE);
	pgtable_flush(ent, ent + SPAGES_PER_LPAGE);

	size = LPAGE_SIZE;
	priv->lv2entcnt[lv1ent_offset(iova)] += SPAGES_PER_LPAGE;
done:
	spin_unlock_irqrestore(&priv->pgtablelock, flags);

	spin_lock_irqsave(&priv->lock, flags);
	list_for_each_entry(owner, &priv->clients, client)
		sysmmu_tlb_invalidate_entry(owner->dev, iova);
	spin_unlock_irqrestore(&priv->lock, flags);

	return size;
err:
	spin_unlock_irqrestore(&priv->pgtablelock, flags);

	err_page = (
		((unsigned long)ent - (unsigned long)priv->pgtable)
					< (NUM_LV1ENTRIES * sizeof(long))
			) ?  SECT_SIZE : LPAGE_SIZE;

	pr_err("%s: Failed due to size(%#lx) @ %#x is"\
		" smaller than page size %#x\n",
		__func__, iova, size, err_page);

	return 0;
}

static void exynos_iommu_tlb_flush_all(struct iommu_domain *domain)
{
	struct exynos_iommu_domain *priv = domain->priv;
	struct exynos_iommu_owner *owner;
	unsigned long flags;

	/* TODO. it should be checked if cache flush all already was called. */

	spin_lock_irqsave(&priv->lock, flags);
	list_for_each_entry(owner, &priv->clients, client)
		exynos_sysmmu_tlb_invalidate(owner->dev);
	spin_unlock_irqrestore(&priv->lock, flags);
}

static phys_addr_t exynos_iommu_iova_to_phys(struct iommu_domain *domain,
					  unsigned long iova)
{
	struct exynos_iommu_domain *priv = domain->priv;
	unsigned long *entry;
	unsigned long flags;
	phys_addr_t phys = 0;

	spin_lock_irqsave(&priv->pgtablelock, flags);

	entry = section_entry(priv->pgtable, iova);

	if (lv1ent_section(entry)) {
		phys = section_phys(entry) + section_offs(iova);
	} else if (lv1ent_page(entry)) {
		entry = page_entry(entry, iova);

		if (lv2ent_large(entry))
			phys = lpage_phys(entry) + lpage_offs(iova);
		else if (lv2ent_small(entry))
			phys = spage_phys(entry) + spage_offs(iova);
	}

	spin_unlock_irqrestore(&priv->pgtablelock, flags);

	return phys;
}

static struct iommu_ops exynos_iommu_ops = {
	.domain_init = &exynos_iommu_domain_init,
	.domain_destroy = &exynos_iommu_domain_destroy,
	.attach_dev = &exynos_iommu_attach_device,
	.detach_dev = &exynos_iommu_detach_device,
	.map = &exynos_iommu_map,
	.unmap = &exynos_iommu_unmap,
	.tlb_flush_all = &exynos_iommu_tlb_flush_all,
	.iova_to_phys = &exynos_iommu_iova_to_phys,
	.pgsize_bitmap = SECT_SIZE | LPAGE_SIZE | SPAGE_SIZE,
};

static int __init exynos_bus_set_iommu(void)
{
	struct page *page;
	int ret = -ENOMEM;

	lv2table_kmem_cache = kmem_cache_create("exynos-iommu-lv2table",
		LV2TABLE_SIZE, LV2TABLE_SIZE, 0, NULL);
	if (!lv2table_kmem_cache) {
		pr_err("%s: failed to create kmem cache\n", __func__);
		return -ENOMEM;
	}

	page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!page) {
		pr_err("%s: failed to allocate fault page\n", __func__);
		goto err_fault_page;
	}

	fault_page = page_to_phys(page);

	ret = bus_set_iommu(&platform_bus_type, &exynos_iommu_ops);
	if (ret) {
		pr_err("%s: Failed to register IOMMU ops\n", __func__);
		goto err_set_iommu;
	}

	zero_lv2_table = kmem_cache_zalloc(lv2table_kmem_cache, GFP_KERNEL);
	if (zero_lv2_table == NULL) {
		pr_err("%s: Failed to allocate zero level2 page table\n",
			__func__);
		ret = -ENOMEM;
		goto err_zero_lv2;
	}

	pgtable_flush(zero_lv2_table, zero_lv2_table + NUM_LV2ENTRIES);

	return 0;

err_zero_lv2:
	bus_set_iommu(&platform_bus_type, NULL);
err_set_iommu:
	__free_page(page);
err_fault_page:
	kmem_cache_destroy(lv2table_kmem_cache);
	return ret;
}
arch_initcall(exynos_bus_set_iommu);

static struct dentry *sysmmu_debugfs_root; /* /sys/kernel/debug/sysmmu */

static int __init exynos_iommu_init(void)
{
	sysmmu_debugfs_root = debugfs_create_dir("sysmmu", NULL);
	if (!sysmmu_debugfs_root)
		pr_err("%s: Failed to create debugfs entry, 'sysmmu'\n",
							__func__);
	if (IS_ERR(sysmmu_debugfs_root))
		sysmmu_debugfs_root = NULL;
	return platform_driver_register(&exynos_sysmmu_driver);
}
subsys_initcall(exynos_iommu_init);

static int debug_string_show(struct seq_file *s, void *unused)
{
	char *str = s->private;

	seq_printf(s, "%s\n", str);

	return 0;
}

static int debug_sysmmu_list_show(struct seq_file *s, void *unused)
{
	struct sysmmu_drvdata *drvdata = s->private;
	struct platform_device *pdev = to_platform_device(drvdata->sysmmu);
	int idx, maj, min, ret;

	seq_printf(s, "SysMMU Name | Ver | SFR Base\n");

	if (pm_runtime_enabled(drvdata->sysmmu)) {
		ret = pm_runtime_get_sync(drvdata->sysmmu);
		if (ret < 0)
			return ret;
	}

	for (idx = 0; idx < drvdata->nsfrs; idx++) {
		struct resource *res;

		res = platform_get_resource(pdev, IORESOURCE_MEM, idx);
		if (!res)
			break;

		maj = __sysmmu_version(drvdata, idx, &min);

		if (maj == 0)
			seq_printf(s, "%11.s | N/A | 0x%08x\n",
					res->name, res->start);
		else
			seq_printf(s, "%11.s | %d.%d | 0x%08x\n",
					res->name, maj, min, res->start);
	}

	if (pm_runtime_enabled(drvdata->sysmmu))
		pm_runtime_put(drvdata->sysmmu);

	return 0;
}

static int debug_next_sibling_show(struct seq_file *s, void *unused)
{
	struct device *dev = s->private;

	if (dev->parent &&
		!strncmp(dev_name(dev->parent),
			MODULE_NAME, strlen(MODULE_NAME)))
		seq_printf(s, "%s\n", dev_name(dev->parent));
	return 0;
}

static int __show_master(struct device *dev, void *data)
{
	struct seq_file *s = data;

	if (strncmp(dev_name(dev), MODULE_NAME, strlen(MODULE_NAME)))
		seq_printf(s, "%s\n", dev_name(dev));
	return 0;
}

static int debug_master_show(struct seq_file *s, void *unused)
{
	struct device *dev = s->private;

	device_for_each_child(dev, s, __show_master);
	return 0;
}

static int debug_string_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_string_show, inode->i_private);
}

static int debug_sysmmu_list_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_sysmmu_list_show, inode->i_private);
}

static int debug_next_sibling_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_next_sibling_show, inode->i_private);
}

static int debug_master_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_master_show, inode->i_private);
}

static const struct file_operations debug_string_fops = {
	.open = debug_string_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations debug_sysmmu_list_fops = {
	.open = debug_sysmmu_list_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations debug_next_sibling_fops = {
	.open = debug_next_sibling_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations debug_master_fops = {
	.open = debug_master_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void __init __create_debugfs_entry(struct sysmmu_drvdata *drvdata)
{
	if (!sysmmu_debugfs_root)
		return;

	drvdata->debugfs_root = debugfs_create_dir(dev_name(drvdata->sysmmu),
							sysmmu_debugfs_root);
	if (!drvdata->debugfs_root)
		dev_err(drvdata->sysmmu, "Failed to create debugfs dentry\n");
	if (IS_ERR(drvdata->debugfs_root))
		drvdata->debugfs_root = NULL;

	if (!drvdata->debugfs_root)
		return;

	if (!debugfs_create_u32("enable", 0664, drvdata->debugfs_root,
						&drvdata->activations))
		dev_err(drvdata->sysmmu,
				"Failed to create debugfs file 'enable'\n");

	if (!debugfs_create_x32("pagetable", 0664, drvdata->debugfs_root,
						(u32 *)&drvdata->pgtable))
		dev_err(drvdata->sysmmu,
				"Failed to create debugfs file 'pagetable'\n");

	if (!debugfs_create_file("name", 0444, drvdata->debugfs_root,
					drvdata->dbgname, &debug_string_fops))
		dev_err(drvdata->sysmmu,
				"Failed to create debugfs file 'name'\n");

	if (!debugfs_create_file("sysmmu_list", 0444, drvdata->debugfs_root,
					drvdata, &debug_sysmmu_list_fops))
		dev_err(drvdata->sysmmu,
			"Failed to create debugfs file 'sysmmu_list'\n");

	if (!debugfs_create_file("next_sibling", 0x444, drvdata->debugfs_root,
				drvdata->sysmmu, &debug_next_sibling_fops))
		dev_err(drvdata->sysmmu,
			"Failed to create debugfs file 'next_siblings'\n");

	if (!debugfs_create_file("master", 0x444, drvdata->debugfs_root,
				drvdata->sysmmu, &debug_master_fops))
		dev_err(drvdata->sysmmu,
			"Failed to create debugfs file 'next_siblings'\n");
}
