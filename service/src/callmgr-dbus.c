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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "callmgr-log.h"
#include "callmgr-dbus.h"
#include "generated-code.h"
#include "callmgr-telephony.h"
#include "callmgr-contact.h"
#include "callmgr-util.h"
#include "callmgr-vconf.h"

static int __callmgr_dbus_get_gv_from_call_data(callmgr_core_data_t *core_data, callmgr_call_data_t *call_data, GVariant **gv);

/*********************************************
************* Signal Handler *********************
**********************************************/
// TODO:
// Need to add prefix
static gboolean __dial_call_handler (GDBusInterfaceSkeleton *di, GDBusMethodInvocation *invoc,
		const gchar *number, gint call_type, gint sim_slot, gint disable_fm, gboolean is_emergency_contact, gpointer user_data)
{
	int err = -1;
	dbg("_dial_handler() is called");
	CM_RETURN_VAL_IF_FAIL(number, FALSE);
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_dial(core_data, number, call_type, sim_slot, disable_fm, is_emergency_contact, FALSE);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}


// TODO:
// Need to add prefix
static gboolean __end_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const guint call_id, const gint release_type, gpointer user_data)
{
	int err = -1;
	dbg("_end_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_end_call(core_data, (unsigned int)call_id, (int)release_type);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __reject_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__reject_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_reject_call(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __swap_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("_swap_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_swap_call(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __hold_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__hold_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_hold_call(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __unhold_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__unhold_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_unhold_call(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}


// TODO:
// Need to add prefix
static gboolean __join_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("_join_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_join_call(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __split_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const guint call_id, gpointer user_data)
{
	int err = -1;
	dbg("_split_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_split_call(core_data, (unsigned int)call_id);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __transfer_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("_transfer_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_transfer_call(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __answer_call_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const gint ans_type, gpointer user_data)
{
	int err = -1;
	dbg("_answer_call_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_answer_call(core_data, (int)ans_type);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __spk_on_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__spk_on_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_spk_on(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __spk_off_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__spk_off_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_spk_off(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __bluetooth_on_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__bluetooth_on_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_bluetooth_on(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __bluetooth_off_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("_set_extra_vol_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_bluetooth_off(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __set_extra_vol_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gboolean is_extra_vol, gpointer user_data)
{
	int err = -1;
	dbg("_set_extra_vol_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_set_extra_vol(core_data, (int)is_extra_vol);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// Need to add prefix
static gboolean __set_noise_reduction_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gboolean is_noise_reduction, gpointer user_data)
{
	int err = -1;
	dbg("__set_noise_reduction_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_set_noise_reduction(core_data, is_noise_reduction);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}


// TODO:
// Need to add prefix
static gboolean __set_mute_state_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gboolean is_mute_state, gpointer user_data)
{
	int err = -1;
	dbg("__set_mute_state_handler() is called [%d]", is_mute_state);
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	err = _callmgr_core_process_set_mute_state(core_data, (int)is_mute_state);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));
	return TRUE;
}

// TODO:
// Need to add prefix
static gboolean __get_call_list_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	GVariant *call_list_gv = NULL;
	GSList *call_list = NULL;
	gboolean ret = -1;
	GVariantBuilder b;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	cm_telephony_call_data_t *call_obj = NULL;
	unsigned int call_id;
	cm_telephony_call_direction_e call_direction;
	char *call_number = NULL;
	char *calling_name = NULL;
	cm_telephony_call_type_e call_type = CM_TEL_CALL_TYPE_INVALID;
	cm_telephony_call_state_e call_state = CM_TEL_CALL_STATE_MAX;
	cm_telephony_name_mode_e name_mode = CM_TEL_NAME_MODE_NONE;
	gboolean is_conference = FALSE;
	gboolean is_ecc = FALSE;
	long start_time = 0;
	int person_id = -1;

	ret = _callmgr_telephony_get_call_list(core_data->telephony_handle, &call_list);
	if(-1 == ret || !call_list) {
		err("callmgr_telephony_get_call_list() failed");
		g_dbus_method_invocation_return_value (invoc, g_variant_new ("(aa{sv})", NULL));
		return -1;
	}

	g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

	while (call_list) {
		call_obj = (cm_telephony_call_data_t*)call_list->data;
		if ( !call_obj ) {
			dbg("[ error ] call object : 0");
			call_list = call_list->next;
			continue;
		}

		/*1. Fetch callId from callData*/
		_callmgr_telephony_get_call_id(call_obj, &call_id);

		/*2. Fetch callDirection from callData*/
		_callmgr_telephony_get_call_direction(call_obj, &call_direction);

		/*3. Fetch callNumber from callData*/
		_callmgr_telephony_get_call_number(call_obj, &call_number);

		/*4. Fetch callingName from callData*/
		_callmgr_telephony_get_calling_name(call_obj, &calling_name);

		/*5. Fetch callType from callData*/
		_callmgr_telephony_get_call_type(call_obj, &call_type);

		/*6. Fetch callState from callData*/
		_callmgr_telephony_get_call_state(call_obj, &call_state);

		/*7. Fetch isConference from callData*/
		_callmgr_telephony_get_is_conference(call_obj, &is_conference);

		/*8. Fetch eccCategory from callData*/
		_callmgr_telephony_get_is_ecc(call_obj, &is_ecc);

		/*9. Fetch start time from callData*/
		_callmgr_telephony_get_call_start_time(call_obj, &start_time);

		/*10. Fetch name mode from callData*/
		_callmgr_telephony_get_call_name_mode(call_obj, &name_mode);

		/*11. Fetch personId from callId*/
		_callmgr_ct_get_person_id(call_id, &person_id);

		dbg("\n\n <<<<<<<<< CallData Info in Daemon START >>>>>>>>>> \n");
		dbg("call_id                : %d, ", call_id);
		dbg("call_direction         : %d, ", call_direction);
		if (call_number) {
			dbg("call_number            : %s, ", call_number);
		}
		if (calling_name) {
			dbg("calling_name            : %s, ", calling_name);
		}
		dbg("call_type         		: %d, ", call_type);
		dbg("call_state         	: %d, ", call_state);
		dbg("is_conference          : %d, ", is_conference);
		dbg("is_ecc         		: %d, ", is_ecc);
		dbg("person_id              : %d", person_id);
		dbg("\n\n <<<<<<<<< CallData Info in Daemon END >>>>>>>>>> \n");

		g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
		g_variant_builder_add(&b, "{sv}", "call_id", g_variant_new_uint32(call_id));
		g_variant_builder_add(&b, "{sv}", "call_direction", g_variant_new_int32(call_direction));
		if (call_number) {
			g_variant_builder_add(&b, "{sv}", "call_number", g_variant_new_string(call_number));
			g_free(call_number);
		} else {
			g_variant_builder_add(&b, "{sv}", "call_number", NULL);
		}
		if (calling_name) {
			g_variant_builder_add(&b, "{sv}", "calling_name", g_variant_new_string(calling_name));
			g_free(calling_name);
		} else {
			g_variant_builder_add(&b, "{sv}", "calling_name", NULL);
		}
		g_variant_builder_add(&b, "{sv}", "call_type", g_variant_new_int32(call_type));
		g_variant_builder_add(&b, "{sv}", "call_state", g_variant_new_int32(call_state));
		g_variant_builder_add(&b, "{sv}", "is_conference", g_variant_new_boolean(is_conference));
		g_variant_builder_add(&b, "{sv}", "is_ecc", g_variant_new_boolean(is_ecc));
		g_variant_builder_add(&b, "{sv}", "person_id", g_variant_new_uint32(person_id));
		g_variant_builder_add(&b, "{sv}", "start_time", g_variant_new_int64(start_time));
		g_variant_builder_add(&b, "{sv}", "name_mode", g_variant_new_int32(name_mode));
		g_variant_builder_close(&b);

		call_list = g_slist_next(call_list);
	}
	call_list_gv = g_variant_builder_end(&b);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(@aa{sv})", call_list_gv));
	return TRUE;
}

static gboolean __get_conf_call_list_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	GVariant *call_list_gv = NULL;
	GSList *call_list = NULL;
	gboolean ret = -1;
	GVariantBuilder b;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	cm_telephony_call_data_t *call_obj = NULL;
	unsigned int call_id;
	char *call_number = NULL;
	gboolean is_conference;
	int person_id;
	cm_telephony_name_mode_e name_mode = CM_TEL_NAME_MODE_NONE;

	ret = _callmgr_telephony_get_call_list(core_data->telephony_handle, &call_list);
	if(-1 == ret || !call_list) {
		err("callmgr_telephony_get_call_list() failed");
		g_dbus_method_invocation_return_value (invoc, g_variant_new ("(aa{sv})", NULL));
		return -1;
	}

	g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

	while (call_list) {
		call_obj = (cm_telephony_call_data_t*)call_list->data;
		if ( !call_obj ) {
			dbg("[ error ] call object : 0");
			call_list = g_slist_next(call_list);
			continue;
		}

		/*1. Fetch isConference from callData*/
		_callmgr_telephony_get_is_conference(call_obj, &is_conference);
		if (is_conference == FALSE) {
			call_list = g_slist_next(call_list);
			continue;
		}

		/*2. Fetch callId from callData*/
		_callmgr_telephony_get_call_id(call_obj, &call_id);

		/*3. Fetch callNumber from callData*/
		_callmgr_telephony_get_call_number(call_obj, &call_number);

		/*4. Fetch personId from callId*/
		_callmgr_ct_get_person_id(call_id, &person_id);

		/*5. Fetch name_mode from callData*/
		_callmgr_telephony_get_call_name_mode(call_obj, &name_mode);

		dbg("\n\n <<<<<<<<< CallData Info in Daemon START >>>>>>>>>> \n");
		dbg("call_id                : %d, ", call_id);
		if (call_number) {
			dbg("call_number            : %s, ", call_number);
		}
		dbg("person_id              : %d", person_id);
		dbg("name_mode              : %d", name_mode);
		dbg("\n\n <<<<<<<<< CallData Info in Daemon END >>>>>>>>>> \n");

		g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
		g_variant_builder_add(&b, "{sv}", "call_id", g_variant_new_uint32(call_id));
		if (call_number) {
			g_variant_builder_add(&b, "{sv}", "call_number", g_variant_new_string(call_number));
			g_free(call_number);
		}
		else {
			g_variant_builder_add(&b, "{sv}", "call_number", NULL);
		}
		g_variant_builder_add(&b, "{sv}", "person_id", g_variant_new_uint32(person_id));
		g_variant_builder_add(&b, "{sv}", "name_mode", g_variant_new_uint32(name_mode));
		g_variant_builder_close(&b);

		call_list = g_slist_next(call_list);
	}
	call_list_gv = g_variant_builder_end(&b);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(@aa{sv})", call_list_gv));
	return TRUE;
}

static gboolean __get_all_call_data_handler (GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	GVariant *incom_gv = NULL, *active_dial_gv = NULL, *held_gv = NULL;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	if (core_data->incom) {
		__callmgr_dbus_get_gv_from_call_data(core_data, core_data->incom, &incom_gv);
	} else {
		incom_gv = g_variant_new ("a{sv}", NULL);
	}

	if (core_data->active_dial) {
		__callmgr_dbus_get_gv_from_call_data(core_data, core_data->active_dial, &active_dial_gv);
	}  else {
		active_dial_gv = g_variant_new ("a{sv}", NULL);
	}

	if (core_data->held) {
		__callmgr_dbus_get_gv_from_call_data(core_data, core_data->held, &held_gv);
	} else {
		held_gv = g_variant_new ("a{sv}", NULL);
	}

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new("(@a{sv}@a{sv}@a{sv})", incom_gv, active_dial_gv, held_gv));
	return TRUE;
}

static gboolean __send_dtmf_resp_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const gint resp, gpointer user_data)
{
	int ret = -1;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	ret = _callmgr_core_process_dtmf_resp(core_data, resp);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", ret));
	return TRUE;
}

static gboolean __start_dtmf_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const gchar dtmf_digit, gpointer user_data)
{
	int err = -1;
	dbg("__start_dtmf_handler() is called");
	CM_RETURN_VAL_IF_FAIL(dtmf_digit, FALSE);
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	err = _callmgr_telephony_start_dtmf(core_data->telephony_handle, dtmf_digit);

	/*out*/
	g_dbus_method_invocation_return_value(invoc, g_variant_new("(i)", err));
	return TRUE;
}

static gboolean __stop_dtmf_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__stop_dtmf_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	err = _callmgr_telephony_stop_dtmf(core_data->telephony_handle);

	/*out*/
	g_dbus_method_invocation_return_value(invoc, g_variant_new("(i)", err));
	return TRUE;
}

static gboolean __burst_dtmf_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const gchar *dtmf_digits, gpointer user_data)
{
	int err = -1;
	dbg("__burst_dtmf_handler() is called");
	CM_RETURN_VAL_IF_FAIL(dtmf_digits, FALSE);
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	err = _callmgr_telephony_burst_dtmf(core_data->telephony_handle, dtmf_digits);

	/*out*/
	g_dbus_method_invocation_return_value(invoc, g_variant_new("(i)", err));
	return TRUE;
}

static gboolean __get_audio_state_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	callmgr_path_type_e audio_state;
	dbg("__get_audio_state_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	_callmgr_core_get_audio_state(core_data, &audio_state);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", audio_state));

	return TRUE;
}

