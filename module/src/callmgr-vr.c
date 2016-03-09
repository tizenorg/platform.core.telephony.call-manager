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

#include <Ecore.h>
#include <recorder.h>
#include <sys/sysinfo.h>
#include <sys/statfs.h>
#include <storage.h>
#include <dirent.h>
#include <unistd.h>
#include <media_content.h>
#if 0 /*remove media product header*/
#include <media_cloud_info_product.h>
#endif

#include "callmgr-vr.h"
#include "callmgr-util.h"
#include "callmgr-log.h"

#define CM_VR_FILENAME_LEN_MAX	4096
#define CM_VR_FILE_NUMBER_MAX	20
#define CM_VR_RECORD_TIME_MIN	1000	/*millisecond*/

struct __vr_data {
	/*Public*/
	callmgr_vr_state_e record_state;			/**<  Holds the current status while recording*/

	/*Private*/
	time_t start_time;						/**<  Use start time for file name*/
	long elapsed_time;					/*recording duration*/
	gboolean is_answering;					/**<  Answering machine feature*/
	char disp_name[256];				/**<  Use call number for file name*/
	recorder_h recorder_handle;				/* Recorder handle */
	unsigned long max_time;					/**<  Holds the max time to be used for recording*/
	char file_path[CM_VR_FILENAME_LEN_MAX];				/**<  Holds the path of the recorder file*/
	char file_name[CM_VR_FILENAME_LEN_MAX];				/**<  Holds the filename of the recorder file*/

	/*For Call back*/
	vr_event_cb cb_fn;
	void *user_data;
};

typedef enum __callmgr_vr_cb_type_t {
	CM_VR_CB_STATE_CHANGED,
	CM_VR_CB_STATUS,
	CM_VR_CB_LIMIT_REACHED,
	CM_VR_CB_ERROR,
	CM_VR_CB_NUM,
} callmgr_vr_cb_type_t;

typedef enum __callmgr_vr_filter_type_t {
	CM_VR_FILTER_ALL,
	CM_VR_FILTER_PLAYED,
	CM_VR_FILTER_UNPLAYED,
	CM_VR_FILTER_MAX
} callmgr_vr_filter_type_t;

typedef struct {
	recorder_state_e previous;
	recorder_state_e current;
	bool by_policy;
} callmgr_vr_state_changed_cb_data_t;

typedef struct {
	unsigned long long elapsed_time;
	unsigned long long file_size;
} callmgr_vr_recording_status_cb_data_t;

typedef struct {
	recorder_recording_limit_type_e limit_type;
} callmgr_vr_limit_reached_cb_data_t;

typedef struct {
	recorder_error_e error;
	recorder_state_e current_state;
} callmgr_vr_error_cb_data_t;

typedef struct {
	callmgr_vr_cb_type_t cb_type;

	union {
		callmgr_vr_state_changed_cb_data_t state_changed ;
		callmgr_vr_recording_status_cb_data_t recording_status;
		callmgr_vr_limit_reached_cb_data_t limit_reached;
		callmgr_vr_error_cb_data_t error;
	} cb_data;

	void *user_data;
} callmgr_vr_cb_info_t;

#define CM_VR_FEX_CLIPS_PATH_PHONE		"/opt/usr/media/Sounds"
#define CM_VR_FEX_CLIPS_PATH_MMC		"/opt/storage/sdcard/Sounds/Voice recorder"
#define CM_VR_ANSWER_MEMO_FOLDER_NAME		".AnswerMessage"

#define CM_VR_DEFAULT_FILENAME		"Voicecall"
#define CM_VR_DEFAULT_EXT			".amr"
#define CM_VR_FILE_NAME				CM_VR_DEFAULT_FILENAME""CM_VR_DEFAULT_EXT
#define CM_VR_TEMP_FILENAME			CM_VR_FEX_CLIPS_PATH_PHONE"/"CM_VR_FILE_NAME

#define CM_VR_SAMPLERATE_LOW		  8000
#define CM_VR_ENCODER_BITRATE_AMR		12200
#define CM_VR_SOURCE_CHANNEL				1
#define CM_VR_ANSWERING_MSG_MAX_TIME_LIMIT		178
#define CM_VR_RECORD_MAX_TIME_LIMIT		(1*60*60) /* 1*60*60 Seconds - 1 hours */

#define CM_VR_INVALID_HANDLE			NULL

static callmgr_vr_store_type_e g_storage_type =  CALLMGR_VR_STORAGE_PHONE;

static gboolean __callmgr_vr_is_available_memory(void);
static int __callmgr_vr_fex_get_available_memory_space(callmgr_vr_store_type_e store_type);
static int __callmgr_vr_stop_record_internal(callmgr_vr_handle_h vr_handle);

