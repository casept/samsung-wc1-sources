#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#include <mach/sec_debug.h>
#include <plat/map-base.h>
#include <plat/map-s5p.h>
#include <asm/mach/map.h>
#include <asm/tlbflush.h>

/*
 * Example usage: sec_log=256K@0x45000000
 * In above case, log_buf size is 256KB and its base address is
 * 0x45000000 physically. Actually, *(int *)(base - 8) is log_magic and
 * *(int *)(base - 4) is log_ptr. So we reserve (size + 8) bytes from
 * (base - 8).
 */
#define LOG_MAGIC 0x4d474f4c	/* "LOGM" */

/* These variables are also protected by logbuf_lock */
static unsigned *sec_log_ptr;
static char *sec_log_buf;
static unsigned sec_log_size;

#ifdef CONFIG_SEC_LOG_LAST_KMSG
#define LAST_LOG_BUF_SHIFT 20
static char *last_kmsg_buffer;
static unsigned last_kmsg_size;
static void __init sec_log_save_old(void);
#else
static inline void sec_log_save_old(void)
{
}
#endif

#ifdef CONFIG_SEC_AVC_LOG
static unsigned *sec_avc_log_ptr;
static char *sec_avc_log_buf;
static unsigned sec_avc_log_size;
#if 0 /* ZERO WARNING */
static struct map_desc avc_log_buf_iodesc[] __initdata = {
	{
		.virtual = (unsigned long)S3C_VA_AUXLOG_BUF,
		.type = MT_DEVICE
	}
};
#endif
#endif

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
static unsigned *sec_tsp_log_ptr;
static char *sec_tsp_log_buf;
static unsigned sec_tsp_log_size;
#endif

#ifdef CONFIG_SEC_LOG_NONCACHED
static struct map_desc log_buf_iodesc[] __initdata = {
	{
		.virtual = (unsigned long)S3C_VA_KLOG_BUF,
		.type = MT_DEVICE
	}
};
#endif

static inline void emit_sec_log_char(char c)
{
	if (sec_log_buf && sec_log_ptr) {
		sec_log_buf[*sec_log_ptr & (sec_log_size - 1)] = c;
		(*sec_log_ptr)++;
	}
}

static int __init sec_log_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long base = 0;
	unsigned *sec_log_mag;

	if (!size || size != roundup_pow_of_two(size) || *str != '@'
	    || kstrtoul(str + 1, 0, &base))
		goto out;

	base = 0x46000000;
	size = 0x200000;

	/* If we encounter any problem parsing str ... */
	if (reserve_bootmem(base - 8, size + 8, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: failed reserving size %d + 8 " \
		       "at base 0x%lx - 8\n", __func__, size, base);
		goto out;
	}

#ifdef CONFIG_SEC_LOG_NONCACHED
	log_buf_iodesc[0].pfn = __phys_to_pfn((unsigned long)base - 0x100000);
	log_buf_iodesc[0].length = (unsigned long)(size + 0x100000);
	iotable_init(log_buf_iodesc, ARRAY_SIZE(log_buf_iodesc));
	flush_tlb_all();
	sec_log_mag = (S3C_VA_KLOG_BUF + 0x100000) - 8;
	sec_log_ptr = (S3C_VA_KLOG_BUF + 0x100000) - 4;
	sec_log_buf = S3C_VA_KLOG_BUF + 0x100000;
#else
	sec_log_mag = phys_to_virt(base) - 8;
	sec_log_ptr = phys_to_virt(base) - 4;
	sec_log_buf = phys_to_virt(base);
#endif

	sec_log_size = size;
	pr_info("%s: *sec_log_mag:%x *sec_log_ptr:%x " \
		"sec_log_buf:%p sec_log_size:%d\n",
		__func__, *sec_log_mag, *sec_log_ptr, sec_log_buf,
		sec_log_size);

	if (*sec_log_mag != LOG_MAGIC) {
		pr_info("%s: no old log found\n", __func__);
		*sec_log_ptr = 0;
		*sec_log_mag = LOG_MAGIC;
	} else
		sec_log_save_old();

	register_log_char_hook(emit_sec_log_char);

	sec_getlog_supply_kloginfo(phys_to_virt(base));

out:
	return 0;
}

