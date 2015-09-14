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

#ifndef _CALLMGR_POPUP_DEBUG_H_
#define _CALLMGR_POPUP_DEBUG_H_

#include <dlog.h>
#include <stdio.h>
#ifndef CALLMGR_POPUP_LOG_TAG
#define CALLMGR_POPUP_LOG_TAG "CALL_MGR_POPUP"
#endif

#define ERR(fmt, arg...) \
	do { \
		LOG(LOG_ERROR, CALLMGR_POPUP_LOG_TAG, fmt"\n", ##arg); \
	} while(0)

#define WARN(fmt, arg...) \
	do { \
		LOG(LOG_WARN, CALLMGR_POPUP_LOG_TAG, fmt"\n", ##arg); \
	} while(0)

#define DBG(fmt, arg...) \
	do { \
		LOG(LOG_DEBUG, CALLMGR_POPUP_LOG_TAG, fmt"\n", ##arg); \
	} while(0)

#define ENTER(arg...) \
	do {\
		LOG(LOG_DEBUG, CALLMGR_POPUP_LOG_TAG, "Enter func=%p", ##arg);\
	}while(0)

#define LEAVE() \
	do {\
		LOG(LOG_DEBUG, CALLMGR_POPUP_LOG_TAG, "Leave func");\
	} while(0)

#define SEC_ERR(fmt, arg...) \
	do { \
		SECURE_LOGE(CALLMGR_POPUP_LOG_TAG, fmt"\n", ##arg); \
	} while(0)

#define SEC_WARN(fmt, arg...) \
	do { \
		SECURE_LOGW(CALLMGR_POPUP_LOG_TAG, fmt"\n", ##arg); \
	} while(0)

#define SEC_DBG(fmt, arg...) \
	do { \
		SECURE_LOGD(CALLMGR_POPUP_LOG_TAG, fmt"\n", ##arg); \
	} while(0)

#define ret_if(expr) do { \
		if(expr) { \
			ERR("(%s) return", #expr); \
			return; \
		} \
	} while (0)

#define retv_if(expr, val) do { \
		if(expr) { \
			ERR("(%s) return", #expr); \
			return (val); \
		} \
	} while (0)

#define retm_if(expr, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
			ERR("(%s) return", #expr); \
			return; \
		} \
	} while (0)

#define retvm_if(expr, val, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
			ERR("(%s) return", #expr); \
			return (val); \
		} \
	} while (0)

#endif	/* _CALLMGR_POPUP_DEBUG_H_ */
