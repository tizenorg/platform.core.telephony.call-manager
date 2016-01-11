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
#include <aul.h>
#include <app_control.h>
#include <appsvc.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <metadata_extractor.h>
#include <app_manager.h>
#include <glib.h>
#include <notification.h>
#include <notification_internal.h>
#include <notification_setting_internal.h>
#include <msg_types.h>
#include <msg.h>
#include <cynara-creds-gdbus.h>
#include <cynara-session.h>

#include "callmgr-util.h"
#include "callmgr-log.h"
#include "callmgr-vconf.h"

#define CALLUI_PKG_NAME "org.tizen.call-ui"
#define SOUND_PATH_SILENT	"silent"
#define CALLMGR_POPUP_NAME "org.tizen.callmgr-popup"
#define CALLMGR_CLI_SHOW_ID "*31#"
#define CALLMGR_CLI_HIDE_ID "#31#"
#define CALLMGR_PHONE_NUMBER_LENGTH_MAX 82

#define PERM_WRITE	"w"
#define PERM_EXECUTE	"x"
#define TELEPHONY_PRIVILEGE		"http://tizen.org/privilege/telephony"
#define TELEPHONY_ADMIN_PRIVILEGE	"http://tizen.org/privilege/telephony.admin"

#define IS_DIGIT(value)		((value) >= '0' && (value) <= '9')

#define CISS_AUL_CMD "org.tizen.ciss"
#define CISS_MODE_OPT "REQ"

typedef struct _callmgr_thread_data {
	unsigned int call_id;
	int sim_slot;
} callmgr_thread_data_t;

typedef struct _callmgr_util_popup_data {
	call_popup_type popup_type;
	callmgr_util_popup_result_cb popup_result_cb;
	void *user_data;
} _callmgr_util_popup_data_t;

gboolean _callmgr_util_check_access_control(cynara *p_cynara, GDBusMethodInvocation *invoc, const char *label, const char *perm)
{
	GDBusConnection *conn;
	const char *sender_unique_name;
	pid_t pid;
	int ret;
	int result = FALSE;
	/* For cynara */
	gchar *client_smack = NULL;
	char *client_session = NULL;
	gchar *uid_string = NULL;
	const char *privilege = NULL;

	if (!p_cynara) {
		warn("access control denied (fail to get cynara handle)");
		goto OUT;
	}

	conn = g_dbus_method_invocation_get_connection(invoc);
	if (!conn) {
		warn("access control denied (no connection info)");
		goto OUT;
	}

	sender_unique_name = g_dbus_method_invocation_get_sender(invoc);

	/* Get PID */
	ret = cynara_creds_gdbus_get_pid(conn, sender_unique_name, &pid);
	if (ret != CYNARA_API_SUCCESS) {
		warn("access control denied (fail to get pid). ret = %d", ret);
		goto OUT;
	}

	/* Get UID */
	ret = cynara_creds_gdbus_get_user(conn, sender_unique_name, USER_METHOD_DEFAULT, &uid_string);
	if (ret != CYNARA_API_SUCCESS) {
		warn("access control denied (fail to get uid for cynara). ret = %d", ret);
		goto OUT;
	}

	/* Get Smack label */
	ret = cynara_creds_gdbus_get_client(conn, sender_unique_name, CLIENT_METHOD_DEFAULT, &client_smack);
	if (ret != CYNARA_API_SUCCESS) {
		warn("access control denied (fail to get smack for cynara). ret = %d", ret);
		goto OUT;
	}

	dbg("sender: %s pid = %u uid = %s smack = %s", sender_unique_name, pid, uid_string, client_smack);

	client_session = cynara_session_from_pid(pid);
	if (!client_session) {
		warn("access control denied (fail to get cynara client session)");
		goto OUT;
	}

	if (g_strrstr(perm, PERM_WRITE) == NULL && g_strrstr(perm, PERM_EXECUTE) == NULL)
		privilege = TELEPHONY_PRIVILEGE;
	else
		privilege = TELEPHONY_ADMIN_PRIVILEGE;

	ret = cynara_check(p_cynara, client_smack, client_session, uid_string, privilege);
	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		warn("pid(%u) access (%s - %s) denied(%d)", pid, label, perm, ret);
	} else {
		result = TRUE;
	}
