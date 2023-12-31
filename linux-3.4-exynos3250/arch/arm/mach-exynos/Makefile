# arch/arm/mach-exynos/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core

obj-$(CONFIG_ARCH_EXYNOS)	+= common.o
ifneq ($(CONFIG_SOC_EXYNOS4415),y)
ifneq ($(CONFIG_SOC_EXYNOS4270),y)
obj-$(CONFIG_ARCH_EXYNOS4)	+= asv.o asv-4x12.o
else
obj-$(CONFIG_ARCH_EXYNOS4)	+= clock-exynos4.o asv.o asv-4x12.o
endif
endif
obj-$(CONFIG_ARM_TRUSTZONE)	+= irq-sgi.o
obj-$(CONFIG_SOC_EXYNOS3250)	+= clock-exynos3250.o asv-exynos.o asv-exynos3250.o
obj-$(CONFIG_CPU_EXYNOS4210)	+= clock-exynos4210.o
obj-$(CONFIG_SOC_EXYNOS3470)	+= clock-exynos3470.o asv-exynos.o asv-exynos3470.o
obj-$(CONFIG_SOC_EXYNOS4212)	+= clock-exynos4212.o
obj-$(CONFIG_SOC_EXYNOS4415)	+= clock-exynos4415.o asv-exynos.o asv-exynos4415.o
obj-$(CONFIG_SOC_EXYNOS5250)	+= clock-exynos5250.o
obj-$(CONFIG_SOC_EXYNOS5260)	+= clock-exynos5260.o asv-exynos.o asv-exynos5260.o
obj-$(CONFIG_SOC_EXYNOS5410)	+= clock-exynos5410.o asv-exynos.o asv-exynos5410.o cci.o exynos-power-mode.o
obj-$(CONFIG_SOC_EXYNOS5420)	+= clock-exynos5420.o asv-exynos.o asv-exynos5420.o cci.o
obj-$(CONFIG_SOC_EXYNOS5260)	+= cci.o asv-exynos.o
obj-$(CONFIG_SOC_EXYNOS5410)	+= exynos-interface.o
obj-$(CONFIG_SOC_EXYNOS3470)	+= pmu-cp.o
obj-$(CONFIG_BL_SWITCHER)	+= sys-if-switcher.o
obj-$(CONFIG_EXYNOS5_MP)	+= core_sysfs.o

obj-$(CONFIG_EXYNOS4_BTS)	+= bts-exynos3470.o
obj-$(CONFIG_EXYNOS5_DEV_BTS)		+= dev-bts.o
obj-$(CONFIG_EXYNOS5410_BTS)		+= bts-exynos5410.o
obj-$(CONFIG_EXYNOS5420_BTS)		+= bts-exynos5420.o
obj-$(CONFIG_EXYNOS5260_BTS)		+= bts-exynos5260.o

# SOC EXYNOS 3470
ifeq ($(CONFIG_SOC_EXYNOS3470),y)
obj-$(CONFIG_PM)			+= pm-exynos3470.o
else ifeq ($(CONFIG_SOC_EXYNOS3250),y)
obj-$(CONFIG_PM)			+= pm-exynos3250.o

# SOC EXYNOS 4415
else ifeq ($(CONFIG_SOC_EXYNOS4415),y)
obj-$(CONFIG_PM)			+= pm-exynos4415.o

else
obj-$(CONFIG_EXYNOS_RCG)		+= rcg-exynos.o
obj-$(CONFIG_PM)			+= pm.o
endif

ifeq ($(CONFIG_SOC_EXYNOS5260),y)
obj-$(CONFIG_PM_GENERIC_DOMAINS)	+= pm_domains_v2.o pm_domains-exynos5260.o
else ifeq ($(CONFIG_SOC_EXYNOS3470),y)
obj-$(CONFIG_PM_GENERIC_DOMAINS)	+= pm_domains3470.o pm_domains4.o
else ifeq ($(CONFIG_SOC_EXYNOS3250),y)
obj-$(CONFIG_PM_GENERIC_DOMAINS)	+= pm_domains.o pm_domains-exynos3250.o pm_interrupt_domains.o
else
ifeq ($(CONFIG_SOC_EXYNOS4415),y)
obj-$(CONFIG_PM_GENERIC_DOMAINS)	+= pm_domains_v2.o pm_domains-exynos4415.o
else
obj-$(CONFIG_PM_GENERIC_DOMAINS)	+= pm_domains.o pm_domains5.o pm_interrupt_domains.o
endif
endif
ifeq ($(CONFIG_SOC_EXYNOS3250),y)
obj-$(CONFIG_PM_RUNTIME_TEST_SYSFS)	+= pm_domains_sysfs.o
else
obj-$(CONFIG_PM_RUNTIME_TEST_SYSFS) += pm_domains_v2_sysfs.o
endif

