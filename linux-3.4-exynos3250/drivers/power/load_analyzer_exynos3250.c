/* drivers/power/load_analyzer_exynos3250.c */

#include <linux/trm.h>

unsigned int get_bimc_clk(void);
unsigned int get_snoc_clk(void);
extern int mali_gpu_clk;
extern int mali_dvfs_utilization;

void store_cpu_load(unsigned int cpufreq[], unsigned int cpu_load[])
{
	unsigned int j = 0, cnt = 0;
	unsigned long long t, t_interval;
	unsigned int  t_interval_us;
	static unsigned long long before_t;
	unsigned long  nanosec_rem;
	int cpu_max = 0, cpu_min = 0;
	struct cpufreq_policy *policy;
#if defined(CONFIG_SLP_BUSY_LEVEL)
    static int cpu_busy_level_before = 0;
#endif

	if (cpu_task_history_onoff == 0)
		return;

	if (++cpu_load_freq_history_cnt >= cpu_load_history_num)
		cpu_load_freq_history_cnt = 0;

	cnt = cpu_load_freq_history_cnt;

	policy = cpufreq_cpu_get(0);

	if (policy !=NULL) {
		cpu_min = pm_qos_request(PM_QOS_CPU_FREQ_MIN);
		if (cpu_min < policy->min)
			cpu_min = policy->min;

		cpu_max = pm_qos_request(PM_QOS_CPU_FREQ_MAX);
		if (cpu_max > policy->max)
			cpu_max = policy->max;

		cpufreq_cpu_put(policy);
	}

	cpu_load_freq_history[cnt].cpu_min_locked_freq = cpu_min;
	cpu_load_freq_history[cnt].cpu_max_locked_freq = cpu_max;

	cpu_load_freq_history[cnt].cpu_min_locked_online = pm_qos_request(PM_QOS_CPU_ONLINE_MIN);
	cpu_load_freq_history[cnt].cpu_max_locked_online = pm_qos_request(PM_QOS_CPU_ONLINE_MAX);

	t = cpu_clock(UINT_MAX);

	if (before_t == 0)
		before_t = t;

	for (j = 0; j < CPU_NUM; j++) {
		cpu_load_freq_history[cnt].task_history_cnt[j]
			= cpu_task_history_cnt[j];
	}

#if defined (CONFIG_CHECK_WORK_HISTORY)
	for (j = 0; j < CPU_NUM; j++) {
		cpu_load_freq_history[cnt].work_history_cnt[j]
			= cpu_work_history_cnt[j];
	}
#endif

#if defined (CONFIG_SLP_INPUT_REC)
	cpu_load_freq_history[cnt].input_rec_history_cnt
		= input_rec_history_cnt;
#endif

	t_interval = t -before_t;
	do_div(t_interval, 1000);
	t_interval_us = t_interval;
	before_t = t;

	if (t_interval != 0) {
		cpu_load_freq_history[cnt].cpu_idle_time[0]
			= (cpuidle_get_idle_residency_time(0) * 1000 + 5) / t_interval_us;
		cpu_load_freq_history[cnt].cpu_idle_time[1]
			= (cpuidle_get_idle_residency_time(1) * 1000 +5) / t_interval_us;
		cpu_load_freq_history[cnt].cpu_idle_time[2]
			= (cpuidle_get_idle_residency_time(2) * 1000 +5) / t_interval_us;
	}

	if (saved_load_factor.suspend_state == 1)
		cpu_load_freq_history[cnt].status = 'S';
	else if (saved_load_factor.suspend_state == 0)
		cpu_load_freq_history[cnt].status = 'N';
	else if (saved_load_factor.suspend_state == -1)
		cpu_load_freq_history[cnt].status = 'F';

	cpu_load_freq_history[cnt].time_stamp = t;
	nanosec_rem = do_div(t, 1000000000);
	snprintf(cpu_load_freq_history[cnt].time, sizeof(cpu_load_freq_history[cnt].time),
		"%2lu.%02lu", (unsigned long) t,(unsigned long)nanosec_rem / 10000000);

	for (j = 0; j < CPU_NUM; j++) {
		cpu_load_freq_history[cnt].cpufreq[j] = cpufreq[j];
		cpu_load_freq_history[cnt].cpu_load[j] = cpu_load[j];
	}
	cpu_load_freq_history[cnt].touch_event = 0;
	cpu_load_freq_history[cnt].nr_onlinecpu = num_online_cpus();
	cpu_load_freq_history[cnt].nr_run_avg
		= saved_load_factor.nr_running_task;

	cpu_load_freq_history[cnt].lcd_brightness = saved_load_factor.lcd_brightness;

	cpu_load_freq_history[cnt].suspend_count = saved_load_factor.suspend_count;

#if defined(CONFIG_SLP_CHECK_BUS_LOAD)
	cpu_load_freq_history[cnt].nr_run_avg
					= saved_load_factor.nr_running_task;

	cpu_load_freq_history[cnt].mif_bus_freq
					= saved_load_factor.mif_bus_freq;
	cpu_load_freq_history[cnt].int_bus_freq
					= saved_load_factor.int_bus_freq;
	cpu_load_freq_history[cnt].mif_bus_load
					= saved_load_factor.mif_bus_load;
	cpu_load_freq_history[cnt].int_bus_load
					= saved_load_factor.int_bus_load;

	cpu_load_freq_history[cnt].gpu_freq
					= mali_gpu_clk;
	cpu_load_freq_history[cnt].gpu_utilization
					= mali_dvfs_utilization;
#endif

#if defined(CONFIG_SLP_CHECK_RESOURCE)
	cpu_load_freq_history[cnt].bt_tx_bytes
		= saved_load_factor.bt_tx_bytes;
	saved_load_factor.bt_tx_bytes = 0;

	cpu_load_freq_history[cnt].bt_rx_bytes
		= saved_load_factor.bt_rx_bytes;
	saved_load_factor.bt_rx_bytes = 0;

	cpu_load_freq_history[cnt].bt_enabled
		= saved_load_factor.bt_enabled;

	cpu_load_freq_history[cnt].wifi_tx_bytes
		= saved_load_factor.wifi_tx_bytes;
	saved_load_factor.wifi_tx_bytes = 0;

	cpu_load_freq_history[cnt].wifi_rx_bytes
		= saved_load_factor.wifi_rx_bytes;
	saved_load_factor.wifi_rx_bytes = 0;

	cpu_load_freq_history[cnt].wifi_enabled
		= saved_load_factor.wifi_enabled;

#endif

	cpu_load_freq_history[cnt].pid = saved_load_factor.active_app_pid;
	cpu_load_freq_history[cnt].battery_soc = saved_load_factor.battery_soc;

#if defined(CONFIG_SLP_BUSY_LEVEL)
	cpu_busy_level = check_load_level(cnt);
	if (cpu_busy_level != cpu_busy_level_before) {
		pr_info("cpu_busy_level=%d->%d", cpu_busy_level_before, cpu_busy_level);
		cpu_busy_level_before = cpu_busy_level;
		update_cpu_busy_level();
	}
#endif

#if defined(CONFIG_SLP_BUS_CLK_CHECK_LOAD)
{
	unsigned int pm_domains;
	unsigned int clk_gates[CLK_GATES_NUM];
	clk_mon_power_domain(&pm_domains);
	clk_mon_clock_gate(clk_gates);

	cpu_load_freq_history[cnt].power_domains[0] = pm_domains;
	memcpy(cpu_load_freq_history[cnt].clk_gates
					, clk_gates, CLK_GATES_NUM*sizeof(unsigned int));
}
#endif

#if defined(CONFIG_SLP_CURRENT_MONITOR)
	if (current_monitor_en == 1)
		current_monitor_manager(cnt);
#endif

}