OUT:
	if (result == FALSE) {
		g_dbus_method_invocation_return_error(invoc,
				G_DBUS_ERROR,
				G_DBUS_ERROR_ACCESS_DENIED,
				"No access rights");
	}
	free(client_session);
	g_free(client_smack);
	g_free(uid_string);

	return result;
}

static gboolean __callmgr_util_thread_finish_cb(gpointer thread_data)
{
	dbg("Thread %p return is complete", thread_data);

	g_thread_join(thread_data);

	dbg("Clean up of thread %p is complete", thread_data);

	return FALSE;
}

static gboolean __callmgr_util_thread_dispatch(GMainContext *main_context, gint priority, GSourceFunc cb, gpointer data)
{
	GSource *request_source = NULL;

	if (main_context == NULL || cb == NULL) {
		err("Failed to dispatch");
		return FALSE;
	}

	request_source = g_idle_source_new();
	g_source_set_callback(request_source, cb, data, NULL);
	g_source_set_priority(request_source, priority);
	g_source_attach(request_source, main_context);
	g_source_unref(request_source);

	return TRUE;
}

static gboolean __callmgr_util_dispatch_job_on_new_thread(gchar *name, GThreadFunc thread_cb, gpointer thread_data)
{
	GThread *thread;
	if (!name || !thread_cb) {
		err("Wrong Input Parameter");
		return FALSE;
	}
	thread = g_thread_new(name, thread_cb, thread_data);
	if (thread == NULL) {
		return FALSE;
	} else {
		dbg("Thread %p is created for %s", thread, name);
	}

	return TRUE;
}

static gpointer __callmgr_util_launch_voice_call(gpointer data)
{
	GThread* selfi = g_thread_self();

	dbg("__callmgr_util_launch_voice_call");
	callmgr_thread_data_t *cb_data = (callmgr_thread_data_t*)data;
	CM_RETURN_VAL_IF_FAIL(cb_data, NULL);
	char buf[2] = {0, };
	bundle *kb	= NULL;

	kb = bundle_create();

	appsvc_set_operation(kb, APPSVC_OPERATION_CALL);
	appsvc_set_uri(kb,"tel:MT");

	g_snprintf(buf, 2, "%d", cb_data->call_id);
	dbg("call_id : [%s]", buf);
	appsvc_add_data(kb, "handle", buf);

	g_snprintf(buf, 2, "%d", cb_data->sim_slot);
	dbg("sim_slot : [%s]", buf);
	appsvc_add_data(kb, "sim_slot", buf);
	appsvc_run_service(kb, 0, NULL, NULL);

	bundle_free(kb);
	g_free(cb_data);

	if (TRUE == __callmgr_util_thread_dispatch(g_main_context_default(), G_PRIORITY_LOW, (GSourceFunc)__callmgr_util_thread_finish_cb, selfi)) {
		dbg("Thread %p processing is complete", selfi);
	}

	return NULL;
}

static gpointer __callmgr_util_launch_voice_call_by_sat(gpointer data)
{
	GThread* selfi = g_thread_self();

	dbg("__callmgr_util_launch_voice_call");
	callmgr_thread_data_t *cb_data = (callmgr_thread_data_t*)data;
	CM_RETURN_VAL_IF_FAIL(cb_data, NULL);
	char buf[500 + 1] = {0, };
	bundle *kb	= NULL;

	kb = bundle_create();

	appsvc_set_operation(kb, APPSVC_OPERATION_CALL);
	appsvc_set_uri(kb, "tel:SAT");

	g_snprintf(buf, 2, "%d", cb_data->sim_slot);
	dbg("sim_slot : [%s]", buf);
	appsvc_add_data(kb, "sim_slot", buf);
	appsvc_run_service(kb, 0, NULL, NULL);

	bundle_free(kb);
	g_free(cb_data);

	if (TRUE == __callmgr_util_thread_dispatch(g_main_context_default(), G_PRIORITY_LOW, (GSourceFunc)__callmgr_util_thread_finish_cb, selfi)) {
		dbg("Thread %p processing is complete", selfi);
	}

	return NULL;
}


