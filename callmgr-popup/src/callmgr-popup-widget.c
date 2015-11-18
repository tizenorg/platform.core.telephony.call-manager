/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact: Sung Joon Won <sungjoon.won@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <Eina.h>
#include <efl_extension.h>
#include <efl_util.h>
/* ToDo: Need check replace */ 
//#include <Ecore_X.h>
//#include <Ecore_X_Atoms.h>
//#include <utilX.h>
#include <notification.h>
#include <notification_internal.h>

#include "callmgr-popup-common.h"
#include "callmgr-popup-widget.h"
#include "callmgr-popup-dbus-if.h"
#include "callmgr-popup-util.h"
#include "callmgr-popup-vconf.h"

/* ToDo: Need check replace */ 
#define KEY_POWER	"XF86PowerOff"
#define SHARED_GRAB		0x000f00

static const char* simIconNode[SIM_ICON_MAX] = {
	"logs_icon_sim_1.png",
	"logs_icon_sim_2.png",
	"logs_icon_sim_phone.png",
	"logs_icon_sim_messages.png",
	"logs_icon_sim_data.png",
	"logs_icon_sim_internet.png",
	"logs_icon_sim_home.png",
	"logs_icon_sim_office.png",
	"logs_icon_sim_heart.png"
};

static void __callmgr_popup_set_win_level(Evas_Object *parent, int bwin_noti);

static void __callmgr_popup_win_del_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_exit();
}

Evas_Object *_callmgr_popup_create_win(void)
{
	Evas_Object *eo = NULL;
	Ecore_Evas *ee;
	Evas *e;
	const char *str = "mobile";

	int	w, h;

	eo = elm_win_add(NULL, PACKAGE, ELM_WIN_DIALOG_BASIC);
	e = evas_object_evas_get(eo);
	if (e) {
		ee = ecore_evas_ecore_evas_get(e);
		if (ee) {
			ecore_evas_name_class_set(ee, "APP_POPUP", "APP_POPUP");
		}
	}

	if (eo) {
		elm_win_profile_set(eo, &str);	/* Desktop mode only */
		elm_win_title_set(eo, PACKAGE);
		elm_win_alpha_set(eo, EINA_TRUE);
		elm_win_raise(eo);

		elm_win_autodel_set(eo, EINA_TRUE);
		/* Do not need to resize routine in application level in 3.0 */
		//ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
		//evas_object_resize(eo, w, h);

		evas_object_smart_callback_add(eo, "delete,request", __callmgr_popup_win_del_cb, NULL);

	       /* Handle rotation */
		if (elm_win_wm_rotation_supported_get(eo)) {
			int rots[4] = {0, 90, 180, 270};
			elm_win_wm_rotation_available_rotations_set(eo, rots, 4);
		}

		if ((_callmgr_popup_is_idle_lock())
			|| (_callmgr_popup_is_pw_lock())
			|| (_callmgr_popup_is_fm_lock())) {
			DBG("Set high window and block noti");
			__callmgr_popup_set_win_level(eo, EINA_TRUE);
		} else {
			DBG("Normal window");
			__callmgr_popup_set_win_level(eo, EINA_FALSE);
		}
		evas_object_show(eo);
		/* Platform does not support changeable UI */
		/*ea_theme_changeable_ui_enabled_set(EINA_TRUE);*/
	}

	return eo;
}

static void __callmgr_popup_set_win_level(Evas_Object *parent, int bwin_noti)
{
	Ecore_X_Window xwin;
	DBG("..");
	//Ecore_X_Atom qp_scroll_state = ecore_x_atom_get("_E_MOVE_PANEL_SCROLLABLE_STATE");;	/* Quick panel scrollable state */

	/* Get x-window */
	//xwin = elm_win_xwindow_get(parent);

	if (bwin_noti == EINA_FALSE) {
		/* Set Normal window */
		/* ToDo: Need check replace */
		//ecore_x_netwm_window_type_set(xwin, ECORE_X_WINDOW_TYPE_NORMAL);
	} else {
		//unsigned int val[3] = {0, 0, 0};

		efl_util_set_notification_window_level(parent, EFL_UTIL_NOTIFICATION_LEVEL_HIGH);

		/* Set Notification window */
		/* ToDo: Need check replace */ 
		/* ecore_x_netwm_window_type_set(xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION); */
		/* ToDo: Need check replace */ 
		/* utilx_set_system_notification_level(ecore_x_display_get(), xwin, UTILX_NOTIFICATION_LEVEL_HIGH); */
		//ecore_x_window_prop_card32_set(xwin, qp_scroll_state, val, 3);
	}
}

