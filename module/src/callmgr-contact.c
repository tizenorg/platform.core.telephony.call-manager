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

#include <contacts.h>
#include <contacts_types.h>
#include <notification.h>
#include <notification_internal.h>
#include <notification_text_domain.h>
#include <badge.h>
#include <badge_internal.h>
#include <app.h>
#include <app_control.h>
#include <stdlib.h>
#include <tzplatform_config.h>

#include "callmgr-contact.h"
#include "callmgr-util.h"
#include "callmgr-vconf.h"
#include "callmgr-log.h"

#define CONTACTS_PKG	"org.tizen.contacts"
#define DIALER_PKG		"org.tizen.phone"
#define CLOUD_PKG		"/usr/bin/cloud-pdm-server"

#define NOTI_IMGDIR		tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.quickpanel/shared/res/noti_icons/Contact")
#define IMGDIR			tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.call-ui/res/images")
#define ICONDIR			tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.call-ui/res/images/icon")
#define IND_IMGDIR			tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.quickpanel/shared/res/noti_icons/Lock screen")

#define NOTIFY_MISSED_CALL_ICON			tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.quickpanel/shared/res/noti_icons/Contact/noti_contact_default.png")
#define NOTIFY_MISSED_CALL_IND_ICON		tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.quickpanel/shared/res/noti_icons/Lock screen/noti_missed_call.png")
#define NOTIFY_MISSED_CALL_ICON_SUB		tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.quickpanel/shared/res/noti_icons/Contact/noti_icon_missed.png")
#define NOTIFY_BLOCKED_CALL_ICON		tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.call-ui/res/images/noti_icon_reject_auto.png")
#define NOTIFY_BLOCKED_CALL_ICON_SUB	tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.call-ui/res/images/noti_icon_reject_auto.png")

#define PHONE_ICON		tzplatform_mkpath(TZ_SYS_RO_ICONS, "default/small/org.tizen.phone.png")
#define INVALID_CT_INDEX -1

#define CALL_MISSED_TAG		"CALL_MISSED"
#define CALL_REC_REJECTED_TAG		"CALL_REC_REJECTED"
#define CALL_DEFAULT_CALLER_ID "reserved:default_caller_id"

/* Call-manager only have call_id / person_id matching data.*/
/* With this person_id, callui will find other information*/
struct __contact_data {
	int call_id;
	int person_id;
	char *number;
	char *caller_ringtone_path;
	cm_ct_plog_reject_type_e log_reject_type;
};

static GSList *g_ct_list = NULL;

typedef enum {
	CALL_LOG_SERVICE_TYPE_EMERGENCY = 1,		/**<  Out : Emergency call */
	CALL_LOG_SERVICE_TYPE_VOICE_MAIL,			/**<  Out : Voice mail */
	CALL_LOG_SERVICE_TYPE_UNAVAILABLE,			/**<  Incom : Unavailable */
	CALL_LOG_SERVICE_TYPE_REJECT_BY_USER,		/**<  Incom : Reject by user */
	CALL_LOG_SERVICE_TYPE_OTHER_SERVICE,		/**<  Incom : Other service */
	CALL_LOG_SERVICE_TYPE_PAYPHONE,				/**<  Incom : Payphone */
} call_log_service_type_t;

static void __callmgr_ct_update_dialer_badge(int missed_cnt);
static int __callmgr_ct_get_missed_call_count(void);
static void __callmgr_ct_delete_notification(void);
static void __callmgr_ct_add_notification(int missed_cnt);
static void __callmgr_ct_add_rec_reject_notification(char *number, int person_id, cm_ct_plog_presentation presentation);

static void __callmgr_ct_get_group_list_with_pserson_id(int person_id, GSList **group_list)
{
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_list_h list = NULL;
	contacts_record_h record = NULL;
	contacts_error_e err = CONTACTS_ERROR_NONE;
	int group_id = 0;
	unsigned int projection[] = {
		 _contacts_person_grouprel.group_id,
	};

	if (contacts_query_create(_contacts_person_grouprel._uri, &query) != CONTACTS_ERROR_NONE) {
		err("contacts_query_create is error");
	} else if (contacts_filter_create(_contacts_person_grouprel._uri, &filter) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_create is error");
	} else if (contacts_filter_add_int(filter, _contacts_person_grouprel.person_id, CONTACTS_MATCH_EQUAL, person_id) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_str is error");
	} else if (contacts_query_set_filter(query, filter) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_filter is error");
	} else if (contacts_query_set_projection(query, projection, sizeof(projection)/sizeof(unsigned int)) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_projection is error");
	} else if (contacts_query_set_distinct(query, TRUE) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_distinct is error");
	}

	err = contacts_db_get_records_with_query(query, 0, 0, &list);

	if (CONTACTS_ERROR_NONE == err) {
		contacts_list_first(list);

		while(CONTACTS_ERROR_NONE == contacts_list_get_current_record_p(list, &record)) {
			contacts_record_get_int(record, _contacts_person_grouprel.group_id, &group_id);
			*group_list = g_slist_append(*group_list, GINT_TO_POINTER(group_id));
			err = contacts_list_next(list);
			if (CONTACTS_ERROR_NONE != err)
				break;
		}
	}

	if (list) {
		err = contacts_list_destroy(list, TRUE);
		if (err != CONTACTS_ERROR_NONE) {
			err("contacts_list_destroy is error");
		}
	}
	contacts_filter_destroy(filter);
	contacts_query_destroy(query);

	return;
}

