/*
********************************************************************
*    File Name: bcm5935x_adc.c
*
*    Abstract: Functions for BCM5935x
*
*    $History:$
*
********************************************************************
*/

#include <linux/battery/bcm5935x_adc.h>

#define VBUCK_TRIM_LSB_BITS 2
#define IBUCK_TRIM_LSB_BITS 2
#define VRECT_TRIM_LSB_BITS 2
#define IRECT_TRIM_LSB_BITS 2

#define OFFSET_TRIM_IRECT_LSB_1_0       0xC0
#define OFFSET_TRIM_IRECT_LSB_1_0_SHIFT 6
#define OFFSET_TRIM_VRECT_LSB_1_0       0x30
#define OFFSET_TRIM_VRECT_LSB_1_0_SHIFT 4
#define OFFSET_TRIM_IBUCK_LSB_1_0       0x0C
#define OFFSET_TRIM_IBUCK_LSB_1_0_SHIFT 2
#define OFFSET_TRIM_VBUCK_LSB_1_0       0x03
#define OFFSET_TRIM_VBUCK_LSB_1_0_SHIFT 0

#define CONVERT_SHIFT_RIGHT	8
#define M_VRECT			6000
#define C_VRECT 		0
#define M_VBUCK 		1650*2
#define C_VBUCK			0

#define WPC_GET_MAXPWR_FROM_REG_VALUE(value)        (value & 0x3F)
#define WPC_GET_PWRCLS_FROM_REG_VALUE(value)        ((value >> 6) & 0x3)

uint32 p_max;
#define A4WP_WPC_GET_WINDOW_FROM_REG_VALUE(value)   ((value >> 2) & 0x3F)
#define A4WP_WPC_GET_WINDOW_OFFSET_FROM_REG_VALUE(value) (value & 0x03)

#define A4WP_WPC_TIME_BETWEEN_CES_AND_RPP       160
#define A4WP_WPC_SAMPLE_TIME_INTERVAL           4
#define A4WP_WPC_MAX_POWER_SAMPLES              16

int32 g_m0 = 0;
int32 g_m1 = 0;
int32 g_c1 = 0;
int32 g_c2 = 0;
int32 g_s0 = 1000;

int32 g_c0 = 1000;
int32 g_v0 = 970;
int32 g_v1 = 0;
int32 g_p0 = 900;
int32 g_p1 = 0;
int32 g_i_rect0 = 64;
int32 g_p2 = 0;
int32 g_p3 = 0;
int32 g_mt = 0;
int32 g_v2 = 0;
int32 g_v3 = 0;

int32 g_i_buck_m1_base = -1280;
int32 g_i_buck_m0_base = -1280;

int32 g_i_buck_c1_base = -8;
int32 g_i_buck_c2_base = -8;

int32 g_i_buck_s0_base = 920;
int32 g_i_rect0_base = 20;
int32 g_c0_base = 850;
int32 g_v0_base = 850;
int32 g_v1_base = -15;
int32 g_p0_base = 850;
int32 g_p1_base = -15;
int32 g_p2_base = -16;
int32 g_p3_base = -16;
int32 g_mt_base = -75;
int32 g_v2_base = -16;
int32 g_v3_base = -16;