ifeq ($(CONFIG_SOC_EXYNOS5250),y)
obj-$(CONFIG_CPU_IDLE)         += cpuidle-exynos5250.o
else ifeq ($(CONFIG_SOC_EXYNOS3470),y)
obj-$(CONFIG_CPU_IDLE)         += exynos3470-cpuidle.o
else ifeq ($(CONFIG_SOC_EXYNOS3250),y)
obj-$(CONFIG_CPU_IDLE)         += cpuidle-exynos3250.o
else ifeq ($(CONFIG_SOC_EXYNOS4415),y)
obj-$(CONFIG_CPU_IDLE)         += cpuidle-exynos4415.o
else
obj-$(CONFIG_CPU_IDLE)         += cpuidle.o
obj-$(CONFIG_ARCH_EXYNOS5)	+= exynos-pm.o
endif

obj-$(CONFIG_SOC_EXYNOS5250)	+= ori-asv-exynos.o ori-abb-exynos.o ori-asv-exynos5250.o

ifeq ($(CONFIG_SOC_EXYNOS3470),y)
obj-$(CONFIG_ARCH_EXYNOS)	+= pmu-exynos3470.o
else ifeq ($(CONFIG_SOC_EXYNOS3250),y)
obj-$(CONFIG_ARCH_EXYNOS)	+= pmu-exynos3250.o
else
obj-$(CONFIG_ARCH_EXYNOS)	+= pmu.o
endif

obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o

obj-$(CONFIG_EXYNOS4_MCT)	+= mct.o

obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o

ifeq ($(CONFIG_SOC_EXYNOS5260),y)
obj-y				+= clock-audss-exynos5260.o
else
obj-$(CONFIG_ARCH_EXYNOS)	+= clock-audss.o
endif

obj-$(CONFIG_EXYNOS_FIQ_DEBUGGER)	+= exynos_fiq_debugger.o

obj-$(CONFIG_EXYNOS_BUSFREQ_OPP)     += ppmu.o busfreq_opp_exynos4.o busfreq_opp_4x12.o

obj-$(CONFIG_EXYNOS5_CORESIGHT)		+= coresight-exynos5.o
obj-$(CONFIG_EXYNOS_PERSISTENT_CLOCK)	+= persistent_clock.o

obj-$(CONFIG_EXYNOS54XX_DEBUG)		+= debug_exynos54XX.o

obj-$(CONFIG_ARM_TRUSTZONE)		+= smc.o
plus_sec := $(call as-instr,.arch_extension sec,+sec)
AFLAGS_smc.o :=-Wa,-march=armv7-a$(plus_sec)

# machine support

obj-$(CONFIG_MACH_SMDKC210)		+= mach-smdkv310.o
obj-$(CONFIG_MACH_SMDKV310)		+= mach-smdkv310.o
obj-$(CONFIG_MACH_ARMLEX4210)		+= mach-armlex4210.o
obj-$(CONFIG_MACH_UNIVERSAL_C210)	+= mach-universal_c210.o
obj-$(CONFIG_MACH_NURI)			+= mach-nuri.o
obj-$(CONFIG_MACH_ORIGEN)		+= mach-origen.o

obj-$(CONFIG_MACH_ESPRESSO3250)		+= mach-espresso3250.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-clock.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-mmc.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-power.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-input.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-display.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-usb.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-media.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-audio.o
obj-$(CONFIG_MACH_ESPRESSO3250)		+= board-espresso3250-camera.o

obj-$(CONFIG_MACH_UNIVERSAL3250)	+= mach-universal3250.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-gpio.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-power.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-input.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-display.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-usb.o
obj-$(CONFIG_MACH_UNIVERSAL3250)	+= board-universal3250-media.o