char cpu_load_freq_menu[] ="=======================================" \
			"===============================================\n" \
			"     TIME    CPU_FREQ  F_LOCK   O_LOCK  [INDEX]    " \
			"CPU0   CPU1   ONLINE   NR_RUN\n";

unsigned int show_cpu_load_freq_sub(int cnt, int show_cnt, char *buf, unsigned int buf_size, int ret)
{
	int j, delta = 0;

	if ((cnt - show_cnt) < 0) {
		delta = cnt - show_cnt;
		cnt = cpu_load_history_num + delta;
	} else
		cnt -= show_cnt;

	if ((cnt+1 >= cpu_load_history_num)
			|| (cpu_load_freq_history_view[cnt+1].time == 0))
		cnt = 0;
	else
		cnt++;

	for (j = 0; j < show_cnt; j++) {
		if (cnt > cpu_load_history_num-1)
			cnt = 0;
		ret +=  snprintf(buf + ret, PAGE_SIZE - ret
			, "%10s\t%d.%d    %d.%d/%d.%d   %d/%d    [%5d]    %3d    %3d     "
				"%3d    %3d.%02d\n"
			, cpu_load_freq_history_view[cnt].time
			, cpu_load_freq_history_view[cnt].cpufreq[0]/1000000
			, (cpu_load_freq_history_view[cnt].cpufreq[0]/100000) % 10
			, cpu_load_freq_history_view[cnt].cpu_min_locked_freq/1000000
			, (cpu_load_freq_history_view[cnt].cpu_min_locked_freq/100000) % 10
			, cpu_load_freq_history_view[cnt].cpu_max_locked_freq/1000000
			, (cpu_load_freq_history_view[cnt].cpu_max_locked_freq/100000) %10
			, cpu_load_freq_history_view[cnt].cpu_min_locked_online
			, cpu_load_freq_history_view[cnt].cpu_max_locked_online
			, cnt
			, cpu_load_freq_history_view[cnt].cpu_load[0]
			, cpu_load_freq_history_view[cnt].cpu_load[1]
			, cpu_load_freq_history_view[cnt].nr_onlinecpu
			, cpu_load_freq_history_view[cnt].nr_run_avg/100
			, cpu_load_freq_history_view[cnt].nr_run_avg%100);

		++cnt;
	}
	return ret;

}