void bcm59350_read_adc_init(uint8 reg_0xb4, uint8 reg_0xb5, uint8 reg_0xb6, uint8 reg_0xb7, uint8 reg_0x141,
                                  uint8 reg_0x143, uint8 reg_0x144, uint8 reg_0x145, uint8 reg_0x146, uint8 reg_0x147) {
    uint8 index;
	uint32 value;

    value = reg_0xb6;
    g_m0 = g_i_buck_m0_base + (value * 10);

    value = reg_0xb7;
    g_m1 = g_i_buck_m1_base + (value * 10);

    value = reg_0xb5;
    index = value & 0x0F;
    g_c1 = g_i_buck_c1_base + index;

    index = (value >> 4) & 0x0F;
    g_c2 = g_i_buck_c2_base + index;

    value = reg_0xb4;
    index = value & 0x0F;
    g_s0 = g_i_buck_s0_base + (index * 10);

    value = reg_0x141;
    index = value & 0x0F;
    g_i_rect0 = g_i_rect0_base + (index * 4);

    value = reg_0x143;
    index = value & 0x0F;
    g_c0 = g_c0_base + (index * 10);

    index = (value >> 4) & 0x0F;
    g_mt = g_mt_base + (index * 5);

    value = reg_0x144;
    index = value & 0x0F;
    g_v0 = g_v0_base + (index * 10);

    index = (value >> 4) & 0x0F;
    g_v1 = g_v1_base + (index * 1);

    value = reg_0x145;
	index = value & 0x0F;
    g_v2 = g_v2_base + (index * 2);

    index = (value >> 4) & 0x0F;
    g_v3 = g_v3_base + (index * 2);

    value = reg_0x146;
	index = value & 0x0F;
    g_p0 = g_p0_base + (index * 10);

    index = (value >> 4) & 0x0F;
    g_p1 = g_p1_base + (index * 1);

    value = reg_0x147;
	index = value & 0x0F;
    g_p2 = g_p2_base + (index * 2);

    index = (value >> 4) & 0x0F;
    g_p3 = g_p3_base + (index * 2);

}

void bcm59350_power_packet_init(uint8 reg_0x11a, uint8 reg_0x11b, uint8 *max_power_samples, uint16 *wait_time_for_sample)
{
    uint8 val;
    uint32 pow_of_10[4] = {1, 10, 100, 1000};

    val = reg_0x11a;

    p_max =((WPC_GET_MAXPWR_FROM_REG_VALUE(val) * 1000) >> 1) *
            pow_of_10[WPC_GET_PWRCLS_FROM_REG_VALUE(val)];

    *max_power_samples =
            A4WP_WPC_GET_WINDOW_FROM_REG_VALUE(reg_0x11b);

    if (*max_power_samples > A4WP_WPC_MAX_POWER_SAMPLES)
    {
        *max_power_samples = A4WP_WPC_MAX_POWER_SAMPLES;
    }

    *wait_time_for_sample =
           A4WP_WPC_TIME_BETWEEN_CES_AND_RPP -
           (A4WP_WPC_GET_WINDOW_OFFSET_FROM_REG_VALUE(reg_0x11b) * 4) -
           ((*max_power_samples - 1) * A4WP_WPC_SAMPLE_TIME_INTERVAL);
}

uint32 get_bcm59350_vrect_trim (uint8 reg_0x8b, uint8 reg_0x8d) {
   uint32 trim, trimLsb, vRectTrim;
   
   trimLsb = reg_0x8d;
   trim = reg_0x8b;
   trim &= 0xFF;
   trim <<= VRECT_TRIM_LSB_BITS;
   vRectTrim = trim | ((trimLsb & OFFSET_TRIM_VRECT_LSB_1_0)>>OFFSET_TRIM_VRECT_LSB_1_0_SHIFT);
   return vRectTrim;
}

uint32 get_bcm59350_vbuck_trim (uint8 reg_0x89, uint8 reg_0x8d) {
   uint32 trim, trimLsb, vBuckTrim;

   trimLsb = reg_0x8d;
   trim = reg_0x89;
   trim &= 0xFF;
   trim <<= VBUCK_TRIM_LSB_BITS;
   vBuckTrim = trim | ((trimLsb & OFFSET_TRIM_VBUCK_LSB_1_0)>>OFFSET_TRIM_VBUCK_LSB_1_0_SHIFT);
   return vBuckTrim;
}

uint32 get_bcm59350_vrect (uint8 reg_0x8b, uint8 reg_0x8d, uint8 reg_0x0a, uint8 reg_0x11) {
   uint32 r;
   r = reg_0x0a;
   r <<=4;
   r |= reg_0x11 & 0xf;
   r >>=2;
   r -= get_bcm59350_vrect_trim(reg_0x8b, reg_0x8d);

   r = (r*M_VRECT + C_VRECT)>>CONVERT_SHIFT_RIGHT;
   return r;
}

