#include "cbhm_helper.h"

#ifdef HAVE_ELEMENTARY_X
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#define ATOM_CBHM_WINDOW_NAME "CBHM_XWIN"
#define ATOM_CBHM_MSG "CBHM_MSG"
#define ATOM_CBHM_COUNT_GET "CBHM_cCOUNT"
#define ATOM_CBHM_TEXT_COUNT_GET "CBHM_TEXT_cCOUNT"
#define ATOM_CBHM_IMAGE_COUNT_GET "CBHM_IMAGE_cCOUNT"
#define ATOM_CBHM_SERIAL_NUMBER "CBHM_SERIAL_NUMBER"
#define MSG_CBHM_COUNT_GET "get count"
#define ATOM_CBHM_ERROR "CBHM_ERROR"
#define ATOM_CBHM_ITEM "CBHM_ITEM"
#endif

#ifdef HAVE_ELEMENTARY_X
Ecore_X_Window
_cbhm_window_get()
{
   Ecore_X_Atom x_atom_cbhm = ecore_x_atom_get(ATOM_CBHM_WINDOW_NAME);
   Ecore_X_Window x_cbhm_win = 0;
   unsigned char *buf = NULL;
   int num = 0;
   int ret = ecore_x_window_prop_property_get(0, x_atom_cbhm, XA_WINDOW, 0, &buf, &num);
   DMSG("ret: %d, num: %d\n", ret, num);
   if (ret && num)
     memcpy(&x_cbhm_win, buf, sizeof(Ecore_X_Window));
   if (buf)
     free(buf);
   return x_cbhm_win;
}
#endif

Eina_Bool
_cbhm_msg_send(Evas_Object *obj, char *msg)
{
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   Ecore_X_Atom x_atom_cbhm_msg = ecore_x_atom_get(ATOM_CBHM_MSG);
   Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   Ecore_X_Window xwin = ecore_evas_software_x11_window_get(ee);
   if (!xwin)
     {
        xwin = ecore_evas_gl_x11_window_get(ee);
        if (!xwin)
          {
             DMSG("ERROR: can't get x window\n");
             return EINA_FALSE;
          }
     }

   DMSG("x_cbhm: 0x%x\n", x_cbhm_win);
   if (!x_cbhm_win || !x_atom_cbhm_msg)
     return EINA_FALSE;

   XClientMessageEvent m;
   memset(&m, 0, sizeof(m));
   m.type = ClientMessage;
   m.display = ecore_x_display_get();
   m.window = xwin;
   m.message_type = x_atom_cbhm_msg;
   m.format = 8;
   snprintf(m.data.b, 20, "%s", msg);

   XSendEvent(ecore_x_display_get(), x_cbhm_win, False, NoEventMask, (XEvent*)&m);

   ecore_x_sync();
   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

#ifdef HAVE_ELEMENTARY_X
void *
_cbhm_reply_get(Ecore_X_Window xwin, Ecore_X_Atom property, Ecore_X_Atom *x_data_type, int *num)
{
   unsigned char *data = NULL;
   if (x_data_type)
     *x_data_type = 0;
   if (!property)
      return NULL;
   ecore_x_sync();
   if (num)
     *num = 0;

   long unsigned int num_ret = 0, bytes = 0;
   int ret = 0, size_ret;
   unsigned int i;
   unsigned char *prop_ret;
   Ecore_X_Atom type_ret;
   ret = XGetWindowProperty(ecore_x_display_get(), xwin, property, 0, LONG_MAX,
                            False, ecore_x_window_prop_any_type(), (Atom *)&type_ret, &size_ret,
                            &num_ret, &bytes, &prop_ret);
   if (ret != Success)
     return NULL;
   if (!num_ret)
     {
        XFree(prop_ret);
        return NULL;
     }

   if (!(data = malloc(num_ret * size_ret / 8)))
     {
        XFree(prop_ret);
        return NULL;
     }

   switch (size_ret) {
      case 8:
        for (i = 0; i < num_ret; i++)
          (data)[i] = prop_ret[i];
        break;

      case 16:
        for (i = 0; i < num_ret; i++)
          ((unsigned short *)data)[i] = ((unsigned short *)prop_ret)[i];
        break;

      case 32:
        for (i = 0; i < num_ret; i++)
          ((unsigned int *)data)[i] = ((unsigned long *)prop_ret)[i];
        break;
     }

   XFree(prop_ret);

   if (num)
     *num = num_ret;
   if (x_data_type)
     *x_data_type = type_ret;

   return data;
}
#endif

int
_cbhm_item_count_get(Evas_Object *obj __UNUSED__, int atom_index)
{
#ifdef HAVE_ELEMENTARY_X
   char *ret, count;
   Ecore_X_Atom x_atom_cbhm_count_get = 0;

   if (atom_index == ATOM_INDEX_CBHM_COUNT_ALL)
     x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_COUNT_GET);
   else if (atom_index == ATOM_INDEX_CBHM_COUNT_TEXT)
     x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_TEXT_COUNT_GET);
   else if (atom_index == ATOM_INDEX_CBHM_COUNT_IMAGE)
     x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_IMAGE_COUNT_GET);

   Ecore_X_Window cbhm_xwin = _cbhm_window_get();
   DMSG("x_win: 0x%x, x_atom: %d\n", cbhm_xwin, x_atom_cbhm_count_get);
   ret = _cbhm_reply_get(cbhm_xwin, x_atom_cbhm_count_get, NULL, NULL);
   if (ret)
     {
        count = atoi(ret);
        DMSG("count: %d\n", count);
        free(ret);
        return count;
     }
   DMSG("ret: 0x%x\n", ret);
