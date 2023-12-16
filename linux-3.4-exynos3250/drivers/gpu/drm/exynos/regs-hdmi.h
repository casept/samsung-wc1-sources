/*
 *
 *  Cloned from drivers/media/video/s5p-tv/regs-hdmi.h
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * HDMI register header file for Samsung TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef SAMSUNG_REGS_HDMI_H
#define SAMSUNG_REGS_HDMI_H

/*
 * Register part
*/

/* HDMI Version 1.3 & Common */
#define HDMI_CTRL_BASE(x)		((x) + 0x00000000)
#define HDMI_CORE_BASE(x)		((x) + 0x00010000)
#define HDMI_SPDIF_BASE(x)		((x) + 0x00030000)
#define HDMI_I2S_BASE(x)		((x) + 0x00040000)
#define HDMI_TG_BASE(x)			((x) + 0x00050000)
#define HDMI_EFUSE_BASE(x)		((x) + 0x00060000)

/* Control registers */
#define HDMI_INTC_CON			HDMI_CTRL_BASE(0x0000)
#define HDMI_INTC_FLAG			HDMI_CTRL_BASE(0x0004)
#define HDMI_HPD_STATUS			HDMI_CTRL_BASE(0x000C)
#define HDMI_V13_PHY_RSTOUT		HDMI_CTRL_BASE(0x0014)
#define HDMI_V13_PHY_VPLL		HDMI_CTRL_BASE(0x0018)
#define HDMI_V13_PHY_CMU		HDMI_CTRL_BASE(0x001C)
#define HDMI_V13_CORE_RSTOUT		HDMI_CTRL_BASE(0x0020)

/* Core registers */
#define HDMI_CON_0			HDMI_CORE_BASE(0x0000)
#define HDMI_CON_1			HDMI_CORE_BASE(0x0004)
#define HDMI_CON_2			HDMI_CORE_BASE(0x0008)
#define HDMI_SYS_STATUS			HDMI_CORE_BASE(0x0010)
#define HDMI_V13_PHY_STATUS		HDMI_CORE_BASE(0x0014)
#define HDMI_STATUS_EN			HDMI_CORE_BASE(0x0020)
#define HDMI_HPD			HDMI_CORE_BASE(0x0030)
#define HDMI_MODE_SEL			HDMI_CORE_BASE(0x0040)
#define HDMI_ENC_EN			HDMI_CORE_BASE(0x0044)
#define HDMI_V13_BLUE_SCREEN_0		HDMI_CORE_BASE(0x0050)
#define HDMI_V13_BLUE_SCREEN_1		HDMI_CORE_BASE(0x0054)
#define HDMI_V13_BLUE_SCREEN_2		HDMI_CORE_BASE(0x0058)
#define HDMI_H_BLANK_0			HDMI_CORE_BASE(0x00A0)
#define HDMI_H_BLANK_1			HDMI_CORE_BASE(0x00A4)
#define HDMI_V13_V_BLANK_0		HDMI_CORE_BASE(0x00B0)
#define HDMI_V13_V_BLANK_1		HDMI_CORE_BASE(0x00B4)
#define HDMI_V13_V_BLANK_2		HDMI_CORE_BASE(0x00B8)
#define HDMI_V13_H_V_LINE_0		HDMI_CORE_BASE(0x00C0)
#define HDMI_V13_H_V_LINE_1		HDMI_CORE_BASE(0x00C4)
#define HDMI_V13_H_V_LINE_2		HDMI_CORE_BASE(0x00C8)
#define HDMI_VSYNC_POL			HDMI_CORE_BASE(0x00E4)
#define HDMI_INT_PRO_MODE		HDMI_CORE_BASE(0x00E8)
#define HDMI_V13_V_BLANK_F_0		HDMI_CORE_BASE(0x0110)
#define HDMI_V13_V_BLANK_F_1		HDMI_CORE_BASE(0x0114)
#define HDMI_V13_V_BLANK_F_2		HDMI_CORE_BASE(0x0118)
#define HDMI_V13_H_SYNC_GEN_0		HDMI_CORE_BASE(0x0120)
#define HDMI_V13_H_SYNC_GEN_1		HDMI_CORE_BASE(0x0124)
#define HDMI_V13_H_SYNC_GEN_2		HDMI_CORE_BASE(0x0128)
#define HDMI_V13_V_SYNC_GEN_1_0		HDMI_CORE_BASE(0x0130)
#define HDMI_V13_V_SYNC_GEN_1_1		HDMI_CORE_BASE(0x0134)
#define HDMI_V13_V_SYNC_GEN_1_2		HDMI_CORE_BASE(0x0138)
#define HDMI_V13_V_SYNC_GEN_2_0		HDMI_CORE_BASE(0x0140)
#define HDMI_V13_V_SYNC_GEN_2_1		HDMI_CORE_BASE(0x0144)
#define HDMI_V13_V_SYNC_GEN_2_2		HDMI_CORE_BASE(0x0148)
#define HDMI_V13_V_SYNC_GEN_3_0		HDMI_CORE_BASE(0x0150)
#define HDMI_V13_V_SYNC_GEN_3_1		HDMI_CORE_BASE(0x0154)
#define HDMI_V13_V_SYNC_GEN_3_2		HDMI_CORE_BASE(0x0158)
#define HDMI_V13_ACR_CON		HDMI_CORE_BASE(0x0180)
#define HDMI_V13_ACR_CTS0		HDMI_CORE_BASE(0x0190)
#define HDMI_V13_AVI_CON		HDMI_CORE_BASE(0x0300)
#define HDMI_V13_AVI_BYTE(n)		HDMI_CORE_BASE(0x0320 + 4 * (n))
#define HDMI_V13_DC_CONTROL		HDMI_CORE_BASE(0x05C0)
#define HDMI_V13_VIDEO_PATTERN_GEN	HDMI_CORE_BASE(0x05C4)
#define HDMI_V13_HPD_GEN		HDMI_CORE_BASE(0x05C8)
#define HDMI_V13_AUI_CON		HDMI_CORE_BASE(0x0360)
#define HDMI_V13_SPD_CON		HDMI_CORE_BASE(0x0400)

/* Timing generator registers */
#define HDMI_TG_CMD			HDMI_TG_BASE(0x0000)
#define HDMI_TG_H_FSZ_L			HDMI_TG_BASE(0x0018)
#define HDMI_TG_H_FSZ_H			HDMI_TG_BASE(0x001C)
#define HDMI_TG_HACT_ST_L		HDMI_TG_BASE(0x0020)
#define HDMI_TG_HACT_ST_H		HDMI_TG_BASE(0x0024)
#define HDMI_TG_HACT_SZ_L		HDMI_TG_BASE(0x0028)
#define HDMI_TG_HACT_SZ_H		HDMI_TG_BASE(0x002C)
#define HDMI_TG_V_FSZ_L			HDMI_TG_BASE(0x0030)
#define HDMI_TG_V_FSZ_H			HDMI_TG_BASE(0x0034)
#define HDMI_TG_VSYNC_L			HDMI_TG_BASE(0x0038)
#define HDMI_TG_VSYNC_H			HDMI_TG_BASE(0x003C)
#define HDMI_TG_VSYNC2_L		HDMI_TG_BASE(0x0040)
#define HDMI_TG_VSYNC2_H		HDMI_TG_BASE(0x0044)
#define HDMI_TG_VACT_ST_L		HDMI_TG_BASE(0x0048)
#define HDMI_TG_VACT_ST_H		HDMI_TG_BASE(0x004C)
#define HDMI_TG_VACT_SZ_L		HDMI_TG_BASE(0x0050)
#define HDMI_TG_VACT_SZ_H		HDMI_TG_BASE(0x0054)
#define HDMI_TG_FIELD_CHG_L		HDMI_TG_BASE(0x0058)
#define HDMI_TG_FIELD_CHG_H		HDMI_TG_BASE(0x005C)
#define HDMI_TG_VACT_ST2_L		HDMI_TG_BASE(0x0060)
#define HDMI_TG_VACT_ST2_H		HDMI_TG_BASE(0x0064)
#define HDMI_TG_VSYNC_TOP_HDMI_L	HDMI_TG_BASE(0x0078)
#define HDMI_TG_VSYNC_TOP_HDMI_H	HDMI_TG_BASE(0x007C)
#define HDMI_TG_VSYNC_BOT_HDMI_L	HDMI_TG_BASE(0x0080)
#define HDMI_TG_VSYNC_BOT_HDMI_H	HDMI_TG_BASE(0x0084)
#define HDMI_TG_FIELD_TOP_HDMI_L	HDMI_TG_BASE(0x0088)
#define HDMI_TG_FIELD_TOP_HDMI_H	HDMI_TG_BASE(0x008C)
#define HDMI_TG_FIELD_BOT_HDMI_L	HDMI_TG_BASE(0x0090)
#define HDMI_TG_FIELD_BOT_HDMI_H	HDMI_TG_BASE(0x0094)

