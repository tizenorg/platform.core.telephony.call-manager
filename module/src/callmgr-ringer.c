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

#include <sound_manager.h>
#include <sound_manager_internal.h>
#include <player.h>
#include <tone_player.h>
#include <vconf.h>
#include <device/haptic.h>
#include <mm_sound.h>
#include <wav_player.h>
#include <stdlib.h>

#include "callmgr-ringer.h"
#include "callmgr-util.h"
#include "callmgr-vconf.h"
#include "callmgr-log.h"

#define CALLMGR_RINGTONE_INCREMENT_TIMER_INTERVAL		2000	/**< Incremental Melody Timer Interval  */
#define CALLMGR_VIBRATION_PLAY_INTERVAL	2000
#define CALLMGR_VIBRATION_PLAY_DURATION	1500
#define CALLMGR_2ND_CALL_BEEP_INTERVAL	2500
#define CALLMGR_2ND_CALL_BEEP_COUNT		2

#define CALLMGR_PLAY_CONNECT_EFFECT_PATH		MODULE_RES_DIR "/Call_ConnectTone.ogg"
#define CALLMGR_PLAY_DISCONNECT_EFFECT_PATH		MODULE_RES_DIR "/Call_DisconnectTone.ogg"

struct __ringer_data {
	cm_ringer_status_e current_status;	/**<Current ringer status */
	sound_stream_info_h ringtone_stream_handle;

	player_h player_handle;			/**< Handle to MM Player */
	haptic_device_h haptic_handle;			 /**< Handle to System Vibration Module*/
	haptic_effect_h effect_handle;		 /**< Handle to System Vibration Effect*/
	int alternate_tone_handle;
	int alternate_tone_play_count;
	int play_effect_handle;			/**< Handle to Call Event Play Effect*/
	int ringback_tone_play_handle;		/**< Handle to Ring back tone Play Effect*/

	int signal_play_handle;			/**< Handle to MM Signal Player */
	guint signal_tone_timer;			/**< Holds a current sound play status for Signal player*/

	guint vibration_timer;
	guint increment_timer;
	guint waiting_tone_timer;
	int vibration_level;

	cm_ringer_effect_finished_cb play_effect_cb_fn;
	void *play_effect_user_data;
	cm_ringer_signal_finished_cb play_signal_cb_fn;
	void *play_signal_user_data;
};

#ifdef SUPPORT_PALM_TOUCH
static void __callmgr_ringer_palm_touch_mute_enabled_cb(keynode_t *in_key, void *data);
#endif

int _callmgr_ringer_init(callmgr_ringer_handle_h *ringer_handle)
{
	struct __ringer_data *handle = NULL;

	handle = calloc(1, sizeof(struct __ringer_data));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	handle->play_effect_handle = -1;
	handle->ringback_tone_play_handle = -1;
	handle->alternate_tone_handle = -1;

	handle->player_handle = NULL;
	handle->haptic_handle = NULL;
	handle->effect_handle = NULL;
	handle->current_status = CM_RINGER_STATUS_IDLE_E;

	*ringer_handle = handle;

#ifdef SUPPORT_PALM_TOUCH
	_callmgr_vconf_init_palm_touch_mute_enabled(__callmgr_ringer_palm_touch_mute_enabled_cb, handle);
#endif
	dbg("Ringer init done");

	return 0;
}

int _callmgr_ringer_deinit(callmgr_ringer_handle_h ringer_handle)
{
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	/* TODO: Release all handles*/

	g_free(ringer_handle);
#ifdef SUPPORT_PALM_TOUCH
	_callmgr_vconf_deinit_palm_touch_mute_enabled(__callmgr_ringer_palm_touch_mute_enabled_cb);
#endif
	return 0;
}

int _callmgr_ringer_get_ringer_status(callmgr_ringer_handle_h ringer_handle, cm_ringer_status_e *status)
{
	CM_RETURN_VAL_IF_FAIL(status, -1);
	warn("current status [%d]", ringer_handle->current_status);
	*status = ringer_handle->current_status;
	return 0;
}

#ifdef SUPPORT_PALM_TOUCH
static void __callmgr_ringer_palm_touch_mute_enabled_cb(keynode_t *in_key, void *data)
{
	const char *name = NULL;
	int palm_mute_enabled = 0;
	callmgr_ringer_handle_h ringer_handle = (callmgr_ringer_handle_h)data;

	name = vconf_keynode_get_name(in_key);
	if (name == NULL) {
		err("vconf key value for parm touch mute is invalid");
		return;
	} else if (strcmp(name, VCONFKEY_SHOT_TIZEN_PALM_TOUCH_MUTE_ENABLED) == 0) {
		palm_mute_enabled = vconf_keynode_get_int(in_key);
		if (palm_mute_enabled == 1) {
			warn("parm touch mute is enabled");

			_callmgr_ringer_stop_alert(ringer_handle);
		}
	}
}
#endif