__setup("sec_log=", sec_log_setup);

#ifdef CONFIG_CORESIGHT_ETM
static int __init sec_etb_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long base = 0;

	/* If we encounter any problem parsing str ... */
	if (!size || size != roundup_pow_of_two(size) || *str != '@'
	    || kstrtoul(str + 1, 0, &base))
		goto out;

	if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: failed reserving size %d " \
		       "at base 0x%lx\n", __func__, size, base);
		goto out;
	}

	return 1;
out:
	return 0;
}

__setup("sec_etb=", sec_etb_setup);
#endif

#ifdef CONFIG_SEC_AVC_LOG
static int __init sec_avc_log_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long base = 0;
	unsigned *sec_avc_log_mag;
	/* If we encounter any problem parsing str ... */
	if (!size || size != roundup_pow_of_two(size) || *str != '@'
		|| kstrtoul(str + 1, 0, &base))
			goto out;

	if (reserve_bootmem(base - 8 , size + 8, BOOTMEM_EXCLUSIVE)) {
			pr_err("%s: failed reserving size %d " \
						"at base 0x%lx\n", __func__, size, base);
			goto out;
	}
	/* TODO: remap noncached area.
	avc_log_buf_iodesc[0].pfn = __phys_to_pfn((unsigned long)base);
	avc_log_buf_iodesc[0].length = (unsigned long)(size);
	iotable_init(avc_log_buf_iodesc, ARRAY_SIZE(avc_log_buf_iodesc));
	sec_avc_log_mag = S3C_VA_KLOG_BUF - 8;
	sec_avc_log_ptr = S3C_VA_AUXLOG_BUF - 4;
	sec_avc_log_buf = S3C_VA_AUXLOG_BUF;
	*/
	sec_avc_log_mag = phys_to_virt(base) - 8;
	sec_avc_log_ptr = phys_to_virt(base) - 4;
	sec_avc_log_buf = phys_to_virt(base);
	sec_avc_log_size = size;

	pr_info("%s: *sec_avc_log_ptr:%x " \
		"sec_avc_log_buf:%p sec_log_size:0x%x\n",
		__func__, *sec_avc_log_ptr, sec_avc_log_buf,
		sec_avc_log_size);

	if (*sec_avc_log_mag != LOG_MAGIC) {
		pr_info("%s: no old log found\n", __func__);
		*sec_avc_log_ptr = 0;
		*sec_avc_log_mag = LOG_MAGIC;
	}
	return 1;
out:
	return 0;
}
__setup("sec_avc_log=", sec_avc_log_setup);

