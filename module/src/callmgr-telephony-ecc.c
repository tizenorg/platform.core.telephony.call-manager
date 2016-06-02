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

/*#include <vconf.h>*/
#include "callmgr-telephony.h"
#include "callmgr-util.h"
#include "callmgr-vconf.h"
#include "callmgr-log.h"
#include <glib.h>
#include <string.h>

#define CALL_ECC_MAX_COUNT_DEFAULT_NO_SIM		8
#define CALL_ECC_MAX_COUNT_1					1
#define CALL_ECC_MAX_COUNT_DEFAULT				2
#define CALL_ECC_MAX_COUNT_3					3
#define CALL_ECC_MAX_COUNT_4					4
#define CALL_ECC_MAX_COUNT_5					5
#define CALL_ECC_MAX_COUNT_6					6
#define CALL_ECC_MAX_COUNT_7					7
#define CALL_ECC_MAX_COUNT_8					8
#define CALL_ECC_MAX_COUNT_9					9
#define CALL_ECC_MAX_COUNT_10					10
#define CALL_ECC_MAX_COUNT_11					11
#define CALL_ECC_MAX_COUNT_14					14

#define CALL_EMERGENCY_CATEGORY_NUMBERS_CNT_JAPAN	3
#define CALL_EMERGENCY_CATEGORY_NUMBERS_CNT_KOREA	8

int __callmgr_telephony_ecc_check_emergency_number_nosim(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category);

static char *gcall_vc_ecc_numbers_default[] = {
	"112",
	"911"
};

/* No SIM - Default*/
static char *gcall_vc_ecc_numbers_nosim_default[] = {
	"000", "08", "112", "110", "118", "119", "911", "999"
};

int _callmgr_telephony_check_ecc_3gpp(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category)
{
	int i = 0;
	int ecc_count = 0;
	char **p_ecc_numbers = NULL;

	ecc_count = CALL_ECC_MAX_COUNT_DEFAULT;
	p_ecc_numbers = gcall_vc_ecc_numbers_default;	/*112, 911 */

	CM_RETURN_VAL_IF_FAIL(ecc_count > 0, -1);
	CM_RETURN_VAL_IF_FAIL(p_ecc_numbers, -1);

	for (i = 0; i < ecc_count; i++) {
		if (!g_strcmp0(pNumber, p_ecc_numbers[i])) {
			/*SEC_DBG("pNumber (%s), p_ecc_numbers[%d] (%s)", pNumber, i, p_ecc_numbers[i]);*/
			*is_ecc = TRUE;
			return 0;
		}
	}

	dbg("No emegency number...");
	*is_ecc = FALSE;
	return 0;

}

int _callmgr_telephony_check_ecc_mcc_mnc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category)
{
	int i = 0;
	int ecc_count = 0;
	char **p_ecc_numbers = NULL;
	unsigned long mcc = 0;	/* for checking Emergency number for each country */
	unsigned long mnc = 0;	/* for checking Emergency number for each operator */
	int salescode = 0;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(pNumber, -1);
	CM_RETURN_VAL_IF_FAIL(is_ecc, -1);
	CM_RETURN_VAL_IF_FAIL(ecc_category, -1);

	_callmgr_telephony_get_mcc_mnc(telephony_handle, &mcc, &mnc);
	_callmgr_vconf_get_salescode(&salescode);
	/*SEC_DBG("mcc : %ld", mcc);*/
	if (mcc == 0) {
		warn("mcc 0");
#ifdef _CDMA_FEATURE_W
		/* CDMA network MCC is 0. So use IMSI */
		unsigned long imsi_mcc = 0; /* IMSI mcc */
		unsigned long imsi_mnc = 0; /* IMSI mnc */

		_call_tapi_util_get_imsi_mcc_mnc(&imsi_mcc, &imsi_mnc);
		mcc = imsi_mcc;
#endif /* _CDMA_FEATURE_W */
	} else if (mcc == CALL_NETWORK_MCC_TEST_USA
		&& mnc == CALL_NETWORK_MNC_01
		&& salescode == CALL_CSC_BTU) {
		/* 00101 is working with UK for BTU salescode */
		/* 00101 is Orange UK on customer spec */

		warn("BTU. Test MCC");
		mcc = CALL_NETWORK_MCC_UK;
	}

	ecc_count = CALL_ECC_MAX_COUNT_DEFAULT;
	p_ecc_numbers = gcall_vc_ecc_numbers_default;	/*112, 911 */

	CM_RETURN_VAL_IF_FAIL(ecc_count > 0, -1);
	CM_RETURN_VAL_IF_FAIL(p_ecc_numbers, -1);

	for (i = 0; i < ecc_count; i++) {
		if (!g_strcmp0(pNumber, p_ecc_numbers[i])) {
			/*SEC_DBG("pNumber (%s), p_ecc_numbers[%d] (%s)", pNumber, i, p_ecc_numbers[i]);*/
			*is_ecc = TRUE;
			return 0;
		}
	}

	dbg("No emegency number...");
	*is_ecc = FALSE;
	return 0;

}