obj-$(CONFIG_MACH_B2)	+= mach-universal3250.o
obj-$(CONFIG_MACH_B2)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_B2)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_B2)	+= board-b2-power.o
obj-$(CONFIG_MACH_B2)	+= board-b2-input.o
obj-$(CONFIG_MACH_B2)	+= board-b2-vibrator.o
obj-$(CONFIG_MACH_B2)	+= board-tizen-display.o
obj-$(CONFIG_MACH_B2)	+= board-b2-usb.o
obj-$(CONFIG_MACH_B2)	+= board-b2-mfd.o
obj-$(CONFIG_MACH_B2)	+= board-b2-audio.o
obj-$(CONFIG_MACH_B2)	+= board-b2-sensor.o
obj-$(CONFIG_MACH_B2)	+= board-universal3250-media.o
obj-$(CONFIG_MACH_B2)	+= board-b2-camera.o
obj-$(CONFIG_MACH_B2)	+= board-b2-thermistor.o
obj-$(CONFIG_MACH_B2)	+= board-b2-fpga.o
obj-$(CONFIG_MACH_B2)	+= sec_reboot.o
obj-$(CONFIG_MACH_B2)	+= board-universal3250-gpio.o

obj-$(CONFIG_MACH_WINGTIP)	+= mach-universal3250.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-w1-power.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-w1-input.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-vibrator.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-w1-display.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-usb.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-mfd.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-sensor.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-universal3250-media.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-camera.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-thermistor.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-b2-fpga.o
obj-$(CONFIG_MACH_WINGTIP)	+= sec_reboot.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-w1-gpio.o

obj-$(CONFIG_MACH_BLINK)	+= mach-universal3250.o
obj-$(CONFIG_MACH_BLINK)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_BLINK)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_BLINK)	+= board-blink-power.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-input.o
#obj-$(CONFIG_MACH_BLINK)	+= board-b2-vibrator.o
obj-$(CONFIG_MACH_BLINK)	+= board-tizen-display.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-usb.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-mfd.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-sensor.o
obj-$(CONFIG_MACH_BLINK)	+= board-universal3250-media.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-camera.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-thermistor.o
obj-$(CONFIG_MACH_BLINK)	+= board-b2-fpga.o
obj-$(CONFIG_MACH_BLINK)	+= sec_reboot.o
obj-$(CONFIG_MACH_BLINK)	+= board-universal3250-gpio.o
obj-$(CONFIG_MACH_BLINK)	+= board-exynos3-i2s-dummy-audio.o

obj-$(CONFIG_MACH_SLIM)	+= mach-universal3250.o
obj-$(CONFIG_MACH_SLIM)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_SLIM)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_SLIM)	+= board-slim-power.o
obj-$(CONFIG_MACH_SLIM)	+= board-b3-input.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-vibrator.o
obj-$(CONFIG_MACH_SLIM)	+= board-tizen-display.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-usb.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-mfd.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-sensor.o
obj-$(CONFIG_MACH_SLIM)	+= board-universal3250-media.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-camera.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-thermistor.o
obj-$(CONFIG_MACH_SLIM)	+= board-b2-fpga.o
obj-$(CONFIG_MACH_SLIM)	+= sec_reboot.o
obj-$(CONFIG_MACH_SLIM)	+= board-universal3250-gpio.o
obj-$(CONFIG_MACH_SLIM)	+= board-exynos3-i2s-dummy-audio.o

obj-$(CONFIG_MACH_WC1)	+= mach-universal3250.o
obj-$(CONFIG_MACH_WC1)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_WC1)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-power.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-input.o
obj-$(CONFIG_MACH_WC1)	+= board-b2-vibrator.o
obj-$(CONFIG_MACH_WC1)	+= board-tizen-display.o
obj-$(CONFIG_MACH_WC1)	+= board-b2-usb.o
obj-$(CONFIG_MACH_WC1)	+= board-b2-mfd.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-sensor.o
obj-$(CONFIG_MACH_WC1)	+= board-universal3250-media.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-thermistor.o
obj-$(CONFIG_MACH_WC1)	+= sec_reboot.o
obj-$(CONFIG_MACH_WC1)	+= board-universal3250-gpio.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-audio.o
obj-$(CONFIG_MACH_WC1)	+= board-tizen-nfc-lsi.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-ese.o

