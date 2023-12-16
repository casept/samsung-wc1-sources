#ifndef __DEF_EFL_Utility_Interface_
#define __DEF_EFL_Utility_Interface_

#define FILE_LOG_PATH                        "/tmp/.efl_util_dump.log"
#define FILE_LOG_VISIBLE_FOR_GUI_PATH        "/tmp/.efl_util_visible_dump.log"
#define FILE_LOG_FULL_FOR_GUI_PATH           "/tmp/.efl_util_all_dump.log"

typedef enum
{
	EFL_UTIL_MSG_NONE,

	EFL_UTIL_MSG_WIDGET_TREE_DUMP_REQ,
	EFL_UTIL_MSG_WIDGET_TREE_DUMP_REP,

	EFL_UTIL_MSG_OBJECT_TREE_DUMP_REQ,
	EFL_UTIL_MSG_OBJECT_TREE_DUMP_REP,

	EFL_UTIL_MSG_OBJECT_INFO_REQ,
	EFL_UTIL_MSG_OBJECT_INFO_REP,

	EFL_UTIL_MSG_EVAS_DUMP_REQ,
	EFL_UTIL_MSG_EVAS_DUMP_REP,

	EFL_UTIL_MSG_EVENT_INFO_REQ,
	EFL_UTIL_MSG_EVENT_INFO_REP,

	EFL_UTIL_MSG_MEMORY_INFO_REQ,
	EFL_UTIL_MSG_MEMORY_INFO_REP,

	EFL_UTIL_MSG_HISTORY_DUMP_REQ,
	EFL_UTIL_MSG_HISTORY_DUMP_REP,

	EFL_UTIL_MSG_ECORE_RESOURCE_DUMP_REQ,
	EFL_UTIL_MSG_ECORE_RESOURCE_DUMP_REP
} EFL_Util_IPC_Msg_Type;


typedef struct _EFL_Util_Ipc_Data
{
	EFL_Util_IPC_Msg_Type    msg;
	unsigned int             obj_num;
	unsigned char            visible_only;
	unsigned char            gui_mode;
        char                     obj_addr[20];
} EFL_Util_Ipc_Data;

#endif
