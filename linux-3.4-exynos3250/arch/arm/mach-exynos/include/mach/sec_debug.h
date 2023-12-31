#ifndef SEC_DEBUG_H
#define SEC_DEBUG_H

#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/input.h>

#ifdef CONFIG_SEC_DEBUG
#define SEC_DEBUG_NAME		"sec_debug"
#define SET_DEBUG_KEY(_key, _state) 	\
{	\
	.code = _key,	\
	.state = _state,	\
}

union sec_debug_level_t {
	struct {
		u16 kernel_fault;
		u16 user_fault;
	} en;
	u32 uint_val;
};

struct input_debug_key_state {
	bool state;
	u32 code;
};

struct input_debug_pdata {
	struct input_debug_key_state *key_state;
	int nkeys;
};

struct input_debug_drv_data {
	struct input_handler input_handler;
	struct input_debug_pdata *pdata;
	struct input_device_id input_ids[2];
	int crash_key_cnt;
	kernel_ulong_t keybit[INPUT_DEVICE_ID_KEY_MAX / BITS_PER_LONG + 1];
};

extern union sec_debug_level_t sec_debug_level;

extern int sec_debug_init(void);

extern int sec_debug_magic_init(void);

extern void sec_debug_check_crash_key(unsigned int code, int value);

extern void sec_getlog_supply_fbinfo(void *p_fb, u32 res_x, u32 res_y, u32 bpp,
				     u32 frames);
extern void sec_getlog_supply_loggerinfo(void *p_main, void *p_radio,
					 void *p_events, void *p_system);
extern void sec_getlog_supply_kloginfo(void *klog_buf);

extern void sec_gaf_supply_rqinfo(unsigned short curr_offset,
				  unsigned short rq_offset);

extern void register_log_char_hook(void (*f) (char c));

extern void sec_debug_save_context(void);

extern void sec_debug_set_upload_magic(unsigned magic, char *str);

extern int get_sec_debug_level(void);
#else

extern void register_log_char_hook(void (*f) (char c));

static inline int sec_debug_init(void)
{
	return 0;
}

static inline int sec_debug_magic_init(void)
{
	return 0;
}

static inline void sec_debug_check_crash_key(unsigned int code, int value)
{
}

static inline void sec_getlog_supply_fbinfo(void *p_fb, u32 res_x, u32 res_y,
					    u32 bpp, u32 frames)
{
}

static inline void sec_getlog_supply_meminfo(u32 size0, u32 addr0, u32 size1,
					     u32 addr1)
{
}

static inline void sec_getlog_supply_loggerinfo(void *p_main,
						void *p_radio, void *p_events,
						void *p_system)
{
}

static inline void sec_getlog_supply_kloginfo(void *klog_buf)
{
}

static inline void sec_gaf_supply_rqinfo(unsigned short curr_offset,
					 unsigned short rq_offset)
{
}

static inline void sec_debug_save_context(void)
{
}

static inline void sec_debug_set_upload_magic(unsigned magic, char *str)
{
}

static inline int get_sec_debug_level(void)
{
}
#endif

struct worker;
struct work_struct;

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
extern void __sec_debug_task_log(int cpu, struct task_struct *task, char *msg);
extern void __sec_debug_irq_log(unsigned int irq, void *fn, int en);
extern void __sec_debug_work_log(struct worker *worker,
				 struct work_struct *work, work_func_t f, int en);
#ifdef CONFIG_SEC_DEBUG_TIMER_LOG
extern void __sec_debug_timer_log(unsigned int type, void *fn);
#endif

static inline void sec_debug_task_log(int cpu, struct task_struct *task)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_task_log(cpu, task, NULL);
}

static inline void sec_debug_task_log_msg(int cpu, char *msg)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_task_log(cpu, NULL, msg);
}

static inline void sec_debug_irq_log(unsigned int irq, void *fn, int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_irq_log(irq, fn, en);
}