obj-$(CONFIG_MACH_HWP)	+= mach-universal3250.o
obj-$(CONFIG_MACH_HWP)	+= board-universal3250-clock.o
obj-$(CONFIG_MACH_HWP)	+= board-universal3250-mmc.o
obj-$(CONFIG_MACH_HWP)	+= board-hwp-power.o
obj-$(CONFIG_MACH_HWP)	+= board-hwp-input.o
obj-$(CONFIG_MACH_HWP)	+= board-b2-vibrator.o
obj-$(CONFIG_MACH_HWP)	+= board-tizen-display.o
obj-$(CONFIG_MACH_HWP)	+= board-b2-usb.o
obj-$(CONFIG_MACH_HWP)	+= board-b2-mfd.o
obj-$(CONFIG_MACH_HWP)	+= board-hwp-sensor.o
obj-$(CONFIG_MACH_HWP)	+= board-universal3250-media.o
obj-$(CONFIG_MACH_HWP)	+= board-orbis-thermistor.o
obj-$(CONFIG_MACH_HWP)	+= sec_reboot.o
obj-$(CONFIG_MACH_HWP)	+= board-universal3250-gpio.o
obj-$(CONFIG_MACH_HWP)	+= board-tizen-nfc-lsi.o
obj-$(CONFIG_MACH_HWP)	+= board-orbis-ese.o

obj-$(CONFIG_MACH_SMDK4212)		+= mach-smdk4x12.o
obj-$(CONFIG_MACH_SMDK4412)		+= mach-smdk4x12.o
obj-$(CONFIG_MACH_SMDK4412)		+= board-smdk4x12-mmc.o
obj-$(CONFIG_MACH_SMDK4412)		+= board-smdk4x12-audio.o
obj-$(CONFIG_MACH_SMDK4412)		+= board-smdk4x12-display.o
obj-$(CONFIG_MACH_SMDK4412)		+= board-smdk4x12-usb.o
obj-$(CONFIG_MACH_SMDK4412)		+= board-smdk4x12-media.o
obj-$(CONFIG_MACH_SMDK4412)		+= board-smdk4x12-power.o

obj-$(CONFIG_MACH_XYREF4415)		+= mach-xyref4415.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-clock.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-usb.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-power.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-mmc.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-display.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-input.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-media.o
obj-$(CONFIG_MACH_XYREF4415)		+= board-xyref4415-audio.o

obj-$(CONFIG_MACH_UNIVERSAL4415)	+= mach-universal4415.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-clock.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-power.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-mmc.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-input.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-display.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-media.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-audio.o
obj-$(CONFIG_MACH_UNIVERSAL4415)	+= board-universal4415-usb.o

obj-$(CONFIG_MACH_KMINI)		+= mach-universal222ap.o

obj-$(CONFIG_MACH_SMDK5410)		+= mach-smdk5410.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-mmc.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-power.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-usb.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-audio.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-input.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-clock.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-media.o
obj-$(CONFIG_MACH_SMDK5410)		+= board-smdk5410-display.o

obj-$(CONFIG_MACH_SMDK5420)		+= mach-smdk5420.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-power.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-clock.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-mmc.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-wifi.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-usb.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-audio.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-input.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-display.o
obj-$(CONFIG_MACH_SMDK5420)		+= board-smdk5420-media.o

obj-$(CONFIG_MACH_KMINI)		+= mach-universal222ap.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-power.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-mmc.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-display.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-input.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-gpio.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-clock.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-usb.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-muic.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-modems.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-modemif.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-media.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-audio.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-mfd.o
obj-$(CONFIG_MACH_KMINI)		+= board-kmini-camera.o

obj-$(CONFIG_MACH_EXYNOS4_DT)		+= mach-exynos4-dt.o
obj-$(CONFIG_MACH_EXYNOS5_DT)		+= mach-exynos5-dt.o

obj-$(CONFIG_MACH_SMDK5250)		+= mach-smdk5250.o

obj-$(CONFIG_MACH_XYREF5260)		+= mach-xyref5260.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-clock.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-mmc.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-power.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-display.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-clock.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-input.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-usb.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-audio.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-media.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-camera.o
obj-$(CONFIG_MACH_XYREF5260)		+= board-xyref5260-gps.o