static int __callmgr_ct_get_group_ringtone(int person_id, callmgr_contact_info_t *contact_info)
{
	char *ringtone_path = NULL;
	GSList *group_list = NULL;
	contacts_record_h group_record = NULL;
	contacts_error_e err = CONTACTS_ERROR_NONE;
	int group_len = 0;
	int idx = 0;
	CM_RETURN_VAL_IF_FAIL(contact_info, -1);

	if (0 >= person_id)
		return -1;

	__callmgr_ct_get_group_list_with_pserson_id(person_id, &group_list);
	CM_RETURN_VAL_IF_FAIL(group_list, -1);

	group_len = g_slist_length(group_list);

	if (group_len > 0) {
		for (idx = 0; idx < group_len; idx++) {
			int group_id = GPOINTER_TO_INT(g_slist_nth_data(group_list, idx));
			err = contacts_db_get_record(_contacts_group._uri, group_id , &group_record);

			if (CONTACTS_ERROR_NONE != err) {
				err("contacts_db_get_record(contact) error %d", err);
			} else {
				/* Get group ringtone path */
				err = contacts_record_get_str(group_record, _contacts_group.ringtone_path, &ringtone_path);
				if (CONTACTS_ERROR_NONE != err) {
					err( "contacts_record_get_str(group id path) error %d", err);
				} else {
					contact_info->caller_ringtone_path = g_strdup(ringtone_path);

					g_free(ringtone_path);
					ringtone_path = NULL;

					if (contact_info->caller_ringtone_path != NULL
						&& strlen(contact_info->caller_ringtone_path) > 0) {
						contacts_record_destroy(group_record, TRUE);
						break;
					}
				}
			}
			contacts_record_destroy(group_record, TRUE);

		}
	}
	g_slist_free(group_list);

	return 0;
}

static int __callmgr_ct_get_caller_ringtone(int person_id, callmgr_contact_info_t *contact_info)
{
	contacts_error_e err = CONTACTS_ERROR_NONE;
	contacts_record_h person_record = NULL;

	CM_RETURN_VAL_IF_FAIL(contact_info, -1);

	err = contacts_db_get_record(_contacts_person._uri, person_id, &person_record);
	if (CONTACTS_ERROR_NONE != err) {
		err("contacts_db_get_record error %d", err);
		return -1;
	} else {
		char *ringtone_path = NULL;

		/* Get ringtone path */
		err = contacts_record_get_str(person_record, _contacts_person.ringtone_path, &ringtone_path);
		if (CONTACTS_ERROR_NONE != err) {
			err("contacts_record_get_str(ringtone path) error %d", err);
		} else {
			info("Caller ringtone");
			contact_info->caller_ringtone_path = g_strdup(ringtone_path);
			free(ringtone_path);
		}
		contacts_record_destroy(person_record, TRUE);
	}

	return 0;
}

static int __callmgr_ct_get_person_id_by_num(const char *phone_number, int *person_id)
{
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_list_h list = NULL;
	contacts_record_h record = NULL;
	contacts_error_e err = CONTACTS_ERROR_NONE;
	unsigned int projection[] = {
		 _contacts_person_number.person_id,
	};
	CM_RETURN_VAL_IF_FAIL(phone_number, -1);
	CM_RETURN_VAL_IF_FAIL(person_id, -1);

	if (contacts_query_create(_contacts_person_number._uri, &query) != CONTACTS_ERROR_NONE) {
		err("contacts_query_create is error");
	} else if (contacts_filter_create(_contacts_person_number._uri, &filter) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_create is error");
	} else if (contacts_filter_add_str(filter, _contacts_person_number.number_filter, CONTACTS_MATCH_EXACTLY, phone_number) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_str is error");
	} else if (contacts_query_set_filter(query, filter) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_filter is error");
	} else if (contacts_query_set_projection(query, projection, sizeof(projection)/sizeof(unsigned int)) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_projection is error");
	} else if (contacts_query_set_distinct(query, TRUE) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_distinct is error");
	}

	err = contacts_db_get_records_with_query(query, 0, 0, &list);

	if (CONTACTS_ERROR_NONE == err) {
		contacts_list_first(list);

		while (CONTACTS_ERROR_NONE == contacts_list_get_current_record_p(list, &record)) {
			contacts_record_get_int(record, _contacts_person_number.person_id, person_id);
			info("person id %d", *person_id);
			err = contacts_list_next(list);
			if (CONTACTS_ERROR_NONE != err)
				break;
		}
	}

	if (list) {
		err = contacts_list_destroy(list, TRUE);
		if (err != CONTACTS_ERROR_NONE) {
			err("contacts_list_destroy is error");
		}
	}
	contacts_filter_destroy(filter);
	contacts_query_destroy(query);

	return 0;
}

