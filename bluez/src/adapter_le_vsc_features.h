#ifdef __TIZEN_PATCH__

typedef enum {
	BLE_ADV_TX_POWER_MIN = 0x00,
	BLE_ADV_TX_POWER_LOW = 0x01,
	BLE_ADV_TX_POWER_MID = 0x02,
	BLE_ADV_TX_POWER_UPPER = 0x03,
	BLE_ADV_TX_POWER_MAX = 0x04,
} adapter_le_tx_power_t;

typedef struct {
	uint8_t inst_id;
	uint8_t bdaddr_type;
	bdaddr_t bdaddr;
} adapter_le_adv_inst_info_t;

typedef struct {
	uint16_t adv_int_min;         /* minimum adv interval */
	uint16_t adv_int_max;         /* maximum adv interval */
	uint8_t adv_type;             /* adv event type (0x00 ~ 0x04) */
	uint8_t channel_map;          /* adv channel map (all channel = 0x07)  */
	uint8_t adv_filter_policy;    /* advertising filter policy (0x00 ~ 0x04) */
	adapter_le_tx_power_t tx_power;    /* adv tx power */
} adapter_le_adv_param_t;

typedef enum {
	ADD,
	DELETE,
	CLEAR,
} adapter_le_scan_filter_action_type;

typedef enum {
	ADDR_LE_PUBLIC,
	ADDR_LE_RANDOM,
} adapter_vsc_le_addr_type;

typedef enum {
	TYPE_DEVICE_ADDRESS = 0x01,
	TYPE_SERVICE_DATA_CHANGED = 0x02,
	TYPE_SERVICE_UUID = 0x04,
	TYPE_SOLICIT_UUID = 0x08,
	TYPE_LOCAL_NAME = 0x10,
	TYPE_MANUFACTURER_DATA = 0x20,
	TYPE_SERVICE_DATA = 0x40,
} adapter_le_scan_filter_type;

#define BDADDR_BREDR           0x00
#define BDADDR_LE_PUBLIC       0x01

#define BROADCAST_ADDR_FILTER 	0x01
#define SERVICE_DATA_CHANGE_FILTER	0x02
#define SERVICE_UUID_CHECK		0x04
#define SERVICE_SOLICITATION_UUID_CHECK	0x08
#define LOCAL_NAME_CHECK			0x10
#define MANUFACTURE_DATA_CHECK		0x20
#define SERVICE_DATA_CHECK			0x40

typedef uint16_t adapter_le_scan_filter_feature_t;

typedef enum {
	OR,
	AND,
} adapter_le_scan_filter_logic_type;

typedef enum {
	IMMEDIATE,
	ON_FOUND,
	BATCHED,
} adapter_le_scan_filter_delivery_mode;

typedef enum {
	UUID_16_LEN=2,
	UUID_32_LEN=4,
	UUID_128_LEN =16,
} adapter_le_uuid_len;

typedef struct {
	adapter_le_scan_filter_action_type action;
	uint8_t index;
	adapter_le_scan_filter_feature_t feature;
	adapter_le_scan_filter_logic_type list_logic_type;
	adapter_le_scan_filter_logic_type filter_logic_type;
	uint8_t rssi_high_threshold;
	adapter_le_scan_filter_delivery_mode delivery_mode;
	uint16_t onfound_timeout;
	uint8_t onfound_timeout_cnt;
	uint8_t rssi_low_threshold;
	uint16_t onlost_timeout;
}adapter_le_scan_filter_param_t;

typedef struct {
	adapter_le_scan_filter_action_type action;
	uint8_t filter_index;
	bdaddr_t broadcaster_addr;
	adapter_vsc_le_addr_type bdaddr_type;
} adapter_le_address_filter_params_t;

typedef struct {
	adapter_le_scan_filter_action_type action;
	uint8_t filter_index;
	uint8_t *uuid;
	uint8_t *uuid_mask;
	adapter_le_uuid_len	uuid_len;
}adapter_le_uuid_params_t;

