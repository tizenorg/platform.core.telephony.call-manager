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
#ifndef __CALLMGR_ANSWER_MSG_H__
#define __CALLMGR_ANSWER_MSG_H__

typedef struct __answer_msg_data *callmgr_answer_msg_h;
typedef void (*callmgr_answer_msg_play_finished_cb) (void *user_data);

int _callmgr_answer_msg_init(callmgr_answer_msg_h *answer_msg_handle);
int _callmgr_answer_msg_deinit(callmgr_answer_msg_h answer_msg_handle);
int _callmgr_answer_msg_play_msg(callmgr_answer_msg_h answer_msg_handle, callmgr_answer_msg_play_finished_cb finished_cb, void *user_data);
int _callmgr_answer_msg_stop_msg(callmgr_answer_msg_h answer_msg_handle);

#endif //__CALLMGR_ANSWER_MSG_H__

