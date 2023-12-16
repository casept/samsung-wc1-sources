#ifdef __TIZEN_PATCH__

#include <errno.h>

#include "log.h"
#include "adapter.h"
#include "eir.h"

#include "adapter_le_vsc_features.h"


static apater_le_vsc_rp_get_vendor_cap ble_vsc_cb = { -1, };

static int send_vsc_command(uint16_t ocf, uint8_t *cp, uint8_t cp_len,
						uint8_t *rp, uint8_t rp_len)
{
	int dd;
	struct hci_request rq;

	dd = hci_open_dev(0);
	if (dd < 0) {
		error("hci_open_dev is failed");
		return -1;
	}

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = ocf;
	rq.cparam = cp;
	rq.clen   = cp_len;
	rq.rparam = rp;
	rq.rlen   = rp_len;

	if (hci_send_req(dd, &rq, 5000) < 0) {
		error("Fail to send VSC");
		hci_close_dev(dd);
		return -1;
	}

	hci_close_dev(dd);
	return 0;
}

gboolean adapter_le_read_ble_feature_info(void)
{
	int ret;

	DBG("");

	ret = send_vsc_command(OCF_BCM_LE_GET_VENDOR_CAP, (uint8_t *) NULL, 0,
						(uint8_t *) &ble_vsc_cb, sizeof(ble_vsc_cb));
	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != ble_vsc_cb.status) {
		error("Fail to read ble feature info");
		return FALSE;
	}

	DBG("======== BLE support info ========");
	DBG("adv_inst_max [%d]", ble_vsc_cb.adv_inst_max);
	DBG("rpa_offloading [%d]", ble_vsc_cb.rpa_offloading);
	DBG("tot_scan_results_strg [%d]", ble_vsc_cb.tot_scan_results_strg);
	DBG("max_irk_list_sz [%d]", ble_vsc_cb.max_irk_list_sz);
	DBG("filter_support [%d]", ble_vsc_cb.filter_support);
	DBG("max_filter [%d]", ble_vsc_cb.max_filter);
	DBG("energy_support [%d]", ble_vsc_cb.energy_support);
	DBG("onlost_follow [%d]", ble_vsc_cb.onlost_follow);
	DBG("=================================");

	return TRUE;
}

int adapter_le_get_max_adv_instance(void)
{
	if (HCI_SUCCESS != ble_vsc_cb.status) {
		error("not yet acquired chipset info");
		return 0;
	}

	/* GearS does not support multi advertising.
	but its official firmware returns adv_inst_max vaule to 5.
	So here check rpa_offloading support and filter_support */
	if (!ble_vsc_cb.rpa_offloading || !ble_vsc_cb.max_filter)
		return 0;

	return ble_vsc_cb.adv_inst_max;
}

gboolean adapter_le_is_supported_multi_advertising(void)
{
	if (HCI_SUCCESS != ble_vsc_cb.status) {
		error("not yet acquired chipset info");
		return FALSE;
	}

	/* GearS does not support multi advertising.
	but its official firmware returns adv_inst_max vaule to 5.
	So here check rpa_offloading support and filter_support */
	if (!ble_vsc_cb.rpa_offloading || !ble_vsc_cb.max_filter)
		return FALSE;

	if (ble_vsc_cb.adv_inst_max >= 5)
		return TRUE;
	else
		return FALSE;
}

gboolean adapter_le_is_supported_offloading(void)
{
	if (HCI_SUCCESS != ble_vsc_cb.status) {
		error("not yet acquired chipset info");
		return FALSE;
	}

	return ble_vsc_cb.rpa_offloading ? TRUE : FALSE;
}

int adapter_le_get_scan_filter_size(void)
{
	if (HCI_SUCCESS != ble_vsc_cb.status) {
		error("not yet acquired chipset info");
		return 0;
	}
#if 0
	if (!ble_vsc_cb.filter_support) {
		error("filter_support is not supported");
		return 0;
	}
#endif
	return ble_vsc_cb.max_filter;
}

