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

#ifndef __CALLMGR_TELEPHONY_H__
#define __CALLMGR_TELEPHONY_H__


#include <glib.h>

#define CALLMGR_TAPI_HANDLE_MAX	2
#define CALLMGR_MAX_CALL_GROUP_MEMBER		5

#define NO_CALL_HANDLE 0

/* Mcc */
#define CALL_NETWORK_MCC_UK				234		/*UK = 234*/
#define CALL_NETWORK_MCC_IRELAND		272		/*Ireland = 272*/
#define CALL_NETWORK_MCC_UAE			424		/*UAE = 424*/
#define CALL_NETWORK_MCC_GHANA			620		/*Ghana = 620*/
#define CALL_NETWORK_MCC_KENYA			630		/*Kenya = 630*/
#define CALL_NETWORK_MCC_ISRAEL			425		/*Israel = 425*/
#define CALL_NETWORK_MCC_CROATIA		219		/*Croatia = 219*/
#define CALL_NETWORK_MCC_SERBIA			220		/*Serbia = 220*/
#define CALL_NETWORK_MCC_RUSSIA			250		/*Russia = 250*/

#define CALL_NETWORK_MCC_TAIWAN			466		/*Taiwan = 466*/
#define CALL_NETWORK_MCC_HONGKONG		454		/*Hongkong = 454*/
#define CALL_NETWORK_MCC_MALAYSIA     502		/*MALAYSIA = 502*/
#define CALL_NETWORK_MCC_AUSTRALIA		505		/*Australia = 505*/
#define CALL_NETWORK_MCC_NEWZEALAND	530		/*NewZealand = 530*/
#define CALL_NETWORK_MCC_PNG			537		/*Papua New Guinea = 537*/
#define CALL_NETWORK_MCC_VANUATU		541		/*Vanuatu = 541*/
#define CALL_NETWORK_MCC_FIJI		542		/*Fiji = 542*/
#define CALL_NETWORK_MCC_SAMOA		549		/*Samoa = 549*/

#define CALL_NETWORK_MCC_USA			310		/*USA = 310*/
#define CALL_NETWORK_MCC_USA_2			311		/*USA = 311*/
#define CALL_NETWORK_MCC_USA_3			312		/*USA = 312*/
#define CALL_NETWORK_MCC_CANADA			302		/*Canada = 302*/
#define CALL_NETWORK_MCC_BRASIL			724		/*Brasil = 724*/
#define CALL_NETWORK_MCC_MEXICO			334		/*Mexico = 334*/
#define CALL_NETWORK_MCC_URGUAY			748		/*Urguay = 748*/
#define CALL_NETWORK_MCC_COLOMBIA		732		/*Colombia = 732*/
#define CALL_NETWORK_MCC_CHILE			730		/*Chile = 730*/
#define CALL_NETWORK_MCC_PERU			716		/*Peru = 716*/
#define CALL_NETWORK_MCC_VENEZUELA		734		/*Venezuela = 734*/
#define CALL_NETWORK_MCC_GUATEMALA		704		/*Guatemala = 704*/
#define CALL_NETWORK_MCC_ELSALVADOR		706		/*El Salvador = 706*/
#define CALL_NETWORK_MCC_NICARAGUA		710		/*Nicaragua = 710*/
#define CALL_NETWORK_MCC_PANAMA			714		/*Panama = 714*/
#define CALL_NETWORK_MCC_BOLIVIA		736		/*Bolivia = 714*/

#define CALL_NETWORK_MCC_JAPAN			440		/*Japan = 440*/
#define CALL_NETWORK_MCC_JAPAN_2		441		/*Japan = 441*/
#define CALL_NETWORK_MCC_KOREA			450		/*Korae = 450*/
#define CALL_NETWORK_MCC_CHINA			460		/*China = 460*/

#define CALL_NETWORK_MCC_FRANCE			208		/*France = 208*/
#define CALL_NETWORK_MCC_PORTUGAL		268		/*Protugal = 268*/
#define CALL_NETWORK_MCC_ROMANIA		226		/*Romania = 226*/
#define CALL_NETWORK_MCC_DE				262		/*DE = 262(0x106)*/

#define CALL_NETWORK_MCC_UZB			262		/*Uzbekistan = 434*/

#define CALL_NETWORK_MCC_TEST_USA		0x001	/*Test MCC*/

