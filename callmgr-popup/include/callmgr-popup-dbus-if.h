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

#ifndef _CALLMGR_POPUP_DBUS_H_
#define _CALLMGR_POPUP_DBUS_H_

#include <glib.h>

#include "callmgr-popup-debug.h"

int _callmgr_popup_dbus_init(void *user_data);
int _callmgr_popup_dbus_deinit(void *user_data);

int _callmgr_popup_dial_call(char *number, int call_type, int sim_slot, int disable_fm, gboolean is_emergency_contact, void *user_data);

#endif	/* _CALLMGR_POPUP_DBUS_H_ */