static void __callmgr_ringer_player_completed_cb(void *data)
{
	info("__callmgr_ringer_player_completed_cb()");
}


static void __callmgr_ringer_player_interrupted_cb(player_interrupted_code_e code, void *data)
{
	warn("__callmgr_ringer_player_interrupted_cb()");
}

static void __callmgr_ringer_player_error_cb(int error_code, void *data)
{
	err("__callmgr_ringer_player_error_cb()");
}

static int __callmgr_ringer_prepare_player(callmgr_ringer_handle_h ringer_handle, char *ringtone_path)
{
	int ret = -1;
	dbg("__callmgr_ringer_prepare_player()");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);
	CM_RETURN_VAL_IF_FAIL(ringtone_path, -1);

	ret = player_set_uri(ringer_handle->player_handle, ringtone_path);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_set_uri() ERR: %d!!!!", ret);
		return -1;
	}

	ret = player_set_sound_type(ringer_handle->player_handle, SOUND_TYPE_RINGTONE);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_set_sound_type() ERR: %d!!!!", ret);
		return -1;
	}

	ret = player_set_looping(ringer_handle->player_handle, TRUE);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_set_looping() ERR: %d!!!!", ret);
		return -1;
	}

	player_set_completed_cb(ringer_handle->player_handle, __callmgr_ringer_player_completed_cb, NULL);
	player_set_interrupted_cb(ringer_handle->player_handle, __callmgr_ringer_player_interrupted_cb, NULL);
	player_set_error_cb(ringer_handle->player_handle, __callmgr_ringer_player_error_cb, NULL);

	ret = player_prepare(ringer_handle->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_prepare() failed %d!!!!", ret);
		return -1;
	}

	return 0;
}

static gboolean __callmgr_ringer_increasing_melody_cb(void *data)
{
	dbg("__callmgr_ringer_increasing_melody_cb()");
	callmgr_ringer_handle_h ringer_handle = (callmgr_ringer_handle_h)data;
	int ret = 0;
	ringer_handle->increment_timer = 0;

	ret = player_set_volume(ringer_handle->player_handle, 1, 1);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_set_volume() failed: %d!!!!", ret);
		return FALSE;
	}

	return FALSE;
}

static void  __callmgr_ringer_get_adjusted_volume_ratio(int volume_level, float *adjusted_ratio)
{
	switch (volume_level) {
		case 1:
			*adjusted_ratio = 0.562;
			break;
		case 2:
			*adjusted_ratio = 0.398;
			break;
		case 3:
			*adjusted_ratio = 0.316;
			break;
		case 4:
			*adjusted_ratio = 0.251;
			break;
		case 5:
			*adjusted_ratio = 0.200;
			break;
		case 6:
			*adjusted_ratio = 0.158;
			break;
		case 7:
			*adjusted_ratio = 0.126;
			break;
		case 8:
			*adjusted_ratio = 0.100;
			break;
		case 9:
			*adjusted_ratio = 0.079;
			break;
		case 10:
			*adjusted_ratio = 0.063;
			break;
		case 11:
			*adjusted_ratio = 0.050;
			break;
		case 12:
			*adjusted_ratio = 0.040;
			break;
		case 13:
			*adjusted_ratio = 0.032;
			break;
		case 14:
			*adjusted_ratio = 0.025;
			break;
		case 15:
			*adjusted_ratio = 0.020;
			break;
		default:
			*adjusted_ratio = 1;
			break;
	}
	if (*adjusted_ratio > 1.0) {
		*adjusted_ratio = 1.0;
	}

	return;
}

static void __callmgr_ringer_ringtone_stream_focus_state_cb(sound_stream_info_h stream_info,
			sound_stream_focus_change_reason_e reason_for_change, const char *additional_info, void *user_data)
{
	dbg("__callmgr_ringer_ringtone_stream_focus_state_cb");
	return;
}