#define BUF_SIZE 256
void sec_debug_avc_log(char *fmt, ...)
{
	va_list args;
	char buf[BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned int size;

	/* In case of sec_avc_log_setup is failed */
	if (!sec_avc_log_size)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = *sec_avc_log_ptr;
	size = strlen(buf);

	/* Overflow buffer size */

	if (idx + size > sec_avc_log_size - 1) {
		len = scnprintf(&sec_avc_log_buf[0],
						size + 1, "%s", buf);
		*sec_avc_log_ptr = len;
	} else {
		len = scnprintf(&sec_avc_log_buf[idx], size + 1, "%s", buf);
		*sec_avc_log_ptr += len;
	}
}
EXPORT_SYMBOL(sec_debug_avc_log);

static ssize_t sec_avc_log_write(struct file *file,
		const char __user *buf,
		size_t count, loff_t *ppos)

{
	char *page = NULL;
	ssize_t ret;
	int new_value;

	if (!sec_avc_log_buf)
		return 0;

	ret = -ENOMEM;
	if (count >= PAGE_SIZE)
		return ret;

	ret = -ENOMEM;
	page = (char *)get_zeroed_page(GFP_KERNEL);
	if (!page)
		return ret;;

	ret = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	ret = -EINVAL;
	if (sscanf(page, "%u", &new_value) != 1) {
		pr_info("%s\n", page);
		/* print avc_log to sec_avc_log_buf */
		sec_debug_avc_log(page);
	} 
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}


static ssize_t sec_avc_log_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (sec_avc_log_buf == NULL)
		return 0;

	if (pos >= *sec_avc_log_ptr)
		return 0;

	count = min(len, (size_t)(*sec_avc_log_ptr - pos));
	if (copy_to_user(buf, sec_avc_log_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static const struct file_operations avc_msg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_avc_log_read,
	.write = sec_avc_log_write,
	.llseek = generic_file_llseek,
};

static int __init sec_avc_log_late_init(void)
{
	struct proc_dir_entry *entry;
	if (sec_avc_log_buf == NULL)
		return 0;

	entry = create_proc_entry("avc_msg", S_IFREG | S_IRUGO, NULL);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	entry->proc_fops = &avc_msg_file_ops;
	entry->size = sec_avc_log_size;
	return 0;
}

late_initcall(sec_avc_log_late_init);

#endif

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
static int __init sec_tsp_log_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long base = 0;
	unsigned *sec_tsp_log_mag;
	/* If we encounter any problem parsing str ... */
	if (!size || size != roundup_pow_of_two(size) || *str != '@'
		|| kstrtoul(str + 1, 0, &base))
			goto out;

	if (reserve_bootmem(base - 8 , size + 8, BOOTMEM_EXCLUSIVE)) {
			pr_err("%s: failed reserving size %d " \
						"at base 0x%lx\n", __func__, size, base);
			goto out;
	}

	sec_tsp_log_mag = phys_to_virt(base) - 8;
	sec_tsp_log_ptr = phys_to_virt(base) - 4;
	sec_tsp_log_buf = phys_to_virt(base);
	sec_tsp_log_size = size;

	pr_info("%s: *sec_tsp_log_ptr:%x " \
		"sec_tsp_log_buf:%p sec_tsp_log_size:0x%x\n",
		__func__, *sec_tsp_log_ptr, sec_tsp_log_buf,
		sec_tsp_log_size);

	if (*sec_tsp_log_mag != LOG_MAGIC) {
		pr_info("%s: no old log found\n", __func__);
		*sec_tsp_log_ptr = 0;
		*sec_tsp_log_mag = LOG_MAGIC;
	}
	return 1;
out:
	return 0;
}
__setup("sec_tsp_log=", sec_tsp_log_setup);

static int sec_tsp_log_timestamp(unsigned int idx)
{
	/* Add the current time stamp */
	char tbuf[50];
	unsigned tlen;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = local_clock();
	nanosec_rem = do_div(t, 1000000000);
	tlen = sprintf(tbuf, "[%5lu.%06lu] ",
			(unsigned long) t,
			nanosec_rem / 1000);

	/* Overflow buffer size */
	if (idx + tlen > sec_tsp_log_size - 1) {
		tlen = scnprintf(&sec_tsp_log_buf[0],
						tlen + 1, "%s", tbuf);
		*sec_tsp_log_ptr = tlen;
	} else {
		tlen = scnprintf(&sec_tsp_log_buf[idx], tlen + 1, "%s", tbuf);
		*sec_tsp_log_ptr += tlen;
	}

	return *sec_tsp_log_ptr;
}

#define TSP_BUF_SIZE 512
void sec_debug_tsp_log(char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned int size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_log_size)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = *sec_tsp_log_ptr;
	size = strlen(buf);

	idx = sec_tsp_log_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size > sec_tsp_log_size - 1) {
		len = scnprintf(&sec_tsp_log_buf[0],
						size + 1, "%s", buf);
		*sec_tsp_log_ptr = len;
	} else {
		len = scnprintf(&sec_tsp_log_buf[idx], size + 1, "%s", buf);
		*sec_tsp_log_ptr += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_log);

