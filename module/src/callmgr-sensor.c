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

#include <vconf.h>
#include <dbus/dbus.h>
#include <stdint.h>
#include <gesture_recognition.h>

#include "callmgr-sensor.h"
#include "callmgr-util.h"
#include "callmgr-log.h"

struct __sensor_data {
	gesture_h gesture_handle;

	cm_sensor_event_cb cb_fn;
	void *user_data;
};



int _callmgr_sensor_init(callmgr_sensor_handle_h *sensor_handle, cm_sensor_event_cb cb_fn, void *user_data)
{
	struct __sensor_data *handle = NULL;

	handle = calloc(1, sizeof(struct __sensor_data));
	CM_RETURN_VAL_IF_FAIL(handle, -1);

	handle->cb_fn = cb_fn;
	handle->user_data = user_data;

	*sensor_handle = handle;

	dbg("Sensor init done");

	return 0;
}
int _callmgr_sensor_deinit(callmgr_sensor_handle_h sensor_handle)
{
	CM_RETURN_VAL_IF_FAIL(sensor_handle, -1);

	g_free(sensor_handle);
	return 0;
}

static void __callmgr_sensor_face_down_cb(gesture_type_e gesture, const gesture_data_h data, double timestamp, gesture_error_e error, void *user_data)

{
	dbg(">>");
	callmgr_sensor_handle_h sensor_handle = (callmgr_sensor_handle_h)user_data;
	gesture_event_e event;
	CM_RETURN_IF_FAIL(sensor_handle);

	if (error != GESTURE_ERROR_NONE) {
		warn("gesture error : %d", error);
		return;
	}

	int ret = gesture_get_event(data, &event);
	if (ret == GESTURE_ERROR_NONE
		&& event == GESTURE_EVENT_DETECTED
		&& gesture == GESTURE_TURN_FACE_DOWN) {
		info("Face down");
		sensor_handle->cb_fn(CM_SENSOR_EVENT_TURN_OVER_E, NULL, sensor_handle->user_data);
	}

	return;
}

int _callmgr_sensor_face_down_start(callmgr_sensor_handle_h sensor_handle)
{
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(sensor_handle, -1);

	int ret = -1;
	bool is_supported = true;

	gboolean motion_action_option = FALSE;
	gboolean motion_turn_over_option = FALSE;

	if (gesture_is_supported(GESTURE_TURN_FACE_DOWN, &is_supported) != GESTURE_ERROR_NONE) {
		err("Err");
		return -1;
	}

	if (!is_supported) {
		warn("NOT support : Face down");
		return -2;
	}

	if (sensor_handle->gesture_handle == NULL) {
		ret = gesture_create(&sensor_handle->gesture_handle);
		if (ret != GESTURE_ERROR_NONE) {
			err("Fail to create gesture : %d", ret);
			return -1;
		}
	}

	if (!vconf_get_bool(VCONFKEY_SETAPPL_MOTION_ACTIVATION, &motion_action_option)) {
		dbg("motion_action_option:[%d]", motion_action_option);
		if (motion_action_option == FALSE) {
			warn("Motion option is Disabled..");
			return -1;
		}
	} else {
		err("vconf_get_bool fail");
		return -1;
	}

	if (!vconf_get_bool(VCONFKEY_SETAPPL_USE_TURN_OVER, &motion_turn_over_option)) {
		dbg("motion_turn_over_option:[%d]", motion_turn_over_option);
		if (motion_turn_over_option == FALSE) {
			warn("Sensor option is Disabled..");
			return -1;
		}
	} else {
		err("vconf_get_bool fail.");
		return -1;
	}

	ret = gesture_start_recognition(sensor_handle->gesture_handle, GESTURE_TURN_FACE_DOWN, GESTURE_OPTION_ALWAYS_ON, __callmgr_sensor_face_down_cb, sensor_handle);
	if (ret != GESTURE_ERROR_NONE) {
		err("Set callback err : %d", ret);
		return -1;
	}

	dbg("<<");
	return 0;
}

int _callmgr_sensor_face_down_stop(callmgr_sensor_handle_h sensor_handle)
{
	dbg(">>");
	CM_RETURN_VAL_IF_FAIL(sensor_handle, -1);

	if (sensor_handle->gesture_handle == NULL) {
		warn("No handle");
		return -1;
	}

	if (gesture_stop_recognition(sensor_handle->gesture_handle) != GESTURE_ERROR_NONE) {
		err("Stop err");
	}

	if (gesture_release(sensor_handle->gesture_handle) != GESTURE_ERROR_NONE) {
		err("Fail to detroy sensor");
	}

	sensor_handle->gesture_handle = NULL;

	dbg("<<");
	return 0;
}