static gpointer __callmgr_util_launch_video_call(gpointer data)
{
	GThread* selfi = g_thread_self();

	dbg("__callmgr_util_launch_video_call");
	callmgr_thread_data_t *cb_data = (callmgr_thread_data_t*)data;
	CM_RETURN_VAL_IF_FAIL(cb_data, NULL);
	char buf[2] = {0, };
	bundle *kb	= NULL;

	kb = bundle_create();
/*
 * This operation will work when VT stack and VT UI are support
	appsvc_set_operation(kb, AUL_SVC_OPERATION_VTCALL);
*/
	appsvc_set_uri(kb, "tel:MT");


	g_snprintf(buf, 2, "%d", cb_data->call_id);
	dbg("call_id : [%s]", buf);
	appsvc_add_data(kb, "handle", buf);

	g_snprintf(buf, 2, "%d", cb_data->sim_slot);
	dbg("sim_slot : [%s]", buf);
	appsvc_add_data(kb, "sim_slot", buf);
	appsvc_run_service(kb, 0, NULL, NULL);

	bundle_free(kb);
	g_free(cb_data);

	if (TRUE == __callmgr_util_thread_dispatch(g_main_context_default(), G_PRIORITY_LOW, (GSourceFunc)__callmgr_util_thread_finish_cb, selfi)) {
		dbg("Thread %p processing is complete", selfi);
	}

	return NULL;
}

int _callmgr_util_is_ringtone_playable(char *ringtone_path, gboolean *is_playable)
{
	int err = METADATA_EXTRACTOR_ERROR_NONE;
	metadata_extractor_h metadata = NULL;
	char *value = NULL;

	CM_RETURN_VAL_IF_FAIL(ringtone_path, -1);

	dbg("_callmgr_util_is_ringtone_playable()");

	if (g_file_test(ringtone_path, G_FILE_TEST_EXISTS) == FALSE) {
		warn("File missing");
		*is_playable = FALSE;
		return -1;
	}

	if (metadata_extractor_create(&metadata) != METADATA_EXTRACTOR_ERROR_NONE) {
		err("metadata_extractor_create() failed: ret(%d)", err);
		return -1;
	}

	err = metadata_extractor_set_path(metadata, ringtone_path);
	if (err != METADATA_EXTRACTOR_ERROR_NONE) {
		err("metadata_extractor_set_path() failed: ret(%d)", err);
	}

	err = metadata_extractor_get_metadata(metadata, METADATA_HAS_AUDIO, &value);
	if (err != METADATA_EXTRACTOR_ERROR_NONE) {
		err("metadata_extractor_get_metadata() failed: ret(%d)", err);
	}

	if (value && g_strcmp0(value, "0")) {
		*is_playable = TRUE;
	} else {
		*is_playable = FALSE;
	}

	CM_SAFE_FREE(value);

	if (metadata) {
		metadata_extractor_destroy(metadata);
	}

	return 0;
}

int _callmgr_util_is_silent_ringtone(char *ringtone_path, gboolean *is_silent_ringtone)
{
	CM_RETURN_VAL_IF_FAIL(ringtone_path, -1);

	dbg("_callmgr_util_is_ringtone_playable()");

	if ((strlen(ringtone_path) == strlen(SOUND_PATH_SILENT)) && ((strncmp(ringtone_path, SOUND_PATH_SILENT, strlen(SOUND_PATH_SILENT)) == 0))) {
		*is_silent_ringtone = TRUE;
	} else {
		*is_silent_ringtone = FALSE;
	}
	return 0;
}

