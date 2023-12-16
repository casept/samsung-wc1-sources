#!/bin/sh

#--------------------------------------
#   isf
#--------------------------------------
export DISPLAY=:0.0
ISF_DEBUG=$1/isf
ISF_HOME=/home/app/.scim
/bin/mkdir -p ${ISF_DEBUG}
/bin/cat ${ISF_HOME}/isf.log > ${ISF_DEBUG}/isf.log
/bin/cat ${ISF_HOME}/config > ${ISF_DEBUG}/config
/bin/cat ${ISF_HOME}/global > ${ISF_DEBUG}/global
/bin/cat ${ISF_HOME}/engines_list > ${ISF_DEBUG}/engines_list
/bin/sync