obj-$(CONFIG_MACH_UNIVERSAL5260)	+= mach-universal5260.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= clock-exynos5260.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-power.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-battery.o
ifeq ($(CONFIG_S5P_DP),y)
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-display-dp.o
else
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= display-exynos5260.o board-universal5260-display.o
endif
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-clock.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-mmc.o
ifeq ($(CONFIG_ICE4_FPGA),y)
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-fpga.o
endif
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-usb.o
ifeq ($(CONFIG_LEDS_AN30259A),y)
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-led.o
endif
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-input.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-media.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-camera.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-vibrator.o
obj-$(CONFIG_BT_BCM4339)			+= board-bluetooth-bcm4339.o
obj-$(CONFIG_BT_BCM4334W)			+= board-bluetooth-bcm4334w.o
obj-$(CONFIG_WLAN)			+= board-tizen-wlan.o
obj-$(CONFIG_MACH_WC1)	+= board-orbis-wlan-hw.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-sensor.o
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-nfc.o

ifeq ($(CONFIG_MACH_KMINI),y)
obj-$(CONFIG_SAMSUNG_MUIC)    += sec-switch-kmini.o
else ifeq ($(CONFIG_MACH_B2),y)
obj-$(CONFIG_SAMSUNG_MUIC)    += sec-switch.o
obj-$(CONFIG_MUIC_MAX14577)	  += sec-switch-b2.o
obj-$(CONFIG_MACH_B2)	+= board-universal3250-battery.o
else ifeq ($(CONFIG_MACH_WINGTIP),y)
obj-$(CONFIG_SAMSUNG_MUIC)    += sec-switch.o
obj-$(CONFIG_MUIC_MAX14577)	  += sec-switch-b2.o
obj-$(CONFIG_MACH_WINGTIP)	+= board-universal3250-battery.o
else ifeq ($(CONFIG_MACH_BLINK),y)
obj-$(CONFIG_SAMSUNG_MUIC)    += sec-switch.o
obj-$(CONFIG_MUIC_MAX14577)	  += sec-switch-b2.o
obj-$(CONFIG_MACH_BLINK)	+= board-blink-battery.o
else ifeq ($(CONFIG_MACH_SLIM),y)
obj-$(CONFIG_SAMSUNG_MUIC)    += sec-switch.o
obj-$(CONFIG_MUIC_MAX14577)	  += sec-switch-b2.o
obj-$(CONFIG_MACH_SLIM)	+= board-universal3250-battery.o
else ifeq ($(CONFIG_MACH_WC1),y)
obj-$(CONFIG_SAMSUNG_MUIC)	+= sec-switch.o
obj-$(CONFIG_MUIC_MAX14577)	+= sec-switch-b2.o
obj-$(CONFIG_MACH_WC1)	+= board-universal3250-battery.o
else ifeq ($(CONFIG_MACH_HWP),y)
obj-$(CONFIG_SAMSUNG_MUIC)	+= sec-switch.o
obj-$(CONFIG_MUIC_MAX14577)	+= sec-switch-b2.o
obj-$(CONFIG_MACH_HWP)	+= board-universal3250-battery.o
else
obj-$(CONFIG_SAMSUNG_MUIC)    += board-universal5260-mfd.o
obj-$(CONFIG_SAMSUNG_MUIC)    += sec-switch.o
endif

obj-$(CONFIG_SND_SOC_ADONISUNIV_YMU831)	+= board-universal5260-audio-y.o
obj-$(CONFIG_SND_SOC_ADONISUNIV_YMU831)	+= mach-exynos5-audio.o
obj-$(CONFIG_MACH_HLLTE)			+= board-hllte-gpio.o
obj-$(CONFIG_MACH_HL3G)			+= board-hl3g-gpio.o
obj-$(CONFIG_MACH_HLLTE)			+= board-hl-thermistor.o
obj-$(CONFIG_MACH_HL3G)			+= board-hl-thermistor.o
obj-$(CONFIG_MACH_HLLTE)			+= board-hl-pmic.o
obj-$(CONFIG_MACH_HL3G)			+= board-hl-pmic.o

obj-$(CONFIG_MACH_UNIVERSAL5260)	+= board-universal5260-wlan.o

obj-$(CONFIG_EXYNOS_SOUND_PLATFORM_DATA) += exynos_sound_platform_data.o

