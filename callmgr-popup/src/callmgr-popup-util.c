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

#include <app_control.h>

#include "callmgr-popup-util.h"

char *_callmgr_popup_get_ss_info_string(callmgr_ss_info_type_e type)
{
	char *string = NULL;

	switch (type) {
	case SS_INFO_MO_CODE_UNCONDITIONAL_CF_ACTIVE_E:
		string = _("IDS_COM_BODY_UNCONDITIONAL_CALL_FORWARDING_ACTIVE");
	break;
	case SS_INFO_MO_CODE_SOME_CF_ACTIVE_E:
		string = _("IDS_CALL_BODY_CONDITIONAL_CALL_FORWARDING_ACTIVE");
	break;
	case SS_INFO_MO_CODE_CALL_FORWARDED_E:
		string = _("IDS_COM_BODY_CALL_FORWARDED");
	break;
	case SS_INFO_MO_CODE_CALL_IS_WAITING_E:
		string = _("IDS_COM_BODY_CALL_IS_WAITING");
	break;
	case SS_INFO_MO_CODE_CUG_CALL_E:
		string = _("IDS_COM_BODY_CUG_CALL");
	break;
	case SS_INFO_MO_CODE_OUTGOING_CALLS_BARRED_E:
		string = _("IDS_SYNCML_POP_OUTGOING_CALLS_BARRED");
	break;
	case SS_INFO_MO_CODE_INCOMING_CALLS_BARRED_E:
		string = _("IDS_CALL_BODY_INCOMING_CALLS_BARRED");
	break;
	case SS_INFO_MO_CODE_CLIR_SUPPRESSION_REJECTED_E:
		string = _("IDS_COM_BODY_CLIR_SUPPRESSION_REJECTED");
	break;
	case SS_INFO_MO_CODE_CALL_DEFLECTED_E:
		string = _("IDS_CALL_POP_CALLDEFLECTED");
	break;
	case SS_INFO_MT_CODE_FORWARDED_CALL_E:
		string = _("IDS_COM_BODY_FORWARDED_CALL");
	break;
	case SS_INFO_MT_CODE_CUG_CALL_E:
		string = _("IDS_COM_BODY_CUG_CALL");
	break;
	case SS_INFO_MT_CODE_CALL_ON_HOLD_E:
		string = _("IDS_COM_BODY_CALL_ON_HOLD");
	break;
	case SS_INFO_MT_CODE_CALL_RETRIEVED_E:
		string = _("IDS_COM_BODY_CALL_RETRIEVED");
	break;
	case SS_INFO_MT_CODE_MULTI_PARTY_CALL_E:
		string = _("IDS_COM_BODY_MULTI_PARTY_CALL");
	break;
	case SS_INFO_MT_CODE_ON_HOLD_CALL_RELEASED_E:
		string = _("IDS_COM_BODY_ON_HOLD_CALL_RELEASED");
	break;
	case SS_INFO_MT_CODE_FORWARD_CHECK_RECEIVED_E:
		string = _("IDS_CALL_BODY_FORWARD_CHECK_RECEIVED");
	break;
	case SS_INFO_MT_CODE_CALL_CONNECTING_ECT_E:
		string = _("IDS_COM_BODY_CALL_CONNECTING_ECT");
	break;
	case SS_INFO_MT_CODE_CALL_CONNECTED_ECT_E:
		string = _("IDS_COM_BODY_CALL_CONNECTED_ECT");
	break;
	case SS_INFO_MT_CODE_DEFLECTED_CALL_E:
		string = _("IDS_COM_BODY_DEFLECTED_CALL");
	break;
	case SS_INFO_MT_CODE_ADDITIONAL_CALL_FORWARDED_E:
		string = _("IDS_COM_BODY_ADDITIONAL_CALL_FORWARDED");
	break;

	default:
		WARN("Invalid info");
		string = NULL;
	}

	return g_strdup(string);
}