static gboolean __stop_alert_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	dbg("__stop_alert_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	int err = -1;

	err = _callmgr_core_process_stop_alert(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));

	return TRUE;
}

static gboolean __start_alert_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	dbg("__start_alert_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	int err = -1;

	err = _callmgr_core_process_start_alert(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));

	return TRUE;
}

static gboolean __activate_ui_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	dbg("__activate_ui_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	int err = -1;

	_callmgr_dbus_send_go_foreground_indi(core_data);

	/*out*/
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", err));

	return TRUE;
}

static gboolean __get_mute_status_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	dbg("__get_mute_status_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	int mute_state = -1;

	mute_state = core_data->is_mute_on;
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(i)", mute_state));

	return TRUE;
}

static gboolean __get_call_status_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	dbg("__get_call_status_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	callmgr_call_type_e call_type = CALL_TYPE_INVALID_E;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);

	/*out*/
	if (core_data->call_status == CALL_MANAGER_CALL_STATUS_RINGING_E) {
		call_type = core_data->incom->call_type;
	}
	else if (core_data->call_status == CALL_MANAGER_CALL_STATUS_OFFHOOK_E) {
		if (core_data->active_dial) {
			call_type = core_data->active_dial->call_type;
		}
		else if (core_data->held) {
			call_type = core_data->held->call_type;
		}
		else {
			err("No call data. Invalid call state");
		}
	}

	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(ii)", core_data->call_status, call_type));

	return TRUE;
}