int _callmgr_ct_add_ct_info(const char *phone_number, unsigned int call_id, callmgr_contact_info_t **o_contact_out_info)
{
	callmgr_contact_info_t *contact_info = NULL;
	int person_id = -1;
	contacts_error_e err = CONTACTS_ERROR_NONE;

	contact_info = (callmgr_contact_info_t*)calloc(1, sizeof(callmgr_contact_info_t));
	CM_RETURN_VAL_IF_FAIL(contact_info, -1);

	_callmgr_ct_delete_ct_info(call_id);

	err = contacts_connect();
	if (CONTACTS_ERROR_NONE != err) {
		err("contacts_connect is error : %d", err);
		g_free(contact_info);
		return -1;
	}

	__callmgr_ct_get_person_id_by_num(phone_number, &person_id);
	if (-1 == person_id) {
		err("no contact saved for this number in db");
	} else {
		info("contact saved with index : %d", person_id);
		__callmgr_ct_get_caller_ringtone(person_id, contact_info);

		if (contact_info->caller_ringtone_path == NULL) {
			__callmgr_ct_get_group_ringtone(person_id, contact_info);
		}
	}

	contact_info->call_id = call_id;
	contact_info->person_id = person_id;
	if (phone_number) {
		contact_info->number = g_strdup(phone_number);
	}
	contact_info->log_reject_type = CM_CT_PLOG_REJECT_TYPE_NONE_E;
	g_ct_list = g_slist_append(g_ct_list, contact_info);

	contacts_disconnect();
	*o_contact_out_info = contact_info;
	return 0;
}

int _callmgr_ct_get_person_id(unsigned int call_id, int *o_person_id)
{
	callmgr_contact_info_t *contact_info = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(g_ct_list, -1);
	CM_RETURN_VAL_IF_FAIL(o_person_id, -1);

	list_len = g_slist_length(g_ct_list);
	for (idx = 0; idx < list_len; idx++) {
		contact_info = g_slist_nth_data(g_ct_list, idx);
		if (contact_info->call_id == call_id) {
			info("Call Index found[%d]", contact_info->person_id);
			*o_person_id = contact_info->person_id;
			return 0;
		}
	}

	*o_person_id = -1;
	return -1;
}

int _callmgr_ct_get_caller_ringtone_path(unsigned int call_id, char **o_caller_ringtone_path)
{
	callmgr_contact_info_t *contact_info = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(g_ct_list, -1);
	CM_RETURN_VAL_IF_FAIL(o_caller_ringtone_path, -1);

	dbg(">>");
	list_len = g_slist_length(g_ct_list);
	for (idx = 0; idx < list_len; idx++) {
		contact_info = g_slist_nth_data(g_ct_list, idx);
		if (contact_info->call_id == call_id) {
			info("Call Index found[%d]", contact_info->person_id);
			if (contact_info->caller_ringtone_path)
				*o_caller_ringtone_path = g_strdup(contact_info->caller_ringtone_path);
			else
				*o_caller_ringtone_path = NULL;

			return 0;
		}
	}

	*o_caller_ringtone_path = NULL;
	return -1;
}

int _callmgr_ct_delete_ct_info(unsigned int call_id)
{
	callmgr_contact_info_t *contact_info = NULL;
	int result = -1;
	GSList *list = NULL;
	CM_RETURN_VAL_IF_FAIL(g_ct_list, -1);

	list = g_ct_list;
	while (list) {
		contact_info = (callmgr_contact_info_t*)list->data;
		list = g_slist_next(list);

		if ((contact_info) && (contact_info->call_id == call_id)) {
			info("Contact(which has ID %d) is removed from the contact list", call_id);
			g_ct_list = g_slist_remove(g_ct_list, contact_info);
			g_free(contact_info->caller_ringtone_path);
			g_free(contact_info->number);
			g_free(contact_info);
			result = 0;
		}
	}
	return result;
}

int _callmgr_ct_get_ct_info(unsigned int call_id, callmgr_contact_info_t **o_contact_info_out)
{
	callmgr_contact_info_t *contact_info = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(g_ct_list, -1);
	CM_RETURN_VAL_IF_FAIL(o_contact_info_out, -1);

	list_len = g_slist_length(g_ct_list);
	for (idx = 0; idx < list_len; idx++) {
		contact_info = g_slist_nth_data(g_ct_list, idx);
		if (contact_info->call_id == call_id) {
			*o_contact_info_out = contact_info;
			return 0;
		}
	}

	return -1;
}