/*
 * Bit definition part
 */

/* HDMI_INTC_CON */
#define HDMI_INTC_EN_GLOBAL		(1 << 6)
#define HDMI_INTC_EN_HPD_PLUG		(1 << 3)
#define HDMI_INTC_EN_HPD_UNPLUG		(1 << 2)
#define HDMI_INTC_EN_HDCP			(1 << 0)

/* HDMI_INTC_FLAG */
#define HDMI_INTC_FLAG_HPD_PLUG		(1 << 3)
#define HDMI_INTC_FLAG_HPD_UNPLUG	(1 << 2)
#define HDMI_INTC_FLAG_HDCP			(1 << 0)

/* HDMI_HDCP_KEY_LOAD */
#define HDMI_HDCP_KEY_LOAD_DONE	(1 << 0)

/* HDMI_PHY_RSTOUT */
#define HDMI_PHY_SW_RSTOUT		(1 << 0)

/* HDMI_CORE_RSTOUT */
#define HDMI_CORE_SW_RSTOUT		(1 << 0)

/* HDMI_CON_0 */
#define HDMI_BLUE_SCR_EN		(1 << 5)
#define HDMI_ASP_EN			(1 << 2)
#define HDMI_ASP_DIS			(0 << 2)
#define HDMI_ASP_MASK			(1 << 2)
#define HDMI_EN				(1 << 0)

/* HDMI_CON_2 */
#define HDMI_VID_PREAMBLE_DIS		(1 << 5)
#define HDMI_GUARD_BAND_DIS		(1 << 1)

/* STATUS */
#define HDMI_AUTHEN_ACK_AUTH			(1 << 7)
#define HDMI_AUTHEN_ACK_NOT			(0 << 7)
#define HDMI_AUD_FIFO_OVF_FULL			(1 << 6)
#define HDMI_AUD_FIFO_OVF_NOT			(0 << 6)
#define HDMI_UPDATE_RI_INT_OCC			(1 << 4)
#define HDMI_UPDATE_RI_INT_NOT			(0 << 4)
#define HDMI_UPDATE_RI_INT_CLEAR		(1 << 4)
#define HDMI_UPDATE_PJ_INT_OCC			(1 << 3)
#define HDMI_UPDATE_PJ_INT_NOT			(0 << 3)
#define HDMI_UPDATE_PJ_INT_CLEAR		(1 << 3)
#define HDMI_WRITE_INT_OCC			(1 << 2)
#define HDMI_WRITE_INT_NOT			(0 << 2)
#define HDMI_WRITE_INT_CLEAR			(1 << 2)
#define HDMI_WATCHDOG_INT_OCC			(1 << 1)
#define HDMI_WATCHDOG_INT_NOT			(0 << 1)
#define HDMI_WATCHDOG_INT_CLEAR			(1 << 1)
#define HDMI_WTFORACTIVERX_INT_OCC		(1)
#define HDMI_WTFORACTIVERX_INT_NOT		(0)
#define HDMI_WTFORACTIVERX_INT_CLEAR		(1)

/* HDMI_PHY_STATUS */
#define HDMI_PHY_STATUS_READY		(1 << 0)

/* HDMI_MODE_SEL */
#define HDMI_MODE_HDMI_EN		(1 << 1)
#define HDMI_MODE_DVI_EN		(1 << 0)
#define HDMI_MODE_MASK			(3 << 0)

/* HDMI_TG_CMD */
#define HDMI_TG_EN			(1 << 0)
#define HDMI_FIELD_EN			(1 << 1)

/* STATUS_EN */
#define HDMI_AUD_FIFO_OVF_EN			(1 << 6)
#define HDMI_AUD_FIFO_OVF_DIS			(0 << 6)
#define HDMI_UPDATE_RI_INT_EN			(1 << 4)
#define HDMI_UPDATE_RI_INT_DIS			(0 << 4)
#define HDMI_UPDATE_PJ_INT_EN			(1 << 3)
#define HDMI_UPDATE_PJ_INT_DIS			(0 << 3)
#define HDMI_WRITE_INT_EN			(1 << 2)
#define HDMI_WRITE_INT_DIS			(0 << 2)
#define HDMI_WATCHDOG_INT_EN			(1 << 1)
#define HDMI_WATCHDOG_INT_DIS			(0 << 1)
#define HDMI_WTFORACTIVERX_INT_EN		(1)
#define HDMI_WTFORACTIVERX_INT_DIS		(0)
#define HDMI_INT_EN_ALL				(HDMI_UPDATE_RI_INT_EN|\
						HDMI_UPDATE_PJ_INT_DIS|\
						HDMI_WRITE_INT_EN|\
						HDMI_WATCHDOG_INT_EN|\
						HDMI_WTFORACTIVERX_INT_EN)
#define HDMI_INT_DIS_ALL			(~0x1F)

/* HDMI_HPD */
#define HDMI_SW_HPD_PLUGGED			(1 << 1)
#define HDMI_SW_HPD_UNPLUGGED			(0 << 1)
#define HDMI_HPD_SEL_I_HPD			(1)
#define HDMI_HPD_SEL_SW_HPD			(0)

/* ENC_EN */
#define HDMI_HDCP_ENC_ENABLE			(1)
#define HDMI_HDCP_ENC_DISABLE			(0)

/* HDCP Register */

/* HDCP_SHA1_00~19 */

/* HDCP_KSV_LIST_0~4 */

/* HDCP_KSV_LIST_CON */
#define HDMI_HDCP_KSV_WRITE_DONE		(0x1 << 3)
#define HDMI_HDCP_KSV_LIST_EMPTY		(0x1 << 2)
#define HDMI_HDCP_KSV_END			(0x1 << 1)
#define HDMI_HDCP_KSV_READ			(0x1 << 0)

/* HDCP_CTRL1 */
#define HDMI_HDCP_EN_PJ_EN			(1 << 4)
#define HDMI_HDCP_EN_PJ_DIS			(~(1 << 4))
#define HDMI_HDCP_SET_REPEATER_TIMEOUT		(1 << 2)
#define HDMI_HDCP_CLEAR_REPEATER_TIMEOUT	(~(1 << 2))
#define HDMI_HDCP_CP_DESIRED_EN			(1 << 1)
#define HDMI_HDCP_CP_DESIRED_DIS		(~(1 << 1))
#define HDMI_HDCP_ENABLE_1_1_FEATURE_EN		(1)
#define HDMI_HDCP_ENABLE_1_1_FEATURE_DIS	(~(1))

/* HDCP_CHECK_RESULT */
#define HDMI_HDCP_PI_MATCH_RESULT_Y		((0x1 << 3) | (0x1 << 2))
#define HDMI_HDCP_PI_MATCH_RESULT_N		((0x1 << 3) | (0x0 << 2))
#define HDMI_HDCP_RI_MATCH_RESULT_Y		((0x1 << 1) | (0x1 << 0))
#define HDMI_HDCP_RI_MATCH_RESULT_N		((0x1 << 1) | (0x0 << 0))
#define HDMI_HDCP_CLR_ALL_RESULTS		(0)

/* HDCP_BKSV0~4 */
/* HDCP_AKSV0~4 */

/* HDCP_BCAPS */
#define HDMI_HDCP_BCAPS_REPEATER		(1 << 6)
#define HDMI_HDCP_BCAPS_READY			(1 << 5)
#define HDMI_HDCP_BCAPS_FAST			(1 << 4)
#define HDMI_HDCP_BCAPS_1_1_FEATURES		(1 << 1)
#define HDMI_HDCP_BCAPS_FAST_REAUTH		(1)

/* HDCP_BSTATUS_0/1 */
/* HDCP_Ri_0/1 */
/* HDCP_I2C_INT */
/* HDCP_AN_INT */
/* HDCP_WATCHDOG_INT */
/* HDCP_RI_INT/1 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_1 */
/* HDCP_Frame_Count */