static void __on_name_appeared (GDBusConnection *connection,
                  const gchar     *name,
                  const gchar     *name_owner,
                  gpointer         user_data)
{
	callmgr_core_data_t *core_data = user_data;
	int i = 0;
	int list_len = 0;
	callmgr_watch_client_data_t *tmp = NULL;
	list_len = g_slist_length(core_data->watch_list);
	for(i = 0; i < list_len; i++){
		tmp = (callmgr_watch_client_data_t *)g_slist_nth_data(core_data->watch_list, i);
		if(0 == g_strcmp0(name, tmp->appid)){
			tmp->is_appeared = TRUE;
			break;
		}
	}
	dbg("appid [%s] appears and client num = %d", name, list_len);
}

static void __on_name_vanished (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
	int i = 0;
	int list_len = 0;
	callmgr_watch_client_data_t *tmp = NULL;
	callmgr_core_data_t *core_data = user_data;
	dbg("..");
	list_len = g_slist_length(core_data->watch_list);
	for(i = 0; i < list_len; i++){
		tmp = (callmgr_watch_client_data_t *)g_slist_nth_data(core_data->watch_list, i);
		/*__on_name_vanished will be invoked as soon as  g_bus_watch_name is called, ignore it before __on_name_appeared invoked*/
		if(0 == g_strcmp0(name, tmp->appid) && TRUE == tmp->is_appeared){
			g_bus_unwatch_name( tmp->watch_id);
			core_data->watch_list = g_slist_remove(core_data->watch_list, tmp);
			dbg("appid [%s] exits , stop watching it, now client num = %d ", tmp->appid, g_slist_length(core_data->watch_list));
			g_free(tmp->appid);
			g_free(tmp);
			break;
		}
	}

	if (NULL == core_data->watch_list){
		dbg("end all calls");
		_callmgr_core_process_end_call(core_data, 0, CM_TEL_CALL_RELEASE_TYPE_ALL_CALLS);
	}
}