/* MNC */
#define CALL_NETWORK_MNC_00			0		/*MNC 00*/
#define CALL_NETWORK_MNC_01			1		/*MNC 01*/
#define CALL_NETWORK_MNC_02			2		/*MNC 02*/
#define CALL_NETWORK_MNC_03			3		/*MNC 03*/
#define CALL_NETWORK_MNC_04			4		/*MNC 04*/
#define CALL_NETWORK_MNC_05			5		/*MNC 05*/
#define CALL_NETWORK_MNC_06			6		/*MNC 06*/
#define CALL_NETWORK_MNC_07			7		/*MNC 07*/
#define CALL_NETWORK_MNC_10			10		/*MNC 10*/
#define CALL_NETWORK_MNC_11			11		/*MNC 11	for +Movil in Panama*/
#define CALL_NETWORK_MNC_12			12		/*MNC 12 */
#define CALL_NETWORK_MNC_20			20		/*MNC 20	for TELCEL in Mexico*/
#define CALL_NETWORK_MNC_123		123		/*MNC 123	for Movistar in Colombia*/
#define CALL_NETWORK_MNC_103		103		/*MNC 103	for TIGO in Colombia*/
#define CALL_NETWORK_MNC_111		111		/*MNC 111	for TIGO in Colombia*/
#define CALL_NETWORK_MNC_187		187		/*MNC 187*/
#define CALL_NETWORK_MNC_130		130		/*MNC 130*/
#define CALL_NETWORK_MNC_30			30		/*MNC 30	for Movistar in Nicaragua*/
#define CALL_NETWORK_MNC_300		300		/*MNC 30	for Movistar in Nicaragua*/
#define CALL_NETWORK_MNC_TEST_USA	0x01	/*Test MNC*/



// Below are internal data for callmgr-telephony.c
// callmgr-core.c will access below data through get/set api from callmgr-telephony.h
typedef struct __callmgr_telephony *callmgr_telephony_t;

typedef struct __cm_tel_sat_data cm_telephony_sat_data_t;
typedef struct __cm_tel_call_data cm_telephony_call_data_t;

/*This enum is inline with cm_call_type_e and callmgr_call_type_e, so update all enum's together*/
typedef enum {
	CM_TEL_CALL_TYPE_CS_VOICE,		/**< CS Voice call type. */
	CM_TEL_CALL_TYPE_CS_VIDEO,		/**< CS Video call type. */
	CM_TEL_CALL_TYPE_VSHARE_RX,		/**< Video Share RX type. */
	CM_TEL_CALL_TYPE_VSHARE_TX,		/**< Video Share TX type. */
	CM_TEL_CALL_TYPE_PS_VOICE,		/**< PS Voice call type. */
	CM_TEL_CALL_TYPE_PS_VIDEO,		/**< PS Video call type. */
	CM_TEL_CALL_TYPE_E911,			/**< E911 call type. */
	CM_TEL_CALL_TYPE_INVALID,		/** Invalid Call Type */
} cm_telephony_call_type_e;

/*This enum is inline with cm_call_state_e, so update both enum's together*/
typedef enum {
	CM_TEL_CALL_STATE_IDLE,			/**< Call is in idle state */
	CM_TEL_CALL_STATE_ACTIVE,		/**< Call is in connected and conversation state */
	CM_TEL_CALL_STATE_HELD,			/**< Call is in held state */
	CM_TEL_CALL_STATE_DIALING,		/**< Call is in dialing state */
	CM_TEL_CALL_STATE_ALERT,		/**< Call is in alerting state */
	CM_TEL_CALL_STATE_INCOMING,		/**< Call is in incoming state */
	CM_TEL_CALL_STATE_WAITING,		/**< Call is in answered state, and waiting for connected indication event */
	CM_TEL_CALL_STATE_MAX			/**< Call state unknown */
} cm_telephony_call_state_e;

/**
 * This enum is inline with cm_call_answer_type_e, so update both enum's together
 */