typedef struct {
	adapter_le_scan_filter_action_type action;
	uint8_t filter_index;
	const char *local_name;
	uint8_t name_len;
}adapter_le_local_name_params_t;

typedef struct {
	adapter_le_scan_filter_action_type action;
	uint8_t filter_index;
	uint16_t company_id;
	uint16_t company_id_mask;
	uint8_t *man_data;
	uint8_t *man_data_mask;
	uint8_t man_data_len;
}adapter_le_manf_data_params_t;

typedef struct {
	adapter_le_scan_filter_action_type action;
	uint8_t filter_index;
	uint8_t *service_data;
	uint8_t *service_data_mask;
	uint8_t service_data_len;
}adapter_le_service_data_params_t;

/*****************************************************************************
**  Defentions for HCI Error Codes that are past in the events
*/
#define HCI_SUCCESS                                     0x00



/*****************************************************************************
**                          Vendor Specific Commands
**
*/

#define OCF_BCM_LE_GET_VENDOR_CAP    0x0153     /* LE Get Vendor Capabilities */

#define OCF_BCM_LE_MULTI_ADV     0x0154     /* Multi adv OCF */

/* subcode for multi adv feature */
#define SUB_CMD_LE_MULTI_ADV_SET_PARAM                     0x01
#define SUB_CMD_LE_MULTI_ADV_WRITE_ADV_DATA                0x02
#define SUB_CMD_LE_MULTI_ADV_WRITE_SCAN_RSP_DATA           0x03
#define SUB_CMD_LE_MULTI_ADV_SET_RANDOM_ADDR               0x04
#define SUB_CMD_LE_MULTI_ADV_ENB                           0x05

/* APCF : Advertising Packet Content Filter feature */
#define OCF_BCM_LE_SCAN_FILTER     0x0157     /* Advertising filter OCF */

/* Sub codes for APCF */
#define SUB_CMD_LE_META_PF_ENABLE          0x00
#define SUB_CMD_LE_META_PF_FEAT_SEL        0x01
#define SUB_CMD_LE_META_PF_ADDR            0x02
#define SUB_CMD_LE_META_PF_UUID            0x03
#define SUB_CMD_LE_META_PF_SOL_UUID        0x04
#define SUB_CMD_LE_META_PF_LOCAL_NAME      0x05
#define SUB_CMD_LE_META_PF_MANU_DATA       0x06
#define SUB_CMD_LE_META_PF_SRVC_DATA       0x07
#define SUB_CMD_LE_META_PF_ALL             0x08

/* Offloaded resolution of private address */
#define OCF_BCM_LE_RPA_OFFLOAD     0x0155     /* RPA Offload OCF */

/* subcode for rpa offloading feature */
#define SUB_CMD_LE_ENABLE_OFFLOADING       0x01
#define SUB_CMD_LE_ADD_IRK_TO_LIST         0x02
#define SUB_CMD_LE_REMOVE_IRK_TO_LIST      0x03
#define SUB_CMD_LE_CLEAR_IRK_TO_LIST       0x04
#define SUB_CMD_LE_READ_IRK_TO_LIST        0x05

/*****************************************************************************
**                          CP & RP for OCF_BCM_LE_GET_VENDOR_CAP
**
*/

/**
*
* RP
*
* (1 octet) status        : Command complete status
* (1 octet) adv_inst_max  : Num of advertisement instances supported
* (1 octet) rpa_offloading: BT chip capability of RPA
*                           (value 0 not capable, value 1 capable)
*                           If supported by chip, it needs enablement by host
* (2 octet) tot_scan_results_strg : Storage for scan results in bytes
* (1 octet) max_irk_list_sz : Num of IRK entries supported in f/w
* (1 octet) filter_support  : Support Filtering in controller.
*                             0 = Not supported / 1 = supported
* (1 octet) max_filter      : Number of filters supported
* (1 octet) energy_support  : Supports reporting of activity and energy info
*                             0 = not capable, 1 = capable
* (1 octet) onlost_follow   : Number of advertisers that can be analysed
*                             for onlost per filter
*/
typedef struct {
	uint8_t status;
	uint8_t adv_inst_max;
	uint8_t rpa_offloading;
	uint16_t tot_scan_results_strg;
	uint8_t max_irk_list_sz;
	uint8_t filter_support;
	uint8_t max_filter;
	uint8_t energy_support;
	uint8_t onlost_follow;
} __attribute__ ((packed)) apater_le_vsc_rp_get_vendor_cap;