static int __callmgr_ringer_create_ringtone_stream(callmgr_ringer_handle_h ringer_handle)
{
	int ret = -1;
	dbg("__callmgr_ringer_create_ringtone_stream()");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	ret = sound_manager_create_stream_information_internal(SOUND_STREAM_TYPE_RINGTONE_CALL, __callmgr_ringer_ringtone_stream_focus_state_cb,
		NULL, &ringer_handle->ringtone_stream_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_create_stream_information_internal() get failed with err[%d]", ret);
		return -1;
	}

	ret = sound_manager_acquire_focus(ringer_handle->ringtone_stream_handle, SOUND_STREAM_FOCUS_FOR_PLAYBACK|SOUND_STREAM_FOCUS_FOR_RECORDING, NULL);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_acquire_focus() get failed with err[%d]", ret);
		return -1;
	}
	return 0;
}

static int __callmgr_ringer_destroy_ringtone_stream(callmgr_ringer_handle_h ringer_handle)
{
	int ret = -1;
	dbg("__callmgr_ringer_destroy_ringtone_stream()");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	ret = sound_manager_release_focus(ringer_handle->ringtone_stream_handle, SOUND_STREAM_FOCUS_FOR_PLAYBACK|SOUND_STREAM_FOCUS_FOR_RECORDING, NULL);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_release_focus() get failed with err[%d]", ret);
	}

	ret = sound_manager_destroy_stream_information(ringer_handle->ringtone_stream_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_destroy_stream_information() get failed with err[%d]", ret);
		return -1;
	}
	ringer_handle->ringtone_stream_handle = NULL;

	return 0;
}

static int __callmgr_ringer_play_melody(callmgr_ringer_handle_h ringer_handle, char *caller_ringtone_path)
{
	int ret = -1;
	int ringtone_volume = 0;
	gboolean increasing_vol = FALSE;
	gboolean is_ringtone_playable = FALSE;
	gboolean is_silent_ringtone = FALSE;
	gboolean is_default_ringtone = FALSE;
	float adjusted_val = 0.0;
	char *ringtone_path = NULL;
	char *default_ringtone_path = NULL;
	char *position = NULL;
	int startposition = 0;
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	if (ringer_handle->player_handle) {
		warn("Already playing melody. Ignore this request");
		return -1;
	}

	ret = __callmgr_ringer_create_ringtone_stream(ringer_handle);
	if (ret < 0) {
		err("__callmgr_ringer_create_ringtone_stream() get failed");
		return -1;
	}

	ret = sound_manager_get_volume(SOUND_TYPE_RINGTONE, &ringtone_volume);
	if (SOUND_MANAGER_ERROR_NONE != ret) {
		err("sound_manager_get_volume() Error: [0x%x]", ret);
		warn("set ringtone value to 5");
		ringtone_volume = 5;
	}

	ret = _callmgr_vconf_get_ringtone_default_path(&default_ringtone_path);
	if ((ret < 0) || (default_ringtone_path == NULL)) {
		err("_callmgr_vconf_get_ringtone_default_path() failed");
		return -1;
	}

	if (caller_ringtone_path) {
		info("caller ringtone");
		_callmgr_util_is_ringtone_playable(caller_ringtone_path, &is_ringtone_playable);
		if (is_ringtone_playable == TRUE) {
			ringtone_path = g_strdup(caller_ringtone_path);
		}
	}

	if (ringtone_path == NULL) {
		ret = _callmgr_vconf_get_ringtone_path(&ringtone_path);
		if ((ret < 0) || (ringtone_path == NULL)) {
			err("_callmgr_vconf_get_ringtone_path() failed");
			ringtone_path = g_strdup(default_ringtone_path);
		}
		is_default_ringtone = TRUE;
	}

	if ((_callmgr_util_is_silent_ringtone(ringtone_path, &is_silent_ringtone) == 0) && (is_silent_ringtone == TRUE)) {
		CM_SAFE_FREE(ringtone_path);
		CM_SAFE_FREE(default_ringtone_path);

		return 0;
	}

	ret = _callmgr_util_is_ringtone_playable(ringtone_path, &is_ringtone_playable);
	if ((ret < 0) || (is_ringtone_playable == FALSE)) {
		CM_SAFE_FREE(ringtone_path);
		ringtone_path = g_strdup(default_ringtone_path);
	}

	ret = player_create(&ringer_handle->player_handle);
	if (ret != PLAYER_ERROR_NONE || ringer_handle->player_handle == NULL) {
		err("player_create() ERR: %x!!!!", ret);
		CM_SAFE_FREE(ringtone_path);
		CM_SAFE_FREE(default_ringtone_path);

		return -1;
	}

	ret = player_set_audio_policy_info(ringer_handle->player_handle, ringer_handle->ringtone_stream_handle);
	if (PLAYER_ERROR_NONE != ret) {
		err("player_set_audio_policy_info() get failed with err[%d]", ret);
		CM_SAFE_FREE(ringtone_path);
		CM_SAFE_FREE(default_ringtone_path);

		return -1;
	}

	ret = __callmgr_ringer_prepare_player(ringer_handle, ringtone_path);
	if (ret < 0) {
		ret = __callmgr_ringer_prepare_player(ringer_handle, default_ringtone_path);
		if (ret < 0) {
			err("__callmgr_audio_prepare_player() failed");
			CM_SAFE_FREE(ringtone_path);
			CM_SAFE_FREE(default_ringtone_path);

			return -1;
		}
		is_default_ringtone = TRUE;
	}

	ret = _callmgr_vconf_get_increasing_volume_setting(&increasing_vol);
	if (ret < 0) {
		err("failed to get increasing volume setting");
		increasing_vol = FALSE;
	}

	if (increasing_vol) {
		__callmgr_ringer_get_adjusted_volume_ratio(ringtone_volume, &adjusted_val);
		ret = player_set_volume(ringer_handle->player_handle, adjusted_val, adjusted_val);
		if (ret != PLAYER_ERROR_NONE) {
			warn("player_set_volume() failed: %d!!!!", ret);
			/* Set volume is failed. But we can play ringtone. */
		}
	}

	ret = player_start(ringer_handle->player_handle);
	if (PLAYER_ERROR_NONE != ret) {
		err("player_start() failed, error: [%d]", ret);
		CM_SAFE_FREE(ringtone_path);
		CM_SAFE_FREE(default_ringtone_path);

		return -1;
	}

	if (is_default_ringtone) {
		info("default ringtone");

		position = vconf_get_str(VCONFKEY_SETAPPL_CALL_RINGTONE_PATH_WITH_RECOMMENDATION_TIME_STR);
		if (position != NULL) {
			startposition = atoi(position);
			info("highlight ringtone: startposition(%d)", startposition);

			g_free(position);
		}
		else {
			warn("position is NULL");
		}
	}
	else {
		// ToDo: in case of contact ringtone and group ringtone
	}

	if (startposition > 0) {
		ret = player_set_play_position(ringer_handle->player_handle, startposition, true, NULL, NULL);
		if (PLAYER_ERROR_NONE != ret) {
			err("player_set_play_position() failed, error: [%d]", ret);
		}
	}

	if (increasing_vol) {
		ringer_handle->increment_timer = g_timeout_add(CALLMGR_RINGTONE_INCREMENT_TIMER_INTERVAL, __callmgr_ringer_increasing_melody_cb, (gpointer)ringer_handle);
	}

	CM_SAFE_FREE(ringtone_path);
	CM_SAFE_FREE(default_ringtone_path);

	ringer_handle->current_status = CM_RINGER_STATUS_RINGING_E;

	return 0;
}

