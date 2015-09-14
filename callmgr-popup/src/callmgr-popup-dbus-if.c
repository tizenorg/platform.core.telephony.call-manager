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

#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>

#include "callmgr-popup-common.h"

/* DBus interface*/
#define DBUS_CALL_MANAGER      "org.tizen.callmgr"
#define DBUS_CALL_MANAGER_PATH "/org/tizen/callmgr"
#define DBUS_CALL_MANAGER_DEFAULT_INTERFACE DBUS_CALL_MANAGER

#define DBUS_CALL_MANAGER_METHOD_DIAL_CALL	"DialCall"

#define CM_DEFAULT_TIMEOUT    (60 * 1000)

static void __callmgr_popup_dial_call_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	GError *error = NULL;
	GDBusConnection *conn = NULL;
	int result = -1;

	GVariant *dbus_result;

	conn = G_DBUS_CONNECTION(source_object);
	dbus_result = g_dbus_connection_call_finish(conn, res, &error);
	if (error) {
		ERR("Failed: %s", error->message);
		g_error_free(error);
	}

	g_variant_get(dbus_result, "(i)", &result);

	DBG("result : %d", result);
}

int _callmgr_popup_dial_call(char *number, int call_type, int sim_slot, int disable_fm, gboolean is_emergency_contact, void *user_data)
{
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)user_data;

	gint value = 0;
	DBG(">>");
	retv_if(!user_data, -1);
	retv_if(!number, -1);

	g_dbus_connection_call(ad->dbus_conn, DBUS_CALL_MANAGER,
			DBUS_CALL_MANAGER_PATH, DBUS_CALL_MANAGER_DEFAULT_INTERFACE, DBUS_CALL_MANAGER_METHOD_DIAL_CALL,
			g_variant_new("(siiib)", number, call_type, sim_slot, disable_fm, is_emergency_contact), NULL, G_DBUS_CALL_FLAGS_NONE,
			CM_DEFAULT_TIMEOUT, ad->ca, __callmgr_popup_dial_call_cb, user_data);

	 DBG("<<");
	 return value;
}

int _callmgr_popup_dbus_init(void *user_data)
{
	DBG(">>");
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)user_data;
	retv_if(!ad, -1);

	GError *error = NULL;

	ad->dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!ad->dbus_conn) {
		ERR("dbus connection get failed: %s", error->message);
		g_error_free(error);
		return -1;
	}

	ad->ca = g_cancellable_new();
	ad->evt_list = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	return 0;
}


int _callmgr_popup_dbus_deinit(void *user_data)
{
	DBG(">>");
	CallMgrPopAppData_t *ad = (CallMgrPopAppData_t *)user_data;
	retv_if(!ad, -1);

	g_cancellable_cancel(ad->ca);
	g_object_unref(ad->ca);

	g_object_unref(ad->dbus_conn);

	return 0;
}

