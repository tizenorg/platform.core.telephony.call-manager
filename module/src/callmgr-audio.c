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
#include <vconf.h>

#include "callmgr-audio.h"
#include "callmgr-util.h"
#include "callmgr-log.h"

struct __audio_data {
	gboolean is_extra_vol;
	gboolean is_noise_reduction;
	gboolean is_mute_state;
	gboolean vstream_status;

	sound_stream_info_h sound_stream_handle;
	sound_device_type_e current_route;
	virtual_sound_stream_h vstream;

	callmgr_audio_session_mode_e current_mode;
	callmgr_audio_device_e current_device;

	audio_event_cb cb_fn;
	void *user_data;
};

int _callmgr_audio_init(callmgr_audio_handle_h *audio_handle, audio_event_cb cb_fn, void *user_data)
{
	struct __audio_data *handle = NULL;
	int b_noise_reduction = 0;

	handle = calloc(1, sizeof(struct __audio_data));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	handle->cb_fn = cb_fn;
	handle->user_data = user_data;
	handle->is_extra_vol = FALSE;
	handle->is_mute_state = FALSE;
	handle->current_mode = CALLMGR_AUDIO_SESSION_NONE_E;
	handle->current_device = CALLMGR_AUDIO_DEVICE_NONE_E;
	handle->current_route = -1;
	handle->sound_stream_handle = NULL;
	handle->vstream = NULL;

	if (vconf_get_bool(VCONFKEY_CISSAPPL_NOISE_REDUCTION_BOOL, &b_noise_reduction) < 0) {
		warn("vconf get bool failed");
	}
	handle->is_noise_reduction = (gboolean)b_noise_reduction;
	*audio_handle = handle;

	dbg("Audio init done");

	return 0;
}
int _callmgr_audio_deinit(callmgr_audio_handle_h audio_handle)
{

	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);

	/* TODO: Release all handles */

	_callmgr_audio_destroy_call_sound_session(audio_handle);
	g_free(audio_handle);

	return 0;
}

static char *__callmgr_audio_convert_device_type_to_string(sound_device_type_e device_type)
{
	switch (device_type) {
	case SOUND_DEVICE_BUILTIN_SPEAKER:
		return "Built-in speaker";
	case SOUND_DEVICE_BUILTIN_RECEIVER:
		return "Built-in receiver";
	case SOUND_DEVICE_BUILTIN_MIC:
		return "Built-in mic";
	case SOUND_DEVICE_AUDIO_JACK:
		return "Audio jac";
	case SOUND_DEVICE_BLUETOOTH:
		return "Bluetooth";
	case SOUND_DEVICE_HDMI:
		return "HDMI";
	case SOUND_DEVICE_FORWARDING:
		return "Forwarding";
	case SOUND_DEVICE_USB_AUDIO:
		return "USB Audio";
	default:
		return "Unknown device";
	}
}

static char *__callmgr_audio_convert_io_direction_to_string(sound_device_io_direction_e direction)
{
	switch (direction) {
	case SOUND_DEVICE_IO_DIRECTION_IN:
		return "IN";
	case SOUND_DEVICE_IO_DIRECTION_OUT:
		return "OUT";
	case SOUND_DEVICE_IO_DIRECTION_BOTH:
		return "BOTH";
	default:
		return "Unknown direction";
	}
}

static char *__callmgr_audio_convert_device_state_to_string(sound_device_state_e device_state)
{
	switch (device_state) {
	case SOUND_DEVICE_STATE_DEACTIVATED:
		return "Deactivated";
	case SOUND_DEVICE_STATE_ACTIVATED:
		return "Activated";
	default:
		return "Unknown state";
	}
}

static void __callmgr_audio_available_route_changed_cb(sound_device_h device, bool is_connected, void *user_data)
{
	CM_RETURN_IF_FAIL(user_data);
	CM_RETURN_IF_FAIL(device);
	callmgr_audio_handle_h audio_handle = (callmgr_audio_handle_h)user_data;
	sound_device_type_e device_type = SOUND_DEVICE_BUILTIN_RECEIVER;

	sound_manager_get_device_type(device, &device_type);
	info("Available route : %s (%d)",
		__callmgr_audio_convert_device_type_to_string(device_type),
		is_connected ? "Connected" : "Disconnected");

	if (device_type == SOUND_DEVICE_AUDIO_JACK) {
		audio_handle->cb_fn(CM_AUDIO_EVENT_EARJACK_CHANGED_E, (void *)is_connected, audio_handle->user_data);
	}
}