/* HDMI Version 1.4 */
/* Control registers */
/* #define HDMI_INTC_CON		HDMI_CTRL_BASE(0x0000) */
/* #define HDMI_INTC_FLAG		HDMI_CTRL_BASE(0x0004) */
#define HDMI_HDCP_KEY_LOAD		HDMI_CTRL_BASE(0x0008)
/* #define HDMI_HPD_STATUS		HDMI_CTRL_BASE(0x000C) */
#define HDMI_INTC_CON_1			HDMI_CTRL_BASE(0x0010)
#define HDMI_INTC_FLAG_1		HDMI_CTRL_BASE(0x0014)
#define HDMI_PHY_STATUS_0		HDMI_CTRL_BASE(0x0020)
#define HDMI_PHY_STATUS_CMU		HDMI_CTRL_BASE(0x0024)
#define HDMI_PHY_STATUS_PLL		HDMI_CTRL_BASE(0x0028)
#define HDMI_PHY_CON_0			HDMI_CTRL_BASE(0x0030)
#define HDMI_HPD_CTRL			HDMI_CTRL_BASE(0x0040)
#define HDMI_HPD_ST			HDMI_CTRL_BASE(0x0044)
#define HDMI_HPD_TH_X			HDMI_CTRL_BASE(0x0050)
#define HDMI_AUDIO_CLKSEL		HDMI_CTRL_BASE(0x0070)
#define HDMI_PHY_RSTOUT			HDMI_CTRL_BASE(0x0074)
#define HDMI_PHY_VPLL			HDMI_CTRL_BASE(0x0078)
#define HDMI_PHY_CMU			HDMI_CTRL_BASE(0x007C)
#define HDMI_CORE_RSTOUT		HDMI_CTRL_BASE(0x0080)

/* Video related registers */
#define HDMI_YMAX			HDMI_CORE_BASE(0x0060)
#define HDMI_YMIN			HDMI_CORE_BASE(0x0064)
#define HDMI_CMAX			HDMI_CORE_BASE(0x0068)
#define HDMI_CMIN			HDMI_CORE_BASE(0x006C)

#define HDMI_V2_BLANK_0			HDMI_CORE_BASE(0x00B0)
#define HDMI_V2_BLANK_1			HDMI_CORE_BASE(0x00B4)
#define HDMI_V1_BLANK_0			HDMI_CORE_BASE(0x00B8)
#define HDMI_V1_BLANK_1			HDMI_CORE_BASE(0x00BC)

#define HDMI_V_LINE_0			HDMI_CORE_BASE(0x00C0)
#define HDMI_V_LINE_1			HDMI_CORE_BASE(0x00C4)
#define HDMI_H_LINE_0			HDMI_CORE_BASE(0x00C8)
#define HDMI_H_LINE_1			HDMI_CORE_BASE(0x00CC)

#define HDMI_HSYNC_POL			HDMI_CORE_BASE(0x00E0)

#define HDMI_V_BLANK_F0_0		HDMI_CORE_BASE(0x0110)
#define HDMI_V_BLANK_F0_1		HDMI_CORE_BASE(0x0114)
#define HDMI_V_BLANK_F1_0		HDMI_CORE_BASE(0x0118)
#define HDMI_V_BLANK_F1_1		HDMI_CORE_BASE(0x011C)

#define HDMI_H_SYNC_START_0		HDMI_CORE_BASE(0x0120)
#define HDMI_H_SYNC_START_1		HDMI_CORE_BASE(0x0124)
#define HDMI_H_SYNC_END_0		HDMI_CORE_BASE(0x0128)
#define HDMI_H_SYNC_END_1		HDMI_CORE_BASE(0x012C)

#define HDMI_V_SYNC_LINE_BEF_2_0	HDMI_CORE_BASE(0x0130)
#define HDMI_V_SYNC_LINE_BEF_2_1	HDMI_CORE_BASE(0x0134)
#define HDMI_V_SYNC_LINE_BEF_1_0	HDMI_CORE_BASE(0x0138)
#define HDMI_V_SYNC_LINE_BEF_1_1	HDMI_CORE_BASE(0x013C)

#define HDMI_V_SYNC_LINE_AFT_2_0	HDMI_CORE_BASE(0x0140)
#define HDMI_V_SYNC_LINE_AFT_2_1	HDMI_CORE_BASE(0x0144)
#define HDMI_V_SYNC_LINE_AFT_1_0	HDMI_CORE_BASE(0x0148)
#define HDMI_V_SYNC_LINE_AFT_1_1	HDMI_CORE_BASE(0x014C)

#define HDMI_V_SYNC_LINE_AFT_PXL_2_0	HDMI_CORE_BASE(0x0150)
#define HDMI_V_SYNC_LINE_AFT_PXL_2_1	HDMI_CORE_BASE(0x0154)
#define HDMI_V_SYNC_LINE_AFT_PXL_1_0	HDMI_CORE_BASE(0x0158)
#define HDMI_V_SYNC_LINE_AFT_PXL_1_1	HDMI_CORE_BASE(0x015C)

#define HDMI_V_BLANK_F2_0		HDMI_CORE_BASE(0x0160)
#define HDMI_V_BLANK_F2_1		HDMI_CORE_BASE(0x0164)
#define HDMI_V_BLANK_F3_0		HDMI_CORE_BASE(0x0168)
#define HDMI_V_BLANK_F3_1		HDMI_CORE_BASE(0x016C)
#define HDMI_V_BLANK_F4_0		HDMI_CORE_BASE(0x0170)
#define HDMI_V_BLANK_F4_1		HDMI_CORE_BASE(0x0174)
#define HDMI_V_BLANK_F5_0		HDMI_CORE_BASE(0x0178)
#define HDMI_V_BLANK_F5_1		HDMI_CORE_BASE(0x017C)

#define HDMI_V_SYNC_LINE_AFT_3_0	HDMI_CORE_BASE(0x0180)
#define HDMI_V_SYNC_LINE_AFT_3_1	HDMI_CORE_BASE(0x0184)
#define HDMI_V_SYNC_LINE_AFT_4_0	HDMI_CORE_BASE(0x0188)
#define HDMI_V_SYNC_LINE_AFT_4_1	HDMI_CORE_BASE(0x018C)
#define HDMI_V_SYNC_LINE_AFT_5_0	HDMI_CORE_BASE(0x0190)
#define HDMI_V_SYNC_LINE_AFT_5_1	HDMI_CORE_BASE(0x0194)
#define HDMI_V_SYNC_LINE_AFT_6_0	HDMI_CORE_BASE(0x0198)
#define HDMI_V_SYNC_LINE_AFT_6_1	HDMI_CORE_BASE(0x019C)

#define HDMI_V_SYNC_LINE_AFT_PXL_3_0	HDMI_CORE_BASE(0x01A0)
#define HDMI_V_SYNC_LINE_AFT_PXL_3_1	HDMI_CORE_BASE(0x01A4)
#define HDMI_V_SYNC_LINE_AFT_PXL_4_0	HDMI_CORE_BASE(0x01A8)
#define HDMI_V_SYNC_LINE_AFT_PXL_4_1	HDMI_CORE_BASE(0x01AC)
#define HDMI_V_SYNC_LINE_AFT_PXL_5_0	HDMI_CORE_BASE(0x01B0)
#define HDMI_V_SYNC_LINE_AFT_PXL_5_1	HDMI_CORE_BASE(0x01B4)
#define HDMI_V_SYNC_LINE_AFT_PXL_6_0	HDMI_CORE_BASE(0x01B8)
#define HDMI_V_SYNC_LINE_AFT_PXL_6_1	HDMI_CORE_BASE(0x01BC)

#define HDMI_VACT_SPACE_1_0		HDMI_CORE_BASE(0x01C0)
#define HDMI_VACT_SPACE_1_1		HDMI_CORE_BASE(0x01C4)
#define HDMI_VACT_SPACE_2_0		HDMI_CORE_BASE(0x01C8)
#define HDMI_VACT_SPACE_2_1		HDMI_CORE_BASE(0x01CC)
#define HDMI_VACT_SPACE_3_0		HDMI_CORE_BASE(0x01D0)
#define HDMI_VACT_SPACE_3_1		HDMI_CORE_BASE(0x01D4)
#define HDMI_VACT_SPACE_4_0		HDMI_CORE_BASE(0x01D8)
#define HDMI_VACT_SPACE_4_1		HDMI_CORE_BASE(0x01DC)
#define HDMI_VACT_SPACE_5_0		HDMI_CORE_BASE(0x01E0)
#define HDMI_VACT_SPACE_5_1		HDMI_CORE_BASE(0x01E4)
#define HDMI_VACT_SPACE_6_0		HDMI_CORE_BASE(0x01E8)
#define HDMI_VACT_SPACE_6_1		HDMI_CORE_BASE(0x01EC)