int _callmgr_util_is_callui_running(gboolean *is_callui_running)
{
	bool running = FALSE;

	dbg("_callmgr_util_is_callui_running");

	app_manager_is_running(CALLUI_PKG_NAME, &running);
	if (running) {
		dbg("call app is already running");
		*is_callui_running = TRUE;
	} else {
		info("call app is not running");
		*is_callui_running = FALSE;
	}
	return 0;
}

int _callmgr_util_launch_callui(gint call_id, int sim_slot, int call_type)
{
	dbg("_callmgr_util_launch_callui");

	callmgr_thread_data_t *cb_data = g_try_malloc0(sizeof(callmgr_thread_data_t));
	if (!cb_data) {
		err("Memory allocation failed");

		return -1;
	}

	cb_data->call_id = call_id;
	cb_data->sim_slot = sim_slot;

	if (CM_UTIL_CALL_TYPE_VOICE_E == call_type) {
		if (FALSE == __callmgr_util_dispatch_job_on_new_thread("Voice Call", __callmgr_util_launch_voice_call, cb_data)) {
			err("Failed to launch Voice Call App");
			g_free(cb_data);
		}
	} else {
		if (FALSE == __callmgr_util_dispatch_job_on_new_thread("Video Call", __callmgr_util_launch_video_call, cb_data)) {
			err("Failed to launch Video Call App");
			g_free(cb_data);
		}
	}

	return 0;
}

int _callmgr_util_launch_callui_by_sat(int sim_slot)
{
	dbg("_callmgr_util_launch_callui");

	callmgr_thread_data_t *cb_data = g_try_malloc0(sizeof(callmgr_thread_data_t));
	if (!cb_data) {
		err("Memory allocation failed");

		return -1;
	}

	cb_data->sim_slot = sim_slot;

	if (FALSE == __callmgr_util_dispatch_job_on_new_thread("Voice Call", __callmgr_util_launch_voice_call_by_sat, cb_data)) {
		err("Failed to launch Voice Call App");
		g_free(cb_data);
	}

	return 0;
}

static void __callmgr_util_popup_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	dbg("__callmgr_util_popup_reply_cb");
	int ret = -1;
	_callmgr_util_popup_data_t *popup_data = (_callmgr_util_popup_data_t *)user_data;
	if (popup_data) {
		if (result == APP_CONTROL_RESULT_SUCCEEDED) {
			char *ext_data = NULL;
			cm_util_popup_result_e popup_result = CM_UTIL_POPUP_RESP_CANCEL;

			ret = app_control_get_extra_data(reply, "RESULT", &ext_data);
			if (APP_CONTROL_ERROR_NONE == ret) {
				if (ext_data) {
					info("popup result: [%s]", ext_data);
					popup_result = atoi(ext_data);
					g_free(ext_data);
					popup_data->popup_result_cb(popup_data->popup_type, (void*)popup_result, popup_data->user_data);
				}
			} else {
				err("app_control_get_extra_data failed");
			}
		}
		g_free(popup_data);
	}
	return;
}

