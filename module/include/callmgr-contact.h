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

#ifndef __CALLMGR_CONTACT_H__
#define __CALLMGR_CONTACT_H__

#include <glib.h>

typedef struct __contact_data callmgr_contact_info_t;

/**
 * @brief Enumeration for Call history type which is mapped to 'contacts_phone_log_type_e' of "contacts_types.h".
 */
typedef enum {
    CM_CT_PLOG_TYPE_NONE_E,                        /**< None */
    CM_CT_PLOG_TYPE_VOICE_INCOMMING_E = 1,         /**< Incoming call */
    CM_CT_PLOG_TYPE_VOICE_OUTGOING_E = 2,          /**< Outgoing call */
    CM_CT_PLOG_TYPE_VIDEO_INCOMMING_E = 3,         /**< Incoming video call */
    CM_CT_PLOG_TYPE_VIDEO_OUTGOING_E = 4,          /**< Outgoing video call */
    CM_CT_PLOG_TYPE_VOICE_INCOMMING_UNSEEN_E = 5,  /**< Not confirmed missed call */
    CM_CT_PLOG_TYPE_VOICE_INCOMMING_SEEN_E = 6,    /**< Confirmed missed call */
    CM_CT_PLOG_TYPE_VIDEO_INCOMMING_UNSEEN_E = 7,  /**< Not confirmed missed video call */
    CM_CT_PLOG_TYPE_VIDEO_INCOMMING_SEEN_E = 8,    /**< Confirmed missed video call */
    CM_CT_PLOG_TYPE_VOICE_REJECT_E = 9,            /**< Rejected call */
    CM_CT_PLOG_TYPE_VIDEO_REJECT_E = 10,           /**< Rejected video call */
    CM_CT_PLOG_TYPE_VOICE_BLOCKED_E = 11,          /**< Blocked call */
    CM_CT_PLOG_TYPE_VIDEO_BLOCKED_E = 12,          /**< Blocked video call */
} cm_ct_plog_type_e;

typedef enum {
	CM_CT_PLOG_REJECT_TYPE_NONE_E,                        /**< None */
	CM_CT_PLOG_REJECT_TYPE_NORMAL_E, 					   /**< Incoming call is rejected */
	CM_CT_PLOG_REJECT_TYPE_REC_REJECT_E, 					   /**< Incoming call is rejected because of recording */
	CM_CT_PLOG_REJECT_TYPE_SETTING_REJECT_E,				/**< Incoming call is rejected because of do not disturb setting */
	CM_CT_PLOG_REJECT_TYPE_BLOCKED_E, 					   /**< Incoming call is blocked */
} cm_ct_plog_reject_type_e;

typedef enum {
	CM_CT_PLOG_PRESENT_DEFAULT = 0,	/**<  Default(presenatation allow)*/
	CM_CT_PLOG_PRESENT_EMERGENCY = 1,		/**<  Emregency*/
	CM_CT_PLOG_PRESENT_VOICEMAIL = 2,		/**< Voice mail */
	CM_CT_PLOG_PRESENT_UNAVAILABLE = 3, 	/**<  unavailable, other service*/
	CM_CT_PLOG_PRESENT_REJECT_BY_USER = 4,	/**<  reject by user*/
	CM_CT_PLOG_PRESENT_OTHER_SERVICE = 5,	/**<  Other service*/
	CM_CT_PLOG_PRESENT_PAYPHONE = 6,		/**<  Payphone*/
	CM_CT_PLOG_PRESENT_MAX,
} cm_ct_plog_presentation;

typedef struct {
	unsigned int call_id;
	cm_ct_plog_type_e log_type;
	int sim_slot;
	int call_duration;
	cm_ct_plog_presentation presentation;
} cm_ct_plog_data_t;

int _callmgr_ct_add_ct_info(const char *phone_number, unsigned int call_id, callmgr_contact_info_t **o_contact_out_info);
int _callmgr_ct_get_person_id(unsigned int call_id, int *o_person_id);
int _callmgr_ct_get_caller_ringtone_path(unsigned int call_id, char **o_caller_ringtone_path);
int _callmgr_ct_delete_ct_info(unsigned int call_id);
int _callmgr_ct_get_ct_info(unsigned int call_id, callmgr_contact_info_t **o_contact_info_out);
int _callmgr_ct_add_log(cm_ct_plog_data_t *log_data);
int _callmgr_ct_set_missed_call_notification(void);
int _callmgr_ct_set_log_reject_type(unsigned int call_id, cm_ct_plog_reject_type_e reject_type);
int _callmgr_ct_get_log_reject_type(unsigned int call_id, cm_ct_plog_reject_type_e *reject_type);
int _callmgr_ct_get_call_name(unsigned int call_id, char **call_name);

#endif	//__CALLMGR_CONTACT_H__