static int __callmgr_ringer_stop_melody(callmgr_ringer_handle_h ringer_handle)
{
	int ret = PLAYER_ERROR_NONE;
	player_state_e player_state = PLAYER_STATE_NONE;

	dbg("__callmgr_ringer_stop_melody()");

	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	if (ringer_handle->increment_timer != 0) {
		g_source_remove(ringer_handle->increment_timer);
		ringer_handle->increment_timer = 0;
	}

	CM_RETURN_VAL_IF_FAIL(ringer_handle->player_handle, -1);

	player_get_state(ringer_handle->player_handle, &player_state);

	info("current player state = %d", player_state);

	if (PLAYER_STATE_PLAYING == player_state || PLAYER_STATE_PAUSED == player_state) {
		ret = player_stop(ringer_handle->player_handle);
		if (PLAYER_ERROR_NONE != ret) {
			err("player_stop() failed: [0x%x]", ret);

			return -1;
		}
		else {
			info("player_stop done");
		}
	}

	ret = player_unprepare(ringer_handle->player_handle);
	if (PLAYER_ERROR_NONE != ret) {
		err("player_unprepare() failed: [0x%x]", ret);

		return -1;
	}
	else {
		info("player_unprepare done");
	}

	ret = player_destroy(ringer_handle->player_handle);
	if (PLAYER_ERROR_NONE != ret) {
		err("player_destroy() failed: [0x%x]", ret);
		return -1;
	}
	else {
		info("player_destroy done");
	}

	ringer_handle->player_handle = NULL;
	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return 0;
}

static gboolean __callmgr_ringer_vibration_timeout_cb(void *data)
{
	callmgr_ringer_handle_h ringer_handle = (callmgr_ringer_handle_h) data;
	int ret = -1;

	dbg("level(%d)", ringer_handle->vibration_level);
	ret = device_haptic_vibrate(ringer_handle->haptic_handle, CALLMGR_VIBRATION_PLAY_DURATION, ringer_handle->vibration_level, &ringer_handle->effect_handle);
	if (ret != DEVICE_ERROR_NONE) {
		err("device_haptic_vibrate error : %d", ret);
	}

	return TRUE;
}

