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

#include <sound_manager_product.h>
#include <sound_manager_internal.h>

#include "callmgr-log.h"
#include "callmgr-audio.h"

int _callmgr_audio_set_link_direction_uplink(void)
{
	int ret = -1;
	dbg("_callmgr_audio_set_link_direction_uplink()");
	ret = sound_manager_set_network_link_direction(SOUND_LINK_DIRECTION_UP_LINK);
	if (ret < 0) {
		err("sound_manager_set_network_link_direction() failed");
		return -1;
	}

	return 0;
}

int _callmgr_audio_set_link_direction_downlink(void)
{
	int ret = -1;
	dbg("_callmgr_audio_set_link_direction_downlink()");
	ret = sound_manager_set_network_link_direction(SOUND_LINK_DIRECTION_DOWN_LINK);
	if (ret < 0) {
		err("sound_manager_set_network_link_direction() failed");
		return -1;
	}

	return 0;
}

int _callmgr_audio_set_media_mode_with_current_device(void)
{
	dbg(">>");
	int ret = -1;

	ret = sound_manager_set_call_session_mode(SOUND_SESSION_CALL_MODE_MEDIA_WITH_CURRENT_DEVICE);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("callmgr_audio_set_audio_route() failed:[%d]", ret);
		return -1;
	}
	return 0;
}



