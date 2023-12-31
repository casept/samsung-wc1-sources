/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define A2DP_CODEC_SBC			0x00
#define A2DP_CODEC_MPEG12		0x01
#define A2DP_CODEC_MPEG24		0x02
#define A2DP_CODEC_ATRAC		0x03
#ifdef __TIZEN_BT__
#define A2DP_CODEC_VENDOR               0xFF
#endif
#define A2DP_CODEC_NON_A2DP		0xFF

#define SBC_SAMPLING_FREQ_16000		(1 << 3)
#define SBC_SAMPLING_FREQ_32000		(1 << 2)
#define SBC_SAMPLING_FREQ_44100		(1 << 1)
#define SBC_SAMPLING_FREQ_48000		1

#define SBC_CHANNEL_MODE_MONO		(1 << 3)
#define SBC_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define SBC_CHANNEL_MODE_STEREO		(1 << 1)
#define SBC_CHANNEL_MODE_JOINT_STEREO	1

#define SBC_BLOCK_LENGTH_4		(1 << 3)
#define SBC_BLOCK_LENGTH_8		(1 << 2)
#define SBC_BLOCK_LENGTH_12		(1 << 1)
#define SBC_BLOCK_LENGTH_16		1

#define SBC_SUBBANDS_4			(1 << 1)
#define SBC_SUBBANDS_8			1

#define SBC_ALLOCATION_SNR		(1 << 1)
#define SBC_ALLOCATION_LOUDNESS		1

#define MPEG_CHANNEL_MODE_MONO		(1 << 3)
#define MPEG_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define MPEG_CHANNEL_MODE_STEREO	(1 << 1)
#define MPEG_CHANNEL_MODE_JOINT_STEREO	1

#define MPEG_LAYER_MP1			(1 << 2)
#define MPEG_LAYER_MP2			(1 << 1)
#define MPEG_LAYER_MP3			1

#define MPEG_SAMPLING_FREQ_16000	(1 << 5)
#define MPEG_SAMPLING_FREQ_22050	(1 << 4)
#define MPEG_SAMPLING_FREQ_24000	(1 << 3)
#define MPEG_SAMPLING_FREQ_32000	(1 << 2)
#define MPEG_SAMPLING_FREQ_44100	(1 << 1)
#define MPEG_SAMPLING_FREQ_48000	1


#ifdef __TIZEN_BT__
#define MAX_BITPOOL 35
#else
#define MAX_BITPOOL 64
#endif
#define MIN_BITPOOL 2

#ifdef BLUETOOTH_APTX_SUPPORT
/*#define APTX_CHANNEL_MODE_STEREO			2 */
/*
 * aptX codec for Bluetooth only supports stereo mode with value 2
 * But we do have sink devices programmed to send capabilities with other channel mode support.
 * So to handle the case and keeping codec symmetry with SBC etc., we do define other channel mode,
 * and we always make sure to set configuration with APTX_CHANNEL_MODE_STEREO only.
 *
 * */

#define APTX_CHANNEL_MODE_MONO			(1 << 3)
#define APTX_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define APTX_CHANNEL_MODE_STEREO		(1 << 1)
#define APTX_CHANNEL_MODE_JOINT_STEREO	1

#define APTX_VENDOR_ID0		0x4F /*APTX codec ID 79*/
#define APTX_VENDOR_ID1		0x0
#define APTX_VENDOR_ID2		0x0
#define APTX_VENDOR_ID3		0x0

#define APTX_CODEC_ID0		0x1
#define APTX_CODEC_ID1		0x0

#define APTX_SAMPLING_FREQ_16000	(1 << 3)
#define APTX_SAMPLING_FREQ_32000	(1 << 2)
#define APTX_SAMPLING_FREQ_44100	(1 << 1)
#define APTX_SAMPLING_FREQ_48000	1
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN

typedef struct {
	uint8_t channel_mode:4;
	uint8_t frequency:4;
	uint8_t allocation_method:2;
	uint8_t subbands:2;
	uint8_t block_length:4;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed)) a2dp_sbc_t;

typedef struct {
	uint8_t channel_mode:4;
	uint8_t crc:1;
	uint8_t layer:3;
	uint8_t frequency:6;
	uint8_t mpf:1;
	uint8_t rfa:1;
	uint16_t bitrate;
} __attribute__ ((packed)) a2dp_mpeg_t;

#ifdef BLUETOOTH_APTX_SUPPORT
typedef struct {
	uint8_t vendor_id[4];
	uint8_t codec_id[2];
	uint8_t channel_mode:4;
	uint8_t frequency:4;
} __attribute__ ((packed)) a2dp_aptx_t;
#endif
#elif __BYTE_ORDER == __BIG_ENDIAN

typedef struct {
	uint8_t frequency:4;
	uint8_t channel_mode:4;
	uint8_t block_length:4;
	uint8_t subbands:2;
	uint8_t allocation_method:2;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed)) a2dp_sbc_t;

typedef struct {
	uint8_t layer:3;
	uint8_t crc:1;
	uint8_t channel_mode:4;
	uint8_t rfa:1;
	uint8_t mpf:1;
	uint8_t frequency:6;
	uint16_t bitrate;
} __attribute__ ((packed)) a2dp_mpeg_t;

#ifdef BLUETOOTH_APTX_SUPPORT
typedef struct {
	uint8_t vendor_id[4];
	uint8_t codec_id[2];
	uint8_t frequency:4;
	uint8_t channel_mode:4;
} __attribute__ ((packed)) a2dp_aptx_t;
#endif

#else
#error "Unknown byte order"
#endif
