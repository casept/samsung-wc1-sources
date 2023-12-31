/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x37
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_DEBUG_DATA			0x03
#define MSG2AP_INST_BIG_DATA			0x04
#define MSG2AP_INST_META_DATA			0x05
#define MSG2AP_INST_TIME_SYNC			0x06
#define MSG2AP_INST_RESET			0x07
#define MSG2AP_INST_DUMP_DATA		0xDD

/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/

static void get_timestamp(struct ssp_data *data, char *pchRcvDataFrame,
		int *iDataIdx, struct sensor_value *sensorsdata)
{
	s32 otimestamp = 0;
	s64 ctimestamp = 0;

	memcpy(&otimestamp, pchRcvDataFrame + *iDataIdx, 4);
	*iDataIdx += 4;

	ctimestamp = (s64) otimestamp * 1000000;
	sensorsdata->timestamp = data->timestamp + ctimestamp;
}

#if defined(CONFIG_SENSORS_SSP_ACCELEROMETER_SENSOR) \
	|| defined(CONFIG_SENSORS_SSP_GYRO_SENSOR)
static void get_3axis_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}
#endif

#ifdef CONFIG_SENSORS_SSP_GYRO_SENSOR
static void get_uncalib_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}
#endif

#ifdef CONFIG_SENSORS_SSP_MAGNETIC_SENSOR
static void get_geomagnetic_uncaldata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}

static void get_geomagnetic_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_geomagnetic_caldata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 7);
	*iDataIdx += 7;
}
#endif

#ifdef CONFIG_SENSORS_SSP_PRESSURE_SENSOR
static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	s16 temperature = 0;
	memcpy(&sensorsdata->pressure[0], pchRcvDataFrame + *iDataIdx, 4);
	memcpy(&temperature, pchRcvDataFrame + *iDataIdx + 4, 2);
	sensorsdata->pressure[1] = temperature;
	*iDataIdx += 6;
}
#endif

#ifdef CONFIG_SENSORS_SSP_LIGHT_SENSOR
static void get_light_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 8);
	*iDataIdx += 8;
}
#endif

#ifdef CONFIG_SENSORS_SSP_TEMP_HUMID_SENSOR
static void get_temp_humidity_sensordata(char *dataframe, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, dataframe + *iDataIdx, 5);
	*iDataIdx += 5;
}
#endif

#ifdef CONFIG_SENSORS_SSP_ROT_VECTOR_SENSOR
static void get_rot_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 17);
	*iDataIdx += 17;
}
#endif

#ifdef CONFIG_SENSORS_SSP_HRM_SENSOR
static void get_hrm_raw_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 40);
	*iDataIdx += 40;
}

static void get_hrm_raw_fac_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 64);
	*iDataIdx += 64;
}

static void get_hrm_lib_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 8);
	*iDataIdx += 8;
}
#endif

#ifdef CONFIG_SENSORS_SSP_FRONT_HRM_SENSOR
static void get_front_hrm_raw_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}
#endif

#ifdef CONFIG_SENSORS_SSP_UV_SENSOR
static void get_uv_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 8);
	*iDataIdx += 8;
}
#endif

#ifdef CONFIG_SENSORS_SSP_GSR_SENSOR
static void get_gsr_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
}
#endif

#ifdef CONFIG_SENSORS_SSP_ECG_SENSOR
static void get_ecg_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}
#endif

#ifdef CONFIG_SENSORS_SSP_GRIP_SENSOR
static void get_grip_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 9);
	*iDataIdx += 9;
}
#endif

static void get_dummy_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	pr_info("[SSP] %s : not supported\n", __func__);
}

void report_dummy_data(struct ssp_data *data, struct sensor_value *value)
{
	pr_info("[SSP] %s : not supported\n", __func__);
}

