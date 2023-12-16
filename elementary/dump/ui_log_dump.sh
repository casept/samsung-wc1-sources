#!/bin/sh

#--------------------------------------
#   ui-info
#--------------------------------------
#export DISPLAY=:0.0
UI_INFO_DEBUG=$1/ui
mkdir -p ${UI_INFO_DEBUG}
cp /tmp/*_crash_* ${UI_INFO_DEBUG}/
cp /tmp/*_log_* ${UI_INFO_DEBUG}/