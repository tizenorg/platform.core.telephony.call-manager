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

#ifndef _CALLMGR_POPUP_UTIL_H_
#define _CALLMGR_POPUP_UTIL_H_

#include "callmgr-popup-debug.h"
#include "callmgr-popup-common.h"

char *_callmgr_popup_get_ss_info_string(callmgr_ss_info_type_e type);
char *_callmgr_popup_get_call_err_string(callmgr_call_err_type_e type, char *number);
void _callmgr_popup_reply_to_launch_request(CallMgrPopAppData_t *ad, char *key, char *value);

#endif	/* _CALLMGR_POPUP_UTIL_H_ */

