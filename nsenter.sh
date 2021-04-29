#!/bin/bash

readonly PID=${1}
PROG=${2}

if [ "x${PROG}" == "x" ]; then
  PROG="/bin/sh"
fi

sudo nsenter -t ${PID} -m -u -i -n -p -U ${PROG}