int _callmgr_ct_add_log(cm_ct_plog_data_t *log_data)
{
	int ret = -1;
	time_t current_time;
	int log_ret = CONTACTS_ERROR_NONE;
	char service_type[4] = {0, };
	gboolean is_datalock = FALSE;
	callmgr_contact_info_t *contact_info = NULL;
	CM_RETURN_VAL_IF_FAIL(log_data, -1);

	dbg("_callmgr_ct_add_log: type(%d)", log_data->log_type);

	_callmgr_vconf_is_data_lock(&is_datalock);
	if (is_datalock) {
		err("Data Locked, not adding call log");
		return -1;
	}

	ret = _callmgr_ct_get_ct_info(log_data->call_id, &contact_info);
	if (-1 == ret) {
		err("No contact info found for the call id = %d, not adding call log", log_data->call_id);
		return -1;
	}

	current_time = time(NULL);

	log_ret = contacts_connect();
	if (log_ret != CONTACTS_ERROR_NONE) {
		err("contacts_connect is failed[%d]", log_ret);
		contacts_disconnect();
		return -1;
	} else {
		contacts_record_h log_record = NULL;

		g_snprintf(service_type, sizeof(service_type), "%03d", log_data->presentation);
		warn("service_type : %s", service_type);

		dbg("data->active_sim_slot_id %d", log_data->sim_slot);
		if (contacts_record_create(_contacts_phone_log._uri, &log_record) != CONTACTS_ERROR_NONE) {
			err("contacts_record_create is failed");
		} else if (contacts_record_set_str(log_record, _contacts_phone_log.address, contact_info->number) != CONTACTS_ERROR_NONE) {
			err("contacts_record_set_str(Number) is failed");
		} else if (contacts_record_set_int(log_record, _contacts_phone_log.log_time, (current_time)) != CONTACTS_ERROR_NONE) {	/* Log time is call end time*/
			err("contacts_record_set_int(Time) is failed");
		} else if (contacts_record_set_int(log_record, _contacts_phone_log.log_type, log_data->log_type) != CONTACTS_ERROR_NONE) {
			err("contacts_record_set_int(Type) is failed");
		} else if (contacts_record_set_int(log_record, _contacts_phone_log.extra_data1, log_data->call_duration) != CONTACTS_ERROR_NONE) {
			err("contacts_record_set_int(Duration) is failed");
		} else if (contacts_record_set_int(log_record, _contacts_phone_log.person_id, contact_info->person_id) != CONTACTS_ERROR_NONE) {
			err("contacts_record_set_int(person id) is failed");
		} else if (contacts_record_set_str(log_record, _contacts_phone_log.extra_data2, service_type) != CONTACTS_ERROR_NONE) {
			err("contacts_record_set_int(extra_data2) is failed");
		} else if (contacts_record_set_int(log_record, _contacts_phone_log.sim_slot_no, log_data->sim_slot) != CONTACTS_ERROR_NONE) {
			err("contacts_record_set_int(person id) is failed");
		} else if (contacts_db_insert_record(log_record, NULL) != CONTACTS_ERROR_NONE) {
			err("contacts_db_insert_record is failed");
		} else {
			info("Call log is added successfully");
		}
		contacts_record_destroy(log_record, TRUE);
	}

	contacts_disconnect();

	if ((log_data->log_type == CM_CT_PLOG_TYPE_VOICE_INCOMING_UNSEEN_E)
		|| (log_data->log_type == CM_CT_PLOG_TYPE_VIDEO_INCOMING_UNSEEN_E)) {
		_callmgr_ct_set_missed_call_notification();
	} else if ((log_data->log_type == CM_CT_PLOG_TYPE_VOICE_BLOCKED_E)
		|| (log_data->log_type == CM_CT_PLOG_TYPE_VIDEO_BLOCKED_E)
		|| (log_data->log_type == CM_CT_PLOG_TYPE_VOICE_REJECT_E)) {
		dbg("reject type %d", contact_info->log_reject_type);
		if ((contact_info->log_reject_type == CM_CT_PLOG_REJECT_TYPE_REC_REJECT_E)
			|| (contact_info->log_reject_type == CM_CT_PLOG_REJECT_TYPE_SETTING_REJECT_E)) {
			__callmgr_ct_add_rec_reject_notification(contact_info->number, contact_info->person_id, log_data->presentation);
		}
	}

	return 0;
}

static void __callmgr_ct_update_dialer_badge(int missed_cnt)
{
	bool bbadge_exist = FALSE;
	badge_error_e err = BADGE_ERROR_NONE;

	dbg("__callmgr_ct_update_dialer_badge: missed_cnt(%d)", missed_cnt);

	err = badge_is_existing(DIALER_PKG, &bbadge_exist);
	if (err == BADGE_ERROR_NONE && bbadge_exist == FALSE) {
		err = badge_create(DIALER_PKG, DIALER_PKG","CLOUD_PKG","CONTACTS_PKG);
		if (err != BADGE_ERROR_NONE) {
			err("badge_create failed: err(%d)", err);

			return;
		}
	}

	badge_set_count(DIALER_PKG, missed_cnt);
}

static int __callmgr_ct_get_missed_call_count(void)
{
	int missed_cnt = 0;
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	int log_ret = CONTACTS_ERROR_NONE;

	dbg("__callmgr_ct_get_missed_call_count");

	log_ret = contacts_connect();
	if (log_ret != CONTACTS_ERROR_NONE) {
		err("contacts_connect is failed with err[%d] ", log_ret);
		return -1;
	}

	if (contacts_query_create(_contacts_phone_log._uri, &query) != CONTACTS_ERROR_NONE) {
		err("contacts_query_create failed");
	} else if (contacts_filter_create(_contacts_phone_log._uri, &filter) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_create failed");
	} else if (contacts_filter_add_int(filter, _contacts_phone_log.log_type, CONTACTS_MATCH_EQUAL, CONTACTS_PLOG_TYPE_VOICE_INCOMING_UNSEEN) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_int failed");
	} else if (contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_OR) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_operator failed");
	} else if (contacts_filter_add_int(filter, _contacts_phone_log.log_type, CONTACTS_MATCH_EQUAL, CONTACTS_PLOG_TYPE_VIDEO_INCOMING_UNSEEN) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_int failed");
	} else if (contacts_query_set_filter(query, filter) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_filter failed");
	} else if (contacts_db_get_count_with_query(query, &missed_cnt) != CONTACTS_ERROR_NONE) {
		err("contacts_db_get_count_with_query failed");
	}

	contacts_filter_destroy(filter);
	contacts_query_destroy(query);
	contacts_disconnect();

	return missed_cnt;
}