int _callmgr_util_launch_popup(call_popup_type popup_type, int info, const char* number, int active_sim, callmgr_util_popup_result_cb cb, void *user_data)
{
	dbg("_callmgr_util_launch_popup: popup_type(%d), info(%d)", popup_type, info);
	char type_buf[10] = { 0, };
	char err_buf[10] = { 0, };
	char sim_buf[10] = { 0, };
	_callmgr_util_popup_data_t *popup_data = NULL;
	int result = APP_CONTROL_ERROR_NONE;

	app_control_h app_control = NULL;
	result = app_control_create(&app_control);
	if (result != APP_CONTROL_ERROR_NONE) {
		warn("app_control_create() return error : %d", result);
		return -1;
	}
	app_control_set_app_id(app_control, CALLMGR_POPUP_NAME);

	snprintf(type_buf, sizeof(type_buf), "%d", popup_type);
	app_control_add_extra_data(app_control, "TYPE", type_buf);

	snprintf(err_buf, sizeof(err_buf), "%d", info);
	app_control_add_extra_data(app_control, "SUB_INFO", err_buf);

	app_control_add_extra_data(app_control, "NUMBER", number);

	snprintf(sim_buf, sizeof(sim_buf), "%d", active_sim);
	app_control_add_extra_data(app_control, "ACTIVE_SIM", sim_buf);

	if (cb) {
		popup_data = (_callmgr_util_popup_data_t*)calloc(1, sizeof(_callmgr_util_popup_data_t));
		if (popup_data == NULL) {
			app_control_destroy(app_control);
			return -1;
		}
		popup_data->popup_type = popup_type;
		popup_data->popup_result_cb = cb;
		popup_data->user_data = user_data;
	}

	result = app_control_send_launch_request(app_control, __callmgr_util_popup_reply_cb, popup_data);
	if (result != APP_CONTROL_ERROR_NONE) {
		warn("Retry");
		result = app_control_send_launch_request(app_control, __callmgr_util_popup_reply_cb, popup_data);
		err("retry : %d", result);
	}
	app_control_destroy(app_control);

	return 0;
}

int _callmgr_util_get_time_diff(long start_time, int *call_duration_in_sec)
{
	struct sysinfo curr_time_info;
	CM_RETURN_VAL_IF_FAIL(call_duration_in_sec, -1);

	if (start_time != 0) {
		if (sysinfo(&curr_time_info) == 0) {
			info("uptime: %ld secs", curr_time_info.uptime);
			*call_duration_in_sec = curr_time_info.uptime - start_time;
			return 0;
		}
	} else {
		*call_duration_in_sec = 0;
		return 0;
	}
	return -1;
}

int _callmgr_util_is_incall_ss_string(const char *number, gboolean *o_is_incall_ss)
{
	int len = 0;
	int num_int = 0;
	CM_RETURN_VAL_IF_FAIL(number, -1);
	CM_RETURN_VAL_IF_FAIL(o_is_incall_ss, -1);

	sec_dbg("number(%s)", number);

	*o_is_incall_ss = FALSE;

	len = strlen(number);
	if (len > 2 || len < 1) {
		*o_is_incall_ss = FALSE;
	} else if (number[0] > '6') {
		*o_is_incall_ss = FALSE;
	} else if (len == 1) {
		/* 0 ~ 4 */
		if (number[0] >= '0' && number[0] <= '4') {
			*o_is_incall_ss = TRUE;
		}
	} else {
		/* 11 ~ 17, 21 ~ 27 */
		num_int = atoi(number);

		if (num_int >= 11 && num_int <= 17) {
			*o_is_incall_ss = TRUE;
		} else if (num_int >= 21 && num_int <= 27)
			*o_is_incall_ss = TRUE;
	}

	return 0;
}

int _callmgr_util_remove_invalid_chars_from_call_number(const char *src, char **dst)
{
	CM_RETURN_VAL_IF_FAIL(src, -1);
	CM_RETURN_VAL_IF_FAIL(dst, -1);
	int i = 0;
	int j = 0;
	int nSrc = 0;
	char *tmpNum = NULL;

	nSrc = strlen(src);
	tmpNum = (char *)calloc(nSrc+1, sizeof(char));
	CM_RETURN_VAL_IF_FAIL(tmpNum, -1);
	for (i = 0; i < nSrc; ++i) {
		switch (src[i]) {
		case '(':
		case ')':
		case '-':
		case ' ':
		case '/':
			break;
		case 'N':
			tmpNum[j++] = '6';
			break;
		default:
			tmpNum[j++] = src[i];
			break;
		}
	}

	tmpNum[j] = '\0';
	*dst = g_strdup(tmpNum);

	g_free(tmpNum);

	return 0;
}