static gboolean __callmgr_vr_state_changed_cb_real(void *user_data)
{
	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)user_data;
	callmgr_vr_handle_h vr_handle = (callmgr_vr_handle_h )info->user_data;
	callmgr_vr_status_extra_type_e extra_data;

	/*recorder_state_e previous, recorder_state_e current, bool by_policy,*/

	info("Current : %d, Previous : %d, by_policy : %d",
			info->cb_data.state_changed.current, info->cb_data.state_changed.previous, info->cb_data.state_changed.by_policy);

	if (info->cb_data.state_changed.previous == RECORDER_STATE_READY &&
			info->cb_data.state_changed.current == RECORDER_STATE_RECORDING) {
		/* Todo Need to send event to UI... */
		/*vc_engine_recording_status event_data;

		memset(&event_data, 0, sizeof(event_data));
		if (papp_rec->bnormal_rec  == TRUE) {
			event_data.type = VCALL_ENGINE_REC_TYPE_NORMAL;
		} else {
			event_data.type = VCALL_ENGINE_REC_TYPE_ANSWER_MSG;
		}

		vcall_engine_send_event_to_client(VC_ENGINE_MSG_START_RECORDING_TO_UI, (void *)&event_data);*/
		if (vr_handle->is_answering)
			extra_data = CALLMGR_VR_STATUS_EXTRA_START_TYPE_ANSWER_MSG;
		else
			extra_data = CALLMGR_VR_STATUS_EXTRA_START_TYPE_NORMAL;

		vr_handle->cb_fn(CALLMGR_VR_STATUS_EVENT_STARTED, extra_data, vr_handle->user_data);
	}

	g_free(info);

	return FALSE;
}

/*static gboolean __callmgr_vr_recording_status_changed_cb_real(void *user_data)
{
	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)user_data;
	callmgr_vr_handle_h *vr_handle = (callmgr_vr_handle_h *)info->user_data;

	g_free(info);

	return FALSE;
}*/

static gboolean __callmgr_vr_limit_reached_cb_real(void *user_data)
{
	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)user_data;
	callmgr_vr_handle_h vr_handle = (callmgr_vr_handle_h )info->user_data;
	callmgr_vr_status_extra_type_e extra_data = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NORMAL;
	int result = 0;

	CM_RETURN_VAL_IF_FAIL(vr_handle, FALSE);

	info("RECORDER LIMIT REACHED (limit_type : [%d])", info->cb_data.limit_reached.limit_type);

	switch (info->cb_data.limit_reached.limit_type) {
	case RECORDER_RECORDING_LIMIT_SIZE:
		dbg("MAX SIZE");
		extra_data = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_MAX_SIZE;
		break;

	case RECORDER_RECORDING_LIMIT_FREE_SPACE:
		dbg("No Free Space");
		extra_data = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NO_FREE_SPACE;
		break;

	case RECORDER_RECORDING_LIMIT_TIME:
		dbg("TIME LIMIT");
		extra_data = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_TIME_LIMIT;
		break;

	default:
		dbg("Not defined.");
		extra_data = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_ERROR;
		break;
	}

	result = __callmgr_vr_stop_record_internal(vr_handle);
	if (result != 0) {
		extra_data = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_ERROR;
	}
	vr_handle->cb_fn(CALLMGR_VR_STATUS_EVENT_STOPPED, extra_data, vr_handle->user_data);

	g_free(info);

	return FALSE;
}

static gboolean __callmgr_vr_error_cb_real(void *user_data)
{
	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)user_data;

	err("!!! ERROR[%d], current state[%d]", info->cb_data.error.error, info->cb_data.error.current_state);

	g_free(info);

	return FALSE;
}

static void __callmgr_vr_state_changed_cb(recorder_state_e previous, recorder_state_e current, bool by_policy, void *user_data)
{
	callmgr_vr_handle_h vr_handle = (callmgr_vr_handle_h)user_data;
	CM_RETURN_IF_FAIL(vr_handle);

	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)calloc(1, sizeof(callmgr_vr_cb_info_t));
	if(NULL == info) {
		err("memory allocation failed...");
		return;
	}

	info("previous[%d], current[%d], by_policy[%d]", previous, current, by_policy);

	info->cb_type = CM_VR_CB_STATE_CHANGED;
	info->cb_data.state_changed.previous = previous;
	info->cb_data.state_changed.current = current;
	info->cb_data.state_changed.by_policy = by_policy;
	info->user_data = vr_handle;
	g_idle_add(__callmgr_vr_state_changed_cb_real, info);
}

static void __callmgr_vr_recording_status_changed_cb(unsigned long long elapsed_time, unsigned long long file_size, void *user_data)
{
	callmgr_vr_handle_h vr_handle = (callmgr_vr_handle_h)user_data;
	CM_RETURN_IF_FAIL(vr_handle);

//	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)calloc(1, sizeof(callmgr_vr_cb_info_t));
//	if(NULL == info) {
//		err("memory allocation failed...");
//		return;
//	}
	vr_handle->elapsed_time = elapsed_time;
	if (elapsed_time % 10000 == 0)	// leave a log every 10 seconds
		info("elapsed_time[%d], file_size[%d]", elapsed_time, file_size);

//	info->cb_type = CM_VR_CB_STATUS;
//	info->cb_data.recording_status.elapsed_time = elapsed_time;
//	info->cb_data.recording_status.file_size = file_size;
//	info->user_data = vr_handle;
//	g_idle_add(__callmgr_vr_recording_status_changed_cb_real, info);
}

