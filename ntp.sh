#!/bin/sh

# change this - might be /dev/gps0 or /dev/ttyUSB0
NAVSPARK_DEVICE=/dev/ttyUSB0

./ntp-shm <$NAVSPARK_DEVICE
