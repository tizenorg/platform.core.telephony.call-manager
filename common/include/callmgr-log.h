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

#ifndef __CALLMGR_LOG_H__
#define __CALLMGR_LOG_H__

__BEGIN_DECLS


#include <dlog.h>

#ifndef CALLMGR_LOG_TAG
#define CALLMGR_LOG_TAG "CALL_MGR"
#endif

#define CALLMGR_RLOG_ENABLED 1

#ifdef CALLMGR_RLOG_ENABLED
#define dbg(fmt,args...)   { RLOG(LOG_DEBUG, CALLMGR_LOG_TAG, fmt "\n", ##args); }
#define info(fmt,args...)   { RLOG(LOG_INFO, CALLMGR_LOG_TAG, fmt "\n", ##args); }
#define warn(fmt,args...)   { RLOG(LOG_WARN, CALLMGR_LOG_TAG, fmt "\n", ##args); }
#define err(fmt,args...)   { RLOG(LOG_ERROR, CALLMGR_LOG_TAG, fmt "\n", ##args); }
#define fatal(fmt,args...)   { RLOG(LOG_FATAL, CALLMGR_LOG_TAG, fmt "\n", ##args); }

#define sec_err(fmt, arg...) {SECURE_RLOG(LOG_ERROR, CALLMGR_LOG_TAG, fmt"\n", ##arg); }
#define sec_warn(fmt, arg...) {SECURE_RLOG(LOG_WARN, CALLMGR_LOG_TAG, fmt"\n", ##arg); }
#define sec_dbg(fmt, arg...) {SECURE_RLOG(LOG_DEBUG, CALLMGR_LOG_TAG, fmt"\n", ##arg); }
#else
#define dbg(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_DEBUG, CALLMGR_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }
#define info(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_WARN, CALLMGR_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }
#define warn(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_WARN, CALLMGR_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }
#define err(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_ERROR, CALLMGR_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }
#define fatal(fmt,args...)   { __dlog_print(LOG_ID_MAIN, DLOG_FATAL, CALLMGR_LOG_TAG, "<%s:%d> " fmt "\n", __func__, __LINE__, ##args); }

#define sec_err(fmt, arg...) {SECURE_LOGE(CALLMGR_LOG_TAG, fmt"\n", ##arg); }
#define sec_warn(fmt, arg...) {SECURE_LOGW(CALLMGR_LOG_TAG, fmt"\n", ##arg); }
#define sec_dbg(fmt, arg...) {SECURE_LOGD(CALLMGR_LOG_TAG, fmt"\n", ##arg); }
#endif

#define CM_RETURN_IF_FAIL(scalar_exp) {\
	if (!(scalar_exp)) \
	{ \
		err("CM_RETURN_IF_FAIL: Failed: Returning from here."); \
		return;	\
	} \
}

#define CM_RETURN_VAL_IF_FAIL(scalar_exp, ret) { \
	if (!(scalar_exp)) \
	{ \
		err("CM_RETURN_VAL_IF_FAIL: Failed: Returning [%d]", ret); \
		return ret; \
	} \
}

// ToDo: this macro should be moved to correct header file
#define CM_SAFE_FREE(src) { \
	g_free(src); \
	src = NULL; \
}

__END_DECLS

#endif	//__CALLMGR_LOG_H__