static void __callmgr_vr_limit_reached_cb(recorder_recording_limit_type_e limit_type, void *user_data)
{
	callmgr_vr_handle_h vr_handle = (callmgr_vr_handle_h)user_data;
	CM_RETURN_IF_FAIL(vr_handle);

	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)calloc(1, sizeof(callmgr_vr_cb_info_t));
	if(NULL == info) {
		err("memory allocation failed...");
		return;
	}

	info("limit_type[%d]", limit_type);

	info->cb_type = CM_VR_CB_LIMIT_REACHED;
	info->cb_data.limit_reached.limit_type = limit_type;
	info->user_data = vr_handle;
	g_idle_add(__callmgr_vr_limit_reached_cb_real, info);
}

static void __callmgr_vr_error_cb(recorder_error_e error, recorder_state_e current_state, void *user_data)
{
	callmgr_vr_handle_h vr_handle = (callmgr_vr_handle_h)user_data;
	CM_RETURN_IF_FAIL(vr_handle);

	callmgr_vr_cb_info_t *info = (callmgr_vr_cb_info_t *)calloc(1, sizeof(callmgr_vr_cb_info_t));
	if(NULL == info) {
		err("memory allocation failed...");
		return;
	}

	info("error[%d], current_state[%d]", error, current_state);

	info->cb_type = CM_VR_CB_ERROR;
	info->cb_data.error.error = error;
	info->cb_data.error.current_state = current_state;
	info->user_data = vr_handle;
	g_idle_add(__callmgr_vr_error_cb_real, info);
}

static int __callmgr_vr_ready(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;

	if (vr_handle->recorder_handle == NULL) {
		err("Invalid recorder handle");
		return -1;
	}

	ret_val = recorder_attr_set_time_limit(vr_handle->recorder_handle, vr_handle->max_time);
	if (ret_val != RECORDER_ERROR_NONE) {
		err("recorder_attr_set_time_limit");
		goto READY_FAIL;
	}

	recorder_state_e rec_status = RECORDER_STATE_NONE;
	recorder_get_state(vr_handle->recorder_handle, &rec_status);
	dbg("rec_status = %d ", rec_status);

	if (rec_status == RECORDER_STATE_READY) {
		dbg("recorder state is already ready.");
		return 0;
	}

	ret_val = recorder_prepare(vr_handle->recorder_handle);
	if (ret_val == RECORDER_ERROR_NONE) {
		dbg("recorder_prepare success");
	} else {
		err("recorder_prepare");
		goto READY_FAIL;
	}

	return 0;

READY_FAIL:
	err("Ready fail : 0x%x", ret_val);
	return -1;
}

static int __callmgr_vr_reset(callmgr_vr_handle_h vr_handle, const char* call_num, const char* call_name, gboolean is_answering_machine)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);

	vr_handle->is_answering = is_answering_machine;
	vr_handle->elapsed_time = 0;

	/* Update disp name */
	memset(vr_handle->disp_name, 0x0, sizeof(vr_handle->disp_name));
	dbg("call_number [%s] call_name[%s]", call_num, call_name);
	if (call_name != NULL && strlen(call_name)) {
		strncpy(vr_handle->disp_name, call_name, 256 - 1);
	} else {
		if (call_num != NULL && strlen(call_num)) {
			strncpy(vr_handle->disp_name, call_num, 256 - 1);
		} else {
			strncpy(vr_handle->disp_name, "Unknown", 256 - 1);
		}
	}
	/* Reset the time */
	vr_handle->start_time = time(NULL);

	if (vr_handle->is_answering == EINA_TRUE) {
		vr_handle->max_time = CM_VR_ANSWERING_MSG_MAX_TIME_LIMIT;
	} else {
		vr_handle->max_time = CM_VR_RECORD_MAX_TIME_LIMIT;
	}

	return 0;
}

static int __callmgr_vr_create(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;

	ret_val = recorder_create_audiorecorder(&(vr_handle->recorder_handle));

	if (ret_val == RECORDER_ERROR_NONE) {
		ret_val = recorder_set_state_changed_cb(vr_handle->recorder_handle,
				__callmgr_vr_state_changed_cb, vr_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_state_changed_cb");
		}

		ret_val = recorder_set_recording_status_cb(vr_handle->recorder_handle,
				__callmgr_vr_recording_status_changed_cb, vr_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_recording_status_cb");
		}

		ret_val = recorder_set_recording_limit_reached_cb(vr_handle->recorder_handle,
				__callmgr_vr_limit_reached_cb, vr_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_recording_limit_reached_cb");
		}

		ret_val = recorder_set_error_cb(vr_handle->recorder_handle,
				__callmgr_vr_error_cb, vr_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_error_cb");
		}

		ret_val = recorder_attr_set_audio_device(vr_handle->recorder_handle, RECORDER_AUDIO_DEVICE_MIC);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_attr_set_audio_device");
		}

		ret_val = recorder_set_audio_encoder(vr_handle->recorder_handle, RECORDER_AUDIO_CODEC_AMR);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_audio_encoder");
		}

		ret_val = recorder_set_file_format(vr_handle->recorder_handle, RECORDER_FILE_FORMAT_AMR);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_file_format");
		}

		ret_val = recorder_attr_set_audio_samplerate(vr_handle->recorder_handle, CM_VR_SAMPLERATE_LOW);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_attr_set_audio_samplerate");
		}

		ret_val = recorder_attr_set_audio_encoder_bitrate(vr_handle->recorder_handle, CM_VR_ENCODER_BITRATE_AMR);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_attr_set_audio_encoder_bitrate");
		}

		ret_val = recorder_set_filename(vr_handle->recorder_handle, CM_VR_TEMP_FILENAME);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_set_filename");
		}

		ret_val = recorder_attr_set_audio_channel(vr_handle->recorder_handle, CM_VR_SOURCE_CHANNEL);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_attr_set_audio_channel");
		}
	} else {
		err("recorder_create_audiorecorder error : %d", ret_val);
		return -1;
	}

	vr_handle->record_state = CALLMGR_VR_INITIALIZED;

	return 0;
}

