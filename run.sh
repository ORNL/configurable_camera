#!/bin/bash

LOGFILE="~/configurable_camera.log"
 
if ! screen -list | grep -q "camera"; then
    screen -L -Logfile $LOGFILE -S camera ./main
else
    screen -r camera
fi