#if defined(CONFIG_SLP_CHECK_BUS_LOAD)

char cpu_bus_load_freq_menu[] ="==============================" \
			"===========================================" \
			"============================================" \
			"===========================================\n" \
			"   TIME    STA  CPU_F   F_LOCK   O_LOCK  [INDEX]    " \
			"CPU0 ( C1/ C2/ C3)    CPU1   ONLINE    NR_RUN    MIF_F    MIF_L    INT_F   INT_L" \
			"    GPU_F  GPU_UTIL   INPUT\n";


unsigned int show_cpu_bus_load_freq_sub(int cnt, int show_cnt
					, char *buf, unsigned int buf_size, int ret)
{
	int j, delta = 0;

	if ((cnt - show_cnt) < 0) {
		delta = cnt - show_cnt;
		cnt = cpu_load_history_num + delta;
	} else
		cnt -= show_cnt;

	if ((cnt+1 >= cpu_load_history_num)
			|| (cpu_load_freq_history_view[cnt+1].time == 0))
		cnt = 0;
	else
		cnt++;

	for (j = 0; j < show_cnt; j++) {
		if (cnt > cpu_load_history_num-1)
			cnt = 0;

		ret +=  snprintf(buf + ret, buf_size - ret
			, "%8s    %c    %d.%d    %d.%d/%d.%d   %d/%d    [%5d]     %3d (%3d/%3d/%3d)    %3d     "
				"%3d     %3d.%02d      %3d      %3d      %3d     %3d      %3d     %3d"
				"     %5d\n"
			, cpu_load_freq_history_view[cnt].time
			, cpu_load_freq_history_view[cnt].status
			, cpu_load_freq_history_view[cnt].cpufreq[0]/1000000
			, (cpu_load_freq_history_view[cnt].cpufreq[0]/100000) % 10
			, cpu_load_freq_history_view[cnt].cpu_min_locked_freq/1000000
			, (cpu_load_freq_history_view[cnt].cpu_min_locked_freq/100000) % 10
			, cpu_load_freq_history_view[cnt].cpu_max_locked_freq/1000000
			, (cpu_load_freq_history_view[cnt].cpu_max_locked_freq/100000) %10
			, cpu_load_freq_history_view[cnt].cpu_min_locked_online
			, cpu_load_freq_history_view[cnt].cpu_max_locked_online
			, cnt
			, cpu_load_freq_history_view[cnt].cpu_load[0]
			, cpu_load_freq_history_view[cnt].cpu_idle_time[0]/10
			, cpu_load_freq_history_view[cnt].cpu_idle_time[1]/10
			, cpu_load_freq_history_view[cnt].cpu_idle_time[2]/10
			, cpu_load_freq_history_view[cnt].cpu_load[1]
			, cpu_load_freq_history_view[cnt].nr_onlinecpu
			, cpu_load_freq_history_view[cnt].nr_run_avg/100
			, cpu_load_freq_history_view[cnt].nr_run_avg%100
			, cpu_load_freq_history_view[cnt].mif_bus_freq/1000
			, cpu_load_freq_history_view[cnt].mif_bus_load
			, cpu_load_freq_history_view[cnt].int_bus_freq/1000
			, cpu_load_freq_history_view[cnt].int_bus_load
			, cpu_load_freq_history_view[cnt].gpu_freq
			, cpu_load_freq_history_view[cnt].gpu_utilization
			, cpu_load_freq_history_view[cnt].input_rec_history_cnt);

		++cnt;
	}
	return ret;

}
#endif