static int __callmgr_vr_cancel(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;

	if (vr_handle->recorder_handle == NULL) {
		err("Invalid recorder handle");
		return -1;
	}

	recorder_state_e rec_status = RECORDER_STATE_NONE;
	recorder_get_state(vr_handle->recorder_handle, &rec_status);
	dbg("rec_status = %d ", rec_status);

	if (rec_status == RECORDER_STATE_PAUSED || rec_status == RECORDER_STATE_RECORDING) {
		ret_val = recorder_cancel(vr_handle->recorder_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_cancel err = %d ", ret_val);
			return -1;
		}
	}

	vr_handle->record_state = CALLMGR_VR_INITIALIZED;
	return 0;
}

static int __callmgr_vr_unrealize(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;

	if (vr_handle->recorder_handle == NULL) {
		err("Invalid recorder handle");
		return -1;
	}

	recorder_state_e rec_status = RECORDER_STATE_NONE;
	recorder_get_state(vr_handle->recorder_handle, &rec_status);
	dbg("rec_status = %d ", rec_status);

	if (rec_status == RECORDER_STATE_READY) {
		ret_val = recorder_unprepare(vr_handle->recorder_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_unprepare err = %d ", ret_val);
			return -1;
		}
	}

	vr_handle->record_state = CALLMGR_VR_INITIALIZED;
	return 0;
}

static int __callmgr_vr_destroy(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;

	if (vr_handle->recorder_handle == NULL) {
		err("Invalid recorder handle");
		return -1;
	}

	recorder_unset_recording_limit_reached_cb(vr_handle->recorder_handle);
	recorder_unset_recording_status_cb(vr_handle->recorder_handle);
	recorder_unset_state_changed_cb(vr_handle->recorder_handle);

	__callmgr_vr_cancel(vr_handle);
	__callmgr_vr_unrealize(vr_handle);

	recorder_state_e rec_status = RECORDER_STATE_NONE;
	recorder_get_state(vr_handle->recorder_handle, &rec_status);
	dbg("rec_status = %d ", rec_status);

	if (rec_status == RECORDER_STATE_CREATED) {
		ret_val = recorder_destroy(vr_handle->recorder_handle);
		dbg("recorder_destroy = %d ", ret_val);
	}

	vr_handle->is_answering = FALSE;
	vr_handle->record_state = CALLMGR_VR_NONE;
	vr_handle->recorder_handle = NULL;

	return 0;
}

static int __callmgr_vr_commit(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;

	if (vr_handle->recorder_handle == NULL) {
		err("Invalid recorder handle");
		return -1;
	}

	recorder_state_e rec_status = RECORDER_STATE_NONE;
	recorder_get_state(vr_handle->recorder_handle, &rec_status);
	dbg("rec_status = %d ", rec_status);

	if (rec_status == RECORDER_STATE_PAUSED || rec_status == RECORDER_STATE_RECORDING) {
		ret_val = recorder_commit(vr_handle->recorder_handle);
		if (ret_val != RECORDER_ERROR_NONE) {
			err("recorder_commit err = %d ", ret_val);
			return -1;
		}
	}

	vr_handle->record_state = CALLMGR_VR_INITIALIZED;
	return 0;
}

int _callmgr_vr_init(callmgr_vr_handle_h *vr_handle, vr_event_cb cb_fn, void *user_data)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	struct __vr_data *handle = NULL;

	handle = calloc(1, sizeof(struct __vr_data));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	/* Initialize Variables */
	handle->cb_fn = cb_fn;
	handle->user_data = user_data;
	handle->is_answering = FALSE;
	handle->record_state = CALLMGR_VR_NONE;
	handle->recorder_handle = NULL;
	*vr_handle = handle;

	return 0;
}