static void __callmgr_ct_delete_notification(void)
{
	/* Can not load notification with tag
	   Because insert pacakge and load package are different.
	   It is Notification policy */
#if 0
	notification_h missed_noti = NULL;

	missed_noti = notification_load_by_tag(CALL_MISSED_TAG);
	if (missed_noti) {
		notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
		dbg("missed noti");
		noti_err = notification_delete(missed_noti);
		if (noti_err != NOTIFICATION_ERROR_NONE) {
			err("Fail to notification_delete");
		}

		noti_err = notification_free(missed_noti);
		if (noti_err != NOTIFICATION_ERROR_NONE) {
			err("Fail to notification_free");
		}
	}
#endif
}

static int __callmgr_ct_get_caller_info(int person_id, char **name, char **caller_id)
{
	contacts_error_e err = CONTACTS_ERROR_NONE;
	contacts_record_h person_record = NULL;

	err = contacts_db_get_record(_contacts_person._uri, person_id, &person_record);
	if (CONTACTS_ERROR_NONE != err) {
		err("contacts_db_get_record error %d", err);
		return -1;
	} else {
		char *display_name = NULL;
		char *caller_id_path = NULL;

		/* Get display name */
		err = contacts_record_get_str(person_record, _contacts_person.display_name, &display_name);
		if (CONTACTS_ERROR_NONE != err) {
			err("contacts_record_get_str(display_name path) error %d", err);
		} else {
			*name = g_strdup(display_name);
			free(display_name);
		}

		err = contacts_record_get_str(person_record, _contacts_person.image_thumbnail_path, &caller_id_path);
		if (CONTACTS_ERROR_NONE != err) {
			err("contacts_record_get_str(display_name path) error %d", err);
		} else {
			*caller_id = g_strdup(caller_id_path);
			free(caller_id_path);
		}

		contacts_record_destroy(person_record, TRUE);
	}

	return 0;
}