gboolean adapter_le_set_multi_adv_params (adapter_le_adv_inst_info_t *p_inst,
					adapter_le_adv_param_t *p_params)
{
	int ret;
	adapter_le_vsc_cp_set_multi_adv_params cp;
	apater_le_vsc_rp_multi_adv rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_MULTI_ADV_SET_PARAM;
	cp.adv_int_min = p_params->adv_int_min;
	cp.adv_int_max = p_params->adv_int_max;
	cp.adv_type = p_params->adv_type;
	cp.bdaddr_type = p_inst->bdaddr_type;
	bacpy(&cp.bdaddr, &p_inst->bdaddr);
	cp.direct_bdaddr_type = 0;
	bacpy(&cp.direct_bdaddr, BDADDR_ANY);

	cp.channel_map = p_params->channel_map;
	cp.adv_filter_policy = p_params->adv_filter_policy;
	cp.inst_id = p_inst->inst_id;
	cp.tx_power = p_params->tx_power;

	ret = send_vsc_command(OCF_BCM_LE_MULTI_ADV, (uint8_t *) &cp, sizeof(cp),
					(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [0x%02x]", rp.subcode, rp.status);
		return FALSE;
	}

	return TRUE;
}

gboolean adapter_le_set_multi_adv_data(uint8_t inst_id, gboolean is_scan_rsp,
					uint8_t data_len, uint8_t *p_data)
{
	int ret;
	adapter_le_vsc_cp_set_multi_adv_data cp;
	apater_le_vsc_rp_multi_adv rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = (is_scan_rsp) ?
                           SUB_CMD_LE_MULTI_ADV_WRITE_SCAN_RSP_DATA :
                           SUB_CMD_LE_MULTI_ADV_WRITE_ADV_DATA;
	cp.data_len = data_len;
	memcpy(&cp.data, p_data, data_len);
	cp.inst_id = inst_id;

	ret = send_vsc_command(OCF_BCM_LE_MULTI_ADV, (uint8_t *) &cp, sizeof(cp),
					(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [0x%02x]", rp.subcode, rp.status);
		return FALSE;
	}

	return TRUE;
}

gboolean adapter_le_enable_multi_adv (gboolean enable, uint8_t inst_id)
{
	int ret;
	adapter_le_vsc_cp_enable_multi_adv cp;
	apater_le_vsc_rp_multi_adv rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_MULTI_ADV_ENB;
	cp.enable = enable;
	cp.inst_id = inst_id;

	ret = send_vsc_command(OCF_BCM_LE_MULTI_ADV, (uint8_t *) &cp, sizeof(cp),
					(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [0x%02x]", rp.subcode, rp.status);
		return FALSE;
	}

	return TRUE;
}

gboolean adapter_le_enable_scan_filtering (gboolean enable)
{
	int ret;
	adapter_le_vsc_cp_enable_scan_filter cp;
	apater_le_vsc_rp_enable_scan_filter rp;

	DBG(" enable[%d]", enable);

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_ENABLE;
	cp.enable = enable;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, sizeof(cp),
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x]", rp.subcode, rp.status);
		return FALSE;
	}

	return TRUE;
}

gboolean adapter_le_set_scan_filter_params(adapter_le_scan_filter_param_t *params)
{
	int ret;
	adapter_le_vsc_cp_apcf_set_filter_params cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;

	DBG("filter_index [%d]", params->index);

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_FEAT_SEL;
	cp.action = params->action;
	cp.filter_index= params->index;
	cp.feature= params->feature;
	cp.feature_list_logic = params->filter_logic_type;
	cp.filter_logic = params->filter_logic_type;
	cp.rssi_high_threshold = params->rssi_high_threshold;
	cp.rssi_low_thresh = params->rssi_low_threshold;
	cp.delivery_mode = params->delivery_mode;
	cp.onfound_timeout = params->onfound_timeout;
	cp.onfound_timeout_cnt = params->onfound_timeout_cnt;
	cp.onlost_timeout = params->onlost_timeout;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, sizeof(cp),
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x] Available space [%x]",
					rp.subcode, rp.status, rp.available_space);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);
	return TRUE;
}

gboolean adapter_le_service_address_scan_filtering(adapter_le_address_filter_params_t *params)
{
	int ret;
	adapter_le_vsc_cp_address_scan_filter cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_ADDR;
	cp.action= params ->action;
	cp.filter_index = params->filter_index;

	bacpy(&cp.bdaddr, &params->broadcaster_addr);
	cp.bdaddr_type = params->bdaddr_type;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, sizeof(cp),
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);

	return TRUE;
}

gboolean adapter_le_service_uuid_scan_filtering(gboolean is_solicited,
					adapter_le_uuid_params_t *params)
{
	int ret;
	adapter_le_vsc_cp_service_uuid_scan_filter cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;
	uint8_t *p = cp.data;
	int cp_len = UUID_SCAN_FILTER_HEADER_SIZE;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = (is_solicited) ? SUB_CMD_LE_META_PF_SOL_UUID :
					SUB_CMD_LE_META_PF_UUID;

	cp.action= params ->action;
	cp.filter_index = params->filter_index;

	memcpy(&cp.data, params->uuid, params->uuid_len);
	cp_len += params->uuid_len;

	memcpy(p + params->uuid_len, params->uuid_mask, params->uuid_len);
	cp_len += params->uuid_len;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, cp_len,
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);

	return TRUE;
}