static gboolean __set_watch_name_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const gchar *appid, gpointer user_data)
{
	dbg(" __set_watch_name_handler() is called");
	int err = 0;
	int i = 0;
	gboolean is_appid_watched = FALSE;
	int list_len = 0;
	callmgr_watch_client_data_t *tmp = NULL;
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	CM_RETURN_VAL_IF_FAIL(appid, FALSE);

	list_len = g_slist_length(core_data->watch_list);
	for(i = 0; i < list_len; i++){
		tmp = (callmgr_watch_client_data_t *)g_slist_nth_data(core_data->watch_list, i);
		if (0 == g_strcmp0(appid, tmp->appid) ){
			dbg("appid[%s] is already being watched now", appid);
			is_appid_watched = TRUE;
			break;
		}
	}

	if(FALSE == is_appid_watched){
		dbg(" __set_watch_name[%s]", appid);
		tmp = (callmgr_watch_client_data_t *)calloc(1, sizeof(callmgr_watch_client_data_t));
		CM_RETURN_VAL_IF_FAIL(tmp, FALSE);
		/*__on_name_vanished will be invoked as soon as  g_bus_watch_name is called, ignore it before __on_name_appeared invoked*/
		tmp->is_appeared = FALSE;
		tmp->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM ,
						appid,
						G_BUS_NAME_WATCHER_FLAGS_NONE ,
						__on_name_appeared,
						__on_name_vanished,
						core_data,
						NULL);
		if (tmp->watch_id == 0){
			g_free(tmp);
			err = -1;
		}
		else{
			tmp->appid = g_strdup(appid);
			core_data->watch_list = g_slist_append(core_data->watch_list, tmp);
		}
	}

	/*out*/
	g_dbus_method_invocation_return_value(invoc, g_variant_new("(i)", err));
	return TRUE;
}

