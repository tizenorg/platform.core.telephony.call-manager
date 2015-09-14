SET(COMMON "callmgr_common")

################################################################################
# Set external libraries                                                       #
################################################################################
INCLUDE(FindPkgConfig)
pkg_check_modules(common_pkgs REQUIRED
	glib-2.0
	dlog
	vconf
	aul
	appsvc
	capi-appfw-app-manager
	capi-media-metadata-extractor
	capi-appfw-application
	cynara-client
	cynara-creds-gdbus
	cynara-session
	capi-media-recorder
	notification
	msg-service
)

FOREACH(flag ${common_pkgs_CFLAGS})
        SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_LDFLAGS "-Wl,-zdefs" )
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

SET(COMMON_SRCS
	src/callmgr-util.c
	src/callmgr-vconf.c
)
IF(_ENABLE_EXT_FEATURE)
SET(COMMON_SRCS ${COMMON_SRCS}
	src/callmgr-vconf-ext.c
	src/callmgr-util-ext.c
)
ELSE(_ENABLE_EXT_FEATURE)
SET(COMMON_SRCS ${COMMON_SRCS}
	src/callmgr-vconf-dummy.c
	src/callmgr-util-dummy.c
)
ENDIF(_ENABLE_EXT_FEATURE)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/)

ADD_LIBRARY(${COMMON} STATIC ${COMMON_SRCS})
TARGET_LINK_LIBRARIES(${COMMON} ${common_pkgs_LDFLAGS})