/*****************************************************************************
**                          CP & RP for OCF_BCM_LE_MULTI_ADV
**
*/

/**
*
* CP for  OCF_BCM_LE_MULTI_ADV & SUB_CMD_LE_MULTI_ADV_SET_PARAM
*
* (1 octet) subcode            : SUB_CMD_LE_MULTI_ADV_SET_PARAM
* (2 octet) adv_int_min        : per spec
* (2 octet) adv_int_max        : per spec
* (1 octet) adv_type           : per spec
* (1 octet) bdaddr_type        : per spec
* (6 octet) bdaddr             : per spec
* (1 octet) direct_bdaddr_type : per spec
* (6 octet) direct_bdaddr      : per spec
* (1 octet) channel_map        : per spec
* (1 octet) adv_filter_policy  : per spec
* (1 octet) inst_id            : Specifies the applicability of the above parameters to an instance
* (1 octet) tx_power           : Transmit_Power Unit - in dBm (signed integer) Range (70 to +20)
*/
typedef struct {
	uint8_t subcode;
	uint16_t adv_int_min;
	uint16_t adv_int_max;
	uint8_t adv_type;
	uint8_t bdaddr_type;
	bdaddr_t bdaddr;
	uint8_t direct_bdaddr_type;
	bdaddr_t direct_bdaddr;
	uint8_t channel_map;
	uint8_t adv_filter_policy;
	uint8_t inst_id;
	uint8_t tx_power;
} __attribute__ ((packed)) adapter_le_vsc_cp_set_multi_adv_params;

/**
*
* CP for SUB_CMD_LE_MULTI_ADV_WRITE_ADV_DATA
* CP for SUB_CMD_LE_MULTI_ADV_WRITE_SCAN_RSP_DATA
*
* ( 1 octet) subcode     : SUB_CMD_LE_MULTI_ADV_WRITE_ADV_DATA
*                          or SUB_CMD_LE_MULTI_ADV_WRITE_SCAN_RSP_DATA
* ( 1 octet) data_len    : per spec
* (31 octet) data        : per spec
* ( 1 octet) inst_id     : Specifies the applicability of the above parameters to an instance.
*/
typedef struct {
	uint8_t subcode;
	uint8_t data_len;
	uint8_t data[31];
	uint8_t inst_id;
} __attribute__ ((packed)) adapter_le_vsc_cp_set_multi_adv_data;

/**
*
* CP for SUB_CMD_LE_MULTI_ADV_ENB
*
* (1 octet) subcode     : SUB_CMD_LE_MULTI_ADV_ENB
* (1 octet) enable      : When set to 1, it means enable, otherwise disable.
* (1 octet) inst_id     : Specifies the applicability of the above parameters
*                         to an instance. Instance 0 has special meaning this
*                         refers to std HCI instance.
*/
typedef struct {
	uint8_t subcode;
	uint8_t enable;
	uint8_t inst_id;
} __attribute__ ((packed)) adapter_le_vsc_cp_enable_multi_adv;

/**
*
* RP
*
* (1 octet) status      : Command complete status
* (1 octet) subcode     : subcode of OCF_BCM_LE_MULTI_ADV
*/
typedef struct {
	uint8_t status;
	uint8_t subcode;
} __attribute__ ((packed)) apater_le_vsc_rp_multi_adv;



/*****************************************************************************
**                          CP & RP for OCF_BCM_LE_SCAN_FILTER
**
*/