int _callmgr_telephony_check_ecc_nosim_mcc_mnc(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category)
{
	int i = 0;
	int ecc_count = 0;
	char **p_nosim_ecc_numbers = NULL;
	unsigned long mcc = 0;	/* for checking Emergency number for each country */
	unsigned long mnc = 0;	/* for checking Emergency number for each operator */

	*is_ecc = FALSE;
	*ecc_category = 0;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(pNumber, -1);
	CM_RETURN_VAL_IF_FAIL(is_ecc, -1);
	CM_RETURN_VAL_IF_FAIL(ecc_category, -1);

	if (strlen(pNumber) == 0) {
		err("pNumber's length is ZERO so return FALSE");
		return -1;
	}
	_callmgr_telephony_get_mcc_mnc(telephony_handle, &mcc, &mnc);

	/*SEC_DBG("mcc : %ld", mcc);*/
	int salescode = 0;
	_callmgr_vconf_get_salescode(&salescode);

	if(salescode == CALL_CSC_TMB) {
		/* In case that mcc, mnc value is invalid for TMO */
		warn("TMB");
		mcc = CALL_NETWORK_MCC_USA;
		mnc = 220;
	}

	ecc_count = CALL_ECC_MAX_COUNT_DEFAULT_NO_SIM;
	p_nosim_ecc_numbers = gcall_vc_ecc_numbers_nosim_default;	/*3GPP SPecs */

	CM_RETURN_VAL_IF_FAIL(ecc_count > 0, -1);
	CM_RETURN_VAL_IF_FAIL(p_nosim_ecc_numbers, -1);

	for (i = 0; i < ecc_count; i++) {
		if (!g_strcmp0(pNumber, p_nosim_ecc_numbers[i])) {
			/*SEC_DBG("pNumber (%s), p_nosim_ecc_numbers[%d] (%s)", pNumber, i, p_nosim_ecc_numbers[i]);*/
			*is_ecc = TRUE;
			return 0;
		}
	}

	dbg("No emegency number...");
	return 0;
}

int _callmgr_telephony_check_ecc_nosim_3gpp(callmgr_telephony_t telephony_handle, const char *pNumber, gboolean *is_ecc, int *ecc_category)
{
	int i = 0;
	int ecc_count = 0;
	char **p_nosim_ecc_numbers = NULL;

	CM_RETURN_VAL_IF_FAIL(telephony_handle, -1);
	CM_RETURN_VAL_IF_FAIL(pNumber, -1);
	CM_RETURN_VAL_IF_FAIL(is_ecc, -1);
	CM_RETURN_VAL_IF_FAIL(ecc_category, -1);

	if (strlen(pNumber) == 0) {
		err("pNumber's length is ZERO so return FALSE");
		*is_ecc = FALSE;
		return -1;
	}

	ecc_count = CALL_ECC_MAX_COUNT_DEFAULT_NO_SIM;
	p_nosim_ecc_numbers = gcall_vc_ecc_numbers_nosim_default;	/*3GPP SPecs */

	for (i = 0; i < ecc_count; i++) {
		if (!g_strcmp0(pNumber, p_nosim_ecc_numbers[i])) {
			/*SEC_DBG("pNumber (%s), p_nosim_ecc_numbers[%d] (%s)", pNumber, i, p_nosim_ecc_numbers[i]);*/
			*is_ecc = TRUE;
			return 0;
		}
	}

	dbg("No emegency number...");

	*is_ecc = FALSE;
	return -1;
}
