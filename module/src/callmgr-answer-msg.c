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

#include <glib.h>
#include <player.h>

#include "callmgr-log.h"
#include "callmgr-answer-msg.h"

#define CALLMGR_ANSWERING_BEEP_PATH		MODULE_RES_DIR "/Beep.ogg"

static void *g_finished_cb;
static void *g_user_data;

struct __answer_msg_data {
	player_h player_handle;		/**< Bike mode Handle to MM Player */

	callmgr_answer_msg_play_finished_cb finished_cb;
	void *user_data;
};

static int __callmgr_answer_msg_destory_player(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");

	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);
	int ret = PLAYER_ERROR_NONE;
	int ret_val = 0;

	if (answer_msg_handle->player_handle!= NULL) {
		/* Destroy the player */
		ret = player_destroy(answer_msg_handle->player_handle);
		if (ret != PLAYER_ERROR_NONE) {
			err("player_destroy ERR: %x!!!!", ret);
			ret_val = -1;
		}
		answer_msg_handle->player_handle = NULL;
	}
	return ret_val;
}

static int __callmgr_answer_msg_create_player(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");

	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);
	int ret = PLAYER_ERROR_NONE;

	if (answer_msg_handle->player_handle!= NULL) {
		__callmgr_answer_msg_destory_player(answer_msg_handle);
	}

	/* Create the Player */
	ret = player_create(&answer_msg_handle->player_handle);
	if (ret != PLAYER_ERROR_NONE || answer_msg_handle->player_handle == NULL) {
		err("player_create ERR: %x!!!!", ret);
		return -1;
	}

	return 0;
}

static void __callmgr_answer_msg_player_prepare_cb(void *data)
{
	dbg("..");
	CM_RETURN_IF_FAIL(data);

	player_h h_player_handle = NULL;
	int ret = PLAYER_ERROR_NONE;

	if (data == NULL) {
		err("Handle = NULL, returning");
		return;
	}

	h_player_handle = (player_h)data;

	/* Start the player */
	ret = player_start(h_player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_start ERR: %x!!!", ret);
	}

	return;
}

static int __callmgr_answer_msg_start_play(callmgr_answer_msg_h answer_msg_handle, const char *uri, player_completed_cb  callback)
{
	dbg("..");
	int ret = PLAYER_ERROR_NONE;
	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);

	if (answer_msg_handle->player_handle == NULL) {
		err("player_handle = NULL, returning");
		return -1;
	}

	dbg("------ URI:[%s] ------", uri);
	/* Set the URI */
	ret = player_set_uri(answer_msg_handle->player_handle, uri);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_set_uri() ERR: %x!!!!", ret);
		return -1;
	}

	/* Set the Sound type */
	ret = player_set_sound_type(answer_msg_handle->player_handle, SOUND_TYPE_CALL);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_set_sound_type() ERR: %x!!!!", ret);
		return -1;
	}

	/* Register the callback to be called upon completion of tone play */
	if (callback) {
		player_set_completed_cb(answer_msg_handle->player_handle, callback, answer_msg_handle);
	}

	/* Prepare the media player for playback */
	ret = player_prepare_async(answer_msg_handle->player_handle, __callmgr_answer_msg_player_prepare_cb,
		answer_msg_handle->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_prepare_async ERR: %x!!!!", ret);
		return -1;
	}

	return 0;
}

static int __callmgr_answer_msg_stop_play(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");
	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);

	int ret = PLAYER_ERROR_NONE;
	int ret_val = 0;

	if (answer_msg_handle->player_handle== NULL) {
		err("player_handle = NULL, returning");
		return -1;
	}

	/* Stop the player */
	ret = player_stop(answer_msg_handle->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_stop ERR: %x!!!!", ret);
		ret_val = -1;
	}

	/* Unprepare the player */
	ret = player_unprepare(answer_msg_handle->player_handle);
	if (ret != PLAYER_ERROR_NONE) {
		err("player_unprepare ERR: %x!!!!", ret);
		ret_val = -1;
	}

	return ret_val;
}

static gboolean __callmgr_answer_msg_play_beep_finish_idle_cb(gpointer user_data)
{
	dbg("..");
	callmgr_answer_msg_h answer_msg_handle = (callmgr_answer_msg_h)user_data;

	if (answer_msg_handle->finished_cb) {
		answer_msg_handle->finished_cb(answer_msg_handle->user_data);
	}
	_callmgr_answer_msg_stop_msg(answer_msg_handle);
	return FALSE;
}

static void __callmgr_answer_msg_play_beep_finish_cb(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");
	CM_RETURN_IF_FAIL(answer_msg_handle);

	g_idle_add(__callmgr_answer_msg_play_beep_finish_idle_cb, answer_msg_handle);
}

static gboolean __callmgr_answer_msg_play_ment_finish_idle_cb(gpointer user_data)
{
	dbg("..");
	callmgr_answer_msg_h answer_msg_handle = (callmgr_answer_msg_h)user_data;
	/* Stop and unprepare player */
	__callmgr_answer_msg_stop_play(answer_msg_handle);
	__callmgr_answer_msg_start_play(answer_msg_handle, CALLMGR_ANSWERING_BEEP_PATH, __callmgr_answer_msg_play_beep_finish_cb);
	return FALSE;
}

static void __callmgr_answer_msg_play_ment_finish_cb(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");
	CM_RETURN_IF_FAIL(answer_msg_handle);
	g_idle_add(__callmgr_answer_msg_play_ment_finish_idle_cb, answer_msg_handle);
}

int _callmgr_answer_msg_init(callmgr_answer_msg_h *answer_msg_handle)
{
	dbg("..");
	struct __answer_msg_data *handle = NULL;
	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);

	handle = calloc(1, sizeof(struct __answer_msg_data));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	*answer_msg_handle = handle;

	dbg("answer msg init done");

	return 0;
}
int _callmgr_answer_msg_deinit(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");
	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);

	_callmgr_answer_msg_stop_msg(answer_msg_handle);
	g_free(answer_msg_handle);

	return 0;
}

int _callmgr_answer_msg_play_msg(callmgr_answer_msg_h answer_msg_handle, callmgr_answer_msg_play_finished_cb finished_cb, void *user_data)
{
	dbg("..");
	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);
	char *answer_msg_path = NULL;
	int ret_val = 0;

	if (__callmgr_answer_msg_create_player(answer_msg_handle) < 0) {
		err("__callmgr_answer_msg_create_player() ERR");
		return -1;
	}

	_callmgr_vconf_get_answer_message_path(&answer_msg_path);
	CM_RETURN_VAL_IF_FAIL(answer_msg_path, -1);

	answer_msg_handle->finished_cb = finished_cb;
	answer_msg_handle->user_data = user_data;

	if(__callmgr_answer_msg_start_play(answer_msg_handle, answer_msg_path, __callmgr_answer_msg_play_ment_finish_cb) < 0) {
		err("__callmgr_answer_msg_start_play() ERR");
		ret_val = -1;
	}
	g_free(answer_msg_path);
	return ret_val;
}

int _callmgr_answer_msg_stop_msg(callmgr_answer_msg_h answer_msg_handle)
{
	dbg("..");
	CM_RETURN_VAL_IF_FAIL(answer_msg_handle, -1);

	answer_msg_handle->finished_cb = NULL;
	answer_msg_handle->user_data = NULL;

	__callmgr_answer_msg_stop_play(answer_msg_handle);
	__callmgr_answer_msg_destory_player(answer_msg_handle);

	return 0;
}