static gboolean __start_voice_record_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, const gchar *call_num, gpointer user_data)
{
	int err = -1;
	dbg("__start_voice_record_handler() is called");
	CM_RETURN_VAL_IF_FAIL(call_num, FALSE);
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	err = _callmgr_core_process_record_start(core_data, call_num, FALSE);

	/*out*/
	g_dbus_method_invocation_return_value(invoc, g_variant_new("(i)", err));
	return TRUE;
}

static gboolean __stop_voice_record_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	int err = -1;
	dbg("__stop_voice_record_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	err = _callmgr_core_process_record_stop(core_data);

	/*out*/
	g_dbus_method_invocation_return_value(invoc, g_variant_new("(i)", err));
	return TRUE;
}

static gboolean __get_answering_machine_status_handler(GDBusInterfaceSkeleton *di,
		GDBusMethodInvocation *invoc, gpointer user_data)
{
	dbg("__get_answering_machine_status_handler() is called");
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_VAL_IF_FAIL(core_data, FALSE);
	gboolean is_answering_machine_on = FALSE;
	gboolean is_bike_mode_enabled = FALSE;

	_callmgr_vconf_is_bike_mode_enabled(&is_bike_mode_enabled);

	if (core_data->is_auto_answered && is_bike_mode_enabled) {
		is_answering_machine_on = TRUE;
	} else {
		is_answering_machine_on = FALSE;
	}
	g_dbus_method_invocation_return_value (invoc, g_variant_new ("(b)", is_answering_machine_on));

	return TRUE;
}

/********************************************/
static gchar *__callmgr_dbus_convert_name_to_path (const gchar *name)
{
	int len;
	int i;
	int j = 0;
	gchar *buf;
	char hex[3];

	if (!name)
		return NULL;

	len = strlen (name);
	/* test-name -> test_2D_name */
	buf = g_malloc0 (len * 4);
	if (!buf)
		return NULL;

	for (i = 0; i < len; i++) {
		if (g_ascii_isalnum (name[i]) || name[i] == '_' || name[i] == '/') {
			buf[j] = name[i];
			j++;
		}
		else {
			snprintf (hex, 3, "%02X", name[i]);
			buf[j++] = '_';
			buf[j++] = hex[0];
			buf[j++] = hex[1];
			buf[j++] = '_';
		}
	}

	return buf;
}