typedef enum {
	CM_TEL_CALL_ANSWER_TYPE_NORMAL = 0,				/**< Only single call exist, Accept the Incoming call*/
	CM_TEL_CALL_ANSWER_TYPE_HOLD_ACTIVE_AND_ACCEPT,	/**< Put the active call on hold and accepts the call*/
	CM_TEL_CALL_ANSWER_TYPE_RELEASE_ACTIVE_AND_ACCEPT,	/**< Releases the active call and accept the call*/
	CM_TEL_CALL_ANSWER_TYPE_RELEASE_HOLD_AND_ACCEPT,	/**< Releases the held call and accept the call*/
	CM_TEL_CALL_ANSWER_TYPE_RELEASE_ALL_AND_ACCEPT		/**< Releases all calls and accept the call*/
} cm_telephony_call_answer_type_e;

/**
 * This enum is inline with cm_call_release_type_e, so update both enum's together
 */
typedef enum {
	CM_TEL_CALL_RELEASE_TYPE_BY_CALL_HANDLE = 0, /**< Release call using given call_handle*/
	CM_TEL_CALL_RELEASE_TYPE_ALL_CALLS,			/**< Release all Calls*/
	CM_TEL_CALL_RELEASE_TYPE_ALL_HOLD_CALLS,	/**< Releases all hold calls*/
	CM_TEL_CALL_RELEASE_TYPE_ALL_ACTIVE_CALLS,	/**< Releases all active calls*/
} cm_telephony_call_release_type_e;

typedef enum {
	CM_TEL_CALL_ERR_NONE,
	CM_TEL_CALL_ERR_FDN_ONLY,
	CM_TEL_CALL_ERR_DIAL_FAIL,
	CM_TEL_CALL_ERR_FM_OFF_FAIL,		/* Fail to Flight Mode*/
	CM_TEL_CALL_ERR_REJ_SAT_CALL_CTRL,
}cm_telephony_call_err_cause_type_e;

typedef enum {
	CM_TEL_DTMF_SEND_SUCCESS,
	CM_TEL_DTMF_SEND_FAIL,
}cm_telephony_dtmf_result_e;

typedef enum {
	CM_TEL_CALL_DIRECTION_MO,
	CM_TEL_CALL_DIRECTION_MT,
} cm_telephony_call_direction_e;

typedef enum {
	CM_TELEPHONY_SIM_1 = 0,
	CM_TELEPHONY_SIM_2,
	CM_TELEPHONY_SIM_UNKNOWN,
} cm_telepony_sim_slot_type_e;

typedef enum {
	CM_TEL_PREFERRED_SIM_1_E = 0,
	CM_TEL_PREFERRED_SIM_2_E,
	CM_TEL_PREFERRED_CURRENT_NETWORK_E,
	CM_TEL_PREFERRED_ALWAYS_ASK_E,
	CM_TEL_PREFERRED_UNKNOWN_E,
} cm_telepony_preferred_sim_type_e;

typedef enum {
	CM_CARD_TYPE_UNKNOWN, /**< Unknown card */
	CM_CARD_TYPE_GSM, /**< SIM(GSM) card */
	CM_CARD_TYPE_USIM, /**< USIM card */
} cm_telephony_card_type_e;

typedef enum {
	CM_NETWORK_STATUS_OUT_OF_SERVICE,
	CM_NETWORK_STATUS_IN_SERVICE_2G,
	CM_NETWORK_STATUS_IN_SERVICE_3G,
	CM_NETWORK_STATUS_EMERGENCY_ONLY,
} cm_network_status_e;

typedef enum {
	CM_TELEPHONY_EVENT_MIN,	/* 0 */

	CM_TELEPHONY_EVENT_IDLE,   /* 1 */
	CM_TELEPHONY_EVENT_ACTIVE,
	CM_TELEPHONY_EVENT_HELD,
	CM_TELEPHONY_EVENT_DIALING,
	CM_TELEPHONY_EVENT_ALERT,

	CM_TELEPHONY_EVENT_INCOMING,	 /* 6 */
	CM_TELEPHONY_EVENT_WAITING,
	CM_TELEPHONY_EVENT_JOIN,
	CM_TELEPHONY_EVENT_SPLIT,
	CM_TELEPHONY_EVENT_SWAP,

	CM_TELEPHONY_EVENT_RETRIEVED,	 /* 11 */
	CM_TELEPHONY_EVENT_NETWORK_ERROR,
	CM_TELEPHONY_EVENT_SAT_SETUP_CALL,
	CM_TELEPHONY_EVENT_SAT_SEND_DTMF,
	CM_TELEPHONY_EVENT_SAT_CALL_CONTROL_RESULT,

	CM_TELEPHON_EVENT_DTMF_START_ACK,	/* 16 */
	CM_TELEPHON_EVENT_DTMF_STOP_ACK,

	/* Do not update call data for below events */
	CM_TELEPHONY_EVENT_SOUND_WBAMR = 51, 	/* 51*/
	CM_TELEPHONY_EVENT_SOUND_CLOCK_STATUS,
	CM_TELEPHONY_EVENT_SOUND_RINGBACK_TONE,
	CM_TELEPHONY_EVENT_SS_INFO,
	CM_TELEPHONY_EVENT_FLIGHT_MODE_TIME_OVER_E,
	CM_TELEPHONY_EVENT_PREFERRED_SIM_CHANGED,
} cm_telephony_event_type_e;