static void __callmgr_ct_add_notification(int missed_cnt)
{
	dbg("..");

	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_list_h list = NULL;
	contacts_record_h record = NULL;
	contacts_error_e error = CONTACTS_ERROR_NONE;

	int noti_flags = 0;
	int log_cnt = 0;
	int person_id = 0;
	int log_type = CONTACTS_PLOG_TYPE_NONE;
	char str_buf[1024] = { 0, };
	char str_cnt[10] = { 0, };
	char current_num[83] = { 0, };
	char *str_id = NULL;
	char *disp_img_path = NULL;
	char disp_name_str[300] = { 0, };
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	app_control_h service = NULL;
	gboolean b_is_single = true;

	unsigned int projection[] = {
		 _contacts_phone_log.person_id,
		 _contacts_phone_log.address,
		 _contacts_phone_log.log_time,
		 _contacts_phone_log.log_type,
		 _contacts_phone_log.extra_data2,
	};

	__callmgr_ct_delete_notification();

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (noti == NULL) {
		err("Fail to notification_create");
		return;
	}

	/* If we use tag, notification is updated automatically */

	noti_err = notification_set_tag(noti, CALL_MISSED_TAG);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
			  err("Fail to notification_set_tag : %d", noti_err);
	}

	noti_err = notification_set_text_domain(noti, "callmgr-popup", tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.callmgr-popup/shared/res/locale"));
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		warn("Fail to notification_set_text_domain");
	}

	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		err("Fail to app_control_create");
	} else if (service == NULL) {
		err("Fail to app_control_create");
	} else {
		dbg("Create service");
		if (app_control_set_app_id(service, DIALER_PKG)  != APP_CONTROL_ERROR_NONE) {
			err("Fail to app_control_set_app_id");
		} else if (app_control_set_operation(service, APP_CONTROL_OPERATION_VIEW)  != APP_CONTROL_ERROR_NONE) {
			err("Fail to app_control_set_operation");
		} else if (app_control_add_extra_data(service, "logs", "missed_call")  != APP_CONTROL_ERROR_NONE) {
			err("Fail to app_control_add_extra_data");
		}

		if (missed_cnt == 1) {
			strncpy(str_buf, _("IDS_CALL_POP_CALLMISSED"), sizeof(str_buf) - 1);
			str_id = "IDS_CALL_POP_CALLMISSED";
			noti_err = notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_SINGLE);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_layout : %d", noti_err);
			}

			noti_err = notification_set_launch_option(noti, NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, service);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_launch_option : %d", noti_err);
			}
		} else {
			strncpy(str_buf, _("IDS_CALL_POP_MISSED_CALLS_ABB"), sizeof(str_buf) - 1);
			str_id = "IDS_CALL_POP_MISSED_CALLS_ABB";
			noti_err = notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_MULTIPLE);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_layout : %d", noti_err);
			}
			noti_err = notification_set_launch_option(noti, NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, service);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_launch_option : %d", noti_err);
			}
		}

		app_control_destroy(service);
	}

	g_snprintf(str_cnt, sizeof(str_cnt), "%d", missed_cnt);
	info("Notification string :[%s] [%s]", str_buf, str_cnt);

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, str_buf, str_id, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_text : %d", noti_err);
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, str_cnt, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_text : %d", noti_err);
	}

	noti_err = notification_set_led(noti, NOTIFICATION_LED_OP_ON, 0x0);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_led : %d", noti_err);
	}

	if (contacts_connect() != CONTACTS_ERROR_NONE) {
		err("contacts_connect is error");
	}

	if (contacts_query_create(_contacts_phone_log._uri, &query) != CONTACTS_ERROR_NONE) {
		err("contacts_query_create is error");
	} else if (contacts_filter_create(_contacts_phone_log._uri, &filter) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_create is error");
	} else if (contacts_filter_add_int(filter, _contacts_phone_log.log_type, CONTACTS_MATCH_EQUAL, CONTACTS_PLOG_TYPE_VOICE_INCOMING_UNSEEN) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_int is error");
	} else if (contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_OR) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_operator is error");
	} else if (contacts_filter_add_int(filter, _contacts_phone_log.log_type, CONTACTS_MATCH_EQUAL, CONTACTS_PLOG_TYPE_VIDEO_INCOMING_UNSEEN) != CONTACTS_ERROR_NONE) {
		err("contacts_filter_add_int is error");
	} else if (contacts_query_set_filter(query, filter) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_filter is error");
	} else if (contacts_query_set_projection(query, projection, sizeof(projection)/sizeof(unsigned int)) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_projection is error");
	} else if (contacts_query_set_distinct(query, TRUE) != CONTACTS_ERROR_NONE) {
		err("contacts_query_set_distinct is error");
	} else if (contacts_query_set_sort(query, _contacts_phone_log.log_time, FALSE)) {
		err("contacts_query_set_sort is error");
	}

	error = contacts_db_get_records_with_query(query, 0, 0, &list);

	if (CONTACTS_ERROR_NONE == error) {
		contacts_list_first(list);

		while (CONTACTS_ERROR_NONE == contacts_list_get_current_record_p(list, &record)) {
			int log_time = 0;
			char *disp_str = NULL;
			char *contact_path = NULL;
			char *disp_id = NULL;
			char *service_type = NULL;

			person_id = 0;
			log_cnt++;

			err("log_cnt : %d", log_cnt);
			error = contacts_record_get_int(record, _contacts_phone_log.log_time, &log_time);
			if (CONTACTS_ERROR_NONE != error) {
				err("Fail to contacts_record_get_int : %d", error);
			}

			if (log_cnt == 1) {
				/* Display last log*/
				error = contacts_record_get_str(record, _contacts_phone_log.address, &disp_str);
				if (CONTACTS_ERROR_NONE != error) {
					err("Fail to contacts_record_get_str : %d", error);
				}
				if (disp_str) {
					strncpy(current_num, disp_str, sizeof(current_num) - 1);
					g_free(disp_str);
					disp_str = NULL;
				}

				error = contacts_record_get_int(record, _contacts_phone_log.log_type, &log_type);
				if (CONTACTS_ERROR_NONE != error) {
					err("Fail to contacts_record_get_int : %d", error);
				}
				noti_err = notification_set_time(noti, log_time);
				if (noti_err != NOTIFICATION_ERROR_NONE) {
					err("Fail to notification_set_time : %d", noti_err);
				}
			} else {
				break;
			}

			while (CONTACTS_ERROR_NONE == contacts_list_get_current_record_p(list, &record)) {
				error = contacts_record_get_int(record, _contacts_phone_log.person_id, &person_id);
				if (CONTACTS_ERROR_NONE != error) {
					err("Fail to contacts_record_get_int : %d", error);
				}
				if (person_id == 0) {
					error = contacts_record_get_str(record, _contacts_phone_log.address, &disp_str);
					if (CONTACTS_ERROR_NONE != error) {
						err("Fail to contacts_record_get_str : %d", error);
					}

					error = contacts_record_get_str(record, _contacts_phone_log.extra_data2, &service_type);
					if (CONTACTS_ERROR_NONE != error) {
						err("Fail to contacts_record_get_str : %d", error);
					}
				} else {
					__callmgr_ct_get_caller_info(person_id, &disp_str, &contact_path);
				}

				if (contact_path) {
					if (disp_img_path == NULL) {
						disp_img_path = g_strdup(contact_path);
					}
					g_free(contact_path);
					contact_path = NULL;
				}

				if (disp_str) {
					sec_dbg("person id : %d, disp_str : %s[%d], img : %s", person_id, disp_str, log_cnt, disp_img_path);
				} else {
					if (service_type) {
						int service_num = atoi(service_type);
						int service_code = service_num % 100;
						switch (service_code) {
						case CALL_LOG_SERVICE_TYPE_UNAVAILABLE:
						case CALL_LOG_SERVICE_TYPE_OTHER_SERVICE:
							disp_str = g_strdup(_("IDS_CALL_BODY_UNKNOWN"));
							disp_id = "IDS_CALL_BODY_UNKNOWN";
							break;
						case CALL_LOG_SERVICE_TYPE_REJECT_BY_USER:
							disp_str = g_strdup(_("IDS_COM_BODY_PRIVATE_NUMBER"));
							disp_id = "IDS_COM_BODY_PRIVATE_NUMBER";
							break;
						case CALL_LOG_SERVICE_TYPE_PAYPHONE:
							disp_str = g_strdup(_("IDS_CALL_BODY_PAYPHONE"));
							disp_id = "IDS_CALL_BODY_PAYPHONE";
							break;
						default:
							disp_str = g_strdup(_("IDS_CALL_BODY_UNKNOWN"));
							disp_id = "IDS_CALL_BODY_UNKNOWN";
							break;
						}
						g_free(service_type);
						service_type = NULL;
					} else {
						disp_str = g_strdup(_("IDS_CALL_BODY_UNKNOWN"));
						disp_id = "IDS_CALL_BODY_UNKNOWN";
					}

					/*notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, disp_str, disp_id, NOTIFICATION_VARIABLE_TYPE_NONE);*/
				}

				if (strstr(disp_name_str, disp_str) == NULL) {
					int remain_max = sizeof(disp_name_str) - strlen((const char *)disp_name_str) - 1;

					if ((remain_max > 0) && (strlen((const char *)disp_name_str) >= 1)) {
						strncat(disp_name_str, ",", sizeof(char));
						b_is_single = false;
						remain_max = remain_max - sizeof(char);
					}

					if ((remain_max > 0) && (strlen(disp_str) > 0)) {
						if (strlen(disp_str) < remain_max) {
							strncat(disp_name_str, disp_str, strlen(disp_str));
						}
						else {
							strncat(disp_name_str, disp_str, remain_max);
						}
					}
					sec_dbg("disp_str(%s), disp_name_str(%s)", disp_str, disp_name_str);

					g_free(disp_str);
					disp_str = NULL;

					if (strlen((const char *)disp_name_str) > 30) {
						break;
					}
				}

				error = contacts_list_next(list);
				if (CONTACTS_ERROR_NONE != error) {
					err("No more log entries. End of loop");
					break;
				}
			}

			noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, (const char *)disp_name_str, disp_id, NOTIFICATION_VARIABLE_TYPE_NONE);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_text : %d", noti_err);
			}

			if ((CONTACTS_ERROR_NONE != error)
				|| (log_cnt == 2))
				break;
		}
	}

	if (list) {
		error = contacts_list_destroy(list, TRUE);
		if (error != CONTACTS_ERROR_NONE) {
			err("Fail to contacts_list_destroy");
		}
	}
	contacts_filter_destroy(filter);
	contacts_query_destroy(query);
	contacts_disconnect();

	if (disp_img_path == NULL){
		disp_img_path = g_strdup(NOTIFY_MISSED_CALL_ICON);
	} else if (b_is_single == false) {
		g_free(disp_img_path);
		disp_img_path = g_strdup(NOTIFY_MISSED_CALL_ICON);
	} else {
		dbg("contact");
	}

	noti_flags = NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY | NOTIFICATION_DISPLAY_APP_LOCK | NOTIFICATION_DISPLAY_APP_INDICATOR | NOTIFICATION_DISPLAY_APP_TICKER;
	noti_err = notification_set_display_applist(noti, noti_flags);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_display_applist : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, NOTIFY_MISSED_CALL_IND_ICON);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB, NOTIFY_MISSED_CALL_ICON_SUB);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_LOCK, disp_img_path);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, disp_img_path);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_pkgname(noti, CONTACTS_PKG);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_pkgname : %d", noti_err);
	}

	noti_err = notification_insert(noti, NULL);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_insert");
	}

	noti_err = notification_free(noti);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_free");
	}

	g_free(disp_img_path);
}

