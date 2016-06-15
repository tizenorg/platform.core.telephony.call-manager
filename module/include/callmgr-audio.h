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

#ifndef __CALLMGR_AUDIO_H__
#define __CALLMGR_AUDIO_H__

#include <glib.h>

typedef enum  {
	CALLMGR_AUDIO_SESSION_NONE_E,				/**< none */
	CALLMGR_AUDIO_SESSION_VOICE_E,				/**< Voice Session */
	CALLMGR_AUDIO_SESSION_VIDEO_E,				/**< Video Session >**/
	CALLMGR_AUDIO_SESSION_INVALID_E,			/**< Invalid Session >**/
} callmgr_audio_session_mode_e;

typedef enum  {
	CALLMGR_AUDIO_DEVICE_NONE_E = 0x00,				/**< none */
	CALLMGR_AUDIO_DEVICE_SPEAKER_E = 0x01,			/**< System LoudSpeaker */
	CALLMGR_AUDIO_DEVICE_RECEIVER_E = 0x02,			/**< System Receiver*/
	CALLMGR_AUDIO_DEVICE_EARJACK_E = 0x04,			/**< Earjack*/
	CALLMGR_AUDIO_DEVICE_BT_E = 0x08,				/**< BT Headset */
} callmgr_audio_device_e;

typedef enum  {
	CALLMGR_AUDIO_ROUTE_NONE_E = 0x00,				/**< none */
	CALLMGR_AUDIO_ROUTE_SPEAKER_E = CALLMGR_AUDIO_DEVICE_SPEAKER_E,			/**< System LoudSpeaker path */
	CALLMGR_AUDIO_ROUTE_RECEIVER_E = CALLMGR_AUDIO_DEVICE_RECEIVER_E,			/**< System Receiver*/
	CALLMGR_AUDIO_ROUTE_EARJACK_E = CALLMGR_AUDIO_DEVICE_EARJACK_E,			/**< Earjack path*/
	CALLMGR_AUDIO_ROUTE_BT_E = CALLMGR_AUDIO_DEVICE_BT_E,				/**< System BT Headset path */
	CALLMGR_AUDIO_ROUTE_RECEIVER_EARJACK_E = CALLMGR_AUDIO_DEVICE_RECEIVER_E|CALLMGR_AUDIO_DEVICE_EARJACK_E,
	CALLMGR_AUDIO_ROUTE_SPEAKER_EARJACK_E = CALLMGR_AUDIO_DEVICE_SPEAKER_E|CALLMGR_AUDIO_DEVICE_EARJACK_E,
} callmgr_audio_route_e;

// TODO:
//Add more events
typedef enum {
	CM_AUDIO_EVENT_PATH_CHANGED_E,
	CM_AUDIO_EVENT_EARJACK_CHANGED_E,
	CM_AUDIO_EVENT_VOLUME_CHANGED_E,
	CM_AUDIO_EVENT_BT_CHANGED_E,
} cm_audio_event_type_e;

typedef struct __audio_data *callmgr_audio_handle_h;
typedef void (*audio_event_cb) (cm_audio_event_type_e event_type, void *event_data, void *user_data);

int _callmgr_audio_init(callmgr_audio_handle_h *audio_handle, audio_event_cb cb_fn, void *user_data);
int _callmgr_audio_deinit(callmgr_audio_handle_h audio_handle);
int _callmgr_audio_create_call_sound_session(callmgr_audio_handle_h audio_handle, callmgr_audio_session_mode_e session_mode);
int _callmgr_audio_stop_virtual_stream(callmgr_audio_handle_h audio_handle);
int _callmgr_audio_destroy_call_sound_session(callmgr_audio_handle_h audio_handle);
int _callmgr_audio_set_audio_route(callmgr_audio_handle_h audio_handle, callmgr_audio_route_e route);
int _callmgr_audio_get_audio_route(callmgr_audio_handle_h audio_handle, callmgr_audio_route_e *o_route);
int _callmgr_audio_set_extra_vol(callmgr_audio_handle_h audio_handle, gboolean is_extra_vol);
int _callmgr_audio_get_extra_vol(callmgr_audio_handle_h audio_handle, gboolean *o_is_extra_vol);

int _callmgr_audio_set_audio_tx_mute(callmgr_audio_handle_h audio_handle, gboolean is_mute_state);
int _callmgr_audio_get_audio_tx_mute_state(callmgr_audio_handle_h audio_handle, gboolean *o_is_mute_state);

int _callmgr_audio_set_noise_reduction(callmgr_audio_handle_h audio_handle, gboolean is_noise_reduction_on);
int _callmgr_audio_get_noise_reduction(callmgr_audio_handle_h audio_handle, gboolean *o_is_noise_reduction_on);

int _callmgr_audio_is_sound_device_available(callmgr_audio_device_e device_type, gboolean *is_available);
int _callmgr_audio_is_ringtone_mode(callmgr_audio_handle_h audio_handle, gboolean *o_is_ringtone_mode);

int _callmgr_audio_set_link_direction_uplink(void);
int _callmgr_audio_set_link_direction_downlink(void);
int _callmgr_audio_set_media_mode_with_current_device(void);

int _callmgr_audio_get_current_volume(callmgr_audio_handle_h audio_handle, int *o_volume);
int _callmgr_audio_get_session_mode(callmgr_audio_handle_h audio_handle, callmgr_audio_session_mode_e *o_session_mode);

#endif	//__CALLMGR_AUDIO_H__