/* CP for SUB_CMD_LE_META_PF_ENABLE */
typedef struct {
	uint8_t subcode;
	uint8_t enable;
} __attribute__ ((packed)) adapter_le_vsc_cp_enable_scan_filter;

/* RP for SUB_CMD_LE_META_PF_ENABLE */
typedef struct {
	uint8_t status;
	uint8_t subcode;
	uint8_t enable;
} __attribute__ ((packed)) apater_le_vsc_rp_enable_scan_filter;

/* CP for SUB_CMD_LE_META_PF_FEAT_SEL */
typedef struct {
	uint8_t  subcode;
	uint8_t  action;
	uint8_t  filter_index;
	uint16_t feature;
	uint16_t feature_list_logic;
	uint8_t  filter_logic;
	int8_t   rssi_high_threshold;
	uint8_t  delivery_mode;
	uint16_t onfound_timeout;
	uint8_t  onfound_timeout_cnt;
	uint8_t  rssi_low_thresh;
	uint16_t onlost_timeout;
} __attribute__ ((packed)) adapter_le_vsc_cp_apcf_set_filter_params;

/* CP for SUB_CMD_LE_META_PF_ADDR */
typedef struct {
	uint8_t subcode;
	uint8_t action;
	uint8_t filter_index;
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;
} __attribute__ ((packed))adapter_le_vsc_cp_address_scan_filter;

/* CP for SUB_CMD_LE_META_PF_UUID & SUB_CMD_LE_META_PF_SOL_UUID */
typedef struct {
	uint8_t subcode;
	uint8_t action;
	uint8_t filter_index;
	uint8_t data[40]; 		/* UUID + UUID_MASK */
} __attribute__ ((packed))adapter_le_vsc_cp_service_uuid_scan_filter;
#define UUID_SCAN_FILTER_HEADER_SIZE 3

#define SCAN_FILTER_DATA_MAX_LEN 29

/* CP for SUB_CMD_LE_META_PF_LOCAL_NAME*/
typedef struct {
	uint8_t subcode;
	uint8_t action;
	uint8_t filter_index;
	uint8_t name[SCAN_FILTER_DATA_MAX_LEN];
} __attribute__ ((packed)) adapter_le_vsc_cp_local_name_scan_filter;
#define NAME_SCAN_FILTER_HEADER_SIZE 3

/* CP for SUB_CMD_LE_META_PF_MANU_DATA*/
typedef struct {
	uint8_t subcode;
	uint8_t action;
	uint8_t filter_index;
	uint8_t data[SCAN_FILTER_DATA_MAX_LEN * 2];	/* data + mask filed */
} __attribute__ ((packed)) adapter_le_vsc_cp_manf_data_scan_filter;
#define MANF_DATA_SCAN_FILTER_HEADER_SIZE 3

/* CP for SUB_CMD_LE_META_PF_SRVC_DATA*/
typedef struct {
	uint8_t subcode;
	uint8_t action;
	uint8_t filter_index;
	uint8_t data[SCAN_FILTER_DATA_MAX_LEN * 2];	/* data + mask filed */
} __attribute__ ((packed)) adapter_le_vsc_cp_service_data_scan_filter;
#define SERVICE_DATA_SCAN_FILTER_HEADER_SIZE 3

/* RP for SUB_CMD_LE_META_PF_ADDR & SUB_CMD_LE_META_PF_FEAT_SEL &
		SUB_CMD_LE_META_PF_UUID & SUB_CMD_LE_META_PF_SOL_UUID &
		SUB_CMD_LE_META_PF_LOCAL_NAME & SUB_CMD_LE_META_PF_MANU_DATA &
		SUB_CMD_LE_META_PF_SRVC_DATA */
typedef struct {
	uint8_t	status;
	uint8_t	subcode;
	uint8_t 	action;
	uint8_t 	available_space;
} __attribute__ ((packed)) adapter_le_vsc_rp_apcf_set_scan_filter;


/*****************************************************************************
**                          CP & RP for OCF_BCM_LE_RPA_OFFLOAD
**
*/

