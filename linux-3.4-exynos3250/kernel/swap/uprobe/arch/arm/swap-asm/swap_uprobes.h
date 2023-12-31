/**
 * @file uprobe/arch/asm-arm/swap_uprobes.h
 * @author Alexey Gerenkov <a.gerenkov@samsung.com> User-Space Probes initial
 * implementation; Support x86/ARM/MIPS for both user and kernel spaces.
 * @author Ekaterina Gorelkina <e.gorelkina@samsung.com>: redesign module for
 * separating core and arch parts
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * @section COPYRIGHT
 *
 * Copyright (C) Samsung Electronics, 2006-2010
 *
 * @section DESCRIPTION
 *
 * Arch-dependent uprobe interface declaration.
 */


#ifndef _ARM_SWAP_UPROBES_H
#define _ARM_SWAP_UPROBES_H


#include <swap-asm/swap_kprobes.h>	/* FIXME: for UPROBES_TRAMP_LEN */


struct kprobe;
struct task_struct;
struct uprobe;
struct uretprobe;
struct uretprobe_instance;

/**
 * @struct arch_specific_tramp
 * @brief Stores arch-dependent trampolines.
 */
struct arch_specific_tramp {
	unsigned long tramp_arm[UPROBES_TRAMP_LEN];     /**< ARM trampoline */
	unsigned long tramp_thumb[UPROBES_TRAMP_LEN];   /**< Thumb trampoline */
	void *utramp;                               /**< Pointer to trampoline */
};


static inline u32 swap_get_urp_float(struct pt_regs *regs)
{
	return regs->ARM_r0;
}

static inline u64 swap_get_urp_double(struct pt_regs *regs)
{

	return regs->ARM_r0 | (u64)regs->ARM_r1 << 32;
}

static inline void arch_ujprobe_return(void)
{
}

int arch_prepare_uprobe(struct uprobe *up);

int setjmp_upre_handler(struct kprobe *p, struct pt_regs *regs);
static inline int longjmp_break_uhandler(struct kprobe *p, struct pt_regs *regs)
{
	return 0;
}

void arch_opcode_analysis_uretprobe(struct uretprobe *rp);
void arch_prepare_uretprobe(struct uretprobe_instance *ri, struct pt_regs *regs);
int arch_disarm_urp_inst(struct uretprobe_instance *ri,
			 struct task_struct *task);

unsigned long arch_get_trampoline_addr(struct kprobe *p, struct pt_regs *regs);
void arch_set_orig_ret_addr(unsigned long orig_ret_addr, struct pt_regs *regs);
void arch_remove_uprobe(struct uprobe *up);

static inline unsigned long swap_get_uarg(struct pt_regs *regs, unsigned long n)
{
	u32 *ptr, addr = 0;

	switch (n) {
	case 0:
		return regs->ARM_r0;
	case 1:
		return regs->ARM_r1;
	case 2:
		return regs->ARM_r2;
	case 3:
		return regs->ARM_r3;
	}

	ptr = (u32 *)regs->ARM_sp + n - 4;
	if (get_user(addr, ptr))
		printk("failed to dereference a pointer, ptr=%p\n", ptr);

	return addr;
}

int swap_arch_init_uprobes(void);
void swap_arch_exit_uprobes(void);

#endif /* _ARM_SWAP_UPROBES_H */
