#!/bin/sh

#--------------------------------------
#   call silent log
#--------------------------------------
CALL_SILENT_LOG_DIR=$1/call_silent_log
mkdir -p ${CALL_SILENT_LOG_DIR}
echo "call silent log dump start"
/bin/cp -fr /opt/usr/data/call/call*  ${CALL_SILENT_LOG_DIR}