#endif
   return -1;
}
/*
int
_cbhm_item_count_get(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_X
   char *ret, count;
   if(_cbhm_msg_send(obj, MSG_CBHM_COUNT_GET))
     {
        DMSG("message send success\n");
        Ecore_X_Atom x_atom_cbhm_count_get = ecore_x_atom_get(ATOM_CBHM_COUNT_GET);
        Ecore_X_Window xwin = ecore_evas_software_x11_window_get(
           ecore_evas_ecore_evas_get(evas_object_evas_get(obj)));
        DMSG("x_win: 0x%x, x_atom: %d\n", xwin, x_atom_cbhm_count_get);
        ret = _cbhm_reply_get(xwin, x_atom_cbhm_count_get, NULL, NULL);
        if (ret)
          {
             count = atoi(ret);
             DMSG("count: %d\n", count);
             free(ret);
             return count;
          }
        DMSG("ret: 0x%x\n", ret);
     }
#endif
   return -1;
}
*/

#ifdef HAVE_ELEMENTARY_X
Eina_Bool
_cbhm_item_get(Evas_Object *obj __UNUSED__, int idx, Ecore_X_Atom *data_type, char **buf)
#else
Eina_Bool
_cbhm_item_get(Evas_Object *obj, int idx, void *data_type, char **buf)
#endif