typedef enum {
	CM_MO_CODE_UNCONDITIONAL_CF_ACTIVE,
	CM_MO_CODE_SOME_CF_ACTIVE,
	CM_MO_CODE_CALL_FORWARDED,
	CM_MO_CODE_CALL_IS_WAITING,
	CM_MO_CODE_CUG_CALL,
	CM_MO_CODE_OUTGOING_CALLS_BARRED,
	CM_MO_CODE_INCOMING_CALLS_BARRED,
	CM_MO_CODE_CLIR_SUPPRESSION_REJECTED,
	CM_MO_CODE_CALL_DEFLECTED,

	CM_MT_CODE_FORWARDED_CALL,
	CM_MT_CODE_CUG_CALL,
	CM_MT_CODE_CALL_ON_HOLD,
	CM_MT_CODE_CALL_RETRIEVED,
	CM_MT_CODE_MULTI_PARTY_CALL,
	CM_MT_CODE_ON_HOLD_CALL_RELEASED,
	CM_MT_CODE_FORWARD_CHECK_RECEIVED,
	CM_MT_CODE_CALL_CONNECTING_ECT,
	CM_MT_CODE_CALL_CONNECTED_ECT,
	CM_MT_CODE_DEFLECTED_CALL,
	CM_MT_CODE_ADDITIONAL_CALL_FORWARDED,
} cm_telephony_ss_info_type;

typedef enum {
	CM_TAPI_SOUND_PATH_HANDSET			=0x01,		/**<Audio path is handset*/
	CM_TAPI_SOUND_PATH_HEADSET	        =0x02,		/**<Audio path is handset*/
	CM_TAPI_SOUND_PATH_HANDSFREE	        =0x03,		/**<Audio path is Handsfree*/
	CM_TAPI_SOUND_PATH_BLUETOOTH	        =0x04,	/**<Audio path is bluetooth*/
	CM_TAPI_SOUND_PATH_STEREO_BLUETOOTH   =0x05,	/**<Audio path is stereo bluetooth*/
	CM_TAPI_SOUND_PATH_SPK_PHONE	        =0x06,	/**<Audio path is speaker phone*/
	CM_TAPI_SOUND_PATH_HEADSET_3_5PI	    =0x07,	/**<Audio path is headset_3_5PI*/
	CM_TAPI_SOUND_PATH_BT_NSEC_OFF	    =0x08,
	CM_TAPI_SOUND_PATH_MIC1		    =0x09,
	CM_TAPI_SOUND_PATH_MIC2		    =0x0A,
	CM_TAPI_SOUND_PATH_HEADSET_HAC	    =0x0B,
} cm_telephony_audio_path_type_e;

typedef enum {
	CM_INCALL_SS_NONE_E, /**< Idle*/
	CM_INCALL_SS_0_E, /**< Releases all held calls or Set UDUB for a waiting call*/
	CM_INCALL_SS_1_E, /**< Releases all active calls and accepts the other(held or waiting) calls*/
	CM_INCALL_SS_1X_E, /**< Releases a specific active call X*/
	CM_INCALL_SS_2_E, /**< Places all active calls (if  any exist) on hold and accepts the other(held or waiting)call*/
	CM_INCALL_SS_2X_E, /**< Places all active calls on hold except call X with which communication shall be supported*/
	CM_INCALL_SS_3_E, /**< Adds a held call to the conversation*/
	CM_INCALL_SS_4_E, /**< ECT */
	CM_INCALL_SS_USSD_E /**< USSD */
} cm_telephony_incall_ss_type_e;