gboolean adapter_le_local_name_scan_filtering(adapter_le_local_name_params_t *params)
{
	int ret;
	adapter_le_vsc_cp_local_name_scan_filter cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;
	int cp_len = NAME_SCAN_FILTER_HEADER_SIZE;
	int name_len;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_LOCAL_NAME;
	cp.action= params->action;
	cp.filter_index = params->filter_index;

	name_len = params->name_len;
	DBG("name [%s], len [%d]",params->local_name, name_len);

	if (name_len > SCAN_FILTER_DATA_MAX_LEN)
		name_len = SCAN_FILTER_DATA_MAX_LEN;

	if (name_len > 0) {
		memcpy(&cp.name, params->local_name, name_len);
		cp_len += name_len;
	}

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, cp_len,
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);

	return TRUE;
}

gboolean adapter_le_manf_data_scan_filtering (adapter_le_manf_data_params_t *params)
{
	int ret;
	adapter_le_vsc_cp_manf_data_scan_filter cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;
	uint8_t *p = cp.data;
	int data_len = 0;
	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_MANU_DATA;
	cp.action= params->action;
	cp.filter_index = params->filter_index;

	/* add company_id and data */
	cp.data[data_len++] = (uint8_t) params->company_id;
	cp.data[data_len++] = (uint8_t) (params->company_id >> 8);
	DBG("");
	memcpy(p + data_len, params->man_data, params->man_data_len);
	data_len += params->man_data_len;

	/* add company_id mask and data mask */
	cp.data[data_len++] = (uint8_t) params->company_id_mask;
	cp.data[data_len++] = (uint8_t) (params->company_id_mask >> 8);
	memcpy(p + data_len, params->man_data_mask, params->man_data_len);
	data_len += params->man_data_len;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp,
						MANF_DATA_SCAN_FILTER_HEADER_SIZE + data_len,
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);

	return TRUE;
}

gboolean adapter_le_service_data_scan_filtering (adapter_le_service_data_params_t *params)
{
	int ret;
	adapter_le_vsc_cp_service_data_scan_filter cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;
	uint8_t *p = cp.data;
	int cp_len = SERVICE_DATA_SCAN_FILTER_HEADER_SIZE;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_SRVC_DATA;
	cp.action= params->action;
	cp.filter_index = params->filter_index;

	memcpy(&cp.data, params->service_data, params->service_data_len);
	cp_len += params->service_data_len;

	memcpy(p+params->service_data_len, params->service_data_mask,
						params->service_data_len);
	cp_len += params->service_data_len;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, cp_len,
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);

	return TRUE;
}

gboolean adapter_le_set_scan_filter_data(int client_if, int action,
							int filt_type, int filter_index,
							int company_id,
							int company_id_mask,
							int uuid_len, uint8_t *p_uuid,
							int uuid_mask_len, uint8_t *p_uuid_mask,
							gchar *string, int addr_type,
							int data_len, uint8_t *p_data,
							int mask_len, uint8_t *p_mask)
{
	gboolean ret;

	DBG("");

	switch (filt_type) {
	case TYPE_DEVICE_ADDRESS: {
		/* TYPE_DEVICE_ADDRESS */
		adapter_le_address_filter_params_t params;
		bdaddr_t bd_addr;

		str2ba(string, &bd_addr);

		params.action = action;
		params.filter_index = filter_index;
		bacpy(&params.broadcaster_addr, &bd_addr);
		params.bdaddr_type = addr_type;

		ret = adapter_le_service_address_scan_filtering(&params);
		break;
	}

	case TYPE_SERVICE_UUID:
	case TYPE_SOLICIT_UUID: {
		adapter_le_uuid_params_t params;
		gboolean is_solicited = (filt_type == TYPE_SOLICIT_UUID) ? TRUE : FALSE;

		if (uuid_len != UUID_16_LEN && uuid_len != UUID_32_LEN
			&& uuid_len != UUID_128_LEN) {
			DBG("UUID length error");
			return FALSE;
		}

		if (uuid_len != uuid_mask_len) {
			DBG("Both UUID and UUID_MASK length shoule be samed");
			return FALSE;
		}

		params.action = action;
		params.filter_index = filter_index;
		params.uuid =  p_uuid;
		params.uuid_mask =  p_uuid_mask;
		params.uuid_len = uuid_len;

		ret = adapter_le_service_uuid_scan_filtering(is_solicited, &params);
		break;
	}

	case TYPE_LOCAL_NAME: {
		adapter_le_local_name_params_t params;

		params.action = action;
		params.filter_index = filter_index;
		params.local_name = string;
		params.name_len = strlen(string);
		ret = adapter_le_local_name_scan_filtering(&params);
		break;
	}

	case TYPE_MANUFACTURER_DATA: {
		adapter_le_manf_data_params_t params;

		if (data_len == 0 || (data_len != mask_len)) {
			DBG("parameter length error");
			return FALSE;
		}

		params.action = action;
		params.filter_index = filter_index;
		params.company_id = company_id;
		params.company_id_mask = company_id_mask;
		params.man_data = p_data;
		params.man_data_mask = p_mask;
		params.man_data_len = data_len;

		ret = adapter_le_manf_data_scan_filtering(&params);
		break;
	}

	case TYPE_SERVICE_DATA: {
		adapter_le_service_data_params_t params;

		if (data_len == 0 || (data_len != mask_len)) {
			DBG("parameter length error");
			return FALSE;
		}

		params.action = action;
		params.filter_index = filter_index;
		params.service_data = p_data;
		params.service_data_mask = p_mask;
		params.service_data_len = data_len;

		ret = adapter_le_service_data_scan_filtering(&params);
		break;
	}

	default:
		DBG("filter_type error");
		ret = FALSE;
	}

	return ret;
}

