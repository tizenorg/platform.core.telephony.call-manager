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

#include <recorder_product.h>
#include <phone-misc.h>

#include "callmgr-util.h"
#include "callmgr-log.h"

int _callmgr_util_is_recording_progress(gboolean *is_recording)
{
	bool b_is_voice_recording = FALSE;
	recorder_is_in_recording(RECORDER_AUDIO_SOURCE, &b_is_voice_recording);

#if 0
	/* Do not check video recording because of performance */
	bool b_is_video_recording = FALSE;
	recorder_is_in_recording(RECORDER_VIDEO_SOURCE, &b_is_video_recording);
	dbg("b_is_voice_recording = [%d] b_is_video_recording = [%d]", b_is_voice_recording, b_is_video_recording);
	*is_recording = b_is_voice_recording || b_is_video_recording;
#else
	*is_recording = b_is_voice_recording;
#endif
	return 0;
}

int _callmgr_util_is_video_recording_progress(gboolean *is_recording)
{
	bool b_is_video_recording = FALSE;

	recorder_is_in_recording(RECORDER_VIDEO_SOURCE, &b_is_video_recording);
	warn("b_is_video_recording = [%d]", b_is_video_recording);
	*is_recording = b_is_video_recording;

	return 0;
}

int _callmgr_util_check_block_mode_num(const char *str_num, gboolean *is_blocked)
{
	int result = 0;
	int ret_val = -1;
	phone_misc_h handle = NULL;
	*is_blocked = FALSE;

	ret_val = phone_misc_connect(&handle);
	if (ret_val != PH_MISC_SUCCESS) {
		err("misc connect fail(%d)", ret_val);
		result = -1;
	} else {
		int block_check = 0;
		ret_val = phone_misc_block_check(handle, str_num, &block_check);
		if (ret_val != PH_MISC_SUCCESS) {
			err("phone_misc_block_check fail(%d)", ret_val);
			result = -1;
		} else {
			if (block_check > 0) {
				*is_blocked = TRUE;
			}
		}
		phone_misc_disconnect(handle);
		handle = NULL;
	}
	info("Call Log is_blocked value:[%d]", *is_blocked);
	return result;
}