#define HDMI_GCP_CON			HDMI_CORE_BASE(0x0200)
#define HDMI_GCP_BYTE1			HDMI_CORE_BASE(0x0210)
#define HDMI_GCP_BYTE2			HDMI_CORE_BASE(0x0214)
#define HDMI_GCP_BYTE3			HDMI_CORE_BASE(0x0218)

/* Audio related registers */
#define HDMI_ASP_CON			HDMI_CORE_BASE(0x0300)
#define HDMI_ASP_SP_FLAT		HDMI_CORE_BASE(0x0304)
#define HDMI_ASP_CHCFG0			HDMI_CORE_BASE(0x0310)
#define HDMI_ASP_CHCFG1			HDMI_CORE_BASE(0x0314)
#define HDMI_ASP_CHCFG2			HDMI_CORE_BASE(0x0318)
#define HDMI_ASP_CHCFG3			HDMI_CORE_BASE(0x031C)

#define HDMI_ACR_CON			HDMI_CORE_BASE(0x0400)
#define HDMI_ACR_MCTS0			HDMI_CORE_BASE(0x0410)
#define HDMI_ACR_MCTS1			HDMI_CORE_BASE(0x0414)
#define HDMI_ACR_MCTS2			HDMI_CORE_BASE(0x0418)
#define HDMI_ACR_CTS0			HDMI_CORE_BASE(0x0420)
#define HDMI_ACR_CTS1			HDMI_CORE_BASE(0x0424)
#define HDMI_ACR_CTS2			HDMI_CORE_BASE(0x0428)
#define HDMI_ACR_N0			HDMI_CORE_BASE(0x0430)
#define HDMI_ACR_N1			HDMI_CORE_BASE(0x0434)
#define HDMI_ACR_N2			HDMI_CORE_BASE(0x0438)

/* Packet related registers */
#define HDMI_ACP_CON			HDMI_CORE_BASE(0x0500)
#define HDMI_ACP_TYPE			HDMI_CORE_BASE(0x0514)
#define HDMI_ACP_DATA(n)		HDMI_CORE_BASE(0x0520 + 4 * (n))

#define HDMI_ISRC_CON			HDMI_CORE_BASE(0x0600)
#define HDMI_ISRC1_HEADER1		HDMI_CORE_BASE(0x0614)
#define HDMI_ISRC1_DATA(n)		HDMI_CORE_BASE(0x0620 + 4 * (n))
#define HDMI_ISRC2_DATA(n)		HDMI_CORE_BASE(0x06A0 + 4 * (n))

#define HDMI_AVI_CON			HDMI_CORE_BASE(0x0700)
#define HDMI_AVI_HEADER0		HDMI_CORE_BASE(0x0710)
#define HDMI_AVI_HEADER1		HDMI_CORE_BASE(0x0714)
#define HDMI_AVI_HEADER2		HDMI_CORE_BASE(0x0718)
#define HDMI_AVI_CHECK_SUM		HDMI_CORE_BASE(0x071C)
#define HDMI_AVI_BYTE(n)		HDMI_CORE_BASE(0x0720 + 4 * (n-1))

#define HDMI_AUI_CON			HDMI_CORE_BASE(0x0800)
#define HDMI_AUI_HEADER0		HDMI_CORE_BASE(0x0810)
#define HDMI_AUI_HEADER1		HDMI_CORE_BASE(0x0814)
#define HDMI_AUI_HEADER2		HDMI_CORE_BASE(0x0818)
#define HDMI_AUI_CHECK_SUM		HDMI_CORE_BASE(0x081C)
#define HDMI_AUI_BYTE(n)		HDMI_CORE_BASE(0x0820 + 4 * (n-1))

#define HDMI_MPG_CON			HDMI_CORE_BASE(0x0900)
#define HDMI_MPG_CHECK_SUM		HDMI_CORE_BASE(0x091C)
#define HDMI_MPG_DATA(n)		HDMI_CORE_BASE(0x0920 + 4 * (n))

#define HDMI_SPD_CON			HDMI_CORE_BASE(0x0A00)
#define HDMI_SPD_HEADER0		HDMI_CORE_BASE(0x0A10)
#define HDMI_SPD_HEADER1		HDMI_CORE_BASE(0x0A14)
#define HDMI_SPD_HEADER2		HDMI_CORE_BASE(0x0A18)
#define HDMI_SPD_DATA(n)		HDMI_CORE_BASE(0x0A20 + 4 * (n))

#define HDMI_GAMUT_CON			HDMI_CORE_BASE(0x0B00)
#define HDMI_GAMUT_HEADER0		HDMI_CORE_BASE(0x0B10)
#define HDMI_GAMUT_HEADER1		HDMI_CORE_BASE(0x0B14)
#define HDMI_GAMUT_HEADER2		HDMI_CORE_BASE(0x0B18)
#define HDMI_GAMUT_METADATA(n)		HDMI_CORE_BASE(0x0B20 + 4 * (n))

#define HDMI_VSI_CON			HDMI_CORE_BASE(0x0C00)
#define HDMI_VSI_HEADER0		HDMI_CORE_BASE(0x0C10)
#define HDMI_VSI_HEADER1		HDMI_CORE_BASE(0x0C14)
#define HDMI_VSI_HEADER2		HDMI_CORE_BASE(0x0C18)
#define HDMI_VSI_DATA(n)		HDMI_CORE_BASE(0x0C20 + 4 * (n))

#define HDMI_DC_CONTROL			HDMI_CORE_BASE(0x0D00)
#define HDMI_VIDEO_PATTERN_GEN		HDMI_CORE_BASE(0x0D04)

#define HDMI_AN_SEED_SEL		HDMI_CORE_BASE(0x0E48)
#define HDMI_AN_SEED_0			HDMI_CORE_BASE(0x0E58)
#define HDMI_AN_SEED_1			HDMI_CORE_BASE(0x0E5C)
#define HDMI_AN_SEED_2			HDMI_CORE_BASE(0x0E60)
#define HDMI_AN_SEED_3			HDMI_CORE_BASE(0x0E64)

/* AVI bit definition */
#define HDMI_AVI_CON_DO_NOT_TRANSMIT	(0 << 1)
#define HDMI_AVI_CON_EVERY_VSYNC	(1 << 1)

#define AVI_ACTIVE_FORMAT_VALID	(1 << 4)
#define AVI_UNDERSCANNED_DISPLAY_VALID	(1 << 1)

/* AUI bit definition */
#define HDMI_AUI_CON_NO_TRAN		(0 << 0)

/* VSI bit definition */
#define HDMI_VSI_CON_DO_NOT_TRANSMIT	(0 << 0)

/* HDMI_YMAX/YMIN/CMAX/CMIN */
#define HDMI_YMAX_VAL(x)			((x) & 0xff)
#define HDMI_YMIN_VAL(x)			((x) & 0xff)
#define HDMI_CMAX_VAL(x)			((x) & 0xff)
#define HDMI_CMIN_VAL(x)			((x) & 0xff)

/* HDCP related registers */
#define HDMI_HDCP_SHA1(n)		HDMI_CORE_BASE(0x7000 + 4 * (n))
#define HDMI_HDCP_KSV_LIST(n)		HDMI_CORE_BASE(0x7050 + 4 * (n))

#define HDMI_HDCP_KSV_LIST_CON		HDMI_CORE_BASE(0x7064)
#define HDMI_HDCP_SHA_RESULT		HDMI_CORE_BASE(0x7070)
#define HDMI_HDCP_CTRL1			HDMI_CORE_BASE(0x7080)
#define HDMI_HDCP_CTRL2			HDMI_CORE_BASE(0x7084)
#define HDMI_HDCP_CHECK_RESULT		HDMI_CORE_BASE(0x7090)
#define HDMI_HDCP_BKSV(n)		HDMI_CORE_BASE(0x70A0 + 4 * (n))
#define HDMI_HDCP_AKSV(n)		HDMI_CORE_BASE(0x70C0 + 4 * (n))
#define HDMI_HDCP_AN(n)			HDMI_CORE_BASE(0x70E0 + 4 * (n))

#define HDMI_HDCP_BCAPS			HDMI_CORE_BASE(0x7100)
#define HDMI_HDCP_BSTATUS_0		HDMI_CORE_BASE(0x7110)
#define HDMI_HDCP_BSTATUS_1		HDMI_CORE_BASE(0x7114)
#define HDMI_HDCP_RI_0			HDMI_CORE_BASE(0x7140)
#define HDMI_HDCP_RI_1			HDMI_CORE_BASE(0x7144)
#define HDMI_HDCP_I2C_INT		HDMI_CORE_BASE(0x7180)
#define HDMI_HDCP_AN_INT		HDMI_CORE_BASE(0x7190)
#define HDMI_HDCP_WDT_INT		HDMI_CORE_BASE(0x71A0)
#define HDMI_HDCP_RI_INT		HDMI_CORE_BASE(0x71B0)
#define HDMI_HDCP_RI_COMPARE_0		HDMI_CORE_BASE(0x71D0)
#define HDMI_HDCP_RI_COMPARE_1		HDMI_CORE_BASE(0x71D4)
#define HDMI_HDCP_FRAME_COUNT		HDMI_CORE_BASE(0x71E0)

