/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Haifeng Deng <haifeng.deng@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */
/**
 *  @ingroup TIZEN_PG
 *  @defgroup InputServiceFramework_PG InputServiceFW
 *  @{

 <h1 class="pg">Introduction</h1>
The Input Service Framework (ISF) is based on the open source project SCIM-1.4.7. The Input Service Framework is capable of supporting advanced input methods like full touch input, voice input, handwriting etc.

 <h2 class="pg">Purpose</h2>
This document is aimed to describe how to develop application using TIZEN ISE (Input Service Engine) based on the ISF (Input Service Framework). The contents include: the introduction which is whole of the TIZEN ISE sample screen followed in GUI, the introduction of type and interface function of TIZEN ISE developing, and use from some sample application programs.

 <h2 class="pg">Scope</h2>
The intention is that this document will provide a guide to enable the application programmer to be familiar with using TIZEN ISE as soon as possible. In this document it will only cover part of TIZEN ISE APIs, which are close to the development of ISF and other ISE.

<h2 class="pg">Intended Readers</h2>
This document is intended for the TIZEN ISE application programmer and the related project managers. (Actually, this document is focused on application developer who uses TIZEN ISE).

<h2 class="pg">Abbreviations</h2>
- ISF - Input Service Framework
- ISE - Input Service Engine

<h2 class="pg">Abstract</h2>
ISF is to monitor and handle all the input events and provide the input service to meet the requirement of applications. Then how does the application to get the input service of ISEs. For KeyboardISE and TouchISE, the interface is the ISF implementation of STKImControl. So the application programmer can easily program to the interface of STKImControl to get the service.
The application programmer who wants to use the traditional input service (like keyboard, soft-keyboard) does not need to change any code.
@}

@defgroup ISF_Architecture Architecture
@ingroup InputServiceFramework_PG
@{
 <h1 class="pg">Architecture</h1>
@image html TIZEN_ISF_PG_architecture.png
<center><b>Figure.</b> UI Framework Architecture Diagram</center>

<h2 class="pg">UI Framework</h2>
The UI framework is a middleware that consists of several modules like UI toolkit and Document-View framework (for building work-based application user interface), UI Contention manager (which implements various UI policies regarding screen state management), resource manager (which manages application and system resources), , font system, Flash engine (which takes care of 3D screens in applications), Input service framework, and windowing and graphics (including Vector, and raster graphics) system.

@image html TIZEN_ISF_PG_isf_diagram.png
<center><b>Figure.</b> UI Framework Architecture Diagram</center>

<h2 class="pg">ISF</h2>
The Input Service Framework (ISF) is an abstraction layer with a well defined interface to allow text editor widgets to utilize the input methods. It is the responsibility of the widgets to request the assistance of the ISF, through the exported interface layer (Ecore IMF/GtkIM).

The Input Service Framework (ISF) monitors and handles all the input events from kernel which means ISF should manage all kinds of input events and dispatch them. On the other hand, as a service provider, ISF provides the input service to fulfill the requirement of applications and users in platform, which means ISF is a container for loading Input Service Engine (ISE) to provide services.

ISF consists of following components
-#	ISF-IMF API Layer
-#	ISM
-#	Panel (UI)

<h3 class="pg">ISM</h3>
- Registration/Load/Unload/Running/Pause/Resume/Mutually exclusive for ISEs
- Interfaces among applications/ISE
- The program of ISM is highly modular. All ISEs are required to program according to specific interface and each ISE will be compiled into individual dynamic library and be placed into the predefine directory. So when ISE module run, ISF will search the predefine directory and dynamically load the ISE module.

@image html TIZEN_ISF_PG_ism_ise_interaction_diagram.png
<center><b>Figure.</b> ISM-ISE interaction Diagram</center>

<h3 class="pg">Panel (UI)</h3>
- Provide the ISF control panel, enable the user configure ISF and ISE, providing the help information, providing the switch of active ISEs.
- Provide the visual feedback from the user, such as candidates, associated phrases.

<h3 class="pg">ISE</h3>
- Helper ISE is based on HelperAgent. HelperAgent is responsible for the communication between Helper ISE and ISF.
- Keyboard ISE is the traditional ISE, and it is running in ISM process.
@}

@defgroup ISF_Use_Cases1  App Dev Guide for EFL
@brief Application Guide for EFL

@defgroup ISF_Use_Cases1_1 How to show soft keyboard as soon as application is launched.
@ingroup ISF_Use_Cases1
@{
<h3 class="pg">How to show soft keyboard as soon as application is launched.</h3>
Call elm_object_focus_set() API to let entry have a focus.
@code
void create_entry(struct appdata *ad)
{
	ad->eb = elm_entry_add(ad->eb_layout);
	elm_object_part_content_set(ad->eb_layout, "btn_text", ad->eb);

	// Set the layout of soft keyboard when entry has a focus or is clicked.
	elm_entry_input_panel_layout_set(ad->eb, ELM_INPUT_PANEL_LAYOUT_URL);
	elm_object_focus_set(ad->eb, EINA_TRUE); //give focus to the entry and then soft keyboard will be shown automatically
}
@endcode
@}
@defgroup ISF_Use_Cases1_2 Example of Applications to be hidden soft keyboard
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Example of Applications that must avoid showing soft keyboard even though entry has a focus or is clicked</h3>
To avoid showing soft keyboard automatically, use elm_entry_input_panel_enabled_set(Evas_Object *obj, Eina_Bool enabled) API.
@code
Evas_Object *eo;

eo = elm_entry_add(ad->win_main);

//avoid showing soft keyboard automatically
elm_entry_input_panel_enabled_set(eo, EINA_FALSE);
@endcode
@}
@defgroup ISF_Use_Cases1_3 How to show soft keyboard manually
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to show the current active soft keyboard manually</h3>
<table>
<tr>
    <td>
        <b>Note:</b> If you use elm_entry widget, you don't need to call this API.<br>
        Please give a focus to entry widget using elm_object_focus_set() if you want to show a soft keyboard.
    </td>
</tr>
</table>
@code
static void entry_application(struct appdata *ad)
{
	Evas_Object *en;
	Ecore_IMF_Context *imf_context = NULL;

	en = elm_entry_add(ad->win_main);
	imf_context = elm_entry_imf_context_get(en);
	ecore_imf_context_input_panel_show(imf_context);
}
@endcode
@}
@defgroup ISF_Use_Cases1_4 How to hide soft keyboard manually
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to hide the current active soft keyboard manually</h3>
<table>
<tr>
    <td>
        <b>Note:</b> If you use elm_entry widget, you don't need to call this API.<br>
        Soft keyboard will be hidden when entry widget loses a focus.
    </td>
</tr>
</table>
@code
static void entry_application(struct appdata * ad)
{
	Evas_Object *bt;
	bt = elm_button_add(ad->win_main);
	evas_object_smart_callback_add(bt, "clicked", button_cb, ad);
}

static void button_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	Ecore_IMF_Context *imf_context = NULL;
	imf_context = elm_entry_imf_context_get(ad->entry);

	if (imf_context)
		ecore_imf_context_input_panel_hide(imf_context);
}
@endcode
@}
@defgroup ISF_Use_Cases1_5 How to set ISE Specific data
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to set ISE Specific data before show ISE</h3>
Use ecore_imf_context_input_panel_imdata_set() API when application wants to deliver specific data to ISE.<br>
In this case, application and ISE negotiate the data format.
@code
#include <Ecore_IMF.h>

void create_entry(struct appdata *ad)
{
	char *im_data = "application sample imdata";
	Ecore_IMF_Context *imf_context = NULL;
	ad->entry = elm_entry_add(ad->layout_main);
	imf_context = elm_entry_imf_context_get(ad->entry);

	if (imf_context)
	{
		ecore_imf_context_input_panel_imdata_set(imf_context, im_data, strlen(im_data)+1);
	}
}
@endcode
@}
@defgroup ISF_Use_Cases1_6 How to get ISE specific data
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to get ISE specific data of current active ISE.</h3>
@code
#include <Ecore_IMF.h>

void get_imdata(struct appdata *ad)
{
	int len = 256;
	char *im_data = (char*) malloc(len);
	Ecore_IMF_Context *imf_context = NULL;
	imf_context = elm_entry_imf_context_get(ad->entry);
	if (imf_context)
		ecore_imf_context_input_panel_imdata_get(imf_context, im_data, &len);

	free(im_data);
}
@endcode
@}
@defgroup ISF_Use_Cases1_7 How to detect whether soft keyboard is shown or hidden
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Example code to detect whether soft keyboard is shown or hidden </h3>
@code
void _input_panel_event_callback(void *data, Ecore_IMF_Context *imf_context, int value)
{
	int x, y, w, h;
	if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
		// ISE state has changed to INPUT_PANEL_STATE_SHOW status
		// Get ISE position of current active ISE
		ecore_imf_context_input_panel_geometry_get(imf_context, &x, &y, &w, &h);
		printf("keypad is shown\n");
		printf("The coordination of input panel. x : %d, y : %d, w : %d, h : %d\n", x, y, w, h);
	} else if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
		// ISE state has changed to INPUT_PANEL_STATE_HIDE status
		printf("keypad is hidden\n");
	}
}

static void create_entry(struct appdata *ad)
{
	Evas_Object *en;
	Ecore_IMF_Context *imf_context = NULL;
	en = elm_entry_add(ad->layout_main);
	elm_object_part_content_set(ad->layout_main, "entry", en);

	imf_context = elm_entry_imf_context_get(en);

	if (imf_context)
	{
		ecore_imf_context_input_panel_event_callback_add(imf_context, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, data);
	}
}
@endcode
@}
@defgroup ISF_Use_Cases1_91 How to set the layout of soft keyboard ISE Layout
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to set the layout of soft keyboard</h3>
The layouts that are currently supported by ISEs are<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER (in qwerty mode)<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL (in qwerty mode)<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_URL (in qwerty mode)<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER (in 4x4 modes)<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_IP (including IPv4, IPv6)<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_MONTH (in 3x4 mode)<br>
ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY (in 3x4 mode)<p>
<b>Sample code</b>: Refer to the example of ecore_imf_context_input_panel_layout_set () API<br>
The snapshots of common layouts supported in Korean ISE are shown below

<table>
<tr>
    <th>NORMAL layout</th>
    <th>NUMBER layout</th>
    <th>EMAIL layout</th>
    <th>URL layout</th>
</tr>
<tr>
    <td>@image html TIZEN_ISF_PG_normal_layout.png</td>
    <td>@image html TIZEN_ISF_PG_number_layout.png</td>
    <td>@image html TIZEN_ISF_PG_email_layout.png</td>
    <td>@image html TIZEN_ISF_PG_url_layout.png</td>
</tr>
</table>

<table>
<tr>
    <th>PHONENUMBER layout</th>
    <th>IP layout</th>
    <th>MONTH layout</th>
    <th>NUMBERONLY layout</th>
</tr>
<tr>
    <td>@image html TIZEN_ISF_PG_phonenumber_layout.png</td>
    <td>@image html TIZEN_ISF_PG_ip_layout.png</td>
    <td>@image html TIZEN_ISF_PG_month_layout.png</td>
    <td>@image html TIZEN_ISF_PG_numberonly_layout.png</td>
</tr>
</table>
@code
static void entry_application(struct appdata * ad)
{
	Evas_Object *en;
	en = elm_entry_add(ad->win_main);
	elm_entry_input_panel_layout_set(en, ELM_INPUT_PANEL_LAYOUT_URL);
}
@endcode
@}
@defgroup ISF_Use_Cases1_92 Get ISE Layout of current active ISE
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Get ISE Layout of current active ISE</h3>
@code
void get_layout(struct appdata *ad)
{
	Evas_Object *en;
	Elm_Input_Panel_Layout layout ;

	en = elm_entry_add(ad->win_main);
	elm_entry_input_panel_layout_set(en, ELM_INPUT_PANEL_LAYOUT_URL);
	layout = elm_entry_input_panel_layout_get(en);
	//here you can see what the current layout is
	printf("the current layout is %d", layout);
}
@endcode
@}
@defgroup ISF_Use_Cases1_93 How to set the return key type of soft keyboard
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to set the return key type of soft keyboard</h3>
The return key types that are currently supported by ISEs are<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_JOIN<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_LOGIN<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH<br>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEND<p>
ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SIGNIN<p>
<b>Sample code</b>: Refer to the example of ecore_imf_context_input_panel_return_key_type_set () API

<table>
<tr>
    <th>DEFAULT type</th>
    <th>DONE type</th>
    <th>GO type</th>
</tr>
<tr>
    <td>@image html TIZEN_ISF_PG_return_default.png</td>
    <td>@image html TIZEN_ISF_PG_return_done.png</td>
    <td>@image html TIZEN_ISF_PG_return_go.png</td>
</tr>
</table>

<table>
<tr>
    <th>JOIN type</th>
    <th>LOGIN type</th>
    <th>NEXT type</th>
</tr>
<tr>
    <td>@image html TIZEN_ISF_PG_return_join.png</td>
    <td>@image html TIZEN_ISF_PG_return_login.png</td>
    <td>@image html TIZEN_ISF_PG_return_next.png</td>
</tr>
</table>

<table>
<tr>
    <th>SEARCH type</th>
    <th>SEND type</th>
    <th>SIGNIN type</th>
</tr>
<tr>
    <td>@image html TIZEN_ISF_PG_return_search.png</td>
    <td>@image html TIZEN_ISF_PG_return_send.png</td>
    <td>@image html TIZEN_ISF_PG_return_signin.png</td>
</tr>
</table>
@code
static void entry_application(struct appdata * ad)
{
	Evas_Object *en;
	en = elm_entry_add(ad->win_main);

	elm_entry_input_panel_return_key_type_set(en, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
}
@endcode
@}

@defgroup ISF_Use_Cases1_94 How to register a callback for any change in ISE values
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">How to register a callback for any change in ISE values</h3>
@code
void _input_panel_event_callback(void *data, Ecore_IMF_Context *ctx, int value)
{
	if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
		// ISE state has changed to ECORE_IMF_INPUT_PANEL_STATE_SHOW status
	} else if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
		// ISE state has changed to ECORE_IMF_INPUT_PANEL_STATE_HIDE status
	}
	printf("value: %d\n", value);
}

static void create_entry(struct appdata *ad)
{
	Evas_Object *en;
	en = elm_entry_add(ad->layout_main);
	elm_object_part_content_set(ad->layout_main, "entry", en);

	Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(en); // Get the input context in the entry

	if (imf_context)
	{
		ecore_imf_context_input_panel_event_callback_add(imf_context, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, data);
	}
}
@endcode
@}
@defgroup ISF_Use_Cases1_95 Unregister a callback for any change in ISE values
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Unregister a callback for any change in ISE values.</h3>
@code
static void deregister_callback(struct appdata *ad)
{
	Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(ad->entry); // Get the input context in the entry

	if (imf_context)
	{
		ecore_imf_context_input_panel_event_callback_del(imf_context, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback);
	}
}
@endcode
@}

<h2 class="pg">Sample codes</h2>
<h3 class="pg">Sample 1 - How application uses IMControl APIs (European ISE Launch)</h3>
@code
static int init(struct appdata *ad)
{
	Evas_Object *en;

	en = elm_entry_add(ad->win_main);
	elm_object_part_content_set(ad->layout_main, "entry", en);
	elm_entry_input_panel_layout_set(en, ELM_INPUT_PANEL_LAYOUT_NUMBER);
}
@endcode
@}
@defgroup ISF_Use_Cases1_96 How Application gets ISE Input String
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Sample 2 - How Application gets ISE Input String</h3>
@code
static Eina_Bool _imf_event_commit_cb(void *data, int type, void *event)
{
	Ecore_IMF_Event_Commit *ev = event;
	printf("input string from ISE is %s \n", ev->str); //here you get the Input String from ISE

    return ECORE_CALLBACK_PASS_ON;
}

Ecore_IMF_Context *imf_context_create(Evas_Object *obj)
{
	Ecore_IMF_Context *imf_context = NULL;
	const char *ctx_id = ecore_imf_context_default_id_get();
	Evas *evas = evas_object_evas_get(obj);
	if (ctx_id)
	{
		imf_context = ecore_imf_context_add(ctx_id);
		if (obj)
		{
			ecore_imf_context_client_window_set(imf_context, (void *)ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)));
			// the canvas information is used for supporting the auto rotation of input panel
			ecore_imf_context_client_canvas_set(imf_context, evas);
		}
	}

	return imf_context;
}

void test_ise_show(void *data, Evas_Object *obj, void *event_info)
{
	Ecore_IMF_Context *imf_context = NULL;
	imf_context = imf_context_create(NULL);
	if (imf_context)
	{
		ecore_imf_context_reset(imf_context);
		ecore_imf_context_focus_in(imf_context);
		ecore_imf_context_input_panel_show(imf_context);
	}

	ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _imf_event_commit_cb, NULL);
	//please use ECORE_IMF_EVENT_PREEDIT_CHANGED if want preedit string
	//ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, _imf_event_changed_cb, NULL);
}

static void my_win_main(void)
{
	...
	elm_list_item_append(li, "ISE SHOW", NULL, NULL, test_ise_show, NULL);
	...
}
@endcode
@}
@defgroup ISF_Use_Cases1_97 How Application gets ISE-Specific Key Event
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Sample 3 - How Application gets ISE-Specific Key Event</h3>
@code
static Eina_Bool _imf_event_commit_cb(void *data, int type, void *event)
{
	Ecore_IMF_Event_Commit *ev = event;
	printf("input string from ISE is %s \n", ev->str); //here you get the Input String from ISE

	return ECORE_CALLBACK_PASS_ON;
};

Ecore_IMF_Context *imf_context_create(Evas_Object *obj)
{
	Ecore_IMF_Context *imf_context = NULL;
	const char *ctx_id = ecore_imf_context_default_id_get();
	Evas_Object *evas = evas_object_evas_get(obj);

	if (ctx_id)
	{
		imf_context = ecore_imf_context_add(ctx_id);
		if (obj)
		{
			ecore_imf_context_client_window_set(imf_context, (void *)ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)));
			// the canvas information is used for supporting the auto rotation of input panel
			ecore_imf_context_client_canvas_set(imf_context, evas_object_evas_get(obj));
		}
	}

	return imf_context;
}

void test_ise_show(void *data, Evas_Object *obj, void *event_info)
{
	Ecore_IMF_Context *imf_context = NULL;
	imf_context = imf_context_create(NULL);

	if (imf_context)
	{
		ecore_imf_context_reset(imf_context);
		ecore_imf_context_focus_in(imf_context);
		ecore_imf_context_input_panel_show(imf_context);
	}
	ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _imf_event_commit_cb, NULL);
	//please use ECORE_IMF_EVENT_PREEDIT_CHANGED if want preedit string
	//ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, 	_imf_event_changed_cb, NULL);
}

static void my_win_main(void)
{
	...
	elm_list_item_append(li, "ISE SHOW", NULL, NULL, test_ise_show, NULL);
	...
}
@endcode
@code
static void
_edje_key_down_cb(void *data, Evas *e , Evas_Object *obj, void *event_info)
{
	Evas_Event_Key_Down *ev = event_info;
	//if (!strcmp(ev->key, "BackSpace"))
	printf("Key Event from ISE is %s \n", ev->key); //here you can get the Key Event from ISE
}

Ecore_IMF_Context *imf_context_create(Evas_Object *obj)
{
	Ecore_IMF_Context *imf_context = NULL;
	const char *ctx_id = ecore_imf_context_default_id_get();
	Evas *evas = evas_object_evas_get(obj);

	if (ctx_id)
	{
		imf_context = ecore_imf_context_add(ctx_id);
		if (obj)
		{
			ecore_imf_context_client_window_set(imf_context, (void *)ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)));
			// the canvas information is used for supporting the auto rotation of input panel
			ecore_imf_context_client_canvas_set(imf_context, evas);
		}
	}

	return imf_context;
}

void test_ise_show(void *data, Evas_Object *obj, void *event_info)
{
	Ecore_IMF_Context *imf_context = NULL;

	imf_context = imf_context_create(obj);
	//printf("imf_context = %d\n", imf_context);

	if (imf_context)
		ecore_imf_context_input_panel_show(imf_context);

	evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, _edje_key_down_cb, NULL);
	evas_object_focus_set(obj, EINA_TRUE);
}

static void my_win_main(void)
{
	...
	elm_list_item_append(li, "ISE SHOW", NULL, NULL, test_ise_show, NULL);
	...
}
@endcode
@}
@defgroup ISF_Use_Cases1_98 How to get key event
@ingroup ISF_Use_Cases1
@{

<h3 class="pg">Sample 4 - How to get key event</h3>
- How to get key event and identify entry
	-# Register the event EVAS_CALLBACK_KEY_UP callback function for each entry which needs to process key up event.
	@code
evas_object_event_callback_add(_entry1, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, (void *)NULL);
	@endcode
	-# Make callback function like below to identify which entry received key event
	@code
static void
_evas_key_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	static char str [100];
	Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;

	if (obj == _entry1)
		sprintf (str, "entry 1  get keyEvent: %s", ev->keyname);
	else if (obj == _entry2)
		sprintf (str, "entry 2  get keyEvent: %s", ev->keyname);
	else
		sprintf (str, "");

	elm_object_text_set(_key_event_label, str);
}
	@endcode

- How to use Ecore event handlers
	-# The ECORE_IMF_EVENT_PREEDIT_CHANGED and ECORE_IMF_EVENT_COMMIT are not bound to object. However ISF always sends the preedit string and commit string to the focused entry. So in the callback functions for the two events, we can determine which entry receives the preedit string and commit string by checking which entry gets the focus. The function to determine which widget gets the focus is elm_object_focus_get().
	-# For preedit string, we need call ecore_imf_context_preedit_string_get()  to get the preedit string in its callback function.
	-# For commit string, the third parameter ev of _ecore_imf_event_commit_cb contain the commit string.

- How to get pre-edit string and identify entry
	-# Register the event ECORE_IMF_EVENT_PREEDIT_CHANGED callback function for each entry which needs to process pre-edit.
	@code
ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, _ecore_imf_event_changed_cb, NULL);
	@endcode
	-# Make callback function like below
	@code
static Eina_Bool _ecore_imf_event_changed_cb(void *data, int type, void *event)
{
	static char str [100];
	char *preedit_string;
	int len;
	Ecore_IMF_Context *imf_context = NULL;

	if (elm_object_focus_get(_entry1))
	{
		imf_context = elm_entry_imf_context_get(_entry1);
		ecore_imf_context_preedit_string_get(imf_context, &preedit_string, &len);

		sprintf(str, "entry 1 get preedit string: %s", preedit_string);
	}
	else if (elm_object_focus_get(_entry2))
	{
		imf_context = elm_entry_imf_context_get(_entry2);
		ecore_imf_context_preedit_string_get(imf_context, &preedit_string, &len);

		sprintf(str, "entry 1 get preedit string: %s", preedit_string);
	}
	else
		sprintf(str, "");

	free(preedit_string);

	elm_object_text_set(_preedit_event_label,str);

	return ECORE_CALLBACK_PASS_ON;
}
	@endcode

- How to get commit string and identify entry
	-# Register the event ECORE_IMF_EVENT_COMMIT callback function for each entry which needs to process pre-edit.
	@code
ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _ecore_imf_event_commit_cb, NULL);
	@endcode
	-# Make callback function like below
	@code
static Eina_Bool _ecore_imf_event_commit_cb(void *data, int type, void *event)
{
	static char str [100];
	Ecore_IMF_Event_Commit *ev = (Ecore_IMF_Event_Commit *) event;

	if (elm_object_focus_get(_entry1))
		sprintf(str, "entry 1 get commit string: %s", ev->str);
	else if (elm_object_focus_get(_entry2))
		sprintf (str, "entry 2 get commit string: %s", ev->str);
	else
		sprintf (str, "");

	elm_object_text_set(_commit_event_label,str);

	return ECORE_CALLBACK_PASS_ON;
}
	@endcode
- How to use Ecore event handlers
	-# The ECORE_IMF_EVENT_PREEDIT_CHANGED and ECORE_IMF_EVENT_COMMIT are not bound to object. But ISF always send the preedit string and commit string to the focused entry. So in the callback functions for the two events, we can determine which entry receive the preedit string and commit string by checking which entry get the focus. The function to determine which widget gets the focus is elm_object_focus_get().
	-# For preedit string, we need call ecore_imf_context_preedit_string_get()  to get the preedit string in its callback function. After getting preedit_string, you should deallocate memory by using free().
	-# For commit string, the third parameter ev of _ecore_imf_event_commit_cb contain the commit string.

- Example - In case the application does not use an Entry
	-# We use elm_label as the sample for showing how to get the key event, commit string and preedit string in case the application does not use an entry. Most part is similar to the application using entry except the following b) and c).
	-# Creating an imcontext instance.
@code
Evas_Object *label;

......
static Eina_Bool _ecore_imf_event_changed_cb(void *data, int type, void *event)
{
	//example how to get preedit string
	static char str [100];
	char *preedit_string;
	int len;
	Ecore_IMF_Context * imf_context = (Ecore_IMF_Context *)data;

	ecore_imf_context_preedit_string_get(imf_context, &preedit_string, &len);
	sprint(str, "preedit string : %s\n", preedit_string);

	elm_object_text_set(label, str);
	free(preedit_string);

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _ecore_imf_event_commit_cb(void *data, int type, void *event)
{
	//example how to get commit string
	Ecore_IMF_Event_Commit *ev = (Ecore_IMF_Event_Commit *) event;

	sprint(str, "commit string : %s\n", ev->str);
	elm_object_text_set(label, ev->str);

	return ECORE_CALLBACK_PASS_ON;
}

void
isf_label_event_demo_bt(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = (struct appdata *)data;
	Ecore_IMF_Context * _imf_context;
	Evas *evas = evas_object_evas_get(obj);

	if (ad == NULL) return;

	label = elm_label_add(ad->win_main);

	const char *ctx_id = ecore_imf_context_default_id_get();
	_imf_context =  ecore_imf_context_add(ctx_id);

	ecore_imf_context_client_window_set(imf_context, (void *)ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)));
	// the canvas information is used for supporting the auto rotation of input panel
	ecore_imf_context_client_canvas_set(imf_context, evas);

	ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _ecore_imf_event_commit_cb, _imf_context);
	ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, _ecore_imf_event_changed_cb, _imf_context);
}
@endcode
@}

@defgroup ISF_Use_Cases3  ISE Dev Guide
@ingroup ISF_Use_Cases
@{
<h1 class="pg">ISE Development Guide</h1>
<h2 class="pg">Purpose</h2>
This document aimed at ISE (Input Service Engine) developers to make their own keyboard (i.e. ISE) quickly, with using of ISF (Input Service Framework) in TIZEN.
The contents include:
- Making ISE step by step
- How to install ISE on target
- An example
- Other you may be interested

<h2 class="pg">Let's get started</h2>
ISF in TIZEN is designed to support a variety of ISEs, including soft keyboard, hand-writing recognizers, and hard keyboard. In this document our focus will be on soft keyboards, since this is most often developing ISE in support TIZEN text input. For more detail about other ISE types, you can refer to "ISF User Manual" for getting more information.
With ISE of soft keyboard, the life-cycle of input flow looks like below:

@image html TIZEN_ISF_PG_lifecycle_of_input_flow.png
<center><b>Figure.</b> Life Cycle of Input Flow</center>
@}

@defgroup ISF_Use_Cases3_1 Step1 : Define and Include
@ingroup ISF_Use_Cases3
@{

<h2 class="pg">For generating a new ISE with soft keyboard, let's follow 6 steps to make it. </h2>
<h3 class="pg">Step 1 : Define and Include</h3>
@code
#define Uses_SCIM_HELPER
#define Uses_SCIM_CONFIG_BASE
#include <scim.h>
using namespace scim;

#define YOUR_ISE_UUID               "ff119040-4062-b8f0-9ff6-3575c0a84f4f"
#define YOUR_ISE_NAME               "Your ISE Name"
#define YOUR_ISE_ICON               (SCIM_ICONDIR "/your_ise_icon.png")
#define YOUR_ISE_DESCRIPTION        "Your ISE Description"
@endcode
Here, HELPER   mainly means ISEs with using touch screen device.
@}

@defgroup ISF_Use_Cases3_2 Step2 : Implement 2 objects
@ingroup ISF_Use_Cases3
@{

<h3 class="pg">Step 2 : Implement 2 objects</h3>
@code
static HelperInfo  helper_info (YOUR_ISE_UUID,
                                YOUR_ISE_NAME,
                                String (YOUR_ISE_ICON),
                                YOUR_ISE_DESCRIPTION,
                                SCIM_HELPER_STAND_ALONE|SCIM_HELPER_AUTO_RESTART);
@endcode

Structure helper_info collects basic information of your ISE. UUID is the unique ID for your ISE, also the NAME and ICON.

The most often using helper_info options:
<table>
<tr><th>Option</th><th>Description</th></tr>
<tr><td>SCIM_HELPER_STAND_ALONE</td><td>compulsory option</td></tr>
<tr><td>SCIM_HELPER_AUTO_RESTART</td><td>Option indicates ISE will be restarted when it exits abnormally. We strongly recommend you use this option only in your release version, for not hiding abnormal cases.</td></tr>
</table>

@code
HelperAgent           _helper_agent;
@endcode

Class HelperAgent is an accessory class to write a Helper object. This class implements all Socket transaction protocol between Helper object and Panel. Helper objects and Panel communicate with each other via the Socket created by Panel. Helper object could use command and signal through Panel Socket to do some operations with Application, Hard Keyboard ISE and Configure Module.

About detailed description of HelperAgent, please refer to "ISF User Manual", and the most often using methods of HelperAgent object are listed here.
@}

@defgroup ISF_Use_Cases3_3 Step3 : Implement 6 interfaces
@ingroup ISF_Use_Cases3
@{

<h3 class="pg">Step 3 : Implement 6 interfaces</h3>
First of all, you need implement scim_module_init and scim_module_exit, for your ISE will be a module loaded by ISF.
scim_module_init will be called by ISF when the ISE module is loaded.  You can do your general module initialization work in it.

scim_module_exit will be called by ISF when the ISE module is unloaded. You can do the general module cleaning work in it.
@code
void scim_module_init(void)
{
	bindtextdomain(GETTEXT_PACKAGE, SCIM_INPUT_PAD_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	...
	Do your initializing work here
	...
}

void scim_module_exit(void)
{
	...
	Do your finalizing work here
	...
}
@endcode

Then you will implement the information collection interfaces for your Helper.
scim_helper_module_number_of_helpers gets the number of Helpers in this module. Because a Helper module can have multiple Helpers in it, and each Helper should run in its own process space.
scim_helper_module_get_helper_info gets the basic information of a Helper.
scim_helper_module_get_helper_language gets supported language for the specific Helper ISE.

@code
unsigned int scim_helper_module_number_of_helpers(void)
{
	return 1; //1 for common use
}

bool scim_helper_module_get_helper_info(unsigned int idx, HelperInfo &info)
{
	if (idx == 0)
	{
		info = _helper_info;
		return true;
	}
	return false;
}

String scim_helper_module_get_helper_language(unsigned int idx)
{
	if (idx == 0)
	{
		String strLanguage("en,de_DE,fr_FR,es,pt,it_IT,nl_NL,ar,ru_RU");
		return strLanguage;
	}
	return "other";
}
@endcode

Then, you need implement scim_helper_module_run_helper.

This interface is the last but the most important one, because most of your real implementation of your ISE will be here.

@code
void scim_helper_module_run_helper(const String &uuid, const ConfigPointer &config, const String &display)
{
	_config = config;
	if (uuid == YOUR_ISE_UUID)
	{
		...
		Read your configure here
		...
		run(display);
	}
}
@endcode

@code
void run(const String &display)
{
	char **argv = new char * [4];
	int    argc = 3;
	argv [0] = const_cast<char *> ("input-pad");
	argv [1] = const_cast<char *> ("--display");
	argv [2] = const_cast<char *> (display.c_str ());
	argv [3] = 0;
	setenv("DISPLAY", display.c_str(), 1);

	elm_init(argc, argv);

	_helper_agent.signal_connect_exit(slot(slot_exit));
	...
	Do your signal connection work here
	...

	int id, fd;
	id = _helper_agent.open_connection(_helper_info, display);
	if (id == -1)
	{
		std::cerr << "    open_connection failed!!!!!!\n";
		goto ise_exit;
	}

	fd = _helper_agent.get_connection_number();
	if (fd >= 0)
	{
		_read_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, helper_agent_input_handler, NULL, NULL, NULL);
	}

	elm_run();
	elm_shutdown();

ise_exit:
	if (argv)
		delete []argv;

}
@endcode

In scim_helper_module_run_helper, you should implement a set of signal connection for combination the signal with your real implemented functions (slots), which we'll describe in step 4.
@}

@defgroup ISF_Use_Cases3_4 Step4 : Implement main signal/slots
@ingroup ISF_Use_Cases3
@{

<h3 class="pg">Step 4 : Implement main signal/slots</h3>

- Introduction of signal/slot
ISF adopts Signal/Slot to implement some callback function mechanism. It defines one-to-many relationship between a signal object and any number of slot objects, so that when the signal is emitted, all its slots will be called automatically.
The class diagram of Signal/Slot shown like this:

@image html TIZEN_ISF_PG_slot_and_signal.png

SignalN
- Knows the prototype of its slots
- Can hold any number of slots
- Provide a interface to attach slot object at run time
SlotN
- Provides a call interface to let signal object call.
FunctionSlotN
- Derived from SlotN
- Implement the call operation.

- Implementation of signal/slot
ISE can receive many signals from ISF by HelperAgent, and you should implement those slot functions for concerned signals, then call signal connection function to connect those slots functions.  About all slot functions interface and parameters, please reference to scim_helper.h and Input-pad ISE demo. Normally below slot functions should be implemented.<br><br>
_helper_agent.signal_connect_exit(slot(slot_exit));<br>
_helper_agent.signal_connect_focus_out(slot(slot_focus_out));<br>
_helper_agent.signal_connect_focus_in(slot(slot_focus_in));<br>
_helper_agent.signal_connect_reload_config(slot(slot_reload_config));<br>

The mapping of EFLIMFControl APIs and HelperAgent signals:
<table>
<tr>
	<th>EFLIMFControl API</th>
	<th>Signal</th>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_show</td><td>signal_ise_show</td>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_hide</td><td>signal_ise_hide</td>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_imdata_set</td><td>signal_set_imdata</td>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_imdata_get</td><td>signal_get_imdata</td>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_layout_set</td><td>signal_set_layout</td>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_layout_get</td><td>signal_get_layout</td>
</tr>
<tr>
	<td>ecore_imf_context_input_panel_geometry_get</td><td>signal_get_size</td>
</tr>
</table>
@}

@defgroup ISF_Use_Cases3_5 Step5 : Interacting with window manager
@ingroup ISF_Use_Cases3
@{

<h3 class="pg">Step 5 : Interacting with window manager</h3>
In TIZEN, window manager should treat soft keyboard window differently from other normal application windows. When an ISE creates its soft keyboard window, it should notify the window manager that the created window's type is a soft keyboard type, by calling following function.

@code
elm_win_keyboard_win_set(main_window, EINA_TRUE);
@endcode

And when an application rotates its window, the window manager sends a X Client Message to a soft keyboard to let it know the state change of target application window. To handle the X client message, the following handler function should be implemented inside ISE.
@code
static int _client_message_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *)event;
	int angle;

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE) {
		angle = ev->data.l[0];
		...
		Do your keyboard rotation work here
		...
	}

	return ECORE_CALLBACK_RENEW;
}
@endcode

And the handler function should be registered as a handler function to ECORE_X_EVENT_CLIENT_MESSAGE when initializing, as shown below.

@code
Ecore_Event_Handler *handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _client_message_cb, NULL);
@endcode
@}

@defgroup ISF_Use_Cases3_6 Step6 : How to install ISE on target
@ingroup ISF_Use_Cases3
@{

<h3 class="pg">Step 6 : How to install ISE on target</h3>
To integrate an ISE into ISF, you should build ISE module as a dynamic library and install it into the predefined directory \$(TOP_INSTALL_DIR)/lib/scim-1.0/1.4.0/\$(ISE_TYPE)/. Then ISF will search the predefined directory to find the ISE module.<br>
The TOP_INSTALL_DIR is the top-level ISF installation directory. By default its value is "/usr".<br>
The ISE_TYPE is "IMEngine" (KeyboardISE) or "Helper" (HelperISE). The ISE developer should select one value according to the ISE type.<br>
If the ISE has one setup UI module, this setup UI module should be installed into "\$(TOP_INSTALL_DIR)/lib/scim-1.0/1.4.0/SetupUI" directory.<br>
The setup UI module's name should be defined as ISE file name + "-imengine-setup.so".<br>
For example, Input-pad ISE file name is "input-pad.so", its setup UI module's name is "input-pad-imengine-setup.so".

Below are the repository directories for three modules:<br>
- KeyboardISE<br>
\$(TOP_INSTALL_DIR)/lib/scim-1.0/1.4.0/IMEngine.
- HelperISE<br>
\$(TOP_INSTALL_DIR)/lib/scim-1.0/1.4.0/Helper
- ISE Setup Module<br>
\$(TOP_INSTALL_DIR)/lib/scim-1.0/1.4.0/SetupUI
@}

@defgroup ISF_Ref References
@ingroup InputServiceFramework_PG
@{

<h3 class="pg"> Demo for Softkeyboard ISE</h3>
The HelperISE uses an object of accessory class HelperAgent to deal with all Socket Transaction between HelperISE objects and PanelAgent.<br>
It needs to implement the necessary slot functions and connect the slots to the corresponding signals if they want to accept the specific events.
@code
static HelperAgent  helper_agent;
static HelperInfo helper_info(
                              String ("ff110940-b8f0-4062-9ff6-a84f4f3575c0"),
                              "Input Pad",
                 			  String (SCIM_INPUT_PAD_ICON),
                              "",
                              SCIM_HELPER_STAND_ALONE|SCIM_HELPER_NEED_SCREEN_INFO);

void scim_module_init(void)
{
	bindtextdomain(GETTEXT_PACKAGE, SCIM_INPUT_PAD_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	_helper_info.name        = String (_("Input Pad"));
	_helper_info.description = String (_("An On Screen Input Pad to input 	characters easily."));
}

void scim_module_exit(void)
{
}

unsigned int scim_helper_module_number_of_helpers(void)
{
	return 1;
}

bool scim_helper_module_get_helper_info(unsigned int idx, HelperInfo &info)
{
	if (idx == 0) {
		info = helper_info;
		return true;
	}
	return false;
}

String scim_helper_module_get_helper_language(unsigned int idx)
{
	if (idx == 0)
	{
		String
		strLanguage ("en,de_DE,fr_FR,es,pt,tr_TR,el_GR,it_IT,nl_NL,ar,ru_RU,"
			"az_AZ,bn,bg_BG,ca_ES,cs_CZ,cy_GB,da_DK,et_EE,"
			"eu_ES,fi_FI,ga_IE,gl_ES,gu_IN,he_IL,hi_IN,hu_HU,"
			"is_IS,ka_GE,kk_KZ,km,kn_IN,lt_LT,lv_LV,mk_MK,"
			"ml_IN,mn_MN,Marathi,ms_MY,no_NO,pa_IN,pl_PL,"
			"ro_RO,si_LK,sk_SK,sl_SI,sq_AL,sr_CS,sv,ta_IN,"
			"te_IN,th_TH,uk_UA,ur_PK,z_UZ,vi_VN,"
			"zh_CN,zh_TW,zh_HK,zh_SG");

		return strLanguage;
	}
	return "other";
}

void scim_helper_module_run_helper(const String &uuid, const ConfigPointer &config, const String &display)
{
	char **argv = new char * [4];
	int    argc = 3;

	argv [0] = const_cast<char *> ("input-pad");
	argv [1] = const_cast<char *> ("--display");
	argv [2] = const_cast<char *> (display.c_str ());
	argv [3] = 0;

	setenv("DISPLAY", display.c_str(), 1);

	elm_init(argc, argv);
	......
	helper_agent.signal_connect_exit(slot(slot_exit));
	helper_agent.signal_connect_focus_out(slot(slot_focus_out));
	helper_agent.signal_connect_focus_in(slot(slot_focus_in));
	......
	int id, fd;
	id = _helper_agent.open_connection(_helper_info, display);
	if (id == -1)
	{
		std::cerr << "    open_connection failed!!!!!!\n";
		goto ise_exit;
	}

	fd = _helper_agent.get_connection_number();
	if (fd >= 0)
	{
		_read_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ,
		helper_agent_input_handler, NULL, NULL, NULL);
		//ecore_main_fd_handler_active_set(_read_handler, ECORE_FD_READ);
	}

	create_ise_window();
	init_candidate_table_labels();
	......
	_helper_agent.get_keyboard_ise_list(_helper_info.uuid);
	_helper_agent.get_candidate_ui(_helper_info.uuid);
	_helper_agent.get_candidate_window_rect(_helper_info.uuid);
	_helper_agent.get_keyboard_ise(_helper_info.uuid);

	_helper_agent.set_candidate_position(-1, -1);
	elm_run();
	elm_shutdown();

ise_exit:
	if (argv)
		delete []argv;
	......
}
@endcode

Here, we will use the skeleton of InputPad ISE as an example. InputPad is a software keyboard ISE. By studying the code segment example, you can see:
-# As mentioned in step 3, the HelperISE should at least implement the six interfaces "scim_module_init()", "scim_module_exit()", "scim_helper_module_number_of_helpers()", "scim_helper_module_get_helper_info()", "scim_helper_module_get_helper_language()" and "scim_helper_module_run_helper()"
-# How to send the result to the target input widget
HelperAgent class provides two member functions to do this job, helper_agent.commit_string() and helper_agent.send_key_event(). As indicated by the function name, commit_string() is to send a string to the target input widget while send_key_event() is to send a key event to the target input widget.
	-# helper_agent.commit_string(-1, "", scim::utf8_mbstowcs(str));
	-# helper_agent.send_key_event(-1, "", key);
In InputPad, the first parameters of commit_string and send_key_event are assigned with -1 and "", then the result will be sent to the last focused widget.
-# How to receive the ISF events
Here, we adopt Signal/Slot to connect the callback functions (Slot) to the corresponding events (Signal). In InputPad, helper_agent.signal_connect_exit(), helper_agent.signal_connect_update_screen() etc will do the connection. Currently, the InputPad will emit the following signals:
	-# signal_exit
	-# signal_ise_show
	-# signal_ise_hide
	-# signal_get_size
	-# signal_set_layout
	-# signal_update_candidate_ui
	-# signal_update_keyboard_ise

Most often HelperAgent Methods
<table>
<tr>
	<th>Method</th>
	<th>Description</th>
<tr>
	<td>int open_connection(const HelperInfo &info, const String &display);</td>
	<td>Open socket connection to the Panel.
- Param info: The information of this Helper object.
- Param display: The display which this Helper object should run on.
Return the connection socket id. -1 means failed to create the connection
	</td>
</tr>
<tr>
	<td>void close_connection();</td><td>Close the socket connection to Panel.</td>
</tr>
<tr>
	<td>int get_connection_number() const;</td><td>Get the connection id previously returned by open_connection().</td>
</tr>
<tr>
	<td>bool is_connected() const;</td>
	<td>Return whether this HelperAgent has been connected to a Panel.
Return true if there are any events available.
</td>
</tr>
<tr>
	<td>bool has_pending_event() const;</td>
	<td>Check if there are any events available to be processed. If it returns true then Helper object should call HelperAgent::filter_event() to process them.</td>
</tr>
<tr>
	<td>bool filter_event();</td>
	<td>Process the pending events. This function will emit the corresponding signals according to the events.
Return false if the connection is broken, otherwise return true.
</td>
</tr>
<tr>
	<td>void reload_config() const;</td><td>Request ISF to reload all configurations. This function should only be used by Setup Helper to request ISF's reloading the configuration.</td>
</tr>
<tr>
	<td>void send_imengine_event(int ic, const String  &ic_uuid, const Transaction  &trans) const;</td>
	<td>Send a set of events to a Hard Keyboard ISE Instance. All events should be put into a Transaction. And the events can only be received by one IMEngineInstance (Hard keyboard ISE) object.
- Param ic: The handle of the Input Context to receive the events.
- Param ic_uuid: The UUID of the Input Context.
- Param trans: The Transaction object holds the events.
</td>
</tr>
<tr>
	<td>void send_key_event(int ic, const String &ic_uuid, const KeyEvent &key) const;</td>
	<td>Send a KeyEvent to an IMEngineInstance.
- Param ic: The handle of the IMEngineInstance to receive the event. -1 means the currently focused IMEngineInstance.
- Param ic_uuid: The UUID of the IMEngineInstance. Empty means don't match.
- Param key: The KeyEvent to be sent.
</td>
</tr>
<tr>
	<td>void forward_key_event(int ic, const String &ic_uuid, const KeyEvent &key) const;</td>
	<td>Forward a KeyEvent to client application directly.
- Param ic: The handle of the IMEngineInstance to receive the event. -1 means the currently focused IMEngineInstance.
- Param ic_uuid: The UUID of the IMEngineInstance. Empty means don't match.
	</td>
</tr>
<tr>
	<td>void commit_string(int ic, const String &ic_uuid, const WideString &wstr) const;</td>
	<td>Commit a WideString to client application directly.
- Param ic: The handle of the client Input Context to receive the WideString. -1 means the currently focused Input Context.
- Param ic_uuid: The UUID of the IMEngine used by the Input Context. Empty means don't match.
- Param wstr: The WideString to be committed.
	</td>
</tr>
<tr>
	<td>void update_input_context(uint32 type,	 uint32 value) const;</td>
	<td>When the input context of ISE is changed, ISE can call this function to notify application.
- Param type: type of event. The type of event may differ in different project. It is an engagement between ISE and the applications. One example definition is ECORE_IMF_INPUT_PANEL_EVENT in Ecore_IMF.h in TIZEN project.
- Param value: value of event.
	</td>
</tr>
<tr>
	<td>void set_candidate_ui(const ISF_CANDIDATE_STYLE_T style, const ISF_CANDIDATE_MODE_T mode) const;</td>
	<td>Set the style and mode for candidate window.
- Param style: The candidate window style.
ISF_CANDIDATE_STYLE_T may differ in different project. Please refer the latest definition  in scim_utility.h.
- Param mode: The candidate window mode.
ISF_CANDIDATE_MODE_T may differ in different project. Please refer the latest definition in scim_utility.h.
	</td>
</tr>
<tr>
	<td>void get_candidate_ui(const String &uuid) const;</td><td>Send request to get the candidate window's style and mode.</td>
</tr>
<tr>
	<td>void set_candidate_position(int left, int top) const;</td>
	<td>Set the position for candidate window.
- Param left: The candidate window's left position.
- Param top: The candidate window's top position.
- If both Params set <0, the candidate will be default set to cursor following mode.
	</td>
</tr>
<tr>
	<td>void candidate_hide(void) const;</td>
	<td>Send request to hide the candidate window, including Aux, Candidate and Associate area</td>
</tr>
<tr>
	<td>void get_candidate_window_rect(const String &uuid) const;</td>
	<td>Send request to get the candidate window's size and position.
- Param uuid: The Helper ISE's uuid.
	</td>
</tr>
<tr>
	<td>void set_keyboard_ise_by_uuid(const String &uuid) const;</td>
	<td>Set the keyboard ISE by ISE's uuid.
- Param uuid: The keyboard ISE's uuid.
</td>
</tr>
<tr>
	<td>void get_keyboard_ise(const String	&uuid) const;</td>
	<td>Send request to get the current keyboard ISE.
- Param uuid: The Helper ISE's uuid.
</td>
</tr>
</table>
@}
*/

/**
* @addtogroup  InputServiceFramework_PG Input Service FW
 @{
* 	@defgroup ISF_Use_Cases Use Cases
 	@{
*		@defgroup ISF_Use_Cases1
*		@defgroup ISF_Use_Cases2
*		@defgroup ISF_Use_Cases3
		@brief For generating a new ISE with soft keyboard, let's follow 6 steps to make it.
	@}
 @}
*/