char *_callmgr_popup_get_call_err_string(callmgr_call_err_type_e type, char *number)
{
	char *string = NULL;

	switch (type) {
	case ERR_CALL_FAILED_E:
		string = _("IDS_CALL_POP_CALLFAILED");
		break;
	case ERR_CALL_NOT_ALLOWED_E:
		string = _("IDS_CALL_POP_CALLNOTCALLOWED");
		break;
	case ERR_EMERGENCY_ONLY_E:
		string = _("IDS_CALL_POP_CALLING_EMERG_ONLY");
		break;
	case ERR_IS_NOT_EMERGENCY_E:
		string = _("IDS_COM_BODY_UNABLE_TO_CALL_PS_IS_NOT_EMERGENCY_NUMBER");
		break;
	case ERR_WRONG_NUMBER_E:
		string = _("IDS_CALL_POP_CAUSE_WRONG_NUMBER");
		break;
	case ERR_FDN_CALL_ONLY_E:
		string = _("IDS_COM_BODY_OUTGOING_CALLS_ARE_RESTRICTED_BY_FDN");
		break;
	case ERR_OUT_OF_SERVICE_E:
		string = _("IDS_COM_BODY_OUT_OF_SERVICE_AREA");
		break;
	case ERR_FAILED_TO_ADD_CALL_E:
		string = _("IDS_CALL_BODY_FAILED_TO_ADD_CALL");
		break;
	case ERR_NOT_REGISTERED_ON_NETWORK_E:
		string = _("IDS_COM_BODY_NOT_REGISTERED_ON_NETWORK");
		break;
	case ERR_PHONE_NOT_INITIALIZED_E:
		string = _("IDS_VCALL_POP_PHONE_NOT_INITIALISED");
		break;
	case ERR_TRANSFER_CALL_FAIL_E:
		string = _("IDS_CALL_POP_TRANSFER_FAILED");
		break;
	case ERR_LOW_BATTERY_INCOMING_E:
		string = _("IDS_VCALL_TPOP_UNABLE_TO_CONNECT_VIDEO_CALL_BATTERY_POWER_LOW");
		break;
	case ERR_JOIN_CALL_FAILED_E:
		string = _("IDS_CALL_POP_UNABLE_TO_MAKE_CONFERENCE_CALLS");
		break;
	case ERR_HOLD_CALL_FAILED_E:
		string = _("IDS_COM_BODY_UNABLE_TO_SWITCH_CALLS");
		break;
	case ERR_ANSWER_CALL_FAILED_E:
		string = _("IDS_CALL_POP_OPERATION_REFUSED");
		break;
	case ERR_SWAP_CALL_FAILED_E:
	case ERR_RETREIVE_CALL_FAILED_E:
		string = _("IDS_CALL_POP_SWAP_FAILED");
		break;
	case ERR_NO_SIM_E:
		string = _("IDS_CALL_TPOP_INSERT_SIM_CARD");
		break;
	default:
		WARN("Invalid info");
		string = NULL;
		break;
	}

	if (type == ERR_IS_NOT_EMERGENCY_E) {
		return g_strdup_printf(string, number);
	} else {
		return g_strdup(string);
	}
}

void _callmgr_popup_reply_to_launch_request(CallMgrPopAppData_t *ad, char *key, char *value)
{
	app_control_h reply = NULL;
	int result = APP_CONTROL_ERROR_NONE;
	if (!ad || !key || !value) {
		ERR("invalid parameter");
		return;
	}
	result = app_control_create(&reply);
	if(result != APP_CONTROL_ERROR_NONE) {
		WARN("app_control_create() return error : %d", result);
		return;
	}

	app_control_add_extra_data(reply, key, value);
	app_control_reply_to_launch_request(reply, ad->request, APP_CONTROL_RESULT_SUCCEEDED);
	app_control_destroy(reply);
	app_control_destroy(ad->request);
	ad->request = NULL;
	return;
}
