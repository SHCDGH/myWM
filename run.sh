#!/bin/bash

pkill mywm
pkill Xephyr

Xephyr :1 -ac -screen 800x600 &
sleep 1

make clean && make

DISPLAY=:1 ./mywm