static void __callmgr_audio_active_device_changed_cb(sound_device_h device, sound_device_changed_info_e changed_info, void *user_data)
{
	CM_RETURN_IF_FAIL(user_data);
	CM_RETURN_IF_FAIL(device);
	callmgr_audio_handle_h audio_handle = (callmgr_audio_handle_h)user_data;
	sound_device_io_direction_e io_direction = SOUND_DEVICE_IO_DIRECTION_BOTH;
	sound_device_type_e device_type = SOUND_DEVICE_BUILTIN_RECEIVER;
	sound_device_state_e state = SOUND_DEVICE_STATE_DEACTIVATED;
	callmgr_audio_device_e cm_device = CALLMGR_AUDIO_DEVICE_NONE_E;
	int ret = SOUND_MANAGER_ERROR_NONE;

	ret = sound_manager_get_device_io_direction(device, &io_direction);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		err("sound_manager_get_device_io_direction() get failed with err[0x%x]", ret);

	ret = sound_manager_get_device_state(device, &state);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		err("sound_manager_get_device_state() get failed with err[0x%x]", ret);

	ret = sound_manager_get_device_type(device, &device_type);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		err("sound_manager_get_device_type() get failed with err[0x%x]", ret);

	info("Active device[%s], IO direction[%s], State[%s]",
		__callmgr_audio_convert_device_type_to_string(device_type),
		__callmgr_audio_convert_io_direction_to_string(io_direction),
		__callmgr_audio_convert_device_state_to_string(state));

	if ((io_direction == SOUND_DEVICE_IO_DIRECTION_IN) || (state == SOUND_DEVICE_STATE_DEACTIVATED)) {
		info("No need to inform callmgr-core of this change");
		return;
	}

	switch (device_type) {
	case SOUND_DEVICE_BUILTIN_SPEAKER:
		cm_device = CALLMGR_AUDIO_DEVICE_SPEAKER_E;
		break;
	case SOUND_DEVICE_BUILTIN_RECEIVER:
		cm_device = CALLMGR_AUDIO_DEVICE_RECEIVER_E;
		break;
	case SOUND_DEVICE_AUDIO_JACK:
		cm_device = CALLMGR_AUDIO_DEVICE_EARJACK_E;
		break;
	case SOUND_DEVICE_BLUETOOTH:
		cm_device = CALLMGR_AUDIO_DEVICE_BT_E;
		break;
	default:
		warn("Not available : %d", device_type);
		break;
	}

	if ((cm_device == audio_handle->current_device) || cm_device == CALLMGR_AUDIO_DEVICE_NONE_E) {
		warn("Do not update");
	} else {
		//audio_handle->current_device = cm_device;
		//audio_handle->cb_fn(CM_AUDIO_EVENT_PATH_CHANGED_E, (void*)cm_device, audio_handle->user_data);
	}
}

static void __callmgr_audio_call_stream_focus_state_cb(sound_stream_info_h stream_info,
	sound_stream_focus_change_reason_e reason_for_change, const char *additional_info, void *user_data)
{
	dbg("__callmgr_audio_call_stream_focus_state_cb");
	return;
}

static void __callmgr_audio_volume_changed_cb(sound_type_e type, unsigned int volume, void *user_data)
{
	CM_RETURN_IF_FAIL(user_data);
	dbg("__callmgr_audio_volume_changed_cb type - %d, level - %d", type, volume);

	callmgr_audio_handle_h audio_handle = (callmgr_audio_handle_h)user_data;
	sound_type_e snd_type = type;

	if (snd_type == SOUND_TYPE_CALL) {
		info("sound type : %d, volume : %d", snd_type, volume);
		audio_handle->cb_fn(CM_AUDIO_EVENT_VOLUME_CHANGED_E, GUINT_TO_POINTER(volume), audio_handle->user_data);
	}
	return;
}