{
   if (buf)
     *buf = NULL;
   if (data_type)
     *(int *)data_type = 0;

#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window cbhm_xwin = _cbhm_window_get();
   char send_buf[20];
   char *ret;

   snprintf(send_buf, 20, "CBHM_ITEM%d", idx);
   Ecore_X_Atom x_atom_cbhm_item = ecore_x_atom_get(send_buf);
   Ecore_X_Atom x_atom_item_type = 0;

   DMSG("x_win: 0x%x, x_atom: %d\n", cbhm_xwin, x_atom_cbhm_item);
   ret = _cbhm_reply_get(cbhm_xwin, x_atom_cbhm_item, &x_atom_item_type, NULL);
   if (ret)
     {
        DMSG("data_type: %d, buf: %s\n", x_atom_item_type, ret);
        if (buf)
          *buf = ret;
        else
          free(ret);

        if (data_type)
          *data_type = x_atom_item_type;

        Ecore_X_Atom x_atom_cbhm_error = ecore_x_atom_get(ATOM_CBHM_ERROR);
        if (x_atom_item_type == x_atom_cbhm_error)
          return EINA_FALSE;
     }
   DMSG("ret: 0x%x\n", ret);
/*
   Ecore_X_Window xwin = ecore_evas_software_x11_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(obj)));
   char send_buf[20];
   char *ret;

   snprintf(send_buf, 20, "GET_ITEM%d", idx);
   if (_cbhm_msg_send(obj, send_buf))
     {
        DMSG("message send success\n");
        Ecore_X_Atom x_atom_cbhm_item = ecore_x_atom_get(ATOM_CBHM_ITEM);
        Ecore_X_Atom x_atom_item_type = 0;

        DMSG("x_win: 0x%x, x_atom: %d\n", xwin, x_atom_cbhm_item);
        ret = _cbhm_reply_get(xwin, x_atom_cbhm_item, &x_atom_item_type, NULL);
        if (ret)
          {
             DMSG("data_type: %d, buf: %s\n", x_atom_item_type, ret);
             if (buf)
               *buf = ret;
             else
               free(ret);
             if (data_type)
               *data_type = x_atom_item_type;

             Ecore_X_Atom x_atom_cbhm_error = ecore_x_atom_get(ATOM_CBHM_ERROR);
             if (x_atom_item_type == x_atom_cbhm_error)
               return EINA_FALSE;
          }
        DMSG("ret: 0x%x\n", ret);
     }
*/
#endif
   return EINA_FALSE;
}

