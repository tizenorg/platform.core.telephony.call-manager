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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "callmgr-core.h"
#include "callmgr-dbus.h"
#include "callmgr-log.h"
#include "callmgr-telephony.h"

/* TODO:*/
/* Add prefix */
int main(int argc, char *argv[])
{
	struct sysinfo info;
	GMainLoop *loop;
	info("version: " VERSION);
	callmgr_core_data_t *core_data = NULL;

	if (sysinfo(&info) == 0) {
		info("uptime: %ld secs", info.uptime);
	}

#if !GLIB_CHECK_VERSION(2, 35, 0)
	g_type_init();
#endif
	loop = g_main_loop_new(NULL, FALSE);

	/* Init Call manager core */
	_callmgr_core_init(&core_data);

	info("start loop");
	g_main_loop_run(loop);

	info("finish loop");
	_callmgr_core_deinit(core_data);

	return EXIT_SUCCESS;
}