static Eina_Bool __callmgr_popup_win_hard_key_down_cb(void *data, int type, void *event)
{
	DBG("..");
	Ecore_Event_Key *ev = event;
/*	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;*/

	if (!strcmp(ev->keyname, KEY_POWER)) {
	}

	return EINA_FALSE;
}

static Eina_Bool __callmgr_popup_win_hard_key_up_cb(void *data, int type, void *event)
{
	DBG("..");
	Ecore_Event_Key *ev = event;
/*	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;*/

	if (!strcmp(ev->keyname, KEY_POWER)) {
	}
	return EINA_FALSE;
}

void _callmgr_popup_system_grab_key(void *data)
{
	int result = 0;
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	ret_if(NULL == ad);

	//result = utilx_grab_key(ecore_x_display_get(), elm_win_xwindow_get(ad->win_main), KEY_POWER, SHARED_GRAB);
	result = elm_win_keygrab_set(ad->win_main, KEY_POWER, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED);
	if (result) {
		ERR("KEY_POWER key grab failed");
	} else {
		WARN("grab key");
	}

	ad->upkey_handler = ecore_event_handler_add(ECORE_EVENT_KEY_UP, __callmgr_popup_win_hard_key_up_cb, ad);
	ad->downkey_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, __callmgr_popup_win_hard_key_down_cb, ad);
}

void _callmgr_popup_system_ungrab_key(void *data)
{
	int result = 0;
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	ret_if(NULL == ad);

	//result = utilx_ungrab_key(ecore_x_display_get(), elm_win_xwindow_get(ad->win_main), KEY_POWER);
	result = elm_win_keygrab_unset(ad->win_main, KEY_POWER, 0, 0);
	if (result) {
		WARN("KEY_POWER key ungrab failed");
	} else {
		WARN("ungrab key");
	}

	if (ad->upkey_handler != NULL) {
		ecore_event_handler_del(ad->upkey_handler);
		ad->upkey_handler = NULL;
	}
	if (ad->downkey_handler != NULL) {
		ecore_event_handler_del(ad->downkey_handler);
		ad->downkey_handler = NULL;
	}
}

static void __callmgr_popup_hw_key_unload(void *data, Evas_Object *obj, void *event_info)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;

	if (ad->popup) {
		evas_object_del(ad->popup);
	}

	_callmgr_popup_reply_to_launch_request(ad, "RESULT", "0");	/* "0" means CANCEL */
	elm_exit();
}

static void __callmgr_popup_fm_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;

	_callmgr_popup_reply_to_launch_request(ad, "RESULT", "1");	/* "1" means OK */
	_callmgr_popup_dial_call(ad->dial_num, ad->call_type, ad->active_sim, 1, FALSE, ad);
	if (ad->popup) {
		evas_object_del(ad->popup);
	}
}

static void __callmgr_popup_fm_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;

	if (ad->popup) {
		evas_object_del(ad->popup);
	}

	_callmgr_popup_reply_to_launch_request(ad, "RESULT", "0");	/* "0" means CANCEL */
	elm_exit();
}