static int __callmgr_util_is_valid_dtmf_number(char *dtmf_number, gboolean *is_valid)
{
	int len = 0;
	CM_RETURN_VAL_IF_FAIL(dtmf_number, -1);

	len = strlen(dtmf_number);
	dbg("dtmf length = %d", len);
	while (len > 0) {
		if (!(IS_DIGIT(dtmf_number[len - 1]) || (dtmf_number[len - 1] >= 'A' && dtmf_number[len - 1] <= 'D') || \
				(dtmf_number[len - 1] == '*' || dtmf_number[len - 1] == '#') || \
				(dtmf_number[len - 1] == 'P' || dtmf_number[len - 1] == 'p' || dtmf_number[len - 1] == ',') || \
				(dtmf_number[len - 1] == 'T' || dtmf_number[len - 1] == 't') || \
				(dtmf_number[len - 1] == 'W' || dtmf_number[len - 1] == 'w' || dtmf_number[len - 1] == ';'))) {
			err("invalid character encountered...");
			*is_valid = FALSE;
			return 0;
		}
		len--;
	}
	dbg("Fully valid DTMF string.. [%s]", dtmf_number);
	*is_valid = TRUE;
	return 0;
}

int _callmgr_util_extract_call_number(const char *orig_num, char **call_num, char **dtmf_num)
{
	char *pst = NULL;
	char *tmp = NULL;
	char *dup_orig_num = NULL;
	int i, size;
	char *ptr = NULL;

	CM_RETURN_VAL_IF_FAIL(orig_num, -1);
	CM_RETURN_VAL_IF_FAIL(call_num, -1);
	CM_RETURN_VAL_IF_FAIL(dtmf_num, -1);

	dup_orig_num = g_strdup(orig_num);
	CM_RETURN_VAL_IF_FAIL(dup_orig_num, -1);

	if ((!strncmp(dup_orig_num, CALLMGR_CLI_SHOW_ID, 4)) || (!strncmp(dup_orig_num, CALLMGR_CLI_HIDE_ID, 4)))
		pst = (char *)dup_orig_num + 4;
	else
		pst = (char *)dup_orig_num;

	*dtmf_num = NULL;
	size = strlen(pst);
	for (i = 0; i < size; i++) {
		if (pst[i] == 'P' || pst[i] == 'p' || pst[i] == ',' || pst[i] == 'W' || pst[i] == 'w' || pst[i] == ';') {
			if ((strlen(&pst[i]) > 0) && (&pst[i] != dup_orig_num)) {
				gboolean is_valid_dtmf = FALSE;
				__callmgr_util_is_valid_dtmf_number(&pst[i], &is_valid_dtmf);
				if (is_valid_dtmf) {
					*dtmf_num = g_strdup(&pst[i]);
				}
			}

			if (pst[i] == 'P') tmp = strtok_r(pst, "P", &ptr);
			else if (pst[i] == 'p') tmp = strtok_r(pst, "p", &ptr);
			else if (pst[i] == 'W') tmp = strtok_r(pst, "W", &ptr);
			else if (pst[i] == 'w') tmp = strtok_r(pst, "w", &ptr);
			else if (pst[i] == ';') tmp = strtok_r(pst, ";", &ptr);
			else if (pst[i] == ',') tmp = strtok_r(pst, ",", &ptr);
			*call_num = g_strdup(tmp);
			g_free(dup_orig_num);
			return 0;
		}
	}
	*call_num = g_strdup(pst);
	g_free(dup_orig_num);
	return 0;
}