/**
*
* CP for  SUB_CMD_ENABLE_RPA_OFFLOAD
*
* (1 octet) subcode    : SUB_CMD_ENABLE_RPA_OFFLOAD (0x01)
* (2 octet) enable      : When set to 1, it means enable, otherwise disable.
*/
typedef struct {
	uint8_t subcode;
	uint8_t enable;
} __attribute__ ((packed)) adapter_le_vsc_cp_enable_rpa_offload;

/* RP for SUB_CMD_ENABLE_RPA_OFFLOAD */
typedef struct {
	uint8_t status;
	uint8_t subcode;
} __attribute__ ((packed)) adapter_le_vsc_rp_enable_rpa_offload;

/**
*
* CP for  SUB_CMD_ADD_IRK_TO_LIST
*
* (1 octet) subcode    : SUB_CMD_ADD_IRK_TO_LIST (0x02)
* (16 octet) le_irk      : LE IRK (1st byte LSB)
* (1 octet) bdaddr_type : per spec
* (6 octet) bdaddr : per spec
*/
typedef struct {
	uint8_t subcode;
	uint8_t le_irk[16];
	uint8_t bdaddr_type;
	bdaddr_t bdaddr;
} __attribute__ ((packed)) adapter_le_vsc_cp_add_irk_to_list;

/**
*
* CP for  SUB_CMD_REMOVE_IRK_TO_LIST
*
* (1 octet) subcode    : SUB_CMD_REMOVE_IRK_TO_LIST (0x03)
* (16 octet) le_irk      : LE IRK (1st byte LSB)
* (1 octet) bdaddr_type : per spec
* (6 octet) bdaddr : per spec
*/
typedef struct {
	uint8_t subcode;
	uint8_t bdaddr_type;
	bdaddr_t bdaddr;
} __attribute__ ((packed)) adapter_le_vsc_cp_remove_irk_to_list;

/* RP for SUB_CMD_ADD_IRK_TO_LIST & SUB_CMD_REMOVE_IRK_TO_LIST */
typedef struct {
	uint8_t status;
	uint8_t subcode;
	uint8_t available_space;
} __attribute__ ((packed)) adapter_le_vsc_rp_irk_to_list;


/*****************************************************************************
**                          Functions
**
*/

/* Read supported BLE feature info from chipset */
gboolean adapter_le_read_ble_feature_info(void);

gboolean adapter_le_is_supported_multi_advertising(void);

gboolean adapter_le_is_supported_offloading(void);

int adapter_le_get_max_adv_instance(void);

int adapter_le_get_scan_filter_size(void);

gboolean adapter_le_set_multi_adv_params (adapter_le_adv_inst_info_t *p_inst,
					adapter_le_adv_param_t *p_params);

gboolean adapter_le_set_multi_adv_data(uint8_t inst_id, gboolean is_scan_rsp,
					uint8_t data_len, uint8_t *p_data);

gboolean adapter_le_enable_multi_adv (gboolean enable, uint8_t inst_id);


gboolean adapter_le_enable_scan_filtering (gboolean enable);

gboolean adapter_le_set_scan_filter_params(adapter_le_scan_filter_param_t *params);

gboolean adapter_le_set_scan_filter_data(int client_if, int action,
							int filt_type, int filter_index,
							int company_id,
							int company_id_mask,
							int uuid_len, uint8_t *p_uuid,
							int uuid_mask_len, uint8_t *p_uuid_mask,
							gchar *string, int addr_type,
							int data_len, uint8_t *p_data,
							int mask_len, uint8_t *p_mask);
gboolean adapter_le_clear_scan_filter_data(int client_if, int filter_index);

gboolean adapter_le_enable_offloading(gboolean enable);

gboolean adapter_le_add_irk_to_list(uint8_t *le_irk, const bdaddr_t *bdaddr, uint8_t bdaddr_type);

gboolean adapter_le_remove_irk_to_list(const bdaddr_t *bdaddr, uint8_t bdaddr_type);

#endif /* __TIZEN_PATCH__ */