static int __callmgr_ringer_play_vibration(callmgr_ringer_handle_h ringer_handle)
{
	dbg("__callmgr_ringer_play_vibration()");
	device_error_e ret = DEVICE_ERROR_NONE;
	int vib_level = 0;
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);
	CM_RETURN_VAL_IF_FAIL(!ringer_handle->haptic_handle, -1);
	CM_RETURN_VAL_IF_FAIL(!ringer_handle->effect_handle, -1);

	ret = device_haptic_open(0, &ringer_handle->haptic_handle);
	if (ret != DEVICE_ERROR_NONE || ringer_handle->haptic_handle == NULL) {
		err("haptic_open:error : %d, %x", ret, ringer_handle->haptic_handle);
		return -1;
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_CALL_VIBRATION_LEVEL_INT, &vib_level) < 0) {
		err("Fail vconf_get_int()");
	}
	info("level(%d)", vib_level);
	ringer_handle->vibration_level = vib_level;

	ret = device_haptic_vibrate(ringer_handle->haptic_handle, CALLMGR_VIBRATION_PLAY_DURATION, vib_level, &ringer_handle->effect_handle);
	if (ret != DEVICE_ERROR_NONE) {
		err("_voicecall_dvc_vibrate_buffer error : %d", ret);
		ret = device_haptic_close(ringer_handle->haptic_handle);
		if (ret != DEVICE_ERROR_NONE) {
			err("haptic_close error : %d", ret);
		}
		return -1;
	}
	ringer_handle->vibration_timer = g_timeout_add(CALLMGR_VIBRATION_PLAY_DURATION+CALLMGR_VIBRATION_PLAY_INTERVAL,  __callmgr_ringer_vibration_timeout_cb, (void *)ringer_handle);

	return 0;
}

static int __callmgr_ringer_stop_vibration(callmgr_ringer_handle_h ringer_handle)
{
	dbg("..");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);
	CM_RETURN_VAL_IF_FAIL(ringer_handle->haptic_handle, -1);

	if (ringer_handle->vibration_timer != 0) {
		info("vib_timer_id removing..");
		g_source_remove(ringer_handle->vibration_timer);
		ringer_handle->vibration_timer = 0;
	}

	if (device_haptic_stop(ringer_handle->haptic_handle, ringer_handle->effect_handle) != DEVICE_ERROR_NONE) {
		err("haptic_stop_all_effects error");
	} else if (device_haptic_close(ringer_handle->haptic_handle) != DEVICE_ERROR_NONE) {
		err("haptic_close error");
	}
	ringer_handle->haptic_handle = NULL;
	ringer_handle->effect_handle = NULL;
	ringer_handle->vibration_level = 0;

	return 0;
}

static void __callmgr_ringer_play_effect_finish_cb(int id, void *user_data)
{
	info("__callmgr_ringer_play_effect_finish_cb");
	callmgr_ringer_handle_h ringer_handle = (callmgr_ringer_handle_h)user_data;
	CM_RETURN_IF_FAIL(ringer_handle);

	if (ringer_handle->play_effect_cb_fn) {
		ringer_handle->play_effect_cb_fn(ringer_handle->play_effect_user_data);
	}
	ringer_handle->play_effect_cb_fn = NULL;
	ringer_handle->play_effect_user_data = NULL;
	ringer_handle->play_effect_handle = -1;

	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return;
}

int _callmgr_ringer_start_alert(callmgr_ringer_handle_h ringer_handle, char *caller_ringtone_path , gboolean is_earjack_connected)
{
	gboolean is_sound_enabled = FALSE;
	gboolean is_vibration_enabled = FALSE;
	gboolean is_vibrate_when_ring_enabled = FALSE;
	gboolean is_recording_process = FALSE;

	dbg("_callmgr_ringer_start_alert()");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	/* Get sound profile setting*/
	_callmgr_vconf_is_sound_setting_enabled(&is_sound_enabled);
	_callmgr_vconf_is_vibration_setting_enabled(&is_vibration_enabled);
	_callmgr_vconf_is_vibrate_when_ringing_enabled(&is_vibrate_when_ring_enabled);
	_callmgr_util_is_recording_progress(&is_recording_process);

	info("sound[%d], vibration[%d], vibrate when ringing[%d], recording_process[%d],  is_earjack_connected[%d]",
		is_sound_enabled, is_vibration_enabled, is_vibrate_when_ring_enabled, is_recording_process, is_earjack_connected);

	if ((is_sound_enabled && !is_recording_process)
		|| (is_earjack_connected && !is_recording_process)) {
		__callmgr_ringer_play_melody(ringer_handle, caller_ringtone_path);
	}
	if (is_vibration_enabled
			|| (is_sound_enabled && is_vibrate_when_ring_enabled)
			|| (is_sound_enabled && is_recording_process)) {
		__callmgr_ringer_play_vibration(ringer_handle);
	}

	return 0;
}