#ifdef HAVE_ELEMENTARY_X
Eina_Bool
_cbhm_item_set(Evas_Object *obj, Ecore_X_Atom data_type, char *item_data)
{
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   Ecore_X_Atom x_atom_cbhm_item = ecore_x_atom_get(ATOM_CBHM_ITEM);
   ecore_x_window_prop_property_set(x_cbhm_win, x_atom_cbhm_item, data_type, 8, item_data, strlen(item_data) + 1);
   ecore_x_sync();
   if (_cbhm_msg_send(obj, "SET_ITEM"))
     {
        DMSG("message send success\n");
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
#endif

unsigned int
_cbhm_serial_number_get()
{
   unsigned int senum = 0;
#ifdef HAVE_ELEMENTARY_X
   unsigned char *buf = NULL;
   Ecore_X_Atom x_atom_cbhm_SN = ecore_x_atom_get(ATOM_CBHM_SERIAL_NUMBER);
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   buf = _cbhm_reply_get(x_cbhm_win, x_atom_cbhm_SN, NULL, NULL);
   if (buf)
     {
        memcpy(&senum, buf, sizeof(senum));
        free(buf);
     }
#endif
   return senum;
}

//////////////////////////////////////////////////////////////////////////////
// old elm_cbmh_helper                                                      //
//////////////////////////////////////////////////////////////////////////////
//#include <Elementary.h>
//#include "elm_priv.h"
//
//#ifdef HAVE_ELEMENTARY_X
//#include <X11/Xlib.h>
//#include <X11/Xatom.h>
//#endif
//
///**
// * @defgroup CBHM_helper CBHM_helper
// * @ingroup Elementary
// *
// * retrieving date from Clipboard History Manager
// * CBHM_helper supports to get CBHM's contents
// */
//
//
//#define ATOM_CLIPBOARD_NAME "CLIPBOARD"
//#define ATOM_CLIPBOARD_MANGER_NAME "CLIPBOARD_MANAGER"
//#define CLIPBOARD_MANAGER_WINDOW_TITLE_STRING "X11_CLIPBOARD_HISTORY_MANAGER"
//#define ATOM_CBHM_WINDOW_NAME "CBHM_XWIN"
//
//#ifdef HAVE_ELEMENTARY_X
//static Ecore_X_Display *cbhm_disp = NULL;
//static Ecore_X_Window cbhm_win = None;
//static Ecore_X_Window self_win = None;
//#endif
//static Eina_Bool init_flag = EINA_FALSE;
//
//void _get_clipboard_window();
//unsigned int _get_cbhm_serial_number();
//void _search_clipboard_window(Ecore_X_Window w);
//int _send_clipboard_events(char *cmd);
//#ifdef HAVE_ELEMENTARY_X
//int _get_clipboard_data(Atom datom, char **datomptr);
//#endif
//
//void _get_clipboard_window()
//{
//#ifdef HAVE_ELEMENTARY_X
//   Atom actual_type;
//   int actual_format;
//   unsigned long nitems, bytes_after;
//   unsigned char *prop_return = NULL;
//   Atom atomCbhmWin = XInternAtom(cbhm_disp, ATOM_CBHM_WINDOW_NAME, False);
//   if(Success ==
//      XGetWindowProperty(cbhm_disp, DefaultRootWindow(cbhm_disp), atomCbhmWin,
//                         0, sizeof(Ecore_X_Window), False, XA_WINDOW,
//                         &actual_type, &actual_format, &nitems, &bytes_after, &prop_return) &&
//      prop_return)
//     {
//        cbhm_win = *(Ecore_X_Window*)prop_return;
//        XFree(prop_return);
//        fprintf(stderr, "## find clipboard history manager at root\n");
//     }
//#endif
//}
//
//unsigned int _get_cbhm_serial_number()
//{
//   unsigned int senum = 0;
//#ifdef HAVE_ELEMENTARY_X
//   Atom actual_type;
//   int actual_format;
//   unsigned long nitems, bytes_after;
//   unsigned char *prop_return = NULL;
//   Atom atomCbhmSN = XInternAtom(cbhm_disp, "CBHM_SERIAL_NUMBER", False);
//
//   // FIXME : is it really needed?
//   XSync(cbhm_disp, EINA_FALSE);
//
//   if(Success ==
//      XGetWindowProperty(cbhm_disp, cbhm_win, atomCbhmSN,
//                         0, sizeof(Ecore_X_Window), False, XA_INTEGER,
//                         &actual_type, &actual_format, &nitems, &bytes_after, &prop_return) &&
//      prop_return)
//     {
//        senum = *(unsigned int*)prop_return;
//        XFree(prop_return);
//     }
//   fprintf(stderr, "## chbm_serial = %d\n", senum);
//#endif
//   return senum;
//}
//
//void _search_clipboard_window(Ecore_X_Window w)
//{
//#ifdef HAVE_ELEMENTARY_X
//   // Get the PID for the current Window.
//   Atom atomWMName = XInternAtom(cbhm_disp, "_NET_WM_NAME", False);
//   Atom atomUTF8String = XInternAtom(cbhm_disp, "UTF8_STRING", False);
//   Atom type;
//   int format;
//   unsigned long nitems;
//   unsigned long bytes_after;
//   unsigned long nsize = 0;
//   unsigned char *propName = 0;
//   if(Success ==
//      XGetWindowProperty(cbhm_disp, w, atomWMName, 0, (long)nsize, False,
//                         atomUTF8String, &type, &format, &nitems, &bytes_after, &propName))
//
//     {
//        if(propName != 0)
//          {
//             if (strcmp((const char *)CLIPBOARD_MANAGER_WINDOW_TITLE_STRING,(const char *)propName) == 0)
//               cbhm_win = w;
//             XFree(propName);
//          }
//     }
//
//   // Recurse into child windows.
//   Window wroot;
//   Window wparent;
//   Window *wchild;
//   unsigned nchildren;
//   int i;
//   if(0 != XQueryTree(cbhm_disp, w, &wroot, &wparent, &wchild, &nchildren))
//     {
//        for(i = 0; i < nchildren; i++)
//          _search_clipboard_window(wchild[i]);
//        XFree(wchild);
//     }
//#endif
//}
//
//int _send_clipboard_events(char *cmd)
//{
//   if (cmd == NULL)
//     return -1;
//
//#ifdef HAVE_ELEMENTARY_X
//   Atom atomCBHM_MSG = XInternAtom(cbhm_disp, "CBHM_MSG", False);
//
//   XClientMessageEvent m;
//   memset(&m, 0, sizeof(m));
//   m.type = ClientMessage;
//   m.display = cbhm_disp;
//   m.window = self_win;
//   m.message_type = atomCBHM_MSG;
//   m.format = 8;
//   sprintf(m.data.b, "%s", cmd);
//
//   XSendEvent(cbhm_disp, cbhm_win, False, NoEventMask, (XEvent*)&m);
//#endif
//   return 0;
//}
//
//#ifdef HAVE_ELEMENTARY_X
//int _get_clipboard_data(Atom datom, char **datomptr)
//{
//   //	Atom atomUTF8String = XInternAtom(cbhm_disp, "UTF8_STRING", False);
//   Atom type;
//   int format;
//   unsigned long nitems;
//   unsigned long nsize;
//   unsigned char *propname = NULL;
//
//   // FIXME : is it really needed?
//   XSync(cbhm_disp, EINA_FALSE);
//
//   if (Success ==
//       XGetWindowProperty(cbhm_disp, self_win, datom, 0, 0, False,
//                          AnyPropertyType, &type, &format, &nitems, &nsize, &propname))
//     {
//        XFree(propname);
//     }
//   else
//     return -1;
//
//   /*
//      fprintf(stderr, "## format = %d\n", format);
//      fprintf(stderr, "## nsize = %d\n", nsize);
//    */
//
//   if (format != 8)
//     return -1;
//
//   if (Success ==
//       XGetWindowProperty(cbhm_disp, self_win, datom, 0, (long)nsize, False,
//                          AnyPropertyType, &type, &format, &nitems, &nsize, &propname))
//     {
//        if (nsize != 0)
//          {
//             XGetWindowProperty(cbhm_disp, self_win, datom, 0, (long)nsize, False,
//                                AnyPropertyType, &type, &format, &nitems, &nsize, &propname);
//          }
//
//        if(propname != NULL)
//          {
//             //			fprintf(stderr, "## get data(0x%x) : %s\n", propname, propname);
//             //			fprintf(stderr, "## after nsize = %d\n", nsize);
//             *datomptr = (char*)propname;
//             //			XFree(propName);
//          }
//
//        XDeleteProperty(cbhm_disp, self_win, datom);
//        XFlush(cbhm_disp);
//     }
//
//   if (propname != NULL)
//     return 0;
//
//   *datomptr = NULL;
//   return -1;
//}
//#endif
//
//void free_clipboard_data(char *dptr)
//{
//#ifdef HAVE_ELEMENTARY_X
//   XFree(dptr);
//   return;
//#endif
//}
//
//
///**
// * initalizing CBHM_helper
// *
// * @param self The self window object which receive events
// * @return return TRUE or FALSE if it cannot be created
// *
// * @ingroup CBHM_helper
// */
//EAPI Eina_Bool
//elm_cbhm_helper_init(Evas_Object *self)
//{
//   init_flag = EINA_FALSE;
//
//#ifdef HAVE_ELEMENTARY_X
//   cbhm_disp = ecore_x_display_get();
//   if (cbhm_disp == NULL)
//     return init_flag;
//   if (cbhm_win == None)
//     _get_clipboard_window();
//   if (cbhm_win == None)
//     _search_clipboard_window(DefaultRootWindow(cbhm_disp));
//   if (self_win == None)
//     self_win = ecore_evas_software_x11_window_get(ecore_evas_ecore_evas_get(evas_object_evas_get(self)));
//
//   if (cbhm_disp && cbhm_win && self_win)
//     init_flag = EINA_TRUE;
//#endif
//   return init_flag;
//}
//
///**
// * getting serial number of CBHM
// *
// * @return return serial number of clipboard history manager
// *
// * @ingroup CBHM_helper
// */
//EAPI unsigned int
//elm_cbhm_get_serial_number()
//{
//   if (init_flag == EINA_FALSE)
//     return 0;
//
//   unsigned int num = 0;
//   num = _get_cbhm_serial_number();
//   return num;
//}
//
///**
// * getting count of CBHM's contents
// *
// * @return return count of history contents
// *
// * @ingroup CBHM_helper
// */
//EAPI int
//elm_cbhm_get_count()
//{
//   if (init_flag == EINA_FALSE)
//     return -1;
//
//   char *retptr = NULL;
//   int count = 0;
//
//   _send_clipboard_events("get count");
//
//#ifdef HAVE_ELEMENTARY_X
//   Atom atomCBHM_cCOUNT = XInternAtom(cbhm_disp, "CBHM_cCOUNT", False);
//
//   _get_clipboard_data(atomCBHM_cCOUNT, &retptr);
//#endif
//
//   if (retptr != NULL)
//     {
//        fprintf(stderr, "## c get retptr : %s\n", retptr);
//        count = atoi(retptr);
//
//        free_clipboard_data(retptr);
//        retptr = NULL;
//     }
//
//   return count;
//}
//
///**
// * getting raw data of CBHM's contents
// *
// * @return return raw data of history contents
// *
// * @ingroup CBHM_helper
// */
//EAPI int
//elm_cbhm_get_raw_data()
//{
//   if (init_flag == EINA_FALSE)
//     return -1;
//
//   char *retptr = NULL;
//
//   _send_clipboard_events("get raw");
//
//#ifdef HAVE_ELEMENTARY_X
//   Atom atomCBHM_cRAW = XInternAtom(cbhm_disp, "CBHM_cRAW", False);
//
//   _get_clipboard_data(atomCBHM_cRAW, &retptr);
//#endif
//
//   if (retptr != NULL)
//     {
//        free_clipboard_data(retptr);
//        retptr = NULL;
//     }
//
//   return 0;
//}
//
///**
// * sending raw command to CBHM
// *
// * @return void
// *
// * @ingroup CBHM_helper
// */
//EAPI void
//elm_cbhm_send_raw_data(char *cmd)
//{
//   if (init_flag == EINA_FALSE)
//     return;
//
//   if (cmd == NULL)
//     return;
//
//   _send_clipboard_events(cmd);
//   fprintf(stderr, "## cbhm - send raw cmd = %s\n", cmd);
//
//   return;
//}
//
///**
// * getting data by history position of CBHM's contents
// * 0 is current content.
// *
// * @return return data pointer of position of history contents
// *
// * @ingroup CBHM_helper
// */
//EAPI int
//elm_cbhm_get_data_by_position(int pos, char **dataptr)
//{
//   if (init_flag == EINA_FALSE)
//     return -1;
//
//   char reqbuf[16];
//   int ret = 0;
//   sprintf(reqbuf, "get #%d", pos);
//
//   _send_clipboard_events(reqbuf);
//
//   sprintf(reqbuf, "CBHM_c%d", pos);
//
//#ifdef HAVE_ELEMENTARY_X
//   Atom atomCBHM_cPOS = XInternAtom(cbhm_disp, reqbuf, False);
//
//   ret = _get_clipboard_data(atomCBHM_cPOS, dataptr);
//#endif
//
//   if (ret >= 0 && *dataptr != NULL)
//     {
//        //		fprintf(stderr, "## d get retptr : %s\n", *dataptr);
//        //		fprintf(stderr, "## dptr = 0x%x\n", *dataptr);
//
//        return 0;
//     }
//
//   return -1;
//}
//
///**
// * free data by history position of CBHM's contents
// *
// * @return None
// *
// * @ingroup CBHM_helper
// */
//EAPI void
//elm_cbhm_free_data(char *dptr)
//{
//   if (dptr != NULL)
//     free_clipboard_data(dptr);
//}
