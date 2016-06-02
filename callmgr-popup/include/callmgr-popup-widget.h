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

#ifndef _CALLMGR_POPUP_WIDGET_H_
#define _CALLMGR_POPUP_WIDGET_H_

#include <Elementary.h>
#include <glib.h>
#include <vconf.h>

#include "callmgr-popup-debug.h"

Evas_Object *_callmgr_popup_create_win(void);
void _callmgr_popup_system_grab_key(void *data);
void _callmgr_popup_system_ungrab_key(void *data);

void _callmgr_popup_del_popup(void *data);

Evas_Object *_callmgr_popup_create_progressbar(Evas_Object *parent, const char *type);
void _callmgr_popup_create_popup_checking(void *data, char *string);
void _callmgr_popup_create_flight_mode(void *data);
void _callmgr_popup_create_toast_msg(char *string);
void _callmgr_popup_create_sim_selection(void *data);
void _callmgr_popup_create_try_voice_call(void *data);

#endif	/* _CALLMGR_POPUP_WIDGET_H_ */