static ssize_t sec_tsp_log_write(struct file *file,
					     const char __user *buf,
					     size_t count, loff_t *ppos)
{
	char *page = NULL;
	ssize_t ret;
	int new_value;

	if (!sec_tsp_log_buf)
		return 0;

	ret = -ENOMEM;
	if (count >= PAGE_SIZE)
		return ret;

	ret = -ENOMEM;
	page = (char *)get_zeroed_page(GFP_KERNEL);
	if (!page)
		return ret;;

	ret = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	ret = -EINVAL;
	if (sscanf(page, "%u", &new_value) != 1) {
		pr_info("%s\n", page);
		/* print tsp_log to sec_tsp_log_buf */
		sec_debug_tsp_log(page);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}


static ssize_t sec_tsp_log_read(struct file *file, char __user *buf,
								size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (sec_tsp_log_buf == NULL)
		return 0;

	if (pos >= *sec_tsp_log_ptr)
		return 0;

	count = min(len, (size_t)(*sec_tsp_log_ptr - pos));
	if (copy_to_user(buf, sec_tsp_log_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static const struct file_operations tsp_msg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_tsp_log_read,
	.write = sec_tsp_log_write,
	.llseek = generic_file_llseek,
};

static int __init sec_tsp_log_late_init(void)
{
	struct proc_dir_entry *entry;
	if (sec_tsp_log_buf == NULL)
		return 0;

	entry = create_proc_entry("tsp_msg", S_IFREG | S_IRUGO, NULL);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	entry->proc_fops = &tsp_msg_file_ops;
	entry->size = sec_tsp_log_size;

	return 0;
}

late_initcall(sec_tsp_log_late_init);
#endif


#ifdef CONFIG_SEC_LOG_LAST_KMSG
static void __init sec_log_save_old(void)
{
	/* provide previous log as last_kmsg */
	last_kmsg_size =
	    min((unsigned)(1 << LAST_LOG_BUF_SHIFT), *sec_log_ptr);
	last_kmsg_buffer = (char *)alloc_bootmem(last_kmsg_size);

	if (last_kmsg_size && last_kmsg_buffer) {
		unsigned i;
		for (i = 0; i < last_kmsg_size; i++)
			last_kmsg_buffer[i] =
			    sec_log_buf[(*sec_log_ptr - last_kmsg_size +
					 i) & (sec_log_size - 1)];

		pr_info("%s: saved old log at %d@%p\n",
			__func__, last_kmsg_size, last_kmsg_buffer);
	} else
		pr_err("%s: failed saving old log %d@%p\n",
		       __func__, last_kmsg_size, last_kmsg_buffer);
}

static ssize_t sec_log_read_old(struct file *file, char __user *buf,
				size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_kmsg_size)
		return 0;

	count = min(len, (size_t) (last_kmsg_size - pos));
	if (copy_to_user(buf, last_kmsg_buffer + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations last_kmsg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_log_read_old,
};

static int __init sec_log_late_init(void)
{
	struct proc_dir_entry *entry;

	if (last_kmsg_buffer == NULL)
		return 0;

	entry = create_proc_entry("last_kmsg", S_IFREG | S_IRUSR | S_IRGRP, NULL);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	entry->proc_fops = &last_kmsg_file_ops;
	entry->size = last_kmsg_size;
	return 0;
}

late_initcall(sec_log_late_init);
#endif

#ifdef CONFIG_SEC_DEBUG_TIMA_LOG
#ifdef CONFIG_TIMA_RKP
#define TIMA_DEBUG_LOG_START	0x50300000
#define TIMA_DEBUG_LOG_SIZE	(1 << 20)

#define TIMA_SEC_LOG		0x4d800000
#define TIMA_SEC_LOG_SIZE	(256 << 10)

#define TIMA_PHYS_MAP		0x4d900000
#define TIMA_PHYS_MAP_SIZE	(2 << 20)

#define TIMA_SEC_TO_PGT		0x4e000000
#define TIMA_SEC_TO_PGT_SIZE	(1 << 20)

#define TIMA_DASHBOARD_START	0x4d700000
#define TIMA_DASHBOARD_SIZE	(4 << 10)

static int  tima_setup_rkp_mem(void)
{
	if (reserve_bootmem(TIMA_DEBUG_LOG_START, TIMA_DEBUG_LOG_SIZE, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: RKP failed reserving size %d at base 0x%x\n",
			__func__, TIMA_DEBUG_LOG_SIZE, TIMA_DEBUG_LOG_START);
		goto out;
	}
	pr_info("RKP :%s, base:%x, size:%x \n",
			__func__, TIMA_DEBUG_LOG_START, TIMA_DEBUG_LOG_SIZE);

	if (reserve_bootmem(TIMA_SEC_TO_PGT, TIMA_SEC_TO_PGT_SIZE, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: RKP failed reserving size %d at base 0x%x\n",
			__func__, TIMA_SEC_TO_PGT_SIZE, TIMA_SEC_TO_PGT);
		goto out;
	}
	pr_info("RKP :%s, base:%x, size:%x \n",
			__func__, TIMA_SEC_TO_PGT, TIMA_SEC_TO_PGT_SIZE);


	if (reserve_bootmem(TIMA_SEC_LOG, TIMA_SEC_LOG_SIZE, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: RKP failed reserving size %d at base 0x%x\n",
			__func__, TIMA_SEC_LOG_SIZE, TIMA_SEC_LOG);
		goto out;
	}
	pr_info("RKP :%s, base:%x, size:%x \n",
			__func__, TIMA_SEC_LOG, TIMA_SEC_LOG_SIZE);

	if (reserve_bootmem(TIMA_PHYS_MAP,  TIMA_PHYS_MAP_SIZE, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: RKP failed reserving size %d at base 0x%x\n",
			__func__, TIMA_PHYS_MAP_SIZE, TIMA_PHYS_MAP);
		goto out;
	}
	pr_info("RKP :%s, base:%x, size:%x \n",
			__func__, TIMA_PHYS_MAP, TIMA_PHYS_MAP_SIZE);

	if (reserve_bootmem(TIMA_DASHBOARD_START,  TIMA_DASHBOARD_SIZE, BOOTMEM_EXCLUSIVE)) {
		pr_err("%s: RKP failed reserving size %d at base 0x%x\n",
			__func__, TIMA_DASHBOARD_SIZE, TIMA_DASHBOARD_START);
		goto out;
	}
	pr_info("RKP :%s, base:%x, size:%x \n",
			__func__, TIMA_DASHBOARD_START, TIMA_DASHBOARD_SIZE);

	return 1;
 out:
	return 0;
}
#else
static int tima_setup_rkp_mem(void){
	return 1;
}
#endif

static int __init sec_tima_log_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long base = 0;
	/* If we encounter any problem parsing str ... */
	if (!size || size != roundup_pow_of_two(size) || *str != '@'
		|| kstrtoul(str + 1, 0, &base))
			goto out;

	if (reserve_bootmem(base , size, BOOTMEM_EXCLUSIVE)) {
			pr_err("%s: failed reserving size %d " \
						"at base 0x%lx\n", __func__, size, base);
			goto out;
	}
	pr_info("tima :%s, base:%lx, size:%x \n", __func__,base, size);

	if (!tima_setup_rkp_mem())
		goto out;

	return 1;
out:
	return 0;
}
__setup("sec_tima_log=", sec_tima_log_setup);
#endif