int _callmgr_vr_deinit(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	if (vr_handle == NULL) {
		err("vr_handle is NULL");
		return -1;
	} else {
		if (vr_handle->recorder_handle != NULL) {
			__callmgr_vr_destroy(vr_handle);
		}
	}

	g_free(vr_handle);
	vr_handle = NULL;

	return 0;
}
static int __callmgr_vr_record(callmgr_vr_handle_h vr_handle)
{
	int ret_val = RECORDER_ERROR_NONE;

	dbg("<<");
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);

	ret_val = recorder_start(vr_handle->recorder_handle);
	if (RECORDER_ERROR_NONE == ret_val) {
		dbg("Current record state:%d", vr_handle->record_state);
		vr_handle->record_state = CALLMGR_VR_RECORDING;
	} else {
		err("recorder_start api failed , error: [%d]", ret_val);
		return -1;
	}
	return 0;
}
int _callmgr_vr_start_record(callmgr_vr_handle_h vr_handle, const char* call_num, const char* call_name, gboolean is_answering_machine)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret_val = RECORDER_ERROR_NONE;
	callmgr_vr_state_e cur_vr_state = CALLMGR_VR_NONE;

	if (__callmgr_vr_is_available_memory() == FALSE) {
		err("No free space");
		_callmgr_util_launch_popup(CALL_POPUP_REC_STATUS, CM_UTIL_REC_STATUS_STOP_BY_NO_ENOUGH_MEMORY, NULL, 0, NULL, NULL);
		/*
		 * Todo need to launch "Unable to save data" popup.
		 * (refer [Tizen_2.4]Voice_Call_UI_[Z3].pdf, [Tizen_2.4]Pattern_UI_[Z3].pdf)
		 */

		return -1;
	}

	_callmgr_vr_get_recording_state(vr_handle, &cur_vr_state);
	__callmgr_vr_reset(vr_handle, call_num, call_name, is_answering_machine);

	switch (cur_vr_state) {
	case CALLMGR_VR_NONE:
		{
			ret_val = __callmgr_vr_create(vr_handle);
			if (ret_val >= 0) {
/* Todo need to add check for answering machine */
/*
	#ifdef _VC_ANSWERING_MEMO
				if (FALSE == bnormal_rec) {
					voicecall_answer_msg_set_recording_session(pcall_core->papp_snd);
				}
	#endif
*/
				ret_val = __callmgr_vr_ready(vr_handle);
				if (ret_val < 0) {
					err("__callmgr_vr_ready error : %d", ret_val);
					return -1;
				}
				ret_val = __callmgr_vr_record(vr_handle);
				if (ret_val < 0) {
					err("__callmgr_vr_record failed");
					return -1;
				}
			}
		}
		break;

	case CALLMGR_VR_INITIALIZED:
		{
			ret_val = __callmgr_vr_ready(vr_handle);
			if (ret_val < 0) {
				err("__callmgr_vr_ready error : %d", ret_val);
				return -1;
			}
			ret_val = __callmgr_vr_record(vr_handle);
			if (ret_val < 0) {
				err("__callmgr_vr_record failed : %d", ret_val);
				return -1;
			}
		}
		break;

	case CALLMGR_VR_RECORDING:
		{
			dbg("Recording already in Progress");
		}
		break;

	default:
		err("Invalid State[%d].", cur_vr_state);
		break;
	}

	vr_handle->cb_fn(CALLMGR_VR_STATUS_EVENT_STARTED, CALLMGR_VR_STATUS_EXTRA_START_TYPE_NORMAL, vr_handle->user_data);

	return 0;
}
static unsigned long __callmgr_vr_get_file_size_by_filename(const char *filename)
{
	dbg("..");
	struct stat buf;
	if (stat(filename, &buf) != 0)
		return -1;
	return ((unsigned long)buf.st_size);
}
static int __callmgr_vr_is_dir_exist(const char *dir_path)
{
	DIR *dp = NULL;
	dbg("dir_path = %s", dir_path);

	dp = opendir(dir_path);
	if (dp != NULL) {
		closedir(dp);
		return 0;
	} else {
		return -1;
	}

	return -1;
}
static int __callmgr_vr_make_dir(const char *dir_path, mode_t mode)
{
	/* Make dir recursively */
	char *mpath;
	int i;
	char *opath;

	if (dir_path == NULL) {
		return -1;
	}

	if (__callmgr_vr_is_dir_exist(dir_path) >= 0) {
		/* Already exists */
		return 0;
	}

	i = 0;
	mpath = (char *)malloc(strlen(dir_path) + 1);
	if (mpath == NULL) {
		err("Error!! malloc()");
		return -1;
	}

	opath = (char *)dir_path;

	while (*opath) {
		mpath[i++] = *opath++;
		if (*opath == '/') {
			mpath[i] = '\0';
			if (!g_strcmp0(".", mpath) || !g_strcmp0("..", mpath)) {
				continue;
			}

			if (__callmgr_vr_is_dir_exist(mpath) < 0) {
				if (mkdir(mpath, mode)) {
					free(mpath);
					return -1;
				}
			}
		}
	}

	mpath[i] = '\0';
	if (__callmgr_vr_is_dir_exist(mpath) < 0) {
		if (mkdir(mpath, mode)) {
			free(mpath);
			return -1;
		}
	}

	free(mpath);
	return 0;
}
static char *__callmgr_vr_get_start_time(time_t start_time)
{
	char str_time[15] = {0, };
	struct tm *tm_ptr = NULL;
	struct tm tm_local;
	tm_ptr = localtime_r(&start_time, &tm_local);
	if (tm_ptr == NULL) {
		err("Fail to get localtime !");
		return NULL;
	}
	strftime(str_time, sizeof(str_time), "%Y%m%d%H%M%S", tm_ptr);
	dbg("str : %s", str_time);

	return strdup(str_time);
}
static int __callmgr_vr_create_filename(callmgr_vr_handle_h vr_handle)
{
	unsigned long rec_size = 0;
	int fex_remain_size = 0;
	char tmp_str[CM_VR_FILENAME_LEN_MAX];

	dbg(">>");

	rec_size = __callmgr_vr_get_file_size_by_filename(CM_VR_TEMP_FILENAME);
	fex_remain_size = __callmgr_vr_fex_get_available_memory_space(g_storage_type);
	if (rec_size == 0) {
		err("Can't save: empty file!!");
		return -1;
	} else if ((int)(rec_size / 1024) > fex_remain_size
			&& (g_storage_type != CALLMGR_VR_STORAGE_PHONE)) {
		err("Can't save: short of memory!!");
		return -1;
	} else {
		char *str_time = NULL;
		info("Can save: recorded[%d Byte, (=%d KB)], available[%d KB].", (int)rec_size, (int)(rec_size / 1024), (int)(fex_remain_size));

		/*Get the Traget Path */
		memset(vr_handle->file_path, 0, sizeof(vr_handle->file_path));
		memset(vr_handle->file_name, 0, sizeof(vr_handle->file_name));

//		if (vr_handle->is_answering == FALSE) {
			snprintf(vr_handle->file_path, sizeof(vr_handle->file_path), "%s", CM_VR_FEX_CLIPS_PATH_PHONE);
//		} else {
//			if (g_storage_type == CALLMGR_VR_STORAGE_PHONE) {
//				snprintf(vr_handle->file_path, sizeof(vr_handle->file_path), "%s/%s", CM_VR_FEX_CLIPS_PATH_PHONE, CM_VR_ANSWER_MEMO_FOLDER_NAME);
//			} else {
//				snprintf(vr_handle->file_path, sizeof(vr_handle->file_path), "%s/%s", CM_VR_FEX_CLIPS_PATH_MMC, CM_VR_ANSWER_MEMO_FOLDER_NAME);
//			}
//		}

		if (FALSE == __callmgr_vr_is_dir_exist(vr_handle->file_path)) {
			info("Directory is not exist. Create it");
			__callmgr_vr_make_dir(vr_handle->file_path, 0755);
		}
		str_time = __callmgr_vr_get_start_time(vr_handle->start_time);
		if (str_time) {
			snprintf(vr_handle->file_name, sizeof(vr_handle->file_name), "%s %s%s", vr_handle->disp_name, str_time, CM_VR_DEFAULT_EXT);
			g_free(str_time);
		} else {
			snprintf(vr_handle->file_name, sizeof(vr_handle->file_name), "%s%s", vr_handle->disp_name, CM_VR_DEFAULT_EXT);
		}

		snprintf(tmp_str, sizeof(tmp_str), "%s/%s", vr_handle->file_path, vr_handle->file_name);

		/*Set Target File Name to save the media */
		sec_dbg("Voice Recorder File Path: [%s]", vr_handle->file_path);
		sec_dbg("Voice Recorder File Name: [%s]", vr_handle->file_name);

		char dest_path[CM_VR_FILENAME_LEN_MAX] = { 0, };
		snprintf(dest_path, sizeof(dest_path), "%s/%s", vr_handle->file_path, vr_handle->file_name);

		int ret = rename(CM_VR_TEMP_FILENAME, dest_path);
		if (!ret) {
			sync();
			dbg("rename success");
		} else {
			err("rename failed");
			return -1;
		}

		return 0;
	}

}
static int __callmgr_vr_update_media_info(callmgr_vr_handle_h vr_handle)
{
	dbg("<<");

	int result = -1;
	media_info_h info = NULL;
	int ret = MEDIA_CONTENT_ERROR_NONE;
	char full_path[CM_VR_FILENAME_LEN_MAX] = { 0, };
	snprintf(full_path, sizeof(full_path), "%s/%s", vr_handle->file_path, vr_handle->file_name);

	sec_dbg("full path : %s", full_path);
	ret = media_content_connect();
	if (ret == MEDIA_CONTENT_ERROR_NONE) {
		ret = media_info_insert_to_db(full_path, &info);
		if (ret == MEDIA_CONTENT_ERROR_NONE) {
			result = 0;
		} else {
			err("media_info_insert_to_db().. [%d]", ret);
			result = -1;
		}

		if (info) {
			media_info_destroy(info);
			info = NULL;
		}

		media_content_disconnect();
	} else {
		err("media_content_connect()..failed [%d]", ret);
	}
	return result;
}
static int __callmgr_vr_create_filter(callmgr_vr_filter_type_t cond_type, filter_h *o_filter_h)
{
	int ret = MEDIA_CONTENT_ERROR_NONE;
	filter_h filter = NULL;

	CM_RETURN_VAL_IF_FAIL(o_filter_h, -1);
	dbg("filter condition type : %d", cond_type);
	const char *condition = NULL;
	switch (cond_type) {
		case CM_VR_FILTER_ALL:
			condition = "MEDIA_STORAGE_TYPE=122";
			break;
		case CM_VR_FILTER_PLAYED:
			condition = "MEDIA_STORAGE_TYPE=122 AND MEDIA_PLAYED_COUNT>0";
			break;
		case CM_VR_FILTER_UNPLAYED:
			condition = "MEDIA_STORAGE_TYPE=122 AND MEDIA_PLAYED_COUNT=0";
			break;
		default:
			err("Filter type is error");
			goto error;
			break;
	}

	ret = media_filter_create(&filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		err("media_filter_create() failed : [%d]", ret);
		goto error;
	}

	ret = media_filter_set_condition(filter, condition, MEDIA_CONTENT_COLLATE_NOCASE);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		err("media_filter_set_condition() failed : [%d]", ret);
		goto error;
	}

	ret = media_filter_set_order(filter, MEDIA_CONTENT_ORDER_ASC, MEDIA_ADDED_TIME, MEDIA_CONTENT_COLLATE_NOCASE);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		err("media_filter_set_order() failed : [%d]", ret);
		goto error;
	}
	*o_filter_h =  filter;
	return 0;