# device support

obj-y					+= dev-uart.o
obj-$(CONFIG_SND_SOC_SAMSUNG)		+= dev-audio.o
obj-$(CONFIG_EXYNOS4_DEV_AHCI)		+= dev-ahci.o
obj-$(CONFIG_EXYNOS_DEV_DWMCI)		+= dev-dwmci.o
obj-$(CONFIG_EXYNOS4_DEV_MCU_IPC)	+= dev-mcu_ipc.o
obj-$(CONFIG_EXYNOS3_DEV_FIMC_IS)	+= dev-fimc-is.o
obj-$(CONFIG_EXYNOS4_DEV_FIMC_IS)	+= dev-fimc-is.o
obj-$(CONFIG_EXYNOS5_DEV_FIMC_IS)	+= dev-fimc-is.o
obj-$(CONFIG_EXYNOS_DEV_FIMC_IS)	+= dev-fimc-is.o
obj-$(CONFIG_EXYNOS4_DEV_FIMC_LITE)	+= dev-fimc-lite.o
obj-$(CONFIG_EXYNOS_DEV_GSC)		+= dev-gsc.o setup-gsc.o
obj-$(CONFIG_EXYNOS5_DEV_SCALER)	+= dev-scaler.o
obj-$(CONFIG_EXYNOS_DEV_ROTATOR)	+= dev-rotator.o
obj-$(CONFIG_EXYNOS_DEV_SYSMMU)		+= dev-sysmmu.o
obj-$(CONFIG_EXYNOS_DEV_DMA)		+= dma.o
obj-$(CONFIG_EXYNOS_DEV_USB_SWITCH)	+= dev-usb-switch.o
obj-$(CONFIG_EXYNOS4_DEV_USB_OHCI)	+= dev-ohci.o
obj-$(CONFIG_EXYNOS5_DEV_USB3_DRD)	+= dev-usb3-drd.o
obj-$(CONFIG_EXYNOS4_DEV_JPEG)		+= dev-jpeg.o
obj-$(CONFIG_EXYNOS5_DEV_JPEG)			+= dev-jpeg.o
obj-$(CONFIG_EXYNOS5_DEV_JPEG_HX)			+= dev-jpeg-hx.o
ifeq ($(CONFIG_SOC_EXYNOS3470),y)
obj-$(CONFIG_EXYNOS_DEV_TMU)		+= dev-tmu-exynos3470.o
else
obj-$(CONFIG_EXYNOS_DEV_TMU)		+= dev-tmu.o
endif
obj-$(CONFIG_EXYNOS5_DEV_HSI2C0)	+= dev-hs-i2c0.o
obj-$(CONFIG_EXYNOS5_DEV_HSI2C1)	+= dev-hs-i2c1.o
obj-$(CONFIG_EXYNOS5_DEV_HSI2C2)	+= dev-hs-i2c2.o
obj-$(CONFIG_EXYNOS5_DEV_HSI2C3)	+= dev-hs-i2c3.o
obj-$(CONFIG_EXYNOS5_DEV_HSI2C4)	+= dev-hs-i2c4.o
obj-$(CONFIG_EXYNOS5_DEV_HSI2C5)	+= dev-hs-i2c5.o
obj-$(CONFIG_EXYNOS5_DEV_HSI2C6)	+= dev-hs-i2c6.o
obj-$(CONFIG_S5P_DEV_FIMG2D)		+= dev-fimg2d.o