int _callmgr_ringer_play_local_ringback_tone(callmgr_ringer_handle_h ringer_handle)
{
	int ret = SOUND_MANAGER_ERROR_NONE;

	info("_callmgr_ringer_play_local_ringback_tone");

	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	ret = sound_manager_create_stream_information_internal(SOUND_STREAM_TYPE_RINGBACKTONE_CALL, __callmgr_ringer_ringtone_stream_focus_state_cb,
		NULL, &ringer_handle->ringtone_stream_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_create_stream_information_internal() get failed with err[%d]", ret);
		return -1;
	}

	if (TONE_PLAYER_ERROR_NONE != tone_player_start_with_stream_info(TONE_TYPE_ANSI_RINGTONE, ringer_handle->ringtone_stream_handle , -1, &ringer_handle->ringback_tone_play_handle)) {
		err("Error tone_player_start_with_stream_info");
		ringer_handle->ringback_tone_play_handle = -1;
		return -1;
	}
	ringer_handle->current_status = CM_RINGER_STATUS_RINGING_E;

	return 0;
}

int _callmgr_ringer_stop_local_ringback_tone(callmgr_ringer_handle_h ringer_handle)
{
	info("_callmgr_ringer_stop_local_ringback_tone");

	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	if (ringer_handle->ringback_tone_play_handle == -1) {
		err("Wrong handle to stop tone_player");
		return -1;
	}

	if (TONE_PLAYER_ERROR_NONE != tone_player_stop(ringer_handle->ringback_tone_play_handle)) {
		err("tone_player_stop Failed");
		ringer_handle->ringback_tone_play_handle = -1;
		return -1;
	}

	info("tone_player_stop done");

	ringer_handle->ringback_tone_play_handle = -1;
	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return 0;
}
static gboolean __callmgr_waiting_tone_play_end_cb(gpointer pdata)
{
	callmgr_ringer_handle_h ringer_handle = (callmgr_ringer_handle_h)pdata;
	CM_RETURN_VAL_IF_FAIL(ringer_handle, FALSE);
	info("__callmgr_waiting_tone_play_end_cb");

	if (ringer_handle->waiting_tone_timer != -1) {
		dbg("waiting_tone_timer removing..");
		g_source_remove(ringer_handle->waiting_tone_timer);
		ringer_handle->waiting_tone_timer = -1;
	}
	ringer_handle->alternate_tone_play_count++;

	if(CALLMGR_2ND_CALL_BEEP_COUNT == ringer_handle->alternate_tone_play_count) {
		ringer_handle->alternate_tone_play_count = 0;
		_callmgr_ringer_stop_alternate_tone(ringer_handle);
	} else {
		_callmgr_ringer_start_alternate_tone(ringer_handle);
	}
	return FALSE;
}
int _callmgr_ringer_start_alternate_tone(callmgr_ringer_handle_h ringer_handle)
{
	int ret = -1;
	int sound_manager_ret = SOUND_MANAGER_ERROR_NONE;

	info("_callmgr_ringer_start_alternate_tone()");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	sound_manager_ret = sound_manager_create_stream_information_internal(SOUND_STREAM_TYPE_RINGBACKTONE_CALL, __callmgr_ringer_ringtone_stream_focus_state_cb,
		NULL, &ringer_handle->ringtone_stream_handle);
	if (sound_manager_ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_create_stream_information_internal() get failed with err[%d]", sound_manager_ret);
		return -1;
	}

	ret = tone_player_start_with_stream_info(TONE_TYPE_SUP_CALL_WAITING, ringer_handle->ringtone_stream_handle, 4000, &ringer_handle->alternate_tone_handle);
	if (ret != TONE_PLAYER_ERROR_NONE) {
		err("tone_player_start_with_stream_info() failed");
		return -1;
	}

	info("tone_player_start_with_stream_info done");

	ringer_handle->current_status = CM_RINGER_STATUS_RINGING_E;
	ringer_handle->waiting_tone_timer = g_timeout_add(CALLMGR_2ND_CALL_BEEP_INTERVAL, __callmgr_waiting_tone_play_end_cb, (gpointer)ringer_handle);

	return 0;
}