uint32 get_bcm59350_vbuck (uint8 reg_0x89, uint8 reg_0x8d, uint8 reg_0x0c, uint8 reg_0x12) {
   uint32 r;
   r = reg_0x0c;
   r <<=4;
   r |= reg_0x12 & 0xf;
   r >>=2;
   r -= get_bcm59350_vbuck_trim(reg_0x89, reg_0x8d);

   r = (r*M_VBUCK + C_VBUCK)>>CONVERT_SHIFT_RIGHT;
   return r;
}

uint32 get_bcm59350_tdie (uint8 reg_0x0f, uint8 reg_0x13) {
   uint32 r;
   r = reg_0x0f;
   r <<=4;
   r |= (reg_0x13 >> 4) & 0xf;
   r >>=2;
   r = (r>>1) - 275;
   return r;
}

uint32 get_bcm59350_ibuck (uint32 vrect, uint32 tdie, uint8 reg_0x0d, uint8 reg_0x12) {

   uint32 r;
   uint32 i_buck_adc = 0;
   uint32 tdie_adc;
   uint32 ibuck, t1, t2, nr, dr;
   r = reg_0x0d;
   r <<=4;
   r |= (reg_0x12 >> 4) & 0xf;
   r >>=2;

   i_buck_adc = (r * 1000 * 1000) / 1024;
   i_buck_adc = i_buck_adc * 3300;

   i_buck_adc = i_buck_adc/(1000 * 1000);

   tdie_adc = tdie - 27;

   t1 = (1000 - (g_c1 * tdie_adc));
   t2 = (g_m1 * t1) / 10000 ;
   t2  = (t2 * vrect)/1000;
   nr = ((i_buck_adc ) - (g_m0) - (t2));
   nr = nr * 1000;
   t2 = 10000 + (g_c2 * tdie_adc);
   dr = (g_s0 * t2) / 10000;

   t2 = (nr / dr);

   ibuck = (t2 > 0) ? t2 : 0;

   return ibuck;
}

uint32 get_bcm59350_irect (uint32 vrect, uint32 tdie, uint32 vbuck, uint32 ibuck) {
   uint32 r, tdie_adc;
   uint32 t1, t2, t3, p_buck, v_rect_comp, p_buck_comp, mt_comp, k;

   tdie_adc = tdie - 27;

   p_buck = (ibuck * vbuck) / 1000;
 
   t1 = (g_v1 * vrect);
   t2 = (vrect * vrect) / 1000;
   t3 = (t2 * vrect) / 1000;
   t2 = (t2 * g_v2) / 10;
   t3 = (t3 * g_v3) / 100;
 
   v_rect_comp = ((g_v0 * 1000) + t1 + t2 + t3);
 
   t1 = (g_p1 * p_buck);
   t2 = (p_buck * p_buck) / 1000;
   t3 = (t2 * p_buck) / 1000;
   t2 = (p_buck * g_p2) / 10;
   t3 = (t3 * g_p3) / 100;
 
   p_buck_comp = ((g_p0 * 1000) + t1 + t2 + t3);
 
   mt_comp = 100000 + (g_mt * tdie_adc);

   k = ((v_rect_comp / 100) * (p_buck_comp / 100)) / 1000;
   k = (g_c0 * k) / 10000;
   k = (k * (mt_comp / 10)) / 1000;
   t1 = (vrect * k) / 100000;

   r = (p_buck * 1000) / t1;
   r = r + g_i_rect0;
   return r;
}

uint32 get_bcm59350_receivedPower (uint32 vrect, uint32 irect)
{
	return (vrect * irect);
}

uint32 get_bcm59350_PowerRect_value (uint32 recieved_power, uint8 power_sample_count)
{
    uint32 temp = recieved_power;
    uint8  p_recv_val;

    if (power_sample_count)
    {
        temp = temp / power_sample_count;

        temp = (temp << 7) / p_max;
        temp += (temp & 0x200) ? 488 : 0;
        p_recv_val = (temp / 1000);

        return p_recv_val;
    }

	return 0;
}