int _callmgr_ct_set_missed_call_notification(void)
{
	int missed_cnt = 0;

	dbg("_callmgr_ct_set_missed_call_notification");

	missed_cnt = __callmgr_ct_get_missed_call_count();
	if (missed_cnt > 0) {
		__callmgr_ct_update_dialer_badge(missed_cnt);
		__callmgr_ct_add_notification(missed_cnt);
	} else {
		__callmgr_ct_delete_notification();
	}

	return 0;
}

int _callmgr_ct_set_log_reject_type(unsigned int call_id, cm_ct_plog_reject_type_e reject_type)
{
	callmgr_contact_info_t *contact_info = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(g_ct_list, -1);
	dbg("call_id(%d), reject_type(%d)", call_id, reject_type);

	list_len = g_slist_length(g_ct_list);
	for (idx = 0; idx < list_len; idx++) {
		contact_info = g_slist_nth_data(g_ct_list, idx);
		if (contact_info->call_id == call_id) {
			contact_info->log_reject_type = reject_type;
			return 0;
		}
	}

	return -1;
}

int _callmgr_ct_get_log_reject_type(unsigned int call_id, cm_ct_plog_reject_type_e *reject_type)
{
	callmgr_contact_info_t *contact_info = NULL;
	int list_len = -1;
	int idx = -1;
	CM_RETURN_VAL_IF_FAIL(g_ct_list, -1);

	list_len = g_slist_length(g_ct_list);
	for (idx = 0; idx < list_len; idx++) {
		contact_info = g_slist_nth_data(g_ct_list, idx);
		if (contact_info->call_id == call_id) {
			*reject_type = contact_info->log_reject_type;
			return 0;
		}
	}

	return -1;
}

static int __callmgr_ct_get_caller_name(int person_id, char **name)
{
	contacts_error_e err = CONTACTS_ERROR_NONE;
	contacts_record_h person_record = NULL;

	err = contacts_db_get_record(_contacts_person._uri, person_id, &person_record);
	if (CONTACTS_ERROR_NONE != err) {
		err("contacts_db_get_record error %d", err);
		return -1;
	} else {
		char *display_name = NULL;

		/* Get display name */
		err = contacts_record_get_str(person_record, _contacts_person.display_name, &display_name);
		if (CONTACTS_ERROR_NONE != err) {
			err("contacts_record_get_str(display_name path) error %d", err);
		} else {
			*name = g_strdup(display_name);
			free(display_name);
		}
		contacts_record_destroy(person_record, TRUE);
	}

	return 0;
}