error:
	if (filter) {
		media_filter_destroy(filter);
		filter = NULL;
		*o_filter_h = NULL;
	}
	return -1;
}
static int __callmgr_vr_get_file_number_from_db(callmgr_vr_filter_type_t cond_type)
{
	int count = 0;
	int ret = MEDIA_CONTENT_ERROR_NONE;
	filter_h filter = NULL;

	ret = media_content_connect();
	if (ret == MEDIA_CONTENT_ERROR_NONE) {
		__callmgr_vr_create_filter(cond_type, &filter);
		if (filter) {
			ret = media_info_get_media_count_from_db(filter, &count);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
				err("media_info_get_media_count_from_db_with_media_mode() failed : [%d]", ret);
			}

			media_filter_destroy(filter);
			filter = NULL;
		}
		media_content_disconnect();
	} else {
		err("media_content_connect() failed : [%d]", ret);
	}

	info("File count from DB : [%d]", count);
	return count;
}
static bool __callmgr_vr_media_info_cb(media_info_h media, void *user_data)
{
	dbg("<<");
	media_info_h *dest = user_data;

	CM_RETURN_VAL_IF_FAIL(media, FALSE);
	CM_RETURN_VAL_IF_FAIL(dest, FALSE);

	media_info_clone(dest, media);
	return FALSE;
}
static void __callmgr_vr_get_oldest_file(char **media_id, char **file_path)
{
	media_info_h media = NULL;
	int ret = MEDIA_CONTENT_ERROR_NONE;
	filter_h filter = NULL;

	__callmgr_vr_create_filter(CM_VR_FILTER_PLAYED, &filter);
	if (filter) {
		ret = media_filter_set_offset(filter, 0, 1);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			media_filter_destroy(filter);
			return;
		}
		ret = media_info_foreach_media_from_db(filter, __callmgr_vr_media_info_cb, &media);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			err("media_info_get_media_count_from_db_with_media_mode() failed : [%d]", ret);
			media_filter_destroy(filter);
			return;
		}

		media_filter_destroy(filter);

		if (media_id) {
			ret = media_info_get_media_id(media, media_id);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
				err("media_info_get_media_id() failed : [%d]", ret);
			}
		}

		if (file_path) {
			ret = media_info_get_file_path(media, file_path);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
			err("media_info_get_file_path() failed : [%d]", ret);
			}
		}

		media_info_destroy(media);
	}
}
static int __callmgr_vr_files_remove_file_real(const char *full_path)
{
	CM_RETURN_VAL_IF_FAIL(full_path, -1);

	int ret = -1;

	if (access(full_path, F_OK) >= 0) {
		if (unlink(full_path) >= 0) {
			sec_dbg("Removed.. [%s]", full_path);
			ret = 0;
		} else {
			sec_dbg("ecore_file_unlink..Failed [%s]", full_path);
		}
	} else {
		sec_dbg("file is not exist.. [%s]", full_path);
		ret = 0;
	}

	return ret;
}
static int __callmgr_vr_del_read_file(void)
{
	int result = -1;

	int count = __callmgr_vr_get_file_number_from_db(CM_VR_FILTER_ALL);
	dbg("File count from DB : [%d]", count);

	if (count == CM_VR_FILE_NUMBER_MAX) {
		char *media_id = NULL;
		char *file_path = NULL;
		int ret = MEDIA_CONTENT_ERROR_NONE;

		ret = media_content_connect();
		if (ret == MEDIA_CONTENT_ERROR_NONE) {
			__callmgr_vr_get_oldest_file(&media_id, &file_path);

			if (__callmgr_vr_files_remove_file_real(file_path) >= 0) {
				int err = media_info_delete_from_db(media_id);
				if (err != MEDIA_CONTENT_ERROR_NONE) {
					err("media_info_delete_from_db Failed : [%d]", err);
					result = -1;
				}
			} else {
				err("Can not remove file");
				result = -1;
			}

			if (file_path) {
				free(file_path);
				file_path = NULL;
			}
			if (media_id) {
				free(media_id);
				media_id = NULL;
			}

			media_content_disconnect();
		} else {
			err("media_content_connect failed ; [%d]", ret);
			result = -1;
		}
	} else {
		err("No need to delete oldest file");
		result = 0;
	}

	return result;
}
static int __callmgr_vr_update_answering_msg_media_info(callmgr_vr_handle_h vr_handle)
{
	dbg("<<");

	int result = 0;
	media_info_h info = NULL;
	int ret = MEDIA_CONTENT_ERROR_NONE;
	char full_path[CM_VR_FILENAME_LEN_MAX] = { 0, };
	snprintf(full_path, sizeof(full_path), "%s/%s", vr_handle->file_path, vr_handle->file_name);

	dbg("full path : %s", full_path);
	ret = media_content_connect();
	if (ret == MEDIA_CONTENT_ERROR_NONE) {
#if 0 /*remove media product header*/
		ret = media_cloud_info_insert_to_db_without_info(full_path, MEDIA_CONTENT_STORAGE_ANSWERMEMO_EX, &info);
#endif
		if (ret == MEDIA_CONTENT_ERROR_NONE) {
			result = 0;

			ret = media_info_set_display_name(info, vr_handle->disp_name);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
				err("media_info_set_display_name().. [%d]", ret);
				result = -1;
			}

			char *display_name = NULL;
			ret = media_info_get_display_name(info, &display_name);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
				err("media_info_get_display_name().. [%d]", ret);
				result = -1;
			}
			free(display_name);
			display_name = NULL;

			ret = media_info_update_to_db(info);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
				err("media_info_update_to_db().. [%d]", ret);
				result = -1;
			}
		} else {
			err("media_cloud_info_insert_to_db_without_info().. [%d]", ret);
			result = -1;
		}
		/* info can't be true due to media_cloud_info_insert_to_db_without_info blocked*/
		/*if (info) {*/
			media_info_destroy(info);
			info = NULL;
		/*}*/

		media_content_disconnect();
	} else {
		err("media_content_connect()..failed [%d]", ret);
		result = -1;
	}
	return result;
}