int _callmgr_ringer_stop_alert(callmgr_ringer_handle_h ringer_handle)
{
	dbg("_callmgr_ringer_stop_alert()");

	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	__callmgr_ringer_stop_melody(ringer_handle);
	__callmgr_ringer_stop_vibration(ringer_handle);
	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	return 0;
}

int _callmgr_ringer_stop_alternate_tone(callmgr_ringer_handle_h ringer_handle)
{
	int ret = -1;

	dbg("_callmgr_ringer_stop_alternate_tone()");

	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	if (ringer_handle->alternate_tone_handle == -1) {
		err("Invalid tone handle");

		return -1;
	}

	ret = tone_player_stop(ringer_handle->alternate_tone_handle);
	if (ret != TONE_PLAYER_ERROR_NONE) {
		err("tone_player_stop() failed");
		return -1;
	}

	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;
	ringer_handle->alternate_tone_handle = -1;

	if (ringer_handle->waiting_tone_timer != -1) {
		dbg("waiting_tone_timer removing..");
		g_source_remove(ringer_handle->waiting_tone_timer);
		ringer_handle->waiting_tone_timer = -1;
	}

	ringer_handle->alternate_tone_play_count = 0;

	__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return 0;
}

int _callmgr_ringer_play_effect(callmgr_ringer_handle_h ringer_handle, cm_ringer_effect_type_e effect_type, cm_ringer_effect_finished_cb cb_fn, void *user_data)
{
	dbg("_callmgr_ringer_play_effect()");
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);
	int ret = WAV_PLAYER_ERROR_NONE;
	int sound_manager_ret = SOUND_MANAGER_ERROR_NONE;
	char *effect_path = NULL;

	ringer_handle->play_effect_cb_fn = cb_fn;
	ringer_handle->play_effect_user_data = user_data;

	sound_manager_ret = sound_manager_create_stream_information_internal(SOUND_STREAM_TYPE_RINGBACKTONE_CALL, __callmgr_ringer_ringtone_stream_focus_state_cb,
		NULL, &ringer_handle->ringtone_stream_handle);
	if (sound_manager_ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_create_stream_information_internal() get failed with err[%d]", sound_manager_ret);
		return -1;
	}

	switch (effect_type) {
		case CM_RINGER_EFFECT_CONNECT_TONE_E:
			effect_path = g_strdup(CALLMGR_PLAY_CONNECT_EFFECT_PATH);
			break;
		case CM_RINGER_EFFECT_DISCONNECT_TONE_E:
	 		effect_path = g_strdup(CALLMGR_PLAY_DISCONNECT_EFFECT_PATH);
			break;
		default:
			err("Invalid effect type[%d]", effect_type);
			return -1;
	}

	ret = wav_player_start_with_stream_info((const char *)effect_path, ringer_handle->ringtone_stream_handle, __callmgr_ringer_play_effect_finish_cb, (void *)ringer_handle, &(ringer_handle->play_effect_handle));
	if (ret != WAV_PLAYER_ERROR_NONE) {
		err("wav_player_start_with_stream_info failed:[%d]", ret);
		g_free(effect_path);
		return -1;
	}
	g_free(effect_path);

	ringer_handle->current_status = CM_RINGER_STATUS_RINGING_E;

	return 0;
 }

int _callmgr_ringer_stop_effect(callmgr_ringer_handle_h ringer_handle)
{
	int ret = MM_ERROR_NONE;
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	dbg("_callmgr_ringer_stop_effect");

	if (ringer_handle->play_effect_handle < 0) {
		err("Invalid handle");
		return -1;
	}

	ret = wav_player_stop(ringer_handle->play_effect_handle);
	if (ret != MM_ERROR_NONE) {
		err("wav_player_stop failed:[%d]", ret);
	}

	ringer_handle->play_effect_cb_fn = NULL;
	ringer_handle->play_effect_user_data = NULL;
	ringer_handle->play_effect_handle = -1;
	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	//__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return 0;
}

static gboolean __callmgr_ringer_play_signal_end_cb(gpointer pdata)
{
	callmgr_ringer_handle_h ringer_handle = (callmgr_ringer_handle_h)pdata;
	CM_RETURN_VAL_IF_FAIL(ringer_handle, FALSE);
	info("__callmgr_ringer_play_signal_end_cb");

	if (ringer_handle->signal_tone_timer != -1) {
		dbg("play signal end and signal_tone_timer removing..");
		g_source_remove(ringer_handle->signal_tone_timer);
		ringer_handle->signal_tone_timer = -1;
	}

	if (ringer_handle->play_signal_cb_fn) {
		ringer_handle->play_signal_cb_fn(ringer_handle->play_signal_user_data);
	}
	ringer_handle->play_signal_cb_fn = NULL;
	ringer_handle->play_signal_user_data = NULL;
	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return FALSE;
}