static int __callmgr_dbus_object_export (callmgr_core_data_t *core_data, GDBusInterfaceSkeleton *di, const gchar *path)
{
	GError *error = NULL;
	gboolean ret;
	gchar *real_path = NULL;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	if (!di || !path)
		return -1;

	real_path = __callmgr_dbus_convert_name_to_path (path);

	if (g_variant_is_object_path (real_path) == FALSE) {
		err(	"invalid name('%s'). (must only contain [A-Z][a-z][0-9]_)", real_path);
		g_free (real_path);
		return -1;
	}

	ret = g_dbus_interface_skeleton_export (di, core_data->dbus_conn, real_path, &error);
	if (ret == FALSE) {
		if (error) {
			err("error: %s", error->message);
			g_error_free (error);
		}
		else {
			err("export failed");
		}

		g_free (real_path);
		return -1;
	}

	g_free (real_path);

	return 0;
}

static int __callmgr_dbus_init_handlers (callmgr_core_data_t *core_data)
{
	GDBusInterfaceSkeleton *di;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	di = G_DBUS_INTERFACE_SKELETON (callmgr_org_tizen_callmgr_skeleton_new ());

	/*handlers*/
	g_signal_connect(di, "handle-dial-call", G_CALLBACK(__dial_call_handler), core_data);
	g_signal_connect(di, "handle-end-call", G_CALLBACK(__end_call_handler), core_data);
	g_signal_connect(di, "handle-reject-call", G_CALLBACK(__reject_call_handler), core_data);
	g_signal_connect(di, "handle-swap-call", G_CALLBACK(__swap_call_handler), core_data);
	g_signal_connect(di, "handle-hold-call", G_CALLBACK(__hold_call_handler), core_data);
	g_signal_connect(di, "handle-unhold-call", G_CALLBACK(__unhold_call_handler), core_data);
	g_signal_connect(di, "handle-join-call", G_CALLBACK(__join_call_handler), core_data);
	g_signal_connect(di, "handle-split-call", G_CALLBACK(__split_call_handler), core_data);
	g_signal_connect(di, "handle-transfer-call", G_CALLBACK(__transfer_call_handler), core_data);
	g_signal_connect(di, "handle-answer-call", G_CALLBACK(__answer_call_handler), core_data);
	g_signal_connect(di, "handle-get-call-list", G_CALLBACK(__get_call_list_handler), core_data);
	g_signal_connect(di, "handle-get-conf-call-list", G_CALLBACK(__get_conf_call_list_handler), core_data);
	g_signal_connect(di, "handle-set-extra-vol", G_CALLBACK(__set_extra_vol_handler), core_data);
	g_signal_connect(di, "handle-set-noise-reduction", G_CALLBACK(__set_noise_reduction_handler), core_data);
	g_signal_connect(di, "handle-set-mute-state", G_CALLBACK(__set_mute_state_handler), core_data);
	g_signal_connect(di, "handle-spk-on", G_CALLBACK(__spk_on_handler), core_data);
	g_signal_connect(di, "handle-spk-off", G_CALLBACK(__spk_off_handler), core_data);
	g_signal_connect(di, "handle-bluetooth-on", G_CALLBACK(__bluetooth_on_handler), core_data);
	g_signal_connect(di, "handle-bluetooth-off", G_CALLBACK(__bluetooth_off_handler), core_data);
	g_signal_connect(di, "handle-get-all-call-data", G_CALLBACK(__get_all_call_data_handler), core_data);
	g_signal_connect(di, "handle-send-dtmf-resp", G_CALLBACK(__send_dtmf_resp_handler), core_data);
	g_signal_connect(di, "handle-start-dtmf", G_CALLBACK(__start_dtmf_handler), core_data);
	g_signal_connect(di, "handle-stop-dtmf", G_CALLBACK(__stop_dtmf_handler), core_data);
	g_signal_connect(di, "handle-burst-dtmf", G_CALLBACK(__burst_dtmf_handler), core_data);
	g_signal_connect(di, "handle-get-audio-state", G_CALLBACK(__get_audio_state_handler), core_data);
	g_signal_connect(di, "handle-stop-alert", G_CALLBACK(__stop_alert_handler), core_data);
	g_signal_connect(di, "handle-start-alert", G_CALLBACK(__start_alert_handler), core_data);
	g_signal_connect(di, "handle-activate-ui", G_CALLBACK(__activate_ui_handler), core_data);
	g_signal_connect(di, "handle-get-call-status", G_CALLBACK(__get_call_status_handler), core_data);
	g_signal_connect(di, "handle-set-watch-name", G_CALLBACK(__set_watch_name_handler), core_data);
	g_signal_connect(di, "handle-start-voice-record", G_CALLBACK(__start_voice_record_handler), core_data);
	g_signal_connect(di, "handle-stop-voice-record", G_CALLBACK(__stop_voice_record_handler), core_data);
	g_signal_connect(di, "handle-get-mute-status", G_CALLBACK(__get_mute_status_handler), core_data);
	g_signal_connect(di, "handle-get-answering-machine-status", G_CALLBACK(__get_answering_machine_status_handler), core_data);

	__callmgr_dbus_object_export(core_data, di, CALLMGR_DBUS_PATH);
	core_data->dbus_skeleton_interface = di;

	return 0;
}