int _callmgr_audio_create_call_sound_session(callmgr_audio_handle_h audio_handle, callmgr_audio_session_mode_e mode)
{
	int ret = -1;
	sound_stream_type_internal_e stream_type = SOUND_STREAM_TYPE_VOICE_CALL;
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);

	info("_callmgr_audio_create_call_sound_session()");

	if (audio_handle->current_mode == mode) {
		warn("already (%d) set ", mode);
		return -2;
	}

	ret = sound_manager_set_volume_changed_cb(__callmgr_audio_volume_changed_cb, audio_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_set_volume_changed_cb() failed. [%d]", ret);
		return -1;
	}

	if (audio_handle->sound_stream_handle != NULL) {
		err("call sound stream already created.");
		return -1;
	}

	switch (mode) {
		case CALLMGR_AUDIO_SESSION_VOICE_E:
			stream_type = SOUND_STREAM_TYPE_VOICE_CALL;
			break;
		case CALLMGR_AUDIO_SESSION_VIDEO_E:
			stream_type = SOUND_STREAM_TYPE_VIDEO_CALL;
			break;
		case CALLMGR_AUDIO_SESSION_NONE_E:
		default:
			err("unhandled session mode: %d", mode);
			return -1;
	}

	ret = sound_manager_create_stream_information_internal(stream_type, __callmgr_audio_call_stream_focus_state_cb, NULL, &audio_handle->sound_stream_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_create_stream_information_internal() get failed with err[%d]", ret);
		return -1;
	}

	/* Handle EP event  */
	ret = sound_manager_set_device_connected_cb(SOUND_DEVICE_ALL_MASK, __callmgr_audio_available_route_changed_cb, audio_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_set_device_connected_cb() failed. [%d]", ret);
		return -1;
	}

	/* Path is changed by other modules */
	ret = sound_manager_set_device_information_changed_cb(SOUND_DEVICE_ALL_MASK, __callmgr_audio_active_device_changed_cb, audio_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_set_device_information_changed_cb() failed. [%d]", ret);
		return -1;
	}

 	return 0;
}

int _callmgr_audio_stop_virtual_stream(callmgr_audio_handle_h audio_handle)
{
	int ret = -1;
	info("_callmgr_audio_stop_virtual_stream");
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(audio_handle->vstream, -1);

	if (audio_handle->vstream_status) {
		ret = sound_manager_stop_virtual_stream(audio_handle->vstream);
		if (ret != SOUND_MANAGER_ERROR_NONE)
			err("sound_manager_stop_virtual_stream() get failed with err[%d]", ret);
		audio_handle->vstream_status = 0;
	}

	return 0;
}

int _callmgr_audio_destroy_call_sound_session(callmgr_audio_handle_h audio_handle)
{
	int ret = -1;
	info("_callmgr_audio_destroy_call_sound_session()");
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(audio_handle->vstream, -1);
	CM_RETURN_VAL_IF_FAIL(audio_handle->sound_stream_handle, -1);

	sound_manager_unset_volume_changed_cb();
	sound_manager_unset_device_connected_cb();
	sound_manager_unset_device_information_changed_cb();

	ret = sound_manager_release_focus(audio_handle->sound_stream_handle, SOUND_STREAM_FOCUS_FOR_PLAYBACK|SOUND_STREAM_FOCUS_FOR_RECORDING, NULL);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_release_focus() get failed with err[%d]", ret);
		return -1;
	}

	if (audio_handle->vstream_status) {
		ret = sound_manager_stop_virtual_stream(audio_handle->vstream);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			err("sound_manager_stop_virtual_stream() get failed with err[%d]", ret);
		}
		audio_handle->vstream_status = 0;
	}

	ret = sound_manager_destroy_virtual_stream(audio_handle->vstream);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_destroy_virtual_stream() get failed with err[%d]", ret);
	}

	ret = sound_manager_destroy_stream_information(audio_handle->sound_stream_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_destroy_stream_information() get failed with err[%d]", ret);
	}

	audio_handle->vstream = NULL;
	audio_handle->sound_stream_handle = NULL;
	audio_handle->current_route = -1;
	info("_callmgr_audio_destroy_call_sound_session DONE");

	return 0;
}