typedef enum {
	CM_TELEPHONY_ENDCAUSE_CALL_ENDED,						/**< Call ended */

	CM_TELEPHONY_ENDCAUSE_CALL_DISCONNECTED,				/**< Call disconnected */
	CM_TELEPHONY_ENDCAUSE_CALL_SERVICE_NOT_ALLOWED,		/**< Service not allowed */
	CM_TELEPHONY_ENDCAUSE_CALL_BARRED,						/**< Call barred */
	CM_TELEPHONY_ENDCAUSE_NO_SERVICE,						/**< No Service */
	CM_TELEPHONY_ENDCAUSE_NW_BUSY,							/**< Network busy */

	CM_TELEPHONY_ENDCAUSE_NW_FAILED,						/**< Network failed */
	CM_TELEPHONY_ENDCAUSE_NO_ANSWER,						/**< No anwer from other party */
	CM_TELEPHONY_ENDCAUSE_NO_CREDIT,						/**< No credit available */
	CM_TELEPHONY_ENDCAUSE_REJECTED,							/**< Call rejected */
	CM_TELEPHONY_ENDCAUSE_USER_BUSY,						/**< user busy */

	CM_TELEPHONY_ENDCAUSE_WRONG_GROUP,					/**< Wrong group */
	CM_TELEPHONY_ENDCAUSE_CALL_NOT_ALLOWED,				/**< Call not allowed */
	CM_TELEPHONY_ENDCAUSE_TAPI_ERROR,						/**< Tapi error */
	CM_TELEPHONY_ENDCAUSE_CALL_FAILED,						/**< Call Failed */
	CM_TELEPHONY_ENDCAUSE_NO_USER_RESPONDING,				/**< User not responding */

	CM_TELEPHONY_ENDCAUSE_USER_ALERTING_NO_ANSWER,		/**< User Alerting No Answer */
	CM_TELEPHONY_ENDCAUSE_SERVICE_TEMP_UNAVAILABLE,		/**< Circuit Channel Unavailable,Network is out of Order,Switching equipment congestion,Temporary Failure */
	CM_TELEPHONY_ENDCAUSE_USER_UNAVAILABLE,				/**< Called Party Rejects the Call */
	CM_TELEPHONY_ENDCAUSE_INVALID_NUMBER_FORMAT,			/**< Entered number is invalid or incomplete */
	CM_TELEPHONY_ENDCAUSE_NUMBER_CHANGED,				/**< Entered number has been changed */

	CM_TELEPHONY_ENDCAUSE_UNASSIGNED_NUMBER,				/**< Unassigned/Unallocated number*/
	CM_TELEPHONY_ENDCAUSE_USER_DOESNOT_RESPOND,			/**< Called Party does not respond*/
	CM_TELEPHONY_ENDCAUSE_IMEI_REJECTED,			/**< Called Party does not respond*/
	CM_TELEPHONY_ENDCAUSE_FIXED_DIALING_NUMBER_ONLY,			/**< FDN Number only */
	CM_TELEPHONY_ENDCAUSE_REJ_SAT_CALL_CTRL,				/**< SAT call control reject */
	CM_TELEPHONY_ENDCAUSE_MAX				/**< max end cause*/
}cm_telephony_end_cause_type_e;

typedef enum {
	CM_TEL_NAME_MODE_NONE = 0x00,	/**<  Caller Name none*/
	CM_TEL_NAME_MODE_UNKNOWN = 0x01,	/**<  Caller Name Unavailable*/
	CM_TEL_NAME_MODE_PRIVATE = 0x02,		/**<  Caller Name Rejected by the caller*/
	CM_TEL_NAME_MODE_PAYPHONE = 0x03,		/**<  Caller using Payphone*/
	CM_TEL_NAME_MODE_MAX
} cm_telephony_name_mode_e;

typedef enum {
	CM_TEL_INVALID,
	CM_TEL_END_ACTIVE_AFTER_HELD_END,
	CM_TEL_HOLD_ACTIVE_AFTER_HELD_END
} cm_telephony_answer_request_type_e;

typedef enum {
	CM_TELEPHONY_SAT_EVENT_NONE = 0x00,
	CM_TELEPHONY_SAT_EVENT_SETUP_CALL,					/**< Sat setup call request */
	CM_TELEPHONY_SAT_EVENT_SEND_DTMF,						/**< Sat send dtmf request */
	CM_TELEPHONY_SAT_EVENT_CALL_CONTROL_RESULT,
} cm_telephony_sat_event_type_e;