gboolean adapter_le_clear_scan_filter_data(int client_if, int filter_index)
{
	int ret;
	adapter_le_vsc_cp_service_data_scan_filter cp;
	adapter_le_vsc_rp_apcf_set_scan_filter rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_META_PF_FEAT_SEL;
	cp.action= 0x02; // (Add - 0x00, Delete - 0x01, Clear - 0x02)
	cp.filter_index = filter_index;

	ret = send_vsc_command(OCF_BCM_LE_SCAN_FILTER, (uint8_t *) &cp, sizeof(cp),
						(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [%x] Available space [%x]",
					rp.subcode, rp.status, rp.available_space);
		return FALSE;
	}

	DBG("Scan Filter VSC :: sub[%x] - status [%x] Action [%x] Available space [%x]",
					rp.subcode, rp.status, rp.action, rp.available_space);
	return TRUE;
}

gboolean adapter_le_enable_offloading(gboolean enable)
{
	int ret;
	adapter_le_vsc_cp_enable_rpa_offload cp;
	adapter_le_vsc_rp_enable_rpa_offload rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_ENABLE_OFFLOADING;
	cp.enable = enable;

	ret = send_vsc_command(OCF_BCM_LE_RPA_OFFLOAD, (uint8_t *) &cp, sizeof(cp),
					(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [0x%02x]", rp.subcode, rp.status);
		return FALSE;
	}

	return TRUE;
}

gboolean adapter_le_add_irk_to_list(uint8_t *le_irk, const bdaddr_t *bdaddr, uint8_t bdaddr_type)
{
	int ret;
	adapter_le_vsc_cp_add_irk_to_list cp;
	adapter_le_vsc_rp_irk_to_list rp;

	DBG("addr_type %d, irk %x %x %x...", bdaddr_type, le_irk[0], le_irk[1], le_irk[2]);

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_ADD_IRK_TO_LIST;
	memcpy(&cp.le_irk, le_irk, sizeof(cp.le_irk));
	bacpy(&cp.bdaddr, bdaddr);

	if (bdaddr_type == BDADDR_LE_PUBLIC)
		cp.bdaddr_type = 0x0;
	else
		cp.bdaddr_type = 0x1;

	ret = send_vsc_command(OCF_BCM_LE_RPA_OFFLOAD, (uint8_t *) &cp, sizeof(cp),
					(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [0x%02x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Add IRK to VCS :: available space[%d]", rp.available_space);

	return TRUE;
}

gboolean adapter_le_remove_irk_to_list(const bdaddr_t *bdaddr, uint8_t bdaddr_type)
{
	int ret;
	adapter_le_vsc_cp_remove_irk_to_list cp;
	adapter_le_vsc_rp_irk_to_list rp;

	DBG("");

	memset(&cp, 0, sizeof(cp));
	cp.subcode = SUB_CMD_LE_REMOVE_IRK_TO_LIST;
	bacpy(&cp.bdaddr, bdaddr);

	if (bdaddr_type == BDADDR_LE_PUBLIC)
		cp.bdaddr_type = 0x0;
	else
		cp.bdaddr_type = 0x1;

	ret = send_vsc_command(OCF_BCM_LE_RPA_OFFLOAD, (uint8_t *) &cp, sizeof(cp),
					(uint8_t *) &rp, sizeof(rp));

	if (ret < 0)
		return FALSE;

	if (HCI_SUCCESS != rp.status) {
		DBG("Fail to send VSC :: sub[%x] - status [0x%02x]", rp.subcode, rp.status);
		return FALSE;
	}

	DBG("Remove IRK to VCS :: available space[%d]", rp.available_space);

	return TRUE;
}


#endif /* __TIZEN_PATCH__ */