static int __callmgr_audio_get_sound_device(sound_device_type_e device_type, sound_device_h *sound_device)
{
	sound_device_list_h device_list = NULL;
	sound_device_h	device = NULL;
	sound_device_type_e o_device_type;
	int ret = -1;

	ret = sound_manager_get_current_device_list (SOUND_DEVICE_ALL_MASK, &device_list);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_get_current_device_list() get failed");
		return -1;
	}

	while (1) {
		ret = sound_manager_get_next_device (device_list, &device);
		if (ret == SOUND_MANAGER_ERROR_NONE && device) {
			sound_manager_get_device_type(device, &o_device_type);
			if (o_device_type == device_type) {
				info("found device!!");
				break;
			}
		}
		else {
			err("sound_manager_get_next_device() failed with err[%d]", ret);
			return -1;
		}
	}

	*sound_device = device;
	return 0;
}

int _callmgr_audio_is_sound_device_available(callmgr_audio_device_e device_type, gboolean *is_available)
{
	sound_device_list_h device_list = NULL;
	sound_device_h	device = NULL;
	sound_device_type_e o_device_type;
	int ret = -1;

	ret = sound_manager_get_current_device_list(SOUND_DEVICE_ALL_MASK, &device_list);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_get_current_device_list() get failed");
		return -1;
	}

	while (1) {
		ret = sound_manager_get_next_device(device_list, &device);
		if (ret == SOUND_MANAGER_ERROR_NONE && device) {
			sound_manager_get_device_type(device, &o_device_type);
			if ((device_type == CALLMGR_AUDIO_DEVICE_SPEAKER_E) && (o_device_type == SOUND_DEVICE_BUILTIN_SPEAKER)) {
				info("found SPEAKER!!");
				break;
			} else if ((device_type == CALLMGR_AUDIO_DEVICE_RECEIVER_E) && (o_device_type == SOUND_DEVICE_BUILTIN_RECEIVER)) {
				info("found RECEIVER!!");
				break;
			} else if ((device_type == CALLMGR_AUDIO_DEVICE_EARJACK_E) && (o_device_type == SOUND_DEVICE_AUDIO_JACK)) {
				info("found EARJACK!!");
				break;
			} else if ((device_type == CALLMGR_AUDIO_DEVICE_BT_E) && (o_device_type == SOUND_DEVICE_BLUETOOTH)) {
				info("found BLUETOOTH!!");
				break;
			}
		} else {
			err("sound_manager_get_next_device() failed with err[%d]", ret);
			return -1;
		}
	}

	*is_available = TRUE;
	return 0;
}

#if 0
static int __callmgr_audio_get_sound_route(callmgr_audio_handle_h audio_handle, callmgr_audio_route_e route, sound_session_call_mode_e *sound_route, callmgr_audio_device_e *active_device)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);

	if (route & CALLMGR_AUDIO_DEVICE_EARJACK_E) {
		gboolean is_earjack_available = FALSE;
		_callmgr_audio_is_sound_device_available(CALLMGR_AUDIO_DEVICE_EARJACK_E, &is_earjack_available);
		if (is_earjack_available) {
			info("route path : SOUND_SESSION_CALL_MODE_VOICE_WITH_AUDIO_JACK");
			*sound_route = SOUND_SESSION_CALL_MODE_VOICE_WITH_AUDIO_JACK;
			*active_device = CALLMGR_AUDIO_DEVICE_EARJACK_E;
			return 0;
		}
	}

	if (route & CALLMGR_AUDIO_DEVICE_SPEAKER_E) {
		info("route path : SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_SPEAKER");
		*sound_route = SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_SPEAKER;
		*active_device = CALLMGR_AUDIO_DEVICE_SPEAKER_E;
		return 0;
	}

	if (route & CALLMGR_AUDIO_DEVICE_RECEIVER_E) {
		info("route path : SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_RECEIVER");
		*sound_route = SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_RECEIVER;
		*active_device = CALLMGR_AUDIO_DEVICE_RECEIVER_E;
		return 0;
	}

	if (route & CALLMGR_AUDIO_ROUTE_BT_E) {
		info("route path : SOUND_SESSION_CALL_MODE_VOICE_WITH_BLUETOOTH");
		*sound_route = SOUND_SESSION_CALL_MODE_VOICE_WITH_BLUETOOTH;
		*active_device = CALLMGR_AUDIO_DEVICE_BT_E;
		return 0;
	}

	*sound_route = SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_RECEIVER;
	*active_device = CALLMGR_AUDIO_DEVICE_RECEIVER_E;
	err("No available route");
	return -1;
}
#endif