#if defined(CONFIG_SLP_BUS_CLK_CHECK_LOAD)
char cpu_bus_clk_load_freq_menu[] ="============================" \
			"============================================" \
			"============================================" \
			"============================================" \
			"============================================\n" \
			"   TIME    STA  CPU_F   F_LOCK   O_LOCK  [INDEX]    " \
			"CPU0 ( C1/ C2/ C3)    CPU1   ONLINE    NR_RUN    MIF_F    MIF_L    INT_F   INT_L" \
			"    GPU_F  GPU_UTIL   INPUT" \
			"    PD    CLK1      CLK2      CLK3      CLK4\n";


unsigned int show_cpu_bus_clk_load_freq_sub(int cnt
					, int show_cnt, char *buf, int ret)
{
	int j, delta = 0;

	if ((cnt - show_cnt) < 0) {
		delta = cnt - show_cnt;
		cnt = cpu_load_history_num + delta;
	} else
		cnt -= show_cnt;

	if ((cnt+1 >= cpu_load_history_num)
			|| (cpu_load_freq_history_view[cnt+1].time == 0))
		cnt = 0;
	else
		cnt++;

	for (j = 0; j < show_cnt; j++) {

		if (cnt > cpu_load_history_num-1)
			cnt = 0;

		if (ret < PAGE_SIZE - 1) {
		ret +=  snprintf(buf + ret, PAGE_SIZE - ret
			, "%8s    %c    %d.%d    %d.%d/%d.%d   %d/%d    [%5d]     %3d (%3d/%3d/%3d)    %3d     "
				"%3d     %3d.%02d      %3d      %3d      %3d     %3d      %3d     %3d"
				"     %5d   %4X  %8X  %8X  %8X  %8X\n"
			, cpu_load_freq_history_view[cnt].time
			, cpu_load_freq_history_view[cnt].status
			, cpu_load_freq_history_view[cnt].cpufreq[0]/1000000
			, (cpu_load_freq_history_view[cnt].cpufreq[0]/100000) % 10
			, cpu_load_freq_history_view[cnt].cpu_min_locked_freq/1000000
			, (cpu_load_freq_history_view[cnt].cpu_min_locked_freq/100000) % 10
			, cpu_load_freq_history_view[cnt].cpu_max_locked_freq/1000000
			, (cpu_load_freq_history_view[cnt].cpu_max_locked_freq/100000) %10
			, cpu_load_freq_history_view[cnt].cpu_min_locked_online
			, cpu_load_freq_history_view[cnt].cpu_max_locked_online
			, cnt
			, cpu_load_freq_history_view[cnt].cpu_load[0]
			, cpu_load_freq_history_view[cnt].cpu_idle_time[0]/10
			, cpu_load_freq_history_view[cnt].cpu_idle_time[1]/10
			, cpu_load_freq_history_view[cnt].cpu_idle_time[2]/10
			, cpu_load_freq_history_view[cnt].cpu_load[1]
			, cpu_load_freq_history_view[cnt].nr_onlinecpu
			, cpu_load_freq_history_view[cnt].nr_run_avg/100
			, cpu_load_freq_history_view[cnt].nr_run_avg%100
			, cpu_load_freq_history_view[cnt].mif_bus_freq/1000
			, cpu_load_freq_history_view[cnt].mif_bus_load
			, cpu_load_freq_history_view[cnt].int_bus_freq/1000
			, cpu_load_freq_history_view[cnt].int_bus_load
			, cpu_load_freq_history_view[cnt].gpu_freq
			, cpu_load_freq_history_view[cnt].gpu_utilization
			, cpu_load_freq_history_view[cnt].input_rec_history_cnt
			, cpu_load_freq_history_view[cnt].power_domains[0]
			, cpu_load_freq_history_view[cnt].clk_gates[0]
			, cpu_load_freq_history_view[cnt].clk_gates[1]
			, cpu_load_freq_history_view[cnt].clk_gates[2]
			, cpu_load_freq_history_view[cnt].clk_gates[3]
			);
		} else
			break;
		++cnt;
	}

	return ret;

}
#endif