int _callmgr_util_service_check_voice_mail(char *phonenumber, int sim_slot, gboolean *is_voicemail_number)
{
	dbg("_callmgr_util_service_check_voice_mail");
	CM_RETURN_VAL_IF_FAIL(phonenumber, -1);
	CM_RETURN_VAL_IF_FAIL(is_voicemail_number, -1);

	msg_error_t err = MSG_SUCCESS;
	char voice_mail[(CALLMGR_PHONE_NUMBER_LENGTH_MAX+1) + 1] = {0,};
	int result = 0;
	*is_voicemail_number = FALSE;

	msg_handle_t msgHandle = NULL;
	msg_struct_t msg_struct = NULL;

	err = msg_open_msg_handle(&msgHandle);
	if (err != MSG_SUCCESS) {
		err("msg_open_msg_handle error[%d]..", err);
		return FALSE;
	}

	msg_struct = msg_create_struct(MSG_STRUCT_SETTING_VOICE_MSG_OPT);

	err = msg_set_int_value(msg_struct, MSG_VOICEMSG_SIM_INDEX_INT, sim_slot + 1);
	if(MSG_SUCCESS != err) {
		err("msg_set_int_value error[%d]..", err);
		msg_release_struct(&msg_struct);
		msg_close_msg_handle(&msgHandle);
		return FALSE;
	}

	err = msg_get_voice_msg_opt(msgHandle, msg_struct);
	if (MSG_SUCCESS != err) {
		err("msg_get_voice_msg_opt error[%d]..", err);
		msg_release_struct(&msg_struct);
		msg_close_msg_handle(&msgHandle);
		return FALSE;
	}

	err = msg_get_str_value(msg_struct, MSG_VOICEMSG_ADDRESS_STR, voice_mail, (CALLMGR_PHONE_NUMBER_LENGTH_MAX+1));
	if (MSG_SUCCESS != err) {
		err("msg_get_str_value error[%d]..", err);
		msg_release_struct(&msg_struct);
		msg_close_msg_handle(&msgHandle);
		return FALSE;
	}

	dbg("voice mail num [%s]", voice_mail);

	if (g_strcmp0(phonenumber, voice_mail) == 0) {
		*is_voicemail_number = TRUE;
		result =  1;
	}

	msg_release_struct(&msg_struct);
	msg_close_msg_handle(&msgHandle);

	return result;
}

int _callmgr_util_extract_call_number_without_cli(const char *orig_num, char **call_num, char **dtmf_num)
{
	char *pst = NULL;
	char *tmp = NULL;
	char *dup_orig_num = NULL;
	int i, size;
	char *ptr = NULL;

	CM_RETURN_VAL_IF_FAIL(orig_num, -1);
	CM_RETURN_VAL_IF_FAIL(call_num, -1);
	CM_RETURN_VAL_IF_FAIL(dtmf_num, -1);

	dup_orig_num = g_strdup(orig_num);
	CM_RETURN_VAL_IF_FAIL(dup_orig_num, -1);
	pst = (char *)dup_orig_num;
	size = strlen(pst);

	*dtmf_num = NULL;
	for (i = 0; i < size; i++) {
		if (pst[i] == 'P' || pst[i] == 'p' || pst[i] == ',' || pst[i] == 'W' || pst[i] == 'w' || pst[i] == ';') {
			if ((strlen(&pst[i]) > 0) && (&pst[i] != dup_orig_num)) {
				gboolean is_valid_dtmf = FALSE;
				__callmgr_util_is_valid_dtmf_number(&pst[i], &is_valid_dtmf);
				if (is_valid_dtmf) {
					*dtmf_num = g_strdup(&pst[i]);
				}
			}

			if (pst[i] == 'P') tmp = strtok_r(pst, "P", &ptr);
			else if (pst[i] == 'p') tmp = strtok_r(pst, "p", &ptr);
			else if (pst[i] == 'W') tmp = strtok_r(pst, "W", &ptr);
			else if (pst[i] == 'w') tmp = strtok_r(pst, "w", &ptr);
			else if (pst[i] == ';') tmp = strtok_r(pst, ";", &ptr);
			else if (pst[i] == ',') tmp = strtok_r(pst, ",", &ptr);
			sec_warn("token : %s", tmp);
			*call_num = g_strdup(tmp);
			g_free(dup_orig_num);
			return 0;
		}
	}
	*call_num = pst;
	return 0;
}

