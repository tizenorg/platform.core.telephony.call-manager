PROJECT(callmgrd)

SET(LIBDIR ${LIB_INSTALL_DIR})
SET(UNITDIR ${UNIT_INSTALL_DIR})

INCLUDE(FindPkgConfig)
pkg_check_modules(service_pkgs REQUIRED
	glib-2.0
	gio-2.0
	gio-unix-2.0
	dlog
	cynara-client
	cynara-creds-gdbus
	cynara-session
)

FOREACH(flag ${service_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/common/include/
		${CMAKE_SOURCE_DIR}/module/include/
		${CMAKE_SOURCE_DIR}/service/include/
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wextra -fvisibility=hidden -fPIC -fdata-sections -ffunction-sections -Wl,--gc-sections")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Wcast-align")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

# Call Manager Daemon
SET(DAEMON_SRCS
	src/callmgr-main.c
	src/callmgr-dbus.c
	src/callmgr-core.c
)

ADD_CUSTOM_COMMAND(
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT ${CMAKE_BINARY_DIR}/generated-code.c
        COMMAND gdbus-codegen --interface-prefix org.tizen.callmgr. --generate-c-code generated-code --c-namespace Callmgr --c-generate-object-manager --generate-docbook generated-docs
        ${INTROSPECTION}/call-manager.xml
        COMMENT "Generating GDBus .c/.h")

ADD_EXECUTABLE(${PROJECT_NAME} ${DAEMON_SRCS} ${CMAKE_BINARY_DIR}/generated-code.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${service_pkgs_LDFLAGS} callmgr_module callmgr_common)
INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# Configuration files (dbus & smack)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/resource/callmgr.service DESTINATION /usr/lib/systemd/user/)