static void __callmgr_ct_add_rec_reject_notification(char *number, int person_id, cm_ct_plog_presentation presentation)
{
	dbg("..");

	int noti_flags = 0;
	char *disp_img_path = NULL;

	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	app_control_h service = NULL;

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (noti == NULL) {
		err("Fail to notification_create");
		return;
	}

	noti_err = notification_set_tag(noti, CALL_REC_REJECTED_TAG);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
			  err("Fail to notification_set_tag : %d", noti_err);
	}

	noti_err = notification_set_text_domain(noti, "callmgr-popup", tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.callmgr-popup/shared/res/locale"));
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		warn("Fail to notification_set_text_domain");
	}

	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		err("Fail to app_control_create");
	} else if (service == NULL) {
		err("Fail to app_control_create");
	} else {
		dbg("Create service");
		if (app_control_set_app_id(service, DIALER_PKG)  != APP_CONTROL_ERROR_NONE) {
			err("Fail to app_control_set_app_id");
		} else if (app_control_set_operation(service, APP_CONTROL_OPERATION_VIEW)  != APP_CONTROL_ERROR_NONE) {
			err("Fail to app_control_set_operation");
		} else if (app_control_add_extra_data(service, "launch_type", "log")  != APP_CONTROL_ERROR_NONE) {
			err("Fail to app_control_add_extra_data");
		}

		noti_err = notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_SINGLE);
		if (noti_err != NOTIFICATION_ERROR_NONE) {
			err("Fail to notification_set_layout : %d", noti_err);
		}

		noti_err = notification_set_launch_option(noti, NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, service);
		if (noti_err != NOTIFICATION_ERROR_NONE) {
			err("Fail to notification_set_launch_option : %d", noti_err);
		}

		if (person_id >= 1) {
			contacts_error_e err = CONTACTS_ERROR_NONE;

			char *disp_str = NULL;

			err = contacts_connect();
			if (err != CONTACTS_ERROR_NONE) {
				err("contacts_connect is failed[%d]", err);
				disp_str = g_strdup(_("IDS_CALL_BODY_UNKNOWN"));
			} else {
				__callmgr_ct_get_caller_info(person_id, &disp_str, &disp_img_path);
			}

			contacts_disconnect();

			noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, disp_str, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
			g_free(disp_str);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_text : %d", noti_err);
			}
		} else if (number) {
			noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, number, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_text : %d", noti_err);
			}
		} else {
			char *disp_str = NULL;
			char *disp_id = NULL;

			switch (presentation) {
			case CM_CT_PLOG_PRESENT_UNAVAILABLE:
			case CM_CT_PLOG_PRESENT_OTHER_SERVICE:
				disp_str = _("IDS_CALL_BODY_UNKNOWN");
				disp_id = "IDS_CALL_BODY_UNKNOWN";
				break;
			case CM_CT_PLOG_PRESENT_REJECT_BY_USER:
				disp_str = _("IDS_COM_BODY_PRIVATE_NUMBER");
				disp_id = "IDS_COM_BODY_PRIVATE_NUMBER";
				break;
			case CM_CT_PLOG_PRESENT_PAYPHONE:
				disp_str = _("IDS_CALL_BODY_PAYPHONE");
				disp_id = "IDS_CALL_BODY_PAYPHONE";
				break;
			default:
				disp_str = _("IDS_CALL_BODY_UNKNOWN");
				disp_id = "IDS_CALL_BODY_UNKNOWN";
				break;
			}

			noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, disp_str, disp_id, NOTIFICATION_VARIABLE_TYPE_NONE);
			if (noti_err != NOTIFICATION_ERROR_NONE) {
				err("Fail to notification_set_text : %d", noti_err);
			}
		}

		app_control_destroy(service);
	}

	if (disp_img_path == NULL) {
		disp_img_path = g_strdup(NOTIFY_MISSED_CALL_ICON);
	}

	noti_err = notification_set_time(noti, time(NULL));
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_time : %d", noti_err);
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, _("IDS_CALL_MBODY_AUTO_REJECTED_CALL"), "IDS_CALL_MBODY_AUTO_REJECTED_CALL", NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_text : %d", noti_err);
	}

	noti_err = notification_set_led(noti, NOTIFICATION_LED_OP_ON, 0x0);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_led : %d", noti_err);
	}

	noti_flags = NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY | NOTIFICATION_DISPLAY_APP_LOCK | NOTIFICATION_DISPLAY_APP_INDICATOR | NOTIFICATION_DISPLAY_APP_TICKER;
	noti_err = notification_set_display_applist(noti, noti_flags);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_display_applist : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, NOTIFY_BLOCKED_CALL_ICON);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_SUB, NOTIFY_BLOCKED_CALL_ICON_SUB);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_LOCK, disp_img_path);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, disp_img_path);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_image : %d", noti_err);
	}

	g_free(disp_img_path);
	disp_img_path = NULL;

	noti_err = notification_set_pkgname(noti, CONTACTS_PKG);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_set_pkgname : %d", noti_err);
	}

	noti_err = notification_insert(noti, NULL);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_insert");
	}

	noti_err = notification_free(noti);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		err("Fail to notification_free");
	}
}

int _callmgr_ct_get_call_name(unsigned int call_id, char **call_name)
{
	int person_id = -1;
	contacts_error_e err = CONTACTS_ERROR_NONE;
	CM_RETURN_VAL_IF_FAIL(call_name, -1);

	err = contacts_connect();

	if (err != CONTACTS_ERROR_NONE) {
		err("contacts_connect is failed[%d]", err);
	} else {
		_callmgr_ct_get_person_id(call_id, &person_id);
		__callmgr_ct_get_caller_name(person_id, call_name);
	}

	contacts_disconnect();
	return 0;
}