static int __callmgr_vr_stop_record_internal(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret = -1;
	callmgr_vr_status_extra_type_e stop_type= CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_MAX;
	ret = __callmgr_vr_commit(vr_handle);

	if (ret == 0) {
		if (vr_handle->elapsed_time > CM_VR_RECORD_TIME_MIN ) {
			dbg("record duration %ld millisecond", vr_handle->elapsed_time);
			ret =__callmgr_vr_create_filename(vr_handle);
			stop_type = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NORMAL;
		} else {
			warn("The recording time is too short [%ld millisecond]! Do not save record file", vr_handle->elapsed_time);
			stop_type = CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_TIME_SHORT;
		}

		if (ret >= 0 && stop_type == CALLMGR_VR_STATUS_EXTRA_STOP_TYPE_BY_NORMAL) {
			if (FALSE == vr_handle->is_answering) {
				ret = __callmgr_vr_update_media_info(vr_handle);
			} else {
				ret = __callmgr_vr_del_read_file();
				if (ret >= 0) {
					ret = __callmgr_vr_update_answering_msg_media_info(vr_handle);
				} else {
					err("Can not remove oldest file");
				}
			}
		}

	} else {
		err("__callmgr_vr_commit fails[%d]", ret);
		return -1;
	}

	vr_handle->cb_fn(CALLMGR_VR_STATUS_EVENT_STOPPED, stop_type, vr_handle->user_data);

	__callmgr_vr_unrealize(vr_handle);

	return ret;
}