#define HDMI_RGB_ROUND_EN		HDMI_CORE_BASE(0xD500)
#define HDMI_VACT_SPACE_R_0		HDMI_CORE_BASE(0xD504)
#define HDMI_VACT_SPACE_R_1		HDMI_CORE_BASE(0xD508)
#define HDMI_VACT_SPACE_G_0		HDMI_CORE_BASE(0xD50C)
#define HDMI_VACT_SPACE_G_1		HDMI_CORE_BASE(0xD510)
#define HDMI_VACT_SPACE_B_0		HDMI_CORE_BASE(0xD514)
#define HDMI_VACT_SPACE_B_1		HDMI_CORE_BASE(0xD518)

#define HDMI_BLUE_SCREEN_B_0		HDMI_CORE_BASE(0xD520)
#define HDMI_BLUE_SCREEN_B_1		HDMI_CORE_BASE(0xD524)
#define HDMI_BLUE_SCREEN_G_0		HDMI_CORE_BASE(0xD528)
#define HDMI_BLUE_SCREEN_G_1		HDMI_CORE_BASE(0xD52C)
#define HDMI_BLUE_SCREEN_R_0		HDMI_CORE_BASE(0xD530)
#define HDMI_BLUE_SCREEN_R_1		HDMI_CORE_BASE(0xD534)

/* HDMI SPDIF register */
#define HDMI_SPDIFIN_CLK_CTRL		HDMI_SPDIF_BASE(0x0000)
#define HDMI_SPDIFIN_OP_CTRL		HDMI_SPDIF_BASE(0x0004)
#define HDMI_SPDIFIN_IRQ_MASK		HDMI_SPDIF_BASE(0x0008)
#define HDMI_SPDIFIN_IRQ_STATUS		HDMI_SPDIF_BASE(0x000C)
#define HDMI_SPDIFIN_CONFIG_1		HDMI_SPDIF_BASE(0x0010)
#define HDMI_SPDIFIN_CONFIG_2		HDMI_SPDIF_BASE(0x0014)
#define HDMI_SPDIFIN_USER_VALUE_1		HDMI_SPDIF_BASE(0x0020)
#define HDMI_SPDIFIN_USER_VALUE_2		HDMI_SPDIF_BASE(0x0024)
#define HDMI_SPDIFIN_USER_VALUE_3		HDMI_SPDIF_BASE(0x0028)
#define HDMI_SPDIFIN_USER_VALUE_4		HDMI_SPDIF_BASE(0x002C)
#define HDMI_SPDIFIN_CH_STATUS_0_1		HDMI_SPDIF_BASE(0x0030)
#define HDMI_SPDIFIN_CH_STATUS_0_2		HDMI_SPDIF_BASE(0x0034)
#define HDMI_SPDIFIN_CH_STATUS_0_3		HDMI_SPDIF_BASE(0x0038)
#define HDMI_SPDIFIN_CH_STATUS_0_4		HDMI_SPDIF_BASE(0x003C)
#define HDMI_SPDIFIN_CH_STATUS_1		HDMI_SPDIF_BASE(0x0040)
#define HDMI_SPDIFIN_FRAME_PERIOD_1		HDMI_SPDIF_BASE(0x0048)
#define HDMI_SPDIFIN_FRAME_PERIOD_2		HDMI_SPDIF_BASE(0x004C)
#define HDMI_SPDIFIN_Pc_INFO_1		HDMI_SPDIF_BASE(0x0050)
#define HDMI_SPDIFIN_Pc_INFO_2		HDMI_SPDIF_BASE(0x0054)
#define HDMI_SPDIFIN_Pd_INFO_1		HDMI_SPDIF_BASE(0x0058)
#define HDMI_SPDIFIN_Pd_INFO_2		HDMI_SPDIF_BASE(0x005C)
#define HDMI_SPDIFIN_DATA_BUF_0_1		HDMI_SPDIF_BASE(0x0060)
#define HDMI_SPDIFIN_DATA_BUF_0_2		HDMI_SPDIF_BASE(0x0064)
#define HDMI_SPDIFIN_DATA_BUF_0_3		HDMI_SPDIF_BASE(0x0068)
#define HDMI_SPDIFIN_USER_BUF_0		HDMI_SPDIF_BASE(0x006C)
#define HDMI_SPDIFIN_DATA_BUF_1_1		HDMI_SPDIF_BASE(0x0070)
#define HDMI_SPDIFIN_DATA_BUF_1_2		HDMI_SPDIF_BASE(0x0074)
#define HDMI_SPDIFIN_DATA_BUF_1_3		HDMI_SPDIF_BASE(0x0078)
#define HDMI_SPDIFIN_USER_BUF_1		HDMI_SPDIF_BASE(0x007C)

/* HDMI I2S register */
#define HDMI_I2S_CLK_CON		HDMI_I2S_BASE(0x000)
#define HDMI_I2S_CON_1			HDMI_I2S_BASE(0x004)
#define HDMI_I2S_CON_2			HDMI_I2S_BASE(0x008)
#define HDMI_I2S_PIN_SEL_0		HDMI_I2S_BASE(0x00c)
#define HDMI_I2S_PIN_SEL_1		HDMI_I2S_BASE(0x010)
#define HDMI_I2S_PIN_SEL_2		HDMI_I2S_BASE(0x014)
#define HDMI_I2S_PIN_SEL_3		HDMI_I2S_BASE(0x018)
#define HDMI_I2S_DSD_CON		HDMI_I2S_BASE(0x01c)
#define HDMI_I2S_MUX_CON		HDMI_I2S_BASE(0x020)
#define HDMI_I2S_CH_ST_CON		HDMI_I2S_BASE(0x024)
#define HDMI_I2S_CH_ST_0		HDMI_I2S_BASE(0x028)
#define HDMI_I2S_CH_ST_1		HDMI_I2S_BASE(0x02c)
#define HDMI_I2S_CH_ST_2		HDMI_I2S_BASE(0x030)
#define HDMI_I2S_CH_ST_3		HDMI_I2S_BASE(0x034)
#define HDMI_I2S_CH_ST_4		HDMI_I2S_BASE(0x038)
#define HDMI_I2S_CH_ST_SH_0		HDMI_I2S_BASE(0x03c)
#define HDMI_I2S_CH_ST_SH_1		HDMI_I2S_BASE(0x040)
#define HDMI_I2S_CH_ST_SH_2		HDMI_I2S_BASE(0x044)
#define HDMI_I2S_CH_ST_SH_3		HDMI_I2S_BASE(0x048)
#define HDMI_I2S_CH_ST_SH_4		HDMI_I2S_BASE(0x04c)
#define HDMI_I2S_VD_DATA		HDMI_I2S_BASE(0x050)
#define HDMI_I2S_MUX_CH			HDMI_I2S_BASE(0x054)
#define HDMI_I2S_MUX_CUV		HDMI_I2S_BASE(0x058)
#define HDMI_I2S_IRQ_MASK		HDMI_I2S_BASE(0x05c)
#define HDMI_I2S_IRQ_STATUS		HDMI_I2S_BASE(0x060)

/* SPDIF bit definition */

/* SPDIFIN_CLK_CTRL */
#define HDMI_SPDIFIN_READY_CLK_DOWN		(1 << 1)
#define HDMI_SPDIFIN_CLK_ON			(1)

/* SPDIFIN_OP_CTRL */
#define HDMI_SPDIFIN_SW_RESET		(0)
#define HDMI_SPDIFIN_STATUS_CHECK_MODE	(1)
#define HDMI_SPDIFIN_STATUS_CHK_OP_MODE	(3)

/* SPDIFIN_IRQ_MASK */