int handle_big_data(struct ssp_data *data,
	char *pchRcvDataFrame, int *pDataIdx)
{
	u8 bigType = 0;
	struct ssp_big *big = kzalloc(sizeof(*big), GFP_KERNEL);
	big->data = data;
	bigType = pchRcvDataFrame[(*pDataIdx)++];
	memcpy(&big->length, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;
	memcpy(&big->addr, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;

	if (bigType >= BIG_TYPE_MAX) {
		kfree(big);
		return FAIL;
	}

	INIT_WORK(&big->work, data->ssp_big_task[bigType]);
	queue_work(data->debug_wq, &big->work);
	return SUCCESS;
}

void refresh_task(struct work_struct *work)
{
	struct ssp_data *data = container_of((struct delayed_work *)work,
			struct ssp_data, work_refresh);

	if (data->bSspShutdown == true) {
		pr_err("[SSP]: %s - ssp already shutdown\n", __func__);
		return;
	}

	wake_lock(&data->ssp_wake_lock);
	pr_err("[SSP]: %s\n", __func__);
	data->uResetCnt++;

	if (initialize_mcu(data) > 0) {
		sync_sensor_state(data);
		ssp_sensorhub_report_notice(data, MSG2SSP_AP_STATUS_RESET);
		if (data->uLastAPState != 0) {
			pr_err("[SSP]%s, Send last AP state(%d)\n", __func__,
				data->uLastAPState);
			ssp_send_cmd(data, data->uLastAPState, 0);
		}
		if (data->uLastResumeState != 0) {
			pr_err("[SSP]%s, Send last resume state(%d)\n",
				__func__, data->uLastResumeState);
			ssp_send_cmd(data, data->uLastResumeState, 0);
		}
		data->uTimeOutCnt = 0;
	} else
		data->uSensorState = 0;

	wake_unlock(&data->ssp_wake_lock);
#if SSP_STATUS_MONITOR
	data->bRefreshing =  false;
#endif
}

int queue_refresh_task(struct ssp_data *data, int delay)
{
	cancel_delayed_work_sync(&data->work_refresh);

	INIT_DELAYED_WORK(&data->work_refresh, refresh_task);
	queue_delayed_work(data->debug_wq, &data->work_refresh,
			msecs_to_jiffies(delay));
	return SUCCESS;
}

int parse_dataframe(struct ssp_data *data,
	char *pchRcvDataFrame, int iLength)
{
	int iDataIdx, iSensorData;
	u16 length = 0;
	struct sensor_value sensorsdata;
	struct timespec ts;
	int iRet = FAIL;

	getnstimeofday(&ts);

	for (iDataIdx = 0; iDataIdx < iLength;) {
		switch (pchRcvDataFrame[iDataIdx++]) {
		case MSG2AP_INST_BYPASS_DATA:
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) || (iSensorData >= SENSOR_MAX)) {
				pr_err("[SSP]%s-Mcu bypass dataframe err %d\n",
					__func__, iSensorData);
				return ERROR;
			}
			data->get_sensor_data[iSensorData](pchRcvDataFrame, &iDataIdx,
					&sensorsdata);
			get_timestamp(data, pchRcvDataFrame, &iDataIdx, &sensorsdata);
			data->report_sensor_data[iSensorData](data, &sensorsdata);
			break;
		case MSG2AP_INST_DEBUG_DATA:
			iSensorData = print_mcu_debug(pchRcvDataFrame,
				&iDataIdx, iLength);
			if (iSensorData) {
				pr_err("[SSP]%s-Mcu debug dataframe err %d, %d\n",
					__func__, iLength, iSensorData);
				return ERROR;
			}
			break;
		case MSG2AP_INST_LIBRARY_DATA:
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
#ifdef CONFIG_SENSORS_SSP_LPM_MOTION
			if (data->bLpModeEnabled == true)
				iRet = ssp_parse_motion(data, pchRcvDataFrame,
							iDataIdx, iDataIdx + length);
			else
				iRet = ssp_sensorhub_handle_data(data,
					pchRcvDataFrame, iDataIdx,
					iDataIdx + length);
#else
			iRet = ssp_sensorhub_handle_data(data,
				pchRcvDataFrame, iDataIdx,
				iDataIdx + length);
#endif

			if (iRet < 0) {
				pr_err("[SSP]%s-Mcu library dataframe err %d, %d\n",
					__func__, iLength, length);
				return ERROR;
			}
			iDataIdx += length;
			break;
		case MSG2AP_INST_BIG_DATA:
			handle_big_data(data, pchRcvDataFrame, &iDataIdx);
			break;
		case MSG2AP_INST_TIME_SYNC:
			data->bTimeSyncing = true;
			break;
		case MSG2AP_INST_RESET:
			pr_info("[SSP]%s-Reset MSG received from MCU.\n",
				__func__);
			queue_refresh_task(data, 0);
			break;
		case MSG2AP_INST_DUMP_DATA:
			pr_info("[SSP]%s-Dump MSG received from MCU\n",
				__func__);
//			debug_crash_dump(data, pchRcvDataFrame, iLength);
			handle_dump_data(data, pchRcvDataFrame, iLength);
			return SUCCESS;
			break;
		}
	}

	if (data->bTimeSyncing)
		data->timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

	return SUCCESS;
}