int _callmgr_vr_get_recording_state(callmgr_vr_handle_h vr_handle, callmgr_vr_state_e *state)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	CM_RETURN_VAL_IF_FAIL(state, -1);
	*state = vr_handle->record_state;
	return 0;
}

int _callmgr_vr_stop_record(callmgr_vr_handle_h vr_handle)
{
	CM_RETURN_VAL_IF_FAIL(vr_handle, -1);
	int ret = -1;

	ret = __callmgr_vr_stop_record_internal(vr_handle);
	if (ret != 0) {
		err("__callmgr_vr_stop_record_internal() failed[%d].", ret);
	}

	return ret;
}

static int __callmgr_vr_fex_get_available_memory_space(callmgr_vr_store_type_e store_type)
{
	char *path = NULL;
	int available_kilobytes = 0;
	int result;

	if (store_type == CALLMGR_VR_STORAGE_PHONE) {
		struct statvfs s;
		double avail = 0.0;
		int ret;

		path = CM_VR_FEX_CLIPS_PATH_PHONE;
		ret = storage_get_internal_memory_size(&s);
		if (ret < 0) {
			err("storage_get_internal_memory_size failed : %d!", ret);
			return -1;
		} else {
			avail = (double)s.f_bsize*s.f_bavail;
		}

		available_kilobytes = (int)(avail / 1024);
		info("Available size : %lf, KB : %d", avail, available_kilobytes);
	} else {
		struct statfs buf = { 0 };
		callmgr_vr_fex_memory_status_t memory_status = { 0, 0 };

		path = CM_VR_FEX_CLIPS_PATH_MMC;
		result = statfs(path, &buf);
		if (result == -1) {
			err("Getting free space is failed.(%s)", path);
			return -1;
		}

		memory_status.capacity = buf.f_blocks * (buf.f_bsize / 1024);	/* return kilo byte. Calculating division first avoids overflow */
		memory_status.used = memory_status.capacity - (buf.f_bavail * (buf.f_bsize / 1024));
		info(".....capacity = %ld KB, used = %ld KB", memory_status.capacity, memory_status.used);

		available_kilobytes = memory_status.capacity - memory_status.used;
	}

	if (available_kilobytes < 100)
		available_kilobytes = 0;

	return available_kilobytes;
}

static gboolean __callmgr_vr_is_available_memory(void)
{
	int fex_remain_size = 0;

	fex_remain_size = __callmgr_vr_fex_get_available_memory_space(g_storage_type);
	info("The remained memory space size is %d!", fex_remain_size);
	if (fex_remain_size > 0) {
		return TRUE;
	} else {
		err("Short of memory!");
		return FALSE;
	}
}