void _callmgr_popup_create_flight_mode(void *data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	ret_if(NULL == ad);

	Evas_Object *btn_ok = NULL;
	Evas_Object *btn_cancel = NULL;

	if (!ad->win_main) {
		ad->win_main = _callmgr_popup_create_win();
		if (!ad->win_main) {
			ERR("win error");
			elm_exit();
		}
	}
	_callmgr_popup_del_popup(ad);

	ad->popup = elm_popup_add(ad->win_main);
	elm_popup_align_set(ad->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_domain_translatable_part_text_set(ad->popup, "title,text", PACKAGE, _("IDS_CST_HEADER_UNABLE_TO_MAKE_CALL_ABB"));
	elm_object_text_set(ad->popup, _("IDS_KPD_POP_FLIGHT_MODE_ON_TURN_FLIGHT_MODE_OFF_TO_MAKE_CALLS"));

	btn_cancel = elm_button_add(ad->popup);
	elm_object_style_set(btn_cancel, "popup");
	elm_object_text_set(btn_cancel, _("IDS_COM_SK_CANCEL"));
	elm_object_part_content_set(ad->popup, "button1", btn_cancel);
	evas_object_smart_callback_add(btn_cancel, "clicked", __callmgr_popup_fm_popup_cancel_cb, ad);

	btn_ok = elm_button_add(ad->popup);
	elm_object_style_set(btn_ok, "popup");
	elm_object_text_set(btn_ok, _("IDS_COM_SK_OK"));
	elm_object_part_content_set(ad->popup, "button2", btn_ok);
	evas_object_smart_callback_add(btn_ok, "clicked", __callmgr_popup_fm_popup_ok_cb, ad);

	eext_object_event_callback_add(ad->popup, EEXT_CALLBACK_BACK,
				__callmgr_popup_hw_key_unload, ad);

	evas_object_show(ad->popup);
}

Evas_Object *_callmgr_popup_create_progressbar(Evas_Object *parent, const char *type)
{
	Evas_Object *progressbar = NULL;

	progressbar = elm_progressbar_add(parent);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	elm_object_style_set(progressbar, type);
	elm_progressbar_horizontal_set(progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(progressbar);

	return progressbar;
}

void _callmgr_popup_create_popup_checking(void *data, char *string)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	ret_if(NULL == ad);

	char popup_msg[DEF_BUF_LEN_LONG] = { 0, };
	Evas_Object *popup_ly = NULL;
	Evas_Object *progressbar = NULL;

	if (!ad->win_main) {
		ad->win_main = _callmgr_popup_create_win();
		if (!ad->win_main) {
			ERR("win error");
			elm_exit();
		}
	}
	_callmgr_popup_del_popup(ad);

	snprintf(popup_msg, DEF_BUF_LEN_LONG, "%s", string);
	DBG("msg:[%s]", popup_msg);

	ad->popup = elm_popup_add(ad->win_main);
	elm_popup_align_set(ad->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	popup_ly = elm_layout_add(ad->popup);
	elm_layout_file_set(popup_ly, PATH_EDJ, "popup_processingview");
	elm_object_part_text_set(popup_ly, "elm.text", popup_msg);
	evas_object_size_hint_weight_set(popup_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	progressbar = _callmgr_popup_create_progressbar(ad->popup, "process_medium");

	elm_object_part_content_set(popup_ly, "elm.swallow.content", progressbar);
	elm_object_content_set(ad->popup, popup_ly);

	evas_object_show(ad->popup);
}

void _callmgr_popup_create_toast_msg(char *string)
{
	DBG("Noti-String : %s", string);

	if (string) {
		notification_status_message_post(string);
	}

	elm_exit();
}

void _callmgr_popup_del_popup(void *data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	ret_if(NULL == ad);

	if (ad->popup) {
		evas_object_del(ad->popup);
		ad->popup = NULL;
	}
}

static char *__callmgr_popup_sim_list_gl_label_get(void *data, Evas_Object *obj, const char *part)
{
	int idx = GPOINTER_TO_INT(data);
	char *sim_name = NULL;
	if (strcmp(part, "elm.text.main.left") == 0) {
		if (idx == 0) {
			sim_name = vconf_get_str(VCONFKEY_SETAPPL_SIM1_NAME);
		} else {
			sim_name = vconf_get_str(VCONFKEY_SETAPPL_SIM2_NAME);
		}

		if (NULL == sim_name) {
			ERR("vconf_get_str failed returning NULL");
			return NULL;
		} else {
			DBG("sim_name :%s", sim_name);
			return sim_name;
		}
	}

	return NULL;
}

static Evas_Object *__callmgr_popup_sim_list_gl_icon_get(void *data, Evas_Object *obj, const char *part)
{
	int idx = GPOINTER_TO_INT(data);
	Evas_Object *sim_icon = NULL;
	int sim_icon_index = 0;
	Evas_Object *main_ly = NULL;
	if (!obj) {
		return NULL;
	}

	if (strcmp(part, "elm.icon.1") == 0) {
		if (idx == 0) {
			if (vconf_get_int(VCONFKEY_SETAPPL_SIM1_ICON, &sim_icon_index)) {
				ERR("Vconf get error : VCONFKEY_SETAPPL_SIM1_ICON");
			}
		} else {
			if (vconf_get_int(VCONFKEY_SETAPPL_SIM2_ICON, &sim_icon_index)) {
				ERR("Vconf get error : VCONFKEY_SETAPPL_SIM2_ICON");
			}
		}
		DBG("sim_icon_index: %d", idx);

		if (sim_icon_index >= 0 && sim_icon_index < SIM_ICON_MAX) {
			main_ly = elm_layout_add(obj);
			elm_layout_theme_set(main_ly, "layout", "list/B/popup", "sim_select");
			sim_icon = elm_icon_add(main_ly);
			elm_image_file_set(sim_icon, PATH_EDJ, simIconNode[sim_icon_index]);
			elm_layout_content_set(main_ly, "elm.swallow.icon", sim_icon);
			return main_ly;
		} else {
			ERR("icon is not proper");
		}
	}
	return NULL;
}

static Eina_Bool __callmgr_popup_exit_idler_cb(void *data)
{
	DBG("__callmgr_popup_exit_idler_cb.");
	elm_exit();
	return ECORE_CALLBACK_CANCEL;
}

static void __callmgr_popup_sim_list_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	DBG("__callmgr_popup_sim_list_gl_sel.");
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	Elm_Object_Item *item = (Elm_Object_Item *) event_info;

	_callmgr_popup_del_popup(ad);

	if (item != NULL) {
		int index = GPOINTER_TO_INT(elm_object_item_data_get(item));
		DBG("index: %d", index);

		_callmgr_popup_reply_to_launch_request(ad, "RESULT", "1");	/* "1" means OK */
		_callmgr_popup_dial_call(ad->dial_num, ad->call_type, index, 0, FALSE, ad);
	} else {
		ERR("item is NULL");
	}

	if (!_callmgr_popup_is_flight_mode()) {
		/* Do not terminate application to display flight mode popup */
		if (ad->exit_idler) {
			ecore_idler_del(ad->exit_idler);
			ad->exit_idler = NULL;
		}
		ad->exit_idler = ecore_idler_add(__callmgr_popup_exit_idler_cb, NULL);
	}

	return;
}

void _callmgr_popup_create_sim_selection(void *data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	Elm_Object_Item *it = NULL;
	int index = 0;
	ret_if(NULL == ad);

	if (!ad->win_main) {
		ad->win_main = _callmgr_popup_create_win();
		if (!ad->win_main) {
			ERR("win error");
			elm_exit();
		}
	}
	_callmgr_popup_del_popup(ad);

	ad->popup = elm_popup_add(ad->win_main);
	elm_object_style_set(ad->popup,"theme_bg");
	elm_popup_align_set(ad->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_domain_translatable_part_text_set(ad->popup, "title,text", PACKAGE, _("IDS_PB_HEADER_SELECT_SIM_CARD_TO_CALL"));

	Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
	ret_if(NULL == itc);

	itc->item_style = "1line";
	itc->func.text_get = __callmgr_popup_sim_list_gl_label_get;
	itc->func.content_get = __callmgr_popup_sim_list_gl_icon_get;
	itc->func.state_get = NULL;
	itc->func.del = NULL;

	Evas_Object *glist = elm_genlist_add(ad->popup);
	elm_genlist_mode_set(glist, ELM_LIST_COMPRESS);
	evas_object_size_hint_weight_set(glist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(glist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_scroller_content_min_limit(glist, EINA_FALSE, EINA_TRUE);

	it = elm_genlist_item_append(glist, itc, GINT_TO_POINTER(index), NULL, ELM_GENLIST_ITEM_NONE, __callmgr_popup_sim_list_gl_sel, ad);
	elm_object_item_data_set(it, GINT_TO_POINTER(index));
	index++;
	it = elm_genlist_item_append(glist, itc, GINT_TO_POINTER(index), NULL, ELM_GENLIST_ITEM_NONE, __callmgr_popup_sim_list_gl_sel, ad);
	elm_object_item_data_set(it, GINT_TO_POINTER(index));

	evas_object_smart_callback_add(ad->popup, "response",
				__callmgr_popup_hw_key_unload, ad);
	eext_object_event_callback_add(ad->popup, EEXT_CALLBACK_BACK,
				__callmgr_popup_hw_key_unload, ad);
	evas_object_smart_callback_add(ad->popup, "block,clicked",
				__callmgr_popup_hw_key_unload, ad);

	evas_object_show(glist);
	elm_object_content_set(ad->popup, glist);

	evas_object_show(ad->popup);

	return;
}

static void __callmgr_popup_try_voice_call_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;

	_callmgr_popup_reply_to_launch_request(ad, "RESULT", "1");	/* "1" means OK */
	_callmgr_popup_dial_call(ad->dial_num, ad->call_type, ad->active_sim, 0, FALSE, ad);
	if (ad->popup) {
		evas_object_del(ad->popup);
	}
}

static void __callmgr_popup_try_voice_call_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;

	if (ad->popup) {
		evas_object_del(ad->popup);
	}
	_callmgr_popup_reply_to_launch_request(ad, "RESULT", "0");	/* "0" means CANCEL */
	elm_exit();
}

void _callmgr_popup_create_try_voice_call(void *data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)data;
	ret_if(NULL == ad);

	Evas_Object *btn_ok = NULL;
	Evas_Object *btn_cancel = NULL;

	if (!ad->win_main) {
		ad->win_main = _callmgr_popup_create_win();
		if (!ad->win_main) {
			ERR("win error");
			elm_exit();
		}
	}
	_callmgr_popup_del_popup(ad);

	ad->popup = elm_popup_add(ad->win_main);
	elm_popup_align_set(ad->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_object_domain_translatable_part_text_set(ad->popup, "title,text", PACKAGE, _("IDS_CST_HEADER_UNABLE_TO_MAKE_CALL_ABB"));
	elm_object_text_set(ad->popup, _("IDS_VCALL_POP_TEXT_FALLBACK_VOICE_OUTOF3G"));

	btn_cancel = elm_button_add(ad->popup);
	elm_object_style_set(btn_cancel, "popup");
	elm_object_text_set(btn_cancel, _("IDS_COM_SK_CANCEL"));
	elm_object_part_content_set(ad->popup, "button1", btn_cancel);
	evas_object_smart_callback_add(btn_cancel, "clicked", __callmgr_popup_try_voice_call_cancel_cb, ad);

	btn_ok = elm_button_add(ad->popup);
	elm_object_style_set(btn_ok, "popup");
	elm_object_text_set(btn_ok, _("IDS_COM_SK_OK"));
	elm_object_part_content_set(ad->popup, "button2", btn_ok);
	evas_object_smart_callback_add(btn_ok, "clicked", __callmgr_popup_try_voice_call_ok_cb, ad);

	eext_object_event_callback_add(ad->popup, EEXT_CALLBACK_BACK,
				__callmgr_popup_hw_key_unload, ad);

	evas_object_show(ad->popup);
}