#if defined(CONFIG_CHECK_NOT_CPUIDLE_CAUSE)
static int not_lpa_cause_check_sub(char *buf, int buf_size)
{
	int ret = 0;

	ret += snprintf(buf + ret, buf_size - ret, "%s\n",  get_not_w_aftr_cause());

	return ret;
}
#endif

#if defined(CONFIG_SLP_CURRENT_MONITOR)
unsigned int show_current_monitor_read_sub(int cnt, int show_cnt
					, char *buf, unsigned int buf_size, int ret)
{
	int j, delta = 0;

	if ((cnt - show_cnt) < 0) {
		delta = cnt - show_cnt;
		cnt = cpu_load_history_num + delta;
	} else
		cnt -= show_cnt;

	if ((cnt+1 >= cpu_load_history_num)
			|| (cpu_load_freq_history[cnt+1].time[0] == '\0'))
		cnt = 0;
	else
		cnt++;

	for (j = 0; j < show_cnt; j++) {
		char task_name[TASK_COMM_LEN]={0,};

		if (cnt > cpu_load_history_num-1)
			cnt = 0;

		get_name_from_pid(task_name, cpu_load_freq_history[cnt].pid);

		ret +=  snprintf(buf + ret, PAGE_SIZE - ret
			, "%10s %16s %5d %1c %3d  %d.%d  %d.%d %d.%d    %d  %3d  %3d %3d %3d   %3d"
				"  %3d.%02d   %3d %3d  %3d %3d  %3d %3d  %3d"
				"      %d %5d %5d  %d %5d %5d\n"
			, cpu_load_freq_history[cnt].time
			, task_name
			, cpu_load_freq_history[cnt].suspend_count
			, cpu_load_freq_history[cnt].status
			, cpu_load_freq_history[cnt].battery_soc
			, cpu_load_freq_history[cnt].cpufreq[0]/1000000
			, (cpu_load_freq_history[cnt].cpufreq[0]/100000) % 10
			, cpu_load_freq_history[cnt].cpu_min_locked_freq/1000000
			, (cpu_load_freq_history[cnt].cpu_min_locked_freq/100000) % 10
			, cpu_load_freq_history[cnt].cpu_max_locked_freq/1000000
			, (cpu_load_freq_history[cnt].cpu_max_locked_freq/100000) %10
			, cpu_load_freq_history[cnt].nr_onlinecpu
			, cpu_load_freq_history[cnt].cpu_load[0]
			, cpu_load_freq_history[cnt].cpu_idle_time[0]/10
			, cpu_load_freq_history[cnt].cpu_idle_time[1]/10
			, cpu_load_freq_history[cnt].cpu_idle_time[2]/10
			, cpu_load_freq_history[cnt].cpu_load[1]
			, cpu_load_freq_history[cnt].nr_run_avg/100
			, cpu_load_freq_history[cnt].nr_run_avg%100
			, cpu_load_freq_history[cnt].mif_bus_freq/1000
			, cpu_load_freq_history[cnt].mif_bus_load
			, cpu_load_freq_history[cnt].int_bus_freq/1000
			, cpu_load_freq_history[cnt].int_bus_load
			, cpu_load_freq_history[cnt].gpu_freq
			, cpu_load_freq_history[cnt].gpu_utilization
			, cpu_load_freq_history[cnt].lcd_brightness
			, cpu_load_freq_history[cnt].bt_enabled
			, cpu_load_freq_history[cnt].bt_tx_bytes
			, cpu_load_freq_history[cnt].bt_rx_bytes
			, cpu_load_freq_history[cnt].wifi_enabled
			, cpu_load_freq_history[cnt].wifi_tx_bytes
			, cpu_load_freq_history[cnt].wifi_rx_bytes);


		++cnt;
	}
	return ret;

}
#endif


