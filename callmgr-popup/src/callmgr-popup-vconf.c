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

#include <vconf.h>
#include <app_control.h>

#include "callmgr-popup-vconf.h"

gboolean _callmgr_popup_is_idle_lock(void)
{
	int lock_state;
	int ret = 0;

	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &(lock_state));
	if (ret < 0) {
		WARN("vconf_get_int error");
	}

	DBG("Lock state : %d", lock_state);
	if (lock_state == VCONFKEY_IDLE_LOCK) {
		return TRUE;
	} else {
		return FALSE;
	}
}

gboolean _callmgr_popup_is_pw_lock(void)
{
	int pwlock_state = -1;

	if (vconf_get_int(VCONFKEY_PWLOCK_STATE, &pwlock_state) < 0) {
		WARN("vconf_get_int failed.");
		return FALSE;
	}

	DBG("pwlock_state:[%d]", pwlock_state);
	if ((pwlock_state == VCONFKEY_PWLOCK_BOOTING_LOCK)
		|| (pwlock_state == VCONFKEY_PWLOCK_RUNNING_LOCK)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

gboolean _callmgr_popup_is_flight_mode(void)
{
	gboolean flight_mode = FALSE;

	if (vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &flight_mode) < 0) {
		ERR("Get vconf failed");
	}

	DBG("Flight mode : %d", flight_mode);
	return flight_mode;
}