void initialize_function_pointer(struct ssp_data *data)
{
	int sensor_type;
	for (sensor_type = 0; sensor_type < SENSOR_MAX; sensor_type++) {
		data->get_sensor_data[sensor_type] = get_dummy_sensordata;
		data->report_sensor_data[sensor_type] = report_dummy_data;
	}

#ifdef CONFIG_SENSORS_SSP_ACCELEROMETER_SENSOR
	data->get_sensor_data[ACCELEROMETER_SENSOR] = get_3axis_sensordata;
	data->report_sensor_data[ACCELEROMETER_SENSOR] = report_acc_data;
#endif

#ifdef CONFIG_SENSORS_SSP_GYRO_SENSOR
	data->get_sensor_data[GYROSCOPE_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GYRO_UNCALIB_SENSOR] = get_uncalib_sensordata;
	data->report_sensor_data[GYROSCOPE_SENSOR] = report_gyro_data;
	data->report_sensor_data[GYRO_UNCALIB_SENSOR] =
		report_uncalib_gyro_data;
#endif

#ifdef CONFIG_SENSORS_SSP_MAGNETIC_SENSOR
	data->get_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] =
		get_geomagnetic_uncaldata;
	data->get_sensor_data[GEOMAGNETIC_RAW] = get_geomagnetic_rawdata;
	data->get_sensor_data[GEOMAGNETIC_SENSOR] =
		get_geomagnetic_caldata;
	data->report_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] =
		report_mag_uncaldata;
	data->report_sensor_data[GEOMAGNETIC_RAW] = report_geomagnetic_raw_data;
	data->report_sensor_data[GEOMAGNETIC_SENSOR] =
		report_mag_data;
#endif

#ifdef CONFIG_SENSORS_SSP_PRESSURE_SENSOR
	data->get_sensor_data[PRESSURE_SENSOR] = get_pressure_sensordata;
	data->report_sensor_data[PRESSURE_SENSOR] = report_pressure_data;
#endif

#ifdef CONFIG_SENSORS_SSP_LIGHT_SENSOR
	data->get_sensor_data[LIGHT_SENSOR] = get_light_sensordata;
	data->report_sensor_data[LIGHT_SENSOR] = report_light_data;
#endif

#ifdef CONFIG_SENSORS_SSP_TEMP_HUMID_SENSOR
	data->get_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		get_temp_humidity_sensordata;
	data->report_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		report_temp_humidity_data;
#endif

#ifdef CONFIG_SENSORS_SSP_ROT_VECTOR_SENSOR
	data->get_sensor_data[ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[GAME_ROTATION_VECTOR] = get_rot_sensordata;
	data->report_sensor_data[ROTATION_VECTOR] = report_rot_data;
	data->report_sensor_data[GAME_ROTATION_VECTOR] = report_game_rot_data;
#endif

#ifdef CONFIG_SENSORS_SSP_HRM_SENSOR
	data->get_sensor_data[HRM_RAW_SENSOR] = get_hrm_raw_sensordata;
		data->get_sensor_data[HRM_RAW_FAC_SENSOR] =
			get_hrm_raw_fac_sensordata;
	data->get_sensor_data[HRM_LIB_SENSOR] = get_hrm_lib_sensordata;
	data->report_sensor_data[HRM_RAW_SENSOR] = report_hrm_raw_data;
	data->report_sensor_data[HRM_RAW_FAC_SENSOR] = report_hrm_raw_fac_data;
	data->report_sensor_data[HRM_LIB_SENSOR] = report_hrm_lib_data;
#endif
#ifdef CONFIG_SENSORS_SSP_FRONT_HRM_SENSOR
	data->get_sensor_data[FRONT_HRM_RAW_SENSOR] =
		get_front_hrm_raw_sensordata;
	data->get_sensor_data[FRONT_HRM_RAW_FAC_SENSOR] =
		get_hrm_raw_fac_sensordata;
	data->get_sensor_data[FRONT_HRM_LIB_SENSOR] = get_hrm_lib_sensordata;
	data->report_sensor_data[FRONT_HRM_RAW_SENSOR] =
		report_front_hrm_raw_data;
#endif
#ifdef CONFIG_SENSORS_SSP_UV_SENSOR
	data->get_sensor_data[UV_SENSOR] = get_uv_sensordata;
	data->report_sensor_data[UV_SENSOR] = report_uv_data;
#endif
#ifdef CONFIG_SENSORS_SSP_GSR_SENSOR
	data->get_sensor_data[GSR_SENSOR] = get_gsr_sensordata;
	data->report_sensor_data[GSR_SENSOR] = report_gsr_data;
#endif
#ifdef CONFIG_SENSORS_SSP_ECG_SENSOR
	data->get_sensor_data[ECG_SENSOR] = get_ecg_sensordata;
	data->report_sensor_data[ECG_SENSOR] = report_ecg_data;
#endif
#ifdef CONFIG_SENSORS_SSP_GRIP_SENSOR
	data->get_sensor_data[GRIP_SENSOR] = get_grip_sensordata;
	data->report_sensor_data[GRIP_SENSOR] = report_grip_data;
#endif
	data->ssp_big_task[BIG_TYPE_DUMP] = ssp_dump_task;
	data->ssp_big_task[BIG_TYPE_READ_LIB] = ssp_read_big_library_task;
#ifdef CONFIG_SENSORS_SSP_VOICE
	data->ssp_big_task[BIG_TYPE_VOICE_NET] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_GRAM] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_PCM] = ssp_pcm_dump_task;
#endif
}