#if defined (CONFIG_SLP_CPU_TESTER)

enum {
	CPUIDLE_C1,
	CPUIDLE_C2,
	CPUIDLE_C3,
	CPUIDLE_RANDOM,
};

enum {
	CPUFREQ_1200M,
	CPUFREQ_1100M,
	CPUFREQ_1000M,
	CPUFREQ_800M,
	CPUFREQ_533M,
	CPUFREQ_400M,
	CPUFREQ_200M,
	CPUFREQ_RANDOM,
};



struct cpu_test_list_tag cpu_idletest_list[] = {
	{CPU_IDLE_TEST, CPUIDLE_C1, 1000},

	{END_OF_LIST, 0, 0},
};

struct cpu_test_list_tag cpu_freqtest_list[] = {
	{CPU_FREQ_TEST, CPUFREQ_1200M, 1000},
	{CPU_FREQ_TEST, CPUFREQ_1100M, 1000},
	{CPU_FREQ_TEST, CPUFREQ_1000M, 1000},
	{CPU_FREQ_TEST, CPUFREQ_800M, 1000},
	{CPU_FREQ_TEST, CPUFREQ_533M, 1000},
	{CPU_FREQ_TEST, CPUFREQ_400M, 1000},
	{CPU_FREQ_TEST, CPUFREQ_200M, 1000},

	{END_OF_LIST, 0, 0},
};

struct cpu_test_freq_table_tag cpu_test_freq_table[] = {
	{CPUFREQ_1200M},
	{CPUFREQ_1100M},
	{CPUFREQ_1000M},
	{CPUFREQ_800M},
	{CPUFREQ_533M},
	{CPUFREQ_400M},
	{CPUFREQ_200M},
};

struct cpu_test_idle_table_tag cpu_test_idle_table[] ={
	{CPUIDLE_C1},
	{CPUIDLE_C2},
	{CPUIDLE_C3},
};

