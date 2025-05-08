#!/bin/bash

if [ $# -eq 1 ]
then
	ADDRESS=rtsp://127.0.0.1:8554/test
	FILEPATH=$1
else
	ADDRESS=$1
	FILEPATH=$2
fi

gst-launch-1.0 rtspsrc location=$ADDRESS timeout=0 latency=100 ! rtph264depay ! h264parse !  mp4mux ! filesink location="./test.mp4" 