typedef enum {
	CM_TELEPHONY_SAT_RESPONSE_NONE = 0x00,
	CM_TELEPHONY_SAT_RESPONSE_ME_UNABLE_TO_PROCESS_COMMAND,						/**< Unable to process command */
	CM_TELEPHONY_SAT_RESPONSE_NETWORK_UNABLE_TO_PROCESS_COMMAND,					/**< Network unable to process command */
	CM_TELEPHONY_SAT_RESPONSE_NETWORK_UNABLE_TO_PROCESS_COMMAND_WITHOUT_CAUSE,  /**< Network unable to process command without cause */
	CM_TELEPHONY_SAT_RESPONSE_ME_CONTROL_PERMANENT_PROBLEM,							/**< Control permanent problem */
	CM_TELEPHONY_SAT_RESPONSE_ME_CLEAR_DOWN_BEFORE_CONN,							/**< Clear down before connection */
	CM_TELEPHONY_SAT_RESPONSE_ME_RET_SUCCESS											/**< Return success */
} cm_telephony_sat_response_type_e;

typedef enum {
	CM_TELEPHONY_SAT_CALL_CTRL_R_ALLOWED_NO_MOD = 0x00,
	CM_TELEPHONY_SAT_CALL_CTRL_R_NOT_ALLOWED = 0x01,
	CM_TELEPHONY_SAT_CALL_CTRL_R_ALLOWED_MOD = 0x02,
	CM_TELEPHONY_SAT_CALL_CTRL_RESERVED = 0xFF,
} cm_telephony_sat_call_ctrl_type_e;	// in sync with TelSatCallCtrlResultType_t in libslp-tapi

typedef enum {
	CM_TELEPHONY_SAT_SETUP_CALL_IF_ANOTHER_CALL_NOT_BUSY = 0x00, /**< command qualifier for SETUP CALL IF ANOTHER CALL IS NOT BUSY */
	CM_TELEPHONY_SAT_SETUP_CALL_IF_ANOTHER_CALL_NOT_BUSY_WITH_REDIAL = 0x01, /**< command qualifier for SETUP CALL IF ANOTHER CALL IS NOT BUSY WITH REDIAL */
	CM_TELEPHONY_SAT_SETUP_CALL_PUT_ALL_OTHER_CALLS_ON_HOLD = 0x02, /**< command qualifier for SETUP CALL PUTTING ALL OTHER CALLS ON HOLD */
	CM_TELEPHONY_SAT_SETUP_CALL_PUT_ALL_OTHER_CALLS_ON_HOLD_WITH_REDIAL = 0x03, /**< command qualifier for SETUP CALL PUTTING ALL OTHER CALLS ON HOLD WITH REDIAL */
	CM_TELEPHONY_SAT_SETUP_CALL_DISCONN_ALL_OTHER_CALLS = 0x04, /**< command qualifier for SETUP CALL DISCONNECTING ALL OTHER CALLS */
	CM_TELEPHONY_SAT_SETUP_CALL_DISCONN_ALL_OTHER_CALLS_WITH_REDIAL = 0x05, /**< command qualifier for SETUP CALL DISCONNECTING ALL OTHER CALLS WITH REDIAL */
	CM_TELEPHONY_SAT_SETUP_CALL_RESERVED = 0xFF /**< command qualifier for SETUP CALL RESERVED */
} cm_telephony_sat_setup_call_type_e;	// in sync with TelSatCmdQualiSetupCall_t in libslp-tapi


typedef void (*telephony_event_cb) (cm_telephony_event_type_e event_type, void *event_data, void *user_data);