int _callmgr_ringer_play_signal(callmgr_ringer_handle_h ringer_handle, cm_ringer_signal_type_e signal_type, cm_ringer_signal_finished_cb cb_fn, void *user_data)
{
	dbg("_callmgr_ringer_play_signal type[%d]", signal_type);
	int ret = TONE_PLAYER_ERROR_NONE;
	int sound_manager_ret = SOUND_MANAGER_ERROR_NONE;
	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	if (CM_RINGER_SIGNAL_NONE_E == signal_type) {
		err("No Signal Type Assigned");
		return -1;
	}

	/**
	   Always stop the signal before playing another one
	   This is to make sure that previous signal sound is stopeed completely
	 **/
	_callmgr_ringer_stop_signal(ringer_handle);
	_callmgr_ringer_stop_effect(ringer_handle);

	ringer_handle->play_signal_cb_fn = cb_fn;
	ringer_handle->play_signal_user_data = user_data;

	sound_manager_ret = sound_manager_create_stream_information_internal(SOUND_STREAM_TYPE_RINGBACKTONE_CALL, __callmgr_ringer_ringtone_stream_focus_state_cb,
		NULL, &ringer_handle->ringtone_stream_handle);
	if (sound_manager_ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_create_stream_information_internal() get failed with err[%d]", sound_manager_ret);
		return -1;
	}

	switch (signal_type) {
	case CM_RINGER_SIGNAL_USER_BUSY_TONE_E:
		ret = tone_player_start_with_stream_info(TONE_TYPE_SUP_BUSY, ringer_handle->ringtone_stream_handle, 5000, &ringer_handle->signal_play_handle);
		break;
	case CM_RINGER_SIGNAL_WRONG_NUMBER_TONE_E:
	case CM_RINGER_SIGNAL_CALL_FAIL_TONE_E:
		ret = tone_player_start_with_stream_info(TONE_TYPE_SUP_ERROR, ringer_handle->ringtone_stream_handle, 2000, &ringer_handle->signal_play_handle);
		break;
	case CM_RINGER_SIGNAL_NW_CONGESTION_TONE_E:
		ret = tone_player_start_with_stream_info(TONE_TYPE_SUP_CONGESTION, ringer_handle->ringtone_stream_handle, 2000, &ringer_handle->signal_play_handle);
		break;
	default:
		err("Invalid Signal Type");
		break;
	}


	if (TONE_PLAYER_ERROR_NONE != ret) {
		err("one_player_start failed, Error: [0x%x]", ret);
		return -1;
	} else {
		int time_val = 0;
		if (signal_type == CM_RINGER_SIGNAL_USER_BUSY_TONE_E) {
			time_val = 5000;
		} else {
			time_val = 2000;
		}
		ringer_handle->signal_tone_timer = g_timeout_add(time_val, __callmgr_ringer_play_signal_end_cb, (gpointer)ringer_handle);
	}
	ringer_handle->current_status = CM_RINGER_STATUS_RINGING_E;

	return 0;
}

int _callmgr_ringer_stop_signal(callmgr_ringer_handle_h ringer_handle)
{
	int ret = TONE_PLAYER_ERROR_NONE;

	CM_RETURN_VAL_IF_FAIL(ringer_handle, -1);

	dbg("_callmgr_ringer_stop_signal");

	if (ringer_handle->signal_tone_timer != -1) {
		dbg("signal_tone_timer removing..");
		g_source_remove(ringer_handle->signal_tone_timer);
		ringer_handle->signal_tone_timer = -1;
	}

	if (ringer_handle->signal_play_handle != -1) {
		dbg("Stopping Signal Sound");
		ret = tone_player_stop(ringer_handle->signal_play_handle);
		if (ret != TONE_PLAYER_ERROR_NONE) {
			err("tone_player_stop() failed");
			return -1;
		}
		ringer_handle->signal_play_handle = -1;
	}

	ringer_handle->play_signal_cb_fn = NULL;
	ringer_handle->play_signal_user_data = NULL;
	ringer_handle->current_status = CM_RINGER_STATUS_IDLE_E;

	//__callmgr_ringer_destroy_ringtone_stream(ringer_handle);

	return 0;
}