/* SPDIFIN_IRQ_STATUS */
#define HDMI_SPDIFIN_IRQ_OVERFLOW_EN			(1 << 7)
#define HDMI_SPDIFIN_IRQ_ABNORMAL_PD_EN			(1 << 6)
#define HDMI_SPDIFIN_IRQ_SH_NOT_DETECTED_RIGHTTIME_EN	(1 << 5)
#define HDMI_SPDIFIN_IRQ_SH_DETECTED_EN			(1 << 4)
#define HDMI_SPDIFIN_IRQ_SH_NOT_DETECTED_EN			(1 << 3)
#define HDMI_SPDIFIN_IRQ_WRONG_PREAMBLE_EN			(1 << 2)
#define HDMI_SPDIFIN_IRQ_CH_STATUS_RECOVERED_EN		(1 << 1)
#define HDMI_SPDIFIN_IRQ_WRONG_SIG_EN			(1 << 0)

/* SPDIFIN_CONFIG_1 */
#define HDMI_SPDIFIN_CFG_FILTER_3_SAMPLE			(0 << 6)
#define HDMI_SPDIFIN_CFG_FILTER_2_SAMPLE			(1 << 6)
#define HDMI_SPDIFIN_CFG_LINEAR_PCM_TYPE			(0 << 5)
#define HDMI_SPDIFIN_CFG_NO_LINEAR_PCM_TYPE			(1 << 5)
#define HDMI_SPDIFIN_CFG_PCPD_AUTO_SET			(0 << 4)
#define HDMI_SPDIFIN_CFG_PCPD_MANUAL_SET			(1 << 4)
#define HDMI_SPDIFIN_CFG_WORD_LENGTH_A_SET			(0 << 3)
#define HDMI_SPDIFIN_CFG_WORD_LENGTH_M_SET			(1 << 3)
#define HDMI_SPDIFIN_CFG_U_V_C_P_NEGLECT			(0 << 2)
#define HDMI_SPDIFIN_CFG_U_V_C_P_REPORT			(1 << 2)
#define HDMI_SPDIFIN_CFG_BURST_SIZE_1			(0 << 1)
#define HDMI_SPDIFIN_CFG_BURST_SIZE_2			(1 << 1)
#define HDMI_SPDIFIN_CFG_DATA_ALIGN_16BIT			(0 << 0)
#define HDMI_SPDIFIN_CFG_DATA_ALIGN_32BIT			(1 << 0)

/* SPDIFIN_CONFIG_2 */
#define HDMI_SPDIFIN_CFG2_NO_CLK_DIV			(0)

/* HDMI eFUSE registers */
#define HDMI_EFUSE_CTRL			HDMI_EFUSE_BASE(0x000)
#define HDMI_EFUSE_STATUS		HDMI_EFUSE_BASE(0x004)
#define HDMI_EFUSE_ADDR_WIDTH		HDMI_EFUSE_BASE(0x008)
#define HDMI_EFUSE_SIGDEV_ASSERT	HDMI_EFUSE_BASE(0x00c)
#define HDMI_EFUSE_SIGDEV_DE_ASSERT	HDMI_EFUSE_BASE(0x010)
#define HDMI_EFUSE_PRCHG_ASSERT		HDMI_EFUSE_BASE(0x014)
#define HDMI_EFUSE_PRCHG_DE_ASSERT	HDMI_EFUSE_BASE(0x018)
#define HDMI_EFUSE_FSET_ASSERT		HDMI_EFUSE_BASE(0x01c)
#define HDMI_EFUSE_FSET_DE_ASSERT	HDMI_EFUSE_BASE(0x020)
#define HDMI_EFUSE_SENSING		HDMI_EFUSE_BASE(0x024)
#define HDMI_EFUSE_SCK_ASSERT		HDMI_EFUSE_BASE(0x028)
#define HDMI_EFUSE_SCK_DE_ASSERT	HDMI_EFUSE_BASE(0x02c)
#define HDMI_EFUSE_SDOUT_OFFSET		HDMI_EFUSE_BASE(0x030)
#define HDMI_EFUSE_READ_OFFSET		HDMI_EFUSE_BASE(0x034)

/* I2S bit definition */

/* I2S_CLK_CON */
#define HDMI_I2S_CLK_DIS		(0)
#define HDMI_I2S_CLK_EN			(1)

/* I2S_CON_1 */
#define HDMI_I2S_SCLK_FALLING_EDGE	(0 << 1)
#define HDMI_I2S_SCLK_RISING_EDGE	(1 << 1)
#define HDMI_I2S_L_CH_LOW_POL		(0)
#define HDMI_I2S_L_CH_HIGH_POL		(1)

/* I2S_CON_2 */
#define HDMI_I2S_MSB_FIRST_MODE		(0 << 6)
#define HDMI_I2S_LSB_FIRST_MODE		(1 << 6)
#define HDMI_I2S_BIT_CH_32FS		(0 << 4)
#define HDMI_I2S_BIT_CH_48FS		(1 << 4)
#define HDMI_I2S_BIT_CH_RESERVED	(2 << 4)
#define HDMI_I2S_SDATA_16BIT		(1 << 2)
#define HDMI_I2S_SDATA_20BIT		(2 << 2)
#define HDMI_I2S_SDATA_24BIT		(3 << 2)
#define HDMI_I2S_BASIC_FORMAT		(0)
#define HDMI_I2S_L_JUST_FORMAT		(2)
#define HDMI_I2S_R_JUST_FORMAT		(3)
#define HDMI_I2S_CON_2_CLR		(~(0xFF))
#define HDMI_I2S_SET_BIT_CH(x)		(((x) & 0x7) << 4)
#define HDMI_I2S_SET_SDATA_BIT(x)	(((x) & 0x7) << 2)

/* I2S_PIN_SEL_0 */
#define HDMI_I2S_SEL_SCLK(x)		(((x) & 0x7) << 4)
#define HDMI_I2S_SEL_SCLK_DEFAULT_1	(0x7 << 4)
#define HDMI_I2S_SEL_LRCK(x)		((x) & 0x7)
#define HDMI_I2S_SEL_LRCK_DEFAULT_0	(0x7)

/* I2S_PIN_SEL_1 */
#define HDMI_I2S_SEL_SDATA1(x)		(((x) & 0x7) << 4)
#define HDMI_I2S_SEL_SDATA1_DEFAULT_3	(0x7 << 4)
#define HDMI_I2S_SEL_SDATA2(x)		((x) & 0x7)
#define HDMI_I2S_SEL_SDATA2_DEFAULT_2	(0x7)

/* I2S_PIN_SEL_2 */
#define HDMI_I2S_SEL_SDATA3(x)		(((x) & 0x7) << 4)
#define HDMI_I2S_SEL_SDATA3_DEFAULT_5	(0x7 << 4)
#define HDMI_I2S_SEL_SDATA2(x)		((x) & 0x7)
#define HDMI_I2S_SEL_SDATA2_DEFAULT_4	(0x7)

/* I2S_PIN_SEL_3 */
#define HDMI_I2S_SEL_DSD(x)		((x) & 0x7)
#define HDMI_I2S_SEL_DSD_DEFAULT_6	(0x7)

/* I2S_DSD_CON */
#define HDMI_I2S_DSD_CLK_RI_EDGE	(1 << 1)
#define HDMI_I2S_DSD_CLK_FA_EDGE	(0 << 1)
#define HDMI_I2S_DSD_ENABLE		(1)
#define HDMI_I2S_DSD_DISABLE		(0)

/* I2S_MUX_CON */
#define HDMI_I2S_NOISE_FILTER_ZERO	(0 << 5)
#define HDMI_I2S_NOISE_FILTER_2_STAGE	(1 << 5)
#define HDMI_I2S_NOISE_FILTER_3_STAGE	(2 << 5)
#define HDMI_I2S_NOISE_FILTER_4_STAGE	(3 << 5)
#define HDMI_I2S_NOISE_FILTER_5_STAGE	(4 << 5)
#define HDMI_I2S_IN_DISABLE		(1 << 4)
#define HDMI_I2S_IN_ENABLE		(0 << 4)
#define HDMI_I2S_AUD_SPDIF		(0 << 2)
#define HDMI_I2S_AUD_I2S		(1 << 2)
#define HDMI_I2S_AUD_DSD		(2 << 2)
#define HDMI_I2S_CUV_SPDIF_ENABLE	(0 << 1)
#define HDMI_I2S_CUV_I2S_ENABLE		(1 << 1)
#define HDMI_I2S_MUX_DISABLE		(0)
#define HDMI_I2S_MUX_ENABLE		(1)
#define HDMI_I2S_MUX_CON_CLR		(~(0xFF))

/* I2S_CH_ST_CON */
#define HDMI_I2S_CH_STATUS_RELOAD	(1)
#define HDMI_I2S_CH_ST_CON_CLR		(~(1))

