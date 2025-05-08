#!/bin/bash

DATE=$(date +%Y%m%d)
HOSTNAME=$(hostname)
LOGFILE="./logs/$DATE/$HOSTNAME/terminal.log" #/${hostname}/terminal.log"
 
screen -L -Logfile $LOGFILE -S camera ./main