static int __callmgr_audio_set_audio_control_state(callmgr_audio_handle_h audio_handle, sound_device_h route)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
#if 0
	int is_noise_reduction = 0;
 
	/* ToDo : vconf will be changed to MM API */
	switch (route) {
	case SOUND_SESSION_CALL_MODE_VOICE_WITH_BLUETOOTH:
/*	case SOUND_ROUTE_IN_HEADSET_OUT_HEADSET: */
	case SOUND_SESSION_CALL_MODE_VOICE_WITH_AUDIO_JACK:
		if (vconf_set_bool(VCONFKEY_CALL_NOISE_REDUCTION_STATE_BOOL, FALSE) < 0) {
			warn("vconf set bool failed");
		}

		if (sound_manager_call_set_extra_volume(FALSE) < 0) {
			warn("sound_manager_call_set_extra_volume() failed");
		}

		break;

	case SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_SPEAKER:
		if (vconf_set_bool(VCONFKEY_CALL_NOISE_REDUCTION_STATE_BOOL, FALSE) < 0) {
			warn("vconf set bool failed");
		}

		if (audio_handle->is_extra_vol == TRUE) {
			if (sound_manager_call_set_extra_volume(TRUE) < 0) {
				warn("sound_manager_call_set_extra_volume() failed");
			}
		} else {
			if (sound_manager_call_set_extra_volume(FALSE) < 0) {
				warn("sound_manager_call_set_extra_volume() failed");
			}
		}
		break;
	case SOUND_SESSION_CALL_MODE_VOICE_WITH_BUILTIN_RECEIVER:
	default:
		{
			if (vconf_set_bool(VCONFKEY_CALL_NOISE_REDUCTION_STATE_BOOL, audio_handle->is_noise_reduction) < 0) {
				warn("vconf set bool failed");
			}

			if (audio_handle->is_extra_vol == TRUE) {
				if (sound_manager_call_set_extra_volume(TRUE) < 0) {
					warn("sound_manager_call_set_extra_volume() failed");
				}
			} else {
				if (sound_manager_call_set_extra_volume(FALSE) < 0) {
					warn("sound_manager_call_set_extra_volume() failed");
				}
			}
		}
		break;
	}

	/*Save current Noise Reduction status*/
	if (vconf_get_bool(VCONFKEY_CALL_NOISE_REDUCTION_STATE_BOOL, &is_noise_reduction) < 0) {
		warn("vconf get bool failed");
	}

	audio_handle->is_noise_reduction = is_noise_reduction;
	if (vconf_set_bool(VCONFKEY_CISSAPPL_NOISE_REDUCTION_BOOL, is_noise_reduction) < 0) {
		warn("vconf set bool failed");
	}
#endif

	return 0;
}

