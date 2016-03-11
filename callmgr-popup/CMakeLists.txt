CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(callmgr-popup C)

SET(CALLMGR_POP_SRCS
	./src/callmgr-popup-main.c
	./src/callmgr-popup-widget.c
	./src/callmgr-popup-dbus-if.c
	./src/callmgr-popup-util.c
	./src/callmgr-popup-vconf.c
)

IF(_ENABLE_EXT_FEATURE)
SET(CALLMGR_POP_SRCS ${CALLMGR_POP_SRCS}
	./src/callmgr-popup-vconf-ext.c
)
ELSE(_ENABLE_EXT_FEATURE)
SET(CALLMGR_POP_SRCS ${CALLMGR_POP_SRCS}
	./src/callmgr-popup-vconf-dummy.c
)
ENDIF(_ENABLE_EXT_FEATURE)

INCLUDE(FindPkgConfig)
pkg_check_modules(callmgr_popup_pkgs REQUIRED
	elementary
	evas
	eina
	edje
	ecore
	#ecore-x
	dlog
	capi-appfw-application
	appcore-efl
	capi-ui-efl-util
	glib-2.0
	gio-2.0
	vconf
	efl-extension
	capi-system-device
	notification
	libtzplatform-config
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} include)

SET(PREFIX "${TZ_SYS_RO_APP}/org.tizen.callmgr-popup/")
SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(EDJDIR "${RESDIR}/edje")
SET(LOCALEDIR "${RESDIR}/locale")
SET(PKG_NAME "org.tizen.callmgr-popup")

FOREACH(flag ${callmgr_popup_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ${EXTRA_CFLAGS}")

ADD_EXECUTABLE(callmgr-popup ${CALLMGR_POP_SRCS})
TARGET_LINK_LIBRARIES(callmgr-popup "-pie" ${callmgr_popup_pkgs_LDFLAGS})

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")

#install binary & edj files
INSTALL(TARGETS callmgr-popup DESTINATION ${BINDIR})

# install manifest file & icon
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PKG_NAME}.xml DESTINATION /usr/share/packages)

# install application HOME directory
INSTALL(DIRECTORY DESTINATION ${DATADIR})

#EDJE
ADD_CUSTOM_TARGET(popup_custom.edj
	COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/res/edc
	${CMAKE_CURRENT_SOURCE_DIR}/res/edc/popup_custom.edc ${CMAKE_CURRENT_SOURCE_DIR}/res/edc/popup_custom.edj
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/edc/popup_custom.edc
)
ADD_DEPENDENCIES(callmgr-popup popup_custom.edj)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/edc/popup_custom.edj DESTINATION ${EDJDIR})

ADD_SUBDIRECTORY(res/po)