/* I2S_CH_ST_0 / I2S_CH_ST_SH_0 */
#define HDMI_I2S_CH_STATUS_MODE_0	(0 << 6)
#define HDMI_I2S_2AUD_CH_WITHOUT_PREEMPH	(0 << 3)
#define HDMI_I2S_2AUD_CH_WITH_PREEMPH	(1 << 3)
#define HDMI_I2S_DEFAULT_EMPHASIS	(0 << 3)
#define HDMI_I2S_COPYRIGHT		(0 << 2)
#define HDMI_I2S_NO_COPYRIGHT		(1 << 2)
#define HDMI_I2S_LINEAR_PCM		(0 << 1)
#define HDMI_I2S_NO_LINEAR_PCM		(1 << 1)
#define HDMI_I2S_CONSUMER_FORMAT	(0)
#define HDMI_I2S_PROF_FORMAT		(1)
#define HDMI_I2S_CH_ST_0_CLR		(~(0xFF))

/* I2S_CH_ST_1 / I2S_CH_ST_SH_1 */
#define HDMI_I2S_CD_PLAYER		(0x00)
#define HDMI_I2S_DAT_PLAYER		(0x03)
#define HDMI_I2S_DCC_PLAYER		(0x43)
#define HDMI_I2S_MINI_DISC_PLAYER	(0x49)

/* I2S_CH_ST_2 / I2S_CH_ST_SH_2 */
#define HDMI_I2S_CHANNEL_NUM_MASK	(0xF << 4)
#define HDMI_I2S_SOURCE_NUM_MASK	(0xF)
#define HDMI_I2S_SET_CHANNEL_NUM(x)	(((x) & (0xF)) << 4)
#define HDMI_I2S_SET_SOURCE_NUM(x)	((x) & (0xF))

/* I2S_CH_ST_3 / I2S_CH_ST_SH_3 */
#define HDMI_I2S_CLK_ACCUR_LEVEL_1	(1 << 4)
#define HDMI_I2S_CLK_ACCUR_LEVEL_2	(0 << 4)
#define HDMI_I2S_CLK_ACCUR_LEVEL_3	(2 << 4)
#define HDMI_I2S_SMP_FREQ_44_1		(0x0)
#define HDMI_I2S_SMP_FREQ_48		(0x2)
#define HDMI_I2S_SMP_FREQ_32		(0x3)
#define HDMI_I2S_SMP_FREQ_96		(0xA)
#define HDMI_I2S_SET_SMP_FREQ(x)	((x) & (0xF))

/* I2S_CH_ST_4 / I2S_CH_ST_SH_4 */
#define HDMI_I2S_ORG_SMP_FREQ_44_1	(0xF << 4)
#define HDMI_I2S_ORG_SMP_FREQ_88_2	(0x7 << 4)
#define HDMI_I2S_ORG_SMP_FREQ_22_05	(0xB << 4)
#define HDMI_I2S_ORG_SMP_FREQ_176_4	(0x3 << 4)
#define HDMI_I2S_WORD_LEN_NOT_DEFINE	(0x0 << 1)
#define HDMI_I2S_WORD_LEN_MAX24_20BITS	(0x1 << 1)
#define HDMI_I2S_WORD_LEN_MAX24_22BITS	(0x2 << 1)
#define HDMI_I2S_WORD_LEN_MAX24_23BITS	(0x4 << 1)
#define HDMI_I2S_WORD_LEN_MAX24_24BITS	(0x5 << 1)
#define HDMI_I2S_WORD_LEN_MAX24_21BITS	(0x6 << 1)
#define HDMI_I2S_WORD_LEN_MAX20_16BITS	(0x1 << 1)
#define HDMI_I2S_WORD_LEN_MAX20_18BITS	(0x2 << 1)
#define HDMI_I2S_WORD_LEN_MAX20_19BITS	(0x4 << 1)
#define HDMI_I2S_WORD_LEN_MAX20_20BITS	(0x5 << 1)
#define HDMI_I2S_WORD_LEN_MAX20_17BITS	(0x6 << 1)
#define HDMI_I2S_WORD_LEN_MAX_24BITS	(1)
#define HDMI_I2S_WORD_LEN_MAX_20BITS	(0)

/* I2S_VD_DATA */
#define HDMI_I2S_VD_AUD_SMP_RELIABLE	(0)
#define HDMI_I2S_VD_AUD_SMP_UNRELIABLE	(1)

/* I2S_MUX_CH */
#define HDMI_I2S_CH3_R_EN		(1 << 7)
#define HDMI_I2S_CH3_L_EN		(1 << 6)
#define HDMI_I2S_CH3_EN			(3 << 6)
#define HDMI_I2S_CH2_R_EN		(1 << 5)
#define HDMI_I2S_CH2_L_EN		(1 << 4)
#define HDMI_I2S_CH2_EN			(3 << 4)
#define HDMI_I2S_CH1_R_EN		(1 << 3)
#define HDMI_I2S_CH1_L_EN		(1 << 2)
#define HDMI_I2S_CH1_EN			(3 << 2)
#define HDMI_I2S_CH0_R_EN		(1 << 1)
#define HDMI_I2S_CH0_L_EN		(1)
#define HDMI_I2S_CH0_EN			(3)
#define HDMI_I2S_CH_ALL_EN		(0xFF)
#define HDMI_I2S_MUX_CH_CLR		(~HDMI_I2S_CH_ALL_EN)

/* I2S_MUX_CUV */
#define HDMI_I2S_CUV_R_EN		(1 << 1)
#define HDMI_I2S_CUV_L_EN		(1)
#define HDMI_I2S_CUV_RL_EN		(0x03)

/* I2S_IRQ_MASK */
#define HDMI_I2S_INT2_DIS		(0 << 1)
#define HDMI_I2S_INT2_EN		(1 << 1)

/* I2S_IRQ_STATUS */
#define HDMI_I2S_INT2_STATUS		(1 << 1)

/* I2S_CUV_L_R */
#define HDMI_I2S_CUV_R_DATA_MASK	(0x7 << 4)
#define HDMI_I2S_CUV_L_DATA_MASK	(0x7)

/* Audio Related Packet bit definition */

/* ASP_CON */
#define HDMI_AUD_DST_DOUBLE		(1 << 7)
#define HDMI_AUD_NO_DST_DOUBLE		(0 << 7)
#define HDMI_AUD_TYPE_SAMPLE		(0 << 5)
#define HDMI_AUD_TYPE_ONE_BIT		(1 << 5)
#define HDMI_AUD_TYPE_HBR		(2 << 5)
#define HDMI_AUD_TYPE_DST		(3 << 5)
#define HDMI_AUD_MODE_TWO_CH		(0 << 4)
#define HDMI_AUD_MODE_MULTI_CH		(1 << 4)
#define HDMI_AUD_SP_AUD3_EN		(1 << 3)
#define HDMI_AUD_SP_AUD2_EN		(1 << 2)
#define HDMI_AUD_SP_AUD1_EN		(1 << 1)
#define HDMI_AUD_SP_AUD0_EN		(1 << 0)
#define HDMI_AUD_SP_ALL_DIS		(0 << 0)

#define HDMI_AUD_SET_SP_PRE(x)		((x) & 0xF)

/* ASP_SP_FLAT */
#define HDMI_ASP_SP_FLAT_AUD_SAMPLE	(0)