int _callmgr_audio_set_audio_route(callmgr_audio_handle_h audio_handle, callmgr_audio_route_e route)
{
	int ret = -1;
	sound_device_h sound_device = NULL;
	sound_device_type_e device_type;

	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(audio_handle->sound_stream_handle, -1);

	dbg("_callmgr_audio_set_audio_route");

	switch(route) {
		case CALLMGR_AUDIO_ROUTE_SPEAKER_E:
			device_type = SOUND_DEVICE_BUILTIN_SPEAKER;
			break;
		case CALLMGR_AUDIO_ROUTE_RECEIVER_E:
			device_type = SOUND_DEVICE_BUILTIN_RECEIVER;
			break;
		case CALLMGR_AUDIO_ROUTE_EARJACK_E:
			device_type = SOUND_DEVICE_AUDIO_JACK;
			break;
		case CALLMGR_AUDIO_ROUTE_BT_E:
			device_type = SOUND_DEVICE_BLUETOOTH;
			break;
		default:
			err("unhandled route[%d]", route);
			return -1;
	}

	if (audio_handle->vstream && !audio_handle->vstream_status) {
		dbg("Virtual stream is stopped. resume stream");
		ret = sound_manager_start_virtual_stream(audio_handle->vstream);
		if (ret != SOUND_MANAGER_ERROR_NONE)
			err("sound_manager_start_virtual_stream() failed:[%d]", ret);
		audio_handle->vstream_status = 1;
	}

	if (audio_handle->current_route != -1) {
		sound_device_h current_device = NULL;
		if (audio_handle->current_route == device_type) {
			warn("sound path is already set to this device");
			return -1;
		}

		if ((audio_handle->current_route == SOUND_DEVICE_BUILTIN_SPEAKER) || (audio_handle->current_route == SOUND_DEVICE_BUILTIN_RECEIVER)) {
			sound_device_h mic = NULL;
			__callmgr_audio_get_sound_device(SOUND_DEVICE_BUILTIN_MIC, &mic);
			ret = sound_manager_remove_device_for_stream_routing (audio_handle->sound_stream_handle, mic);

			if (ret != SOUND_MANAGER_ERROR_NONE) {
				err("sound_manager_remove_device_for_stream_routing() failed:[%d]", ret);
			}
		}

		__callmgr_audio_get_sound_device(audio_handle->current_route, &current_device);
		ret = sound_manager_remove_device_for_stream_routing (audio_handle->sound_stream_handle, current_device);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			err("sound_manager_remove_device_for_stream_routing() failed:[%d]", ret);
		}
		audio_handle->current_route = -1;
	}

	__callmgr_audio_get_sound_device(device_type, &sound_device);
	__callmgr_audio_set_audio_control_state(audio_handle, sound_device);
	ret = sound_manager_add_device_for_stream_routing (audio_handle->sound_stream_handle, sound_device);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_add_device_for_stream_routing() failed:[%d]", ret);
		return -1;
	}

	info("set audio route to: [%s]", __callmgr_audio_convert_device_type_to_string(device_type));

	if ((route == CALLMGR_AUDIO_ROUTE_SPEAKER_E) || (route == CALLMGR_AUDIO_ROUTE_RECEIVER_E)) {
		sound_device_h mic = NULL;
		__callmgr_audio_get_sound_device(SOUND_DEVICE_BUILTIN_MIC, &mic);
		ret = sound_manager_add_device_for_stream_routing (audio_handle->sound_stream_handle, mic);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			err("sound_manager_add_device_for_stream_routing() failed:[%d]", ret);
			return -1;
		}
	}

	ret = sound_manager_apply_stream_routing (audio_handle->sound_stream_handle);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_apply_stream_routing() failed:[%d]", ret);
		return -1;
	}

	if (audio_handle->vstream == NULL) {
		ret = sound_manager_acquire_focus(audio_handle->sound_stream_handle, SOUND_STREAM_FOCUS_FOR_PLAYBACK|SOUND_STREAM_FOCUS_FOR_RECORDING, NULL);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			err("sound_manager_acquire_focus() get failed with err[%d]", ret);
			return -1;
		}

		ret = sound_manager_create_virtual_stream (audio_handle->sound_stream_handle, &audio_handle->vstream);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			err("sound_manager_create_virtual_stream() failed:[%d]", ret);
			return -1;
		}

		ret = sound_manager_start_virtual_stream (audio_handle->vstream);
		if (ret != SOUND_MANAGER_ERROR_NONE) {
			err("sound_manager_start_virtual_stream() failed:[%d]", ret);
			return -1;
		}
		audio_handle->vstream_status = 1;
	}

	audio_handle->current_route = device_type;
	audio_handle->cb_fn(CM_AUDIO_EVENT_PATH_CHANGED_E, (void*)route, audio_handle->user_data);
	info("Success set route");
	return 0;
}

