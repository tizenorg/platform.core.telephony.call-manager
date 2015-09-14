SET(MODULE "callmgr_module")

################################################################################
# Set external libraries                                                       #
################################################################################
INCLUDE(FindPkgConfig)
pkg_check_modules(module_pkgs REQUIRED
	glib-2.0
	vconf
	tapi
	capi-media-sound-manager
	capi-media-player
	capi-media-tone-player
	contacts-service2
	capi-network-bluetooth
	capi-media-recorder
	capi-content-media-content
	motion
	storage
	capi-system-device
	notification
	badge
	capi-appfw-app-manager
	mm-sound
	cynara-client
	cynara-creds-gdbus
	cynara-session
)

FOREACH(flag ${module_pkgs_CFLAGS})
        SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_LDFLAGS "-Wl,-zdefs" )
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

SET(MODULE_SRCS
	src/callmgr-audio.c
	src/callmgr-ringer.c
	src/callmgr-bt.c
	src/callmgr-telephony.c
	src/callmgr-telephony-ecc.c
	src/callmgr-contact.c
	src/callmgr-vr.c
	src/callmgr-sensor.c
	src/callmgr-answer-msg.c
)
IF(_ENABLE_EXT_FEATURE)
SET(MODULE_SRCS ${MODULE_SRCS}
        src/callmgr-audio-ext.c
)
ELSE(_ENABLE_EXT_FEATURE)
SET(MODULE_SRCS ${MODULE_SRCS}
        src/callmgr-audio-dummy.c
)
ENDIF(_ENABLE_EXT_FEATURE)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/common/include/
		${CMAKE_CURRENT_SOURCE_DIR}/include/
)

ADD_LIBRARY(${MODULE} STATIC ${MODULE_SRCS})
TARGET_LINK_LIBRARIES(${MODULE} ${module_pkgs_LDFLAGS})
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/ DESTINATION ${MODULE_RES_DIR})
