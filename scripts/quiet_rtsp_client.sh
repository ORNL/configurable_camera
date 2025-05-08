#!/bin/bash

if [ $# -eq 0 ]
then
	ADDRESS=rtsp://127.0.0.1:8554/test
else
	ADDRESS=$1
fi

gst-launch-1.0 rtspsrc location=$ADDRESS timeout=0 latency=100 ! fakesink
#gst-launch-1.0 uridecodebin uri=$ADDRESS ! ximagesink
