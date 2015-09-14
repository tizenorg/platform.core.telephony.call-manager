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
#include <stdlib.h>
#include <vconf.h>

#include "callmgr-log.h"
#include "callmgr-vconf.h"

int _callmgr_vconf_is_data_lock(gboolean *is_datalock)
{
	return -1;
}

int _callmgr_vconf_get_salescode(int *sales_code)
{
	return -1;
}

int _callmgr_vconf_is_cradle_status(gboolean *is_connected)
{
	return -1;
}

int _callmgr_vconf_is_bike_mode_enabled(gboolean *is_bike_mode_enabled)
{
	return -1;
}

int _callmgr_vconf_get_answer_message_path(char **answer_msg_path)
{
	*answer_msg_path = NULL;
	return -1;
}

int _callmgr_vconf_is_answer_mode_enabled(gboolean *is_answer_mode_enabled)
{
	return -1;
}

int _callmgr_vconf_get_auto_answer_time(int *auto_answer_time_in_sec)
{
	return -1;
}

int _callmgr_vconf_init_palm_touch_mute_enabled(void *callback, void *data)
{
	return -1;
}

int _callmgr_vconf_deinit_palm_touch_mute_enabled(void *callback)
{
	return -1;
}

int _callmgr_vconf_is_test_auto_answer_mode_enabled(gboolean *is_auto_answer)
{
	return -1;
}

int _callmgr_vconf_is_recording_reject_enabled(gboolean *is_rec_reject_enabled)
{
	return -1;
}

int _callmgr_vconf_is_ui_visible(gboolean *o_is_ui_visible)
{
	return -1;
}

int _callmgr_vconf_is_do_not_disturb(gboolean *do_not_disturb)
{
	return -2;
}