int _callmgr_telephony_init(callmgr_telephony_t *telephony_handle, telephony_event_cb cb_fn, void *user_data);
int _callmgr_telephony_deinit(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_set_active_sim_slot(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e active_slot);
int _callmgr_telephony_get_active_sim_slot(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e *active_slot);
int _callmgr_telephony_get_preferred_sim_type(callmgr_telephony_t telephony_handle, cm_telepony_preferred_sim_type_e *preferred_sim);
int _callmgr_telephony_is_sim_initialized(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e sim_slot, gboolean *is_initialized);
int _callmgr_telephony_is_in_service(callmgr_telephony_t telephony_handle, cm_telepony_sim_slot_type_e sim_slot, gboolean *is_in_service);
int _callmgr_telephony_call_new(callmgr_telephony_t telephony_handle, cm_telephony_call_type_e type,
		cm_telephony_call_direction_e direction, const char* number, cm_telephony_call_data_t **call_data_out);
int _callmgr_telephony_call_delete(callmgr_telephony_t telephony_handle, unsigned int call_id);
int _callmgr_telephony_dial(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t *call);
int _callmgr_telephony_end_call(callmgr_telephony_t telephony_handle, unsigned int call_id, cm_telephony_call_release_type_e release_type);
int _callmgr_telephony_join_call(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_split_call(callmgr_telephony_t telephony_handle, unsigned int call_id);
int _callmgr_telephony_swap_call(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_transfer_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t *call_data);
int _callmgr_telephony_answer_call(callmgr_telephony_t telephony_handle, int ans_type);
int _callmgr_telephony_reject_call(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_hold_call(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_active_call(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_start_dtmf(callmgr_telephony_t telephony_handle, unsigned char dtmf_digit);
int _callmgr_telephony_stop_dtmf(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_burst_dtmf(callmgr_telephony_t telephony_handle, const char *dtmf_digits);
void _callmgr_telephony_send_sat_response(callmgr_telephony_t telephony_handle, cm_telephony_sat_event_type_e event_type,
		cm_telephony_sat_response_type_e resp_type, cm_telepony_sim_slot_type_e active_sim_slot);

/*Start of __cm_call_data get api's*/
int _callmgr_telephony_get_call_id(cm_telephony_call_data_t *call, unsigned int *call_id);
int _callmgr_telephony_get_call_direction(cm_telephony_call_data_t *call, cm_telephony_call_direction_e *call_direction);
int _callmgr_telephony_set_call_number(cm_telephony_call_data_t *call, char *call_number);
int _callmgr_telephony_get_call_number(cm_telephony_call_data_t *call, char **call_number);
int _callmgr_telephony_set_dtmf_number(cm_telephony_call_data_t *call, char *dtmf_number);
int _callmgr_telephony_get_dtmf_number(cm_telephony_call_data_t *call, char **dtmf_number);
int _callmgr_telephony_get_call_type(cm_telephony_call_data_t *call, cm_telephony_call_type_e *call_type);
int _callmgr_telephony_get_call_state(cm_telephony_call_data_t *call, cm_telephony_call_state_e *call_state);
int _callmgr_telephony_get_is_conference(cm_telephony_call_data_t *call, gboolean *is_conference);
int _callmgr_telephony_get_conference_call_count(callmgr_telephony_t telephony_handle, int *member_cnt);
int _callmgr_telephony_get_is_ecc(cm_telephony_call_data_t *call, gboolean *is_ecc);
int _callmgr_telephony_get_call_start_time(cm_telephony_call_data_t *call, long *start_time);
int _callmgr_telephony_get_call_end_cause(cm_telephony_call_data_t *call, cm_telephony_end_cause_type_e *cause);
int _callmgr_telephony_get_call_name_mode(cm_telephony_call_data_t *call, cm_telephony_name_mode_e *name_mode);
/*End of __cm_call_data get api's*/

int _callmgr_telephony_set_ecc_category(cm_telephony_call_data_t *call, int ecc_category);
int _callmgr_telephony_has_call_by_state(callmgr_telephony_t telephony_handle, cm_telephony_call_state_e state, gboolean *b_has_call);
int _callmgr_telephony_get_call_by_state(callmgr_telephony_t telephony_handle, cm_telephony_call_state_e state, cm_telephony_call_data_t **call_data_out);
int _callmgr_telephony_get_sat_event_status(callmgr_telephony_t telephony_handle, cm_telephony_sat_event_type_e event_type, gboolean *b_is_ongoing);
int _callmgr_telephony_get_sat_setup_call_type(callmgr_telephony_t telephony_handle, cm_telephony_sat_setup_call_type_e *sat_setup_call_type);
int _callmgr_telephony_get_sat_originated_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out);
int _callmgr_telephony_get_sat_call_control_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t **call_data_out);
int _callmgr_telephony_set_sat_originated_call_flag(cm_telephony_call_data_t *call, gboolean is_sat_originated_call);
int _callmgr_telephony_get_sat_setup_call_number(callmgr_telephony_t telephony_handle, char **call_number);
int _callmgr_telephony_get_sat_setup_call_name(callmgr_telephony_t telephony_handle, char **call_name);
int _callmgr_telephony_get_video_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t ** call_data_out);
int _callmgr_telephony_get_voice_call(callmgr_telephony_t telephony_handle, cm_telephony_call_data_t ** call_data_out);
int _callmgr_telephony_get_call_by_call_id(callmgr_telephony_t telephony_handle, unsigned int call_id, cm_telephony_call_data_t **call_data_out);
int _callmgr_telephony_get_call_count(callmgr_telephony_t telephony_handle, int *call_count_out);
int _callmgr_telephony_get_card_type(callmgr_telephony_t telephony_handle, cm_telephony_card_type_e *card_type_out);
int _callmgr_telephony_get_network_status(callmgr_telephony_t telephony_handle, cm_network_status_e *netk_status_out);
int _callmgr_telephony_get_modem_count(callmgr_telephony_t telephony_handle, int *modem_cnt_out);
int _callmgr_telephony_is_no_sim(callmgr_telephony_t telephony_handle, gboolean *is_no_sim);
int _callmgr_telephony_is_phone_initialized(callmgr_telephony_t telephony_handle, gboolean *is_initialized);
int _callmgr_telephony_is_flight_mode_enabled(callmgr_telephony_t telephony_handle, gboolean *is_enabled);
int _callmgr_telephony_is_cs_available(callmgr_telephony_t telephony_handle, gboolean *is_available);
int _callmgr_telephony_get_call_list(callmgr_telephony_t telephony_handle, GSList **call_list);
int _callmgr_telephony_set_audio_path(callmgr_telephony_t telephony_handle, cm_telephony_audio_path_type_e path, gboolean is_extra_vol);
int _callmgr_telephony_set_audio_tx_mute(callmgr_telephony_t telephony_handle, gboolean is_mute_state);
int _callmgr_telephony_reset_call_start_time(cm_telephony_call_data_t *call);
int _callmgr_telephony_get_imsi_mcc_mnc(callmgr_telephony_t telephony_handle, unsigned long *mcc, unsigned long *mnc);
int _callmgr_telephony_get_mcc_mnc(callmgr_telephony_t telephony_handle,unsigned long *mcc, unsigned long *mnc);
int _callmgr_telephony_check_ecc_number(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category);
int _callmgr_telephony_is_normal_setup_ecc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_normal_ecc);
int _callmgr_telephony_check_ecc_nosim_3gpp(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category);
int _callmgr_telephony_check_ecc_3gpp(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category);
int _callmgr_telephony_check_sim_ecc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_sim_ecc, int *ecc_category);
int _callmgr_telephony_process_incall_ss(callmgr_telephony_t telephony_handle, const char *number);
int _callmgr_telephony_disable_flight_mode(callmgr_telephony_t telephony_handle);
int _callmgr_telephony_set_modem_volume(callmgr_telephony_t telephony_handle, cm_telephony_audio_path_type_e path_type, int volume);
int _callmgr_telephony_is_ss_string(callmgr_telephony_t telephony_handle, int slot_id, const char *number, gboolean *o_is_incall_ss);
int _callmgr_telephony_is_number_valid(const char *number, gboolean *b_number_valid);
int _callmgr_telephony_is_answer_requesting(callmgr_telephony_t telephony_handle, gboolean *o_is_requesting);
int _callmgr_telephony_get_answer_request_type(callmgr_telephony_t telephony_handle, cm_telephony_answer_request_type_e *o_is_end_active_after_held_end);
int _callmgr_telephony_set_answer_request_type(callmgr_telephony_t telephony_handle, cm_telephony_answer_request_type_e is_active_end);
int _callmgr_telephony_set_calling_name(cm_telephony_call_data_t *call, char * calling_name);
int _callmgr_telephony_get_calling_name(cm_telephony_call_data_t *call, char **calling_name);
/* Remove ecc korea code */
int _callmgr_telephony_check_ecc_nosim_mcc_mnc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category);
int _callmgr_telephony_check_ecc_mcc_mnc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category);
#endif	//__CALLMGR_TELEPHONY_H__