// TODO:
// need to add prefix
static void __on_bus_acquired (GDBusConnection *conn, const gchar *name,
		gpointer user_data)
{
	callmgr_core_data_t *core_data = (callmgr_core_data_t *)user_data;
	CM_RETURN_IF_FAIL(core_data);

	core_data->dbus_conn = conn;

	dbg("acquired");
	__callmgr_dbus_init_handlers(core_data);

	g_dbus_object_manager_server_set_connection (core_data->gdoms, conn);
}

int _callmgr_dbus_init (callmgr_core_data_t *core_data)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	guint id;

	core_data->gdoms = g_dbus_object_manager_server_new (CALLMGR_DBUS_PATH);
	if (!core_data->gdoms)
		return -1;

	id = g_bus_own_name (G_BUS_TYPE_SYSTEM, CALLMGR_DBUS_SERVICE,
			G_BUS_NAME_OWNER_FLAGS_REPLACE, __on_bus_acquired, NULL, NULL,
			core_data, NULL );
	if (id == 0) {
		err("g_bus_own_name() failed");
		return -1;
	}

	core_data->watch_list = NULL;

	return 0;
}


int _callmgr_dbus_deinit(callmgr_core_data_t *core_data)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	int list_len = 0;
	int idx = 0;
	callmgr_watch_client_data_t *tmp = NULL;

	list_len = g_slist_length(core_data->watch_list);
	for (idx = 0; idx < list_len; idx++) {
		tmp = (callmgr_watch_client_data_t *)g_slist_nth_data(core_data->watch_list, idx);
		g_free(tmp->appid);
		tmp->appid = NULL;
		g_free(tmp);
		tmp = NULL;
	}
	g_slist_free(core_data->watch_list);
	core_data->watch_list = NULL;

	g_object_unref(core_data->gdoms);

	return 0;
}

static int __callmgr_dbus_get_gv_from_call_data(callmgr_core_data_t *core_data, callmgr_call_data_t *call_data, GVariant **gv)
{
	dbg("__callmgr_dbus_get_gv_from_call_data");
	CM_RETURN_VAL_IF_FAIL(call_data, -1);
	CM_RETURN_VAL_IF_FAIL(gv, -1);
	GVariantBuilder b;
	int call_member_cnt = 0;
	int person_id = -1;;
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	CM_RETURN_VAL_IF_FAIL(core_data->telephony_handle, -1);

	/*Fetch personId by call ID*/
	_callmgr_ct_get_person_id(call_data->call_id, &person_id);

	dbg("\n\n <<<<<<<<< CallData Info in Daemon START >>>>>>>>>> \n");
	dbg("call_id                : %d, ", call_data->call_id);
	dbg("call_direction         : %d, ", call_data->call_direction);
	if (call_data->call_number) {
		dbg("call_number            : %s, ", call_data->call_number);
	}
	if (call_data->calling_name) {
		dbg("calling_name            : %s, ", call_data->calling_name);
	}
	dbg("call_type         		: %d, ", call_data->call_type);
	dbg("call_state         	: %d, ", call_data->call_state);
	dbg("is_conference          : %d, ", call_data->is_conference);
	dbg("is_ecc         		: %d, ", call_data->is_ecc);
	dbg("is_voicemail_number    : %d, ", call_data->is_voicemail_number);
	dbg("person_id         		: %d", person_id);
	dbg("start time			: %ld", call_data->start_time);
	dbg("\n\n <<<<<<<<< CallData Info in Daemon END >>>>>>>>>> \n");

	g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(&b, "{sv}", "call_id", g_variant_new_uint32(call_data->call_id));
	g_variant_builder_add(&b, "{sv}", "call_direction", g_variant_new_int32(call_data->call_direction));
	if (call_data->call_number) {
		g_variant_builder_add(&b, "{sv}", "call_number", g_variant_new_string(call_data->call_number));
	}
	if (call_data->calling_name) {
		g_variant_builder_add(&b, "{sv}", "calling_name", g_variant_new_string(call_data->calling_name));
	}
	g_variant_builder_add(&b, "{sv}", "call_type", g_variant_new_int32(call_data->call_type));
	g_variant_builder_add(&b, "{sv}", "call_state", g_variant_new_int32(call_data->call_state));
	if (call_data->is_conference) {
		_callmgr_telephony_get_conference_call_count(core_data->telephony_handle, &call_member_cnt );
	}
	else {		// Single call
		call_member_cnt = 1;
	}
	g_variant_builder_add(&b, "{sv}", "member_count", g_variant_new_int32(call_member_cnt));
	g_variant_builder_add(&b, "{sv}", "is_ecc", g_variant_new_boolean(call_data->is_ecc));
	g_variant_builder_add(&b, "{sv}", "is_voicemail_number", g_variant_new_boolean(call_data->is_voicemail_number));
	g_variant_builder_add(&b, "{sv}", "person_id", g_variant_new_uint32(person_id));
	g_variant_builder_add(&b, "{sv}", "start_time", g_variant_new_int64(call_data->start_time));
	g_variant_builder_add(&b, "{sv}", "name_mode", g_variant_new_int32(call_data->name_mode));
	*gv = g_variant_builder_end(&b);

	return 0;
}