void set_cpufreq_force_state(int cpufreq_enum)
{
	switch(cpufreq_enum) {
	case CPUFREQ_1200M :
		cpufreq_force_state = 1190400;
		break;
	case CPUFREQ_1100M :
		cpufreq_force_state = 1094400;
		break;
	case CPUFREQ_1000M :
		cpufreq_force_state = 998400;
		break;
	case CPUFREQ_800M :
		cpufreq_force_state = 800000;
		break;
	case CPUFREQ_533M :
		cpufreq_force_state = 533333;
		break;
	case CPUFREQ_400M :
		cpufreq_force_state = 400000;
		break;
	case CPUFREQ_200M :
		cpufreq_force_state = 200000;
		break;
	}
}




int cpu_freq_to_enum(int cpufreq)
{
	int cpufreq_enum = -1;

	switch (cpufreq) {
	case 1190400 :
		cpufreq_enum = CPUFREQ_1200M;
		break;
	case 1094400 :
		cpufreq_enum = CPUFREQ_1100M;
		break;
	case 998400 :
		cpufreq_enum = CPUFREQ_1000M;
		break;
	case 800000 :
		cpufreq_enum = CPUFREQ_800M;
		break;
	case 533333 :
		cpufreq_enum = CPUFREQ_533M;
		break;
	case 400000 :
		cpufreq_enum = CPUFREQ_400M;
		break;
	case 200000 :
		cpufreq_enum = CPUFREQ_200M;
		break;
	}

	return cpufreq_enum;

}


void cpu_tester_enum_to_str(char *str, int type, int enum_value)
{
	if (type == CPU_FREQ_TEST) {
		switch (enum_value) {
		 	case CPUFREQ_1200M :
				strcpy(str, "1.2Ghz");
				break;
		 	case CPUFREQ_1100M :
				strcpy(str, "1.1Ghz");
				break;
		 	case CPUFREQ_1000M :
				strcpy(str, "1.0Ghz");
				break;
		 	case CPUFREQ_800M :
				strcpy(str, "800Mhz");
				break;
		 	case CPUFREQ_533M :
				strcpy(str, "533Mhz");
				break;
		 	case CPUFREQ_400M :
				strcpy(str, "400Mhz");
				break;
		 	case CPUFREQ_200M :
				strcpy(str, "200Mhz");
				break;
			case CPUFREQ_RANDOM:
				strcpy(str, "RANDOM");
				break;
		}
	}else if (type == CPU_IDLE_TEST) {
		switch (enum_value) {
		 	case CPUIDLE_C1 :
				strcpy(str, "C1");
				break;
		 	case CPUIDLE_C2 :
				strcpy(str, "C2");
				break;
		 	case CPUIDLE_C3 :
				strcpy(str, "C3");
				break;
			case CPUIDLE_RANDOM:
				strcpy(str, "RANDOM");
				break;
		}
	}

}

#endif

#if defined(CONFIG_SLP_BUSY_LEVEL)
static int cpu_busy_gpu_usage = 200;
int gpu_load_checking(unsigned int cnt)
{
	int ret = BUSY_LOAD;

	if ((cpu_load_freq_history[cnt].gpu_freq >= (133 * 1000000))
		&& (cpu_load_freq_history[cnt].gpu_utilization >= cpu_busy_gpu_usage)) {
		ret = BUSY_LOAD;
	}

	return ret;
}

int cpu_load_checking(unsigned int cnt)
{
	int ret = NOT_BUSY_LOAD;
	unsigned int high_cpufreq, high_usage;

	high_cpufreq = cpu_load_freq_history[cnt].cpufreq[0];
	high_usage = max(cpu_load_freq_history[cnt].cpu_load[0], cpu_load_freq_history[cnt].cpu_load[1]);

	if ((high_cpufreq >= 1000000)
		&& (high_usage >= 90)) {
		ret = BUSY_LOAD;
	} else if (high_usage >= 99) {
		ret = BUSY_LOAD;
	}

	return ret;
}
#endif