static inline void sec_debug_work_log(struct worker *worker,
				      struct work_struct *work, work_func_t f, int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_work_log(worker, work, f, en);
}

#ifdef CONFIG_SEC_DEBUG_TIMER_LOG
static inline void sec_debug_timer_log(unsigned int type, void *fn)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_timer_log(type, fn);
}
#else
static inline void sec_debug_timer_log(unsigned int type, void *fn)
{
}
#endif

#ifdef CONFIG_SEC_DEBUG_SOFTIRQ_LOG
static inline void sec_debug_softirq_log(unsigned int irq, void *fn, int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_irq_log(irq, fn, en);
}
#else
static inline void sec_debug_softirq_log(unsigned int irq, void *fn, int en)
{
}
#endif
#else
static inline void sec_debug_task_log(int cpu, struct task_struct *task)
{
}

static inline void sec_debug_task_log_msg(int cpu, char *msg)
{
}

static inline void sec_debug_irq_log(unsigned int irq, void *fn, int en)
{
}

static inline void sec_debug_work_log(struct worker *worker,
				      struct work_struct *work, work_func_t f, int en)
{
}

static inline void sec_debug_timer_log(unsigned int type, void *fn)
{
}

static inline void sec_debug_softirq_log(unsigned int irq, void *fn, int en)
{
}
#endif

#ifdef CONFIG_SEC_DEBUG_IRQ_EXIT_LOG
extern void sec_debug_irq_last_exit_log(void);
#else
static inline void sec_debug_irq_last_exit_log(void)
{
}
#endif

#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
extern void debug_semaphore_init(void);
extern void debug_semaphore_down_log(struct semaphore *sem);
extern void debug_semaphore_up_log(struct semaphore *sem);
extern void debug_rwsemaphore_init(void);
extern void debug_rwsemaphore_down_log(struct rw_semaphore *sem, int dir);
extern void debug_rwsemaphore_up_log(struct rw_semaphore *sem);
#define debug_rwsemaphore_down_read_log(x) \
	debug_rwsemaphore_down_log(x, READ_SEM)
#define debug_rwsemaphore_down_write_log(x) \
	debug_rwsemaphore_down_log(x, WRITE_SEM)
#else
static inline void debug_semaphore_init(void)
{
}

static inline void debug_semaphore_down_log(struct semaphore *sem)
{
}

static inline void debug_semaphore_up_log(struct semaphore *sem)
{
}

static inline void debug_rwsemaphore_init(void)
{
}

static inline void debug_rwsemaphore_down_read_log(struct rw_semaphore *sem)
{
}

static inline void debug_rwsemaphore_down_write_log(struct rw_semaphore *sem)
{
}

static inline void debug_rwsemaphore_up_log(struct rw_semaphore *sem)
{
}
#endif

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
void sched_log_clean_cache(void);
#endif

enum sec_debug_aux_log_idx {
	SEC_DEBUG_AUXLOG_CPU_CLOCK_SWITCH_CHANGE,
	SEC_DEBUG_AUXLOG_RUNTIME_PM_CHANGE,
	SEC_DEBUG_AUXLOG_PM_CHANGE,
	SEC_DEBUG_AUXLOG_NOTIFY_FAIL,
	SEC_DEBUG_AUXLOG_ITEM_MAX,
};

#ifdef CONFIG_SEC_DEBUG_AUXILIARY_LOG
extern void sec_debug_aux_log(int idx, char *fmt, ...);
#else
#define sec_debug_aux_log(idx, ...) do { } while (0)
#endif

#ifdef CONFIG_SEC_AVC_LOG
extern void sec_debug_avc_log(char *fmt, ...);
#endif
#ifdef CONFIG_SEC_DEBUG_TSP_LOG
extern void sec_debug_tsp_log(char *fmt, ...);
#endif

#if defined(CONFIG_MACH_Q1_BD)
extern int sec_debug_panic_handler_safe(void *buf);
#endif

extern void read_lcd_register(void);

#endif				/* SEC_DEBUG_H */