int _callmgr_dbus_send_call_event(callmgr_core_data_t *core_data, int call_event, unsigned int call_id, callmgr_sim_slot_type_e sim_slot, int end_cause)
{
	GVariant *incom_gv = NULL, *active_dial_gv = NULL, *held_gv = NULL;

	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	if (core_data->incom) {
		__callmgr_dbus_get_gv_from_call_data(core_data, core_data->incom, &incom_gv);
	} else {
		incom_gv = g_variant_new ("a{sv}", NULL);
	}

	if (core_data->active_dial) {
		__callmgr_dbus_get_gv_from_call_data(core_data, core_data->active_dial, &active_dial_gv);
	}  else {
		active_dial_gv = g_variant_new ("a{sv}", NULL);
	}

	if (core_data->held) {
		__callmgr_dbus_get_gv_from_call_data(core_data, core_data->held, &held_gv);
	} else {
		held_gv = g_variant_new ("a{sv}", NULL);
	}

	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "call-event", call_event, call_id, (int)sim_slot, (int)end_cause, incom_gv, active_dial_gv, held_gv);
	return 0;
}

int _callmgr_dbus_send_call_status(callmgr_core_data_t *core_data, int call_status, int call_type, const char *call_num)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "call-status", call_status, call_type, call_num);
	return 0;
}

int _callmgr_dbus_send_mute_status(callmgr_core_data_t *core_data, int mute_status)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "mute-status", mute_status);
	return 0;
}


int _callmgr_dbus_send_dial_status(callmgr_core_data_t *core_data, callmgr_dial_status_e dial_status)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "dial-status", dial_status);

	return 0;
}

int _callmgr_dbus_send_dtmf_indi(callmgr_core_data_t *core_data, callmgr_dtmf_indi_type_e indi_type, char *dtmf_number)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	if ((indi_type != CALL_DTMF_INDI_IDLE_E) && (dtmf_number == NULL)) {
		err("dtmf_number is NULL");
		return -1;
	}

	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "dtmf-indi", indi_type, dtmf_number);

	return 0;
}

int _callmgr_dbus_send_audio_status(callmgr_core_data_t *core_data, callmgr_path_type_e audio_path)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);
	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "audio-state", audio_path);

	return 0;
}

int _callmgr_dbus_send_go_foreground_indi(callmgr_core_data_t *core_data)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "go-foreground");

	return 0;
}

int _callmgr_dbus_send_vr_status(callmgr_core_data_t *core_data, int vr_status, int extra_type)
{
	CM_RETURN_VAL_IF_FAIL(core_data, -1);

	g_signal_emit_by_name (core_data->dbus_skeleton_interface, "voice-record-status", vr_status, extra_type);
	return 0;
}