obj-$(CONFIG_ARCH_EXYNOS)		+= setup-i2c0.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC)	+= setup-fimc.o
obj-$(CONFIG_EXYNOS4_SETUP_CSIS)	+= setup-csis.o
ifeq ($(CONFIG_SOC_EXYNOS3470),y)
obj-$(CONFIG_EXYNOS4_SETUP_FIMD0)	+= setup-fimd0.o setup-disp-clock.o
else
obj-$(CONFIG_EXYNOS4_SETUP_FIMD0)	+= setup-fimd0.o
endif
obj-$(CONFIG_EXYNOS_SETUP_FIMD1)	+= setup-fimd1.o setup-disp-clock.o
obj-$(CONFIG_EXYNOS_SETUP_DP)		+= setup-dp.o
obj-$(CONFIG_FB_MIPI_DSIM)		+= setup-mipidsim.o
obj-$(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) += secmem.o
obj-$(CONFIG_EXYNOS3_SETUP_FIMC_IS)	+= setup-exynos-fimc-is.o
obj-$(CONFIG_EXYNOS4_SETUP_FIMC_IS)	+= setup-exynos-fimc-is.o
obj-$(CONFIG_EXYNOS5_SETUP_FIMC_IS)	+= setup-exynos-fimc-is.o
obj-$(CONFIG_EXYNOS_SETUP_FIMC_IS)	+= setup-fimc-is.o
obj-$(CONFIG_EXYNOS_SETUP_FIMC_IS)	+= setup-fimc-is-sensor.o
obj-$(CONFIG_VISION_MODE)		+= setup-fimc-is-vision.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C1)	+= setup-i2c1.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C2)	+= setup-i2c2.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C3)	+= setup-i2c3.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C4)	+= setup-i2c4.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C5)	+= setup-i2c5.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C6)	+= setup-i2c6.o
obj-$(CONFIG_EXYNOS4_SETUP_I2C7)	+= setup-i2c7.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C0)	+= setup-hs-i2c0.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C1)	+= setup-hs-i2c1.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C2)	+= setup-hs-i2c2.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C3)	+= setup-hs-i2c3.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C4)	+= setup-hs-i2c4.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C5)	+= setup-hs-i2c5.o
obj-$(CONFIG_EXYNOS5_SETUP_HSI2C6)	+= setup-hs-i2c6.o
obj-$(CONFIG_EXYNOS4_SETUP_KEYPAD)	+= setup-keypad.o
obj-$(CONFIG_EXYNOS4_SETUP_MFC)		+= setup-mfc.o
obj-$(CONFIG_EXYNOS4_SETUP_SDHCI_GPIO)	+= setup-sdhci-gpio.o
obj-$(CONFIG_EXYNOS4_SETUP_USB_PHY)	+= setup-usb-phy.o
obj-$(CONFIG_EXYNOS_SETUP_SPI)		+= setup-spi.o
obj-$(CONFIG_EXYNOS5_SETUP_TVOUT)	+= setup-tvout.o
obj-$(CONFIG_EXYNOS_SETUP_ADC)		+= setup-adc.o
obj-$(CONFIG_ION_EXYNOS)		+= dev-ion.o
obj-$(CONFIG_CMA)			+= reserve-mem.o
obj-$(CONFIG_EXYNOS4_SETUP_JPEG)	+= setup-jpeg.o
obj-$(CONFIG_EXYNOS5_SETUP_JPEG)			+= setup-jpeg.o
obj-$(CONFIG_ARCH_EXYNOS5)		+= resetreason.o
obj-$(CONFIG_EXYNOS_TMU)		+= tmu-exynos.o
obj-$(CONFIG_EXYNOS_SETUP_MIPIPHY)	+= setup-exynos-mipiphy.o
obj-$(CONFIG_BL_SWITCHER)		+= bL_control.o bL_setup.o
obj-$(CONFIG_GPS_BCM47521)		+= sec_gps_bcm47521.o

# Samsung sec sysfs Feature
obj-$(CONFIG_SEC_SYSFS)			+= sec_sysfs.o

# Samsung Reset Feature
obj-$(CONFIG_MACH_UNIVERSAL5260)	+= sec_reboot.o

# Samsung Debug Feature
obj-$(CONFIG_SEC_DEBUG)			+= sec_debug.o sec_getlog.o sec_gaf.o
obj-$(CONFIG_SEC_LOG)			+= sec_log.o
obj-$(CONFIG_SEC_MODEM_XMM6360)	+= board-xmm6360-modems.o
obj-$(CONFIG_SEC_MODEM_XMM6262)	+= board-xmm626x-modems.o

ifeq ($(CONFIG_MACH_WC1),y)
obj-$(CONFIG_SEC_THERMISTOR)	+= sec_thermistor_orbis.o
else ifeq ($(CONFIG_MACH_HWP),y)
obj-$(CONFIG_SEC_THERMISTOR)	+= sec_thermistor_orbis.o
else
obj-$(CONFIG_SEC_THERMISTOR)	+= sec_thermistor.o
endif

obj-$(CONFIG_SEC_GPIO_DVS)            += board-universal3250-gpiodvs.o

