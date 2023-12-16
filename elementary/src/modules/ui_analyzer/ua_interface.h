#ifndef __UA_INTERFACE___
#define __UA_INTERFACE___

#define FILE_DUMP_PATH          "/tmp/.ua_dump.dat"
#define OBJECT_ADDRESS_LENGTH   20

typedef enum
{
   UA_MSG_NONE,

   UA_MSG_OBJECT_VIEWER_REQ,
   UA_MSG_OBJECT_VIEWER_REP,

   UA_MSG_WIDGET_TREE_DUMP_REQ,
   UA_MSG_WIDGET_TREE_DUMP_REP,

   UA_MSG_OBJECT_TREE_DUMP_REQ,
   UA_MSG_OBJECT_TREE_DUMP_REP,

   UA_MSG_OBJECT_IMAGE_DUMP_REQ,
   UA_MSG_OBJECT_IMAGE_DUMP_REP,

   UA_MSG_OBJECT_INFO_REQ,
   UA_MSG_OBJECT_INFO_REP,

   UA_MSG_XY_OBJECT_REQ,
   UA_MSG_XY_OBJECT_REP,

   UA_MSG_EVAS_DUMP_REQ,
   UA_MSG_EVAS_DUMP_REP,

   UA_MSG_EVENT_INFO_REQ,
   UA_MSG_EVENT_INFO_REP,

   UA_MSG_MEMORY_INFO_REQ,
   UA_MSG_MEMORY_INFO_REP,

   UA_MSG_HISTORY_DUMP_REQ,
   UA_MSG_HISTORY_DUMP_REP,

   UA_MSG_TIMER_SPEED_CHANGE_REQ,
   UA_MSG_TIMER_SPEED_CHANGE_REP,

   UA_MSG_ECORE_RESOURCE_DUMP_REQ,
   UA_MSG_ECORE_RESOURCE_DUMP_REP

} UA_IPC_Msg_Type;

typedef struct _UA_Ipc_Data
{
   UA_IPC_Msg_Type            msg;
   char                       obj_addr[OBJECT_ADDRESS_LENGTH];
   int                        obj_x, obj_y;
   float                      timer_speed;
   unsigned char              visible_only;
   unsigned char              swallow_show;
   unsigned char              gui_mode;

} UA_Ipc_Data;

#endif