/* ASP_CHCFG0/1/2/3 */
#define HDMI_SPK3R_SEL_I_PCM0L		(0 << 27)
#define HDMI_SPK3R_SEL_I_PCM0R		(1 << 27)
#define HDMI_SPK3R_SEL_I_PCM1L		(2 << 27)
#define HDMI_SPK3R_SEL_I_PCM1R		(3 << 27)
#define HDMI_SPK3R_SEL_I_PCM2L		(4 << 27)
#define HDMI_SPK3R_SEL_I_PCM2R		(5 << 27)
#define HDMI_SPK3R_SEL_I_PCM3L		(6 << 27)
#define HDMI_SPK3R_SEL_I_PCM3R		(7 << 27)
#define HDMI_SPK3L_SEL_I_PCM0L		(0 << 24)
#define HDMI_SPK3L_SEL_I_PCM0R		(1 << 24)
#define HDMI_SPK3L_SEL_I_PCM1L		(2 << 24)
#define HDMI_SPK3L_SEL_I_PCM1R		(3 << 24)
#define HDMI_SPK3L_SEL_I_PCM2L		(4 << 24)
#define HDMI_SPK3L_SEL_I_PCM2R		(5 << 24)
#define HDMI_SPK3L_SEL_I_PCM3L		(6 << 24)
#define HDMI_SPK3L_SEL_I_PCM3R		(7 << 24)
#define HDMI_SPK2R_SEL_I_PCM0L		(0 << 19)
#define HDMI_SPK2R_SEL_I_PCM0R		(1 << 19)
#define HDMI_SPK2R_SEL_I_PCM1L		(2 << 19)
#define HDMI_SPK2R_SEL_I_PCM1R		(3 << 19)
#define HDMI_SPK2R_SEL_I_PCM2L		(4 << 19)
#define HDMI_SPK2R_SEL_I_PCM2R		(5 << 19)
#define HDMI_SPK2R_SEL_I_PCM3L		(6 << 19)
#define HDMI_SPK2R_SEL_I_PCM3R		(7 << 19)
#define HDMI_SPK2L_SEL_I_PCM0L		(0 << 16)
#define HDMI_SPK2L_SEL_I_PCM0R		(1 << 16)
#define HDMI_SPK2L_SEL_I_PCM1L		(2 << 16)
#define HDMI_SPK2L_SEL_I_PCM1R		(3 << 16)
#define HDMI_SPK2L_SEL_I_PCM2L		(4 << 16)
#define HDMI_SPK2L_SEL_I_PCM2R		(5 << 16)
#define HDMI_SPK2L_SEL_I_PCM3L		(6 << 16)
#define HDMI_SPK2L_SEL_I_PCM3R		(7 << 16)
#define HDMI_SPK1R_SEL_I_PCM0L		(0 << 11)
#define HDMI_SPK1R_SEL_I_PCM0R		(1 << 11)
#define HDMI_SPK1R_SEL_I_PCM1L		(2 << 11)
#define HDMI_SPK1R_SEL_I_PCM1R		(3 << 11)
#define HDMI_SPK1R_SEL_I_PCM2L		(4 << 11)
#define HDMI_SPK1R_SEL_I_PCM2R		(5 << 11)
#define HDMI_SPK1R_SEL_I_PCM3L		(6 << 11)
#define HDMI_SPK1R_SEL_I_PCM3R		(7 << 11)
#define HDMI_SPK1L_SEL_I_PCM0L		(0 << 8)
#define HDMI_SPK1L_SEL_I_PCM0R		(1 << 8)
#define HDMI_SPK1L_SEL_I_PCM1L		(2 << 8)
#define HDMI_SPK1L_SEL_I_PCM1R		(3 << 8)
#define HDMI_SPK1L_SEL_I_PCM2L		(4 << 8)
#define HDMI_SPK1L_SEL_I_PCM2R		(5 << 8)
#define HDMI_SPK1L_SEL_I_PCM3L		(6 << 8)
#define HDMI_SPK1L_SEL_I_PCM3R		(7 << 8)
#define HDMI_SPK0R_SEL_I_PCM0L		(0 << 3)
#define HDMI_SPK0R_SEL_I_PCM0R		(1 << 3)
#define HDMI_SPK0R_SEL_I_PCM1L		(2 << 3)
#define HDMI_SPK0R_SEL_I_PCM1R		(3 << 3)
#define HDMI_SPK0R_SEL_I_PCM2L		(4 << 3)
#define HDMI_SPK0R_SEL_I_PCM2R		(5 << 3)
#define HDMI_SPK0R_SEL_I_PCM3L		(6 << 3)
#define HDMI_SPK0R_SEL_I_PCM3R		(7 << 3)
#define HDMI_SPK0L_SEL_I_PCM0L		(0)
#define HDMI_SPK0L_SEL_I_PCM0R		(1)
#define HDMI_SPK0L_SEL_I_PCM1L		(2)
#define HDMI_SPK0L_SEL_I_PCM1R		(3)
#define HDMI_SPK0L_SEL_I_PCM2L		(4)
#define HDMI_SPK0L_SEL_I_PCM2R		(5)
#define HDMI_SPK0L_SEL_I_PCM3L		(6)
#define HDMI_SPK0L_SEL_I_PCM3R		(7)

/* ACR_CON */
#define HDMI_ALT_CTS_RATE_CTS_1		(0 << 3)
#define HDMI_ALT_CTS_RATE_CTS_11	(1 << 3)
#define HDMI_ALT_CTS_RATE_CTS_21	(2 << 3)
#define HDMI_ALT_CTS_RATE_CTS_31	(3 << 3)
#define HDMI_ACR_TX_MODE_NO_TX		(0)
#define HDMI_ACR_TX_MODE_TX_ONCE	(1)
#define HDMI_ACR_TX_MODE_TXCNT_VBI	(2)
#define HDMI_ACR_TX_MODE_TX_VPC		(3)
#define HDMI_ACR_TX_MODE_MESURE_CTS	(4)

/* ACR_MCTS0/1/2 */

/* ACR_CTS0/1/2 */

/* ACR_N0/1/2 */

/* ACR_LSB2 */
#define HDMI_ACR_LSB2_MASK		(0xFF)

/* ACR_TXCNT */
#define HDMI_ACR_TXCNT_MASK		(0x1F)

/* ACR_TXINTERNAL */
#define HDMI_ACR_TX_INTERNAL_MASK	(0xFF)

/* ACR_CTS_OFFSET */
#define HDMI_ACR_CTS_OFFSET_MASK	(0xFF)

/* GCP_CON */
#define HDMI_GCP_CON_EN_1ST_VSYNC	(1 << 3)
#define HDMI_GCP_CON_EN_2ST_VSYNC	(1 << 2)
#define HDMI_GCP_CON_TRANS_EVERY_VSYNC	(2)
#define HDMI_GCP_CON_NO_TRAN		(0)
#define HDMI_GCP_CON_TRANS_ONCE		(1)
#define HDMI_GCP_CON_TRANS_EVERY_VSYNC	(2)

/* GCP_BYTE1 */
#define HDMI_GCP_BYTE1_MASK		(0xFF)

/* GCP_BYTE2 */
#define HDMI_GCP_BYTE2_PP_MASK		(0xF << 4)
#define HDMI_GCP_24BPP			(1 << 2)
#define HDMI_GCP_30BPP			(1 << 0 | 1 << 2)
#define HDMI_GCP_36BPP			(1 << 1 | 1 << 2)
#define HDMI_GCP_48BPP			(1 << 0 | 1 << 1 | 1 << 2)

/* GCP_BYTE3 */
#define HDMI_GCP_BYTE3_MASK		(0xFF)

/* HDCP E-FUSE Control Register */
/* HDCP_E_FUSE_CTRL */
#define HDMI_EFUSE_CTRL_HDCP_KEY_READ		(1 << 0)

/* HDCP_E_FUSE_STATUS */
#define HDMI_EFUSE_ECC_FAIL			(1 << 2)
#define HDMI_EFUSE_ECC_BUSY			(1 << 1)
#define HDMI_EFUSE_ECC_DONE			(1)

/* EFUSE_ADDR_WIDTH */
/* EFUSE_SIGDEV_ASSERT */
/* EFUSE_SIGDEV_DE-ASSERT */
/* EFUSE_PRCHG_ASSERT */
/* EFUSE_PRCHG_DE-ASSERT */
/* EFUSE_FSET_ASSERT */
/* EFUSE_FSET_DE-ASSERT */
/* EFUSE_SENSING */
/* EFUSE_SCK_ASSERT */
/* EFUSE_SCK_DEASSERT */
/* EFUSE_SDOUT_OFFSET */
/* EFUSE_READ_OFFSET */

/* HDCP_SHA_RESULT */
#define HDMI_HDCP_SHA_VALID_NO_RD		(0 << 1)
#define HDMI_HDCP_SHA_VALID_RD			(1 << 1)
#define HDMI_HDCP_SHA_VALID			(1)
#define HDMI_HDCP_SHA_NO_VALID			(0)

/* Audio InfoFrame Register */

/* AUI_CON */
#define HDMI_AUI_CON_NO_TRAN			(0 << 0)
#define HDMI_AUI_CON_TRANS_ONCE			(1 << 0)
#define HDMI_AUI_CON_TRANS_EVERY_VSYNC		(2 << 0)

/* Timing generator registers */
/* TG configure/status registers */
#define HDMI_TG_VACT_ST3_L		HDMI_TG_BASE(0x0068)
#define HDMI_TG_VACT_ST3_H		HDMI_TG_BASE(0x006c)
#define HDMI_TG_VACT_ST4_L		HDMI_TG_BASE(0x0070)
#define HDMI_TG_VACT_ST4_H		HDMI_TG_BASE(0x0074)
#define HDMI_TG_3D			HDMI_TG_BASE(0x00F0)

#endif /* SAMSUNG_REGS_HDMI_H */