int _callmgr_audio_get_audio_route(callmgr_audio_handle_h audio_handle, callmgr_audio_route_e *o_route)
{
	callmgr_audio_route_e route = CALLMGR_AUDIO_ROUTE_NONE_E;
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_route, -1);
	dbg(">>");

	if (audio_handle->current_route != -1) {
		switch (audio_handle->current_route) {
			case SOUND_DEVICE_BUILTIN_SPEAKER:
				route = CALLMGR_AUDIO_ROUTE_SPEAKER_E;
				break;
			case SOUND_DEVICE_BUILTIN_RECEIVER:
				route = CALLMGR_AUDIO_ROUTE_RECEIVER_E;
				break;
			case SOUND_DEVICE_AUDIO_JACK:
				route = CALLMGR_AUDIO_ROUTE_EARJACK_E;
				break;
			case SOUND_DEVICE_BLUETOOTH:
				route = CALLMGR_AUDIO_ROUTE_BT_E;
				break;
			default:
				err("unhandled device type[%d]", audio_handle->current_route);
				break;
		}
	}
	*o_route = route;

	info("current device : [%s]", __callmgr_audio_convert_device_type_to_string(audio_handle->current_route));

	return 0;
}

int _callmgr_audio_set_extra_vol(callmgr_audio_handle_h audio_handle, gboolean is_extra_vol)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);

	audio_handle->is_extra_vol = is_extra_vol;
	dbg("updated extra vol : %d", audio_handle->is_extra_vol);

	return 0;
}

int _callmgr_audio_get_extra_vol(callmgr_audio_handle_h audio_handle, gboolean *o_is_extra_vol)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_is_extra_vol, -1);

	*o_is_extra_vol = audio_handle->is_extra_vol;

	return 0;
}

int _callmgr_audio_set_audio_tx_mute(callmgr_audio_handle_h audio_handle, gboolean is_mute_state)
{
#if 0 /* Sound manager API does not support on SPIN. This is requied with Q modem*/
	int ret = -1;
	ret = sound_manager_set_call_mute(SOUND_TYPE_CALL, is_mute_state);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_set_call_mute() failed:[%d]", ret);
		return -1;
	}
	audio_handle->is_mute_state = is_mute_state;
	info("updated mute state : %d", audio_handle->is_mute_state);
#endif
	return 0;
}

int _callmgr_audio_get_audio_tx_mute_state(callmgr_audio_handle_h audio_handle, gboolean *o_is_mute_state)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_is_mute_state, -1);

	*o_is_mute_state = audio_handle->is_mute_state;

	return 0;
}

int _callmgr_audio_set_noise_reduction(callmgr_audio_handle_h audio_handle, gboolean is_noise_reduction_on)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	audio_handle->is_noise_reduction = is_noise_reduction_on;
	dbg("updated noise reduction : %d", audio_handle->is_noise_reduction);

	return 0;
}

int _callmgr_audio_get_noise_reduction(callmgr_audio_handle_h audio_handle, gboolean *o_is_noise_reduction_on)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_is_noise_reduction_on, -1);
	*o_is_noise_reduction_on = audio_handle->is_noise_reduction;
	return 0;
}

int _callmgr_audio_is_ringtone_mode(callmgr_audio_handle_h audio_handle, gboolean *o_is_ringtone_mode)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_is_ringtone_mode, -1);

	sound_type_e sound_type = SOUND_TYPE_SYSTEM;
	*o_is_ringtone_mode = FALSE;

	int ret = sound_manager_get_sound_type(audio_handle->sound_stream_handle, &sound_type);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("sound_manager_get_sound_type() failed:[%d]", ret);
		return -1;
	}

	if (sound_type == SOUND_TYPE_RINGTONE) {
		*o_is_ringtone_mode = TRUE;
	}

	return 0;
}

int _callmgr_audio_get_current_volume(callmgr_audio_handle_h audio_handle, int *o_volume)
{
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_volume, -1);

	int volume = 0, ret;

	ret = sound_manager_get_volume(SOUND_TYPE_CALL, &volume);
	if (ret != SOUND_MANAGER_ERROR_NONE) {
		err("_callmgr_audio_get_current_volume() failed:[%d]", ret);
		return -1;
	}

	*o_volume = volume;

	return 0;
}

int _callmgr_audio_get_session_mode(callmgr_audio_handle_h audio_handle, callmgr_audio_session_mode_e *o_session_mode)
{
	info(">>");
	CM_RETURN_VAL_IF_FAIL(audio_handle, -1);
	CM_RETURN_VAL_IF_FAIL(o_session_mode, -1);

	*o_session_mode = audio_handle->current_mode;

	return 0;
}

