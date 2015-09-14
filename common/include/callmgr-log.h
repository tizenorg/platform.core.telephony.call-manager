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

#define dbg(fmt,args...)   do { RLOG(LOG_DEBUG, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)
#define info(fmt,args...)   do { RLOG(LOG_INFO, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)
#define warn(fmt,args...)   do { RLOG(LOG_WARN, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)
#define err(fmt,args...)   do { RLOG(LOG_ERROR, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)
#define fatal(fmt,args...)   do { RLOG(LOG_FATAL, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)

#define sec_err(fmt, args...) do { SECURE_RLOG(LOG_ERROR, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)
#define sec_warn(fmt, args...) do { SECURE_RLOG(LOG_WARN, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)
#define sec_dbg(fmt, args...) do { SECURE_RLOG(LOG_DEBUG, CALLMGR_LOG_TAG, fmt "\n", __func__, __LINE__, ##args); } while (0)

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