int _callmgr_util_launch_ciss(const char* number, int sim_slot)
{
	dbg("_callmgr_util_launch_ciss");

	app_control_h app_control = NULL;
	int ret = 0;

	ret = app_control_create(&app_control);
	if (ret < 0) {
		warn("app_control_create() return error : %d", ret);
		return -1;
	}

	char *sim_slot_id_string = (char *)calloc(2, sizeof(char));
	if (sim_slot_id_string == NULL) {
		app_control_destroy(app_control);
		return -1;
	}
	sim_slot_id_string[0] = (char)(sim_slot + '0');
	sim_slot_id_string[1] = '\0';

	if (app_control_set_app_id(app_control, CISS_AUL_CMD) != APP_CONTROL_ERROR_NONE) {
		warn("app_control_set_app_id() is failed");
	} else if (app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT) != APP_CONTROL_ERROR_NONE) {
		warn("app_control_set_operation() is failed");
	} else if (app_control_add_extra_data(app_control, "CISS_LAUNCHING_MODE", CISS_MODE_OPT) != APP_CONTROL_ERROR_NONE) {
		warn("app_control_add_extra_data() is failed");
	} else if (app_control_add_extra_data(app_control, "CISS_REQ_STRING", number) != APP_CONTROL_ERROR_NONE) {
		warn("app_control_add_extra_data() is failed");
	} else if (app_control_add_extra_data(app_control, "CISS_SIM_INDEX", (const char *)sim_slot_id_string) != APP_CONTROL_ERROR_NONE) {
		warn("app_control_add_extra_data() is failed");
	} else if (app_control_send_launch_request(app_control, NULL, NULL) != APP_CONTROL_ERROR_NONE) {
		warn("app_control_send_launch_request() is failed");
	}

	free(sim_slot_id_string);
	app_control_destroy(app_control);

	return 0;
}

int _callmgr_util_check_disturbing_setting(gboolean *is_do_not_disturb)
{
	notification_system_setting_h system_setting = NULL;
	notification_setting_h setting = NULL;
	bool do_not_disturb = false;
	bool do_not_disturb_except = false;
	int err = NOTIFICATION_ERROR_NONE;
	int ret = 0;

	ret = _callmgr_vconf_is_do_not_disturb(&do_not_disturb);
	if (ret == -2) {
		err = notification_system_setting_load_system_setting(&system_setting);
		if (err!= NOTIFICATION_ERROR_NONE || system_setting == NULL) {
			err("notification_system_setting_load_system_setting failed [%d]\n", err);
			goto out;
		}
		notification_system_setting_get_do_not_disturb(system_setting, &do_not_disturb);
	}

	info("do_not_disturb [%d]", do_not_disturb);

	if(do_not_disturb) {
		err = notification_setting_get_setting_by_package_name(CALLUI_PKG_NAME, &setting);
		if(err != NOTIFICATION_ERROR_NONE || setting == NULL) {
			err("notification_setting_get_setting_by_package_name failed [%d]", err);
			goto out;
		}

		notification_setting_get_do_not_disturb_except(setting, &do_not_disturb_except);
		dbg("do_not_disturb_excepted [%d]", do_not_disturb_except);

		if (do_not_disturb_except)
			*is_do_not_disturb = FALSE;
		else
			*is_do_not_disturb = TRUE;
	} else {
		*is_do_not_disturb = FALSE;
	}
out:
	if (system_setting)
		notification_system_setting_free_system_setting(system_setting);
	if (setting)
		notification_setting_free_notification(setting);

	return 0;
}

