#!/bin/bash

pkill mywm
pkill Xephyr

Xephyr :1 -ac -screen 800x600 &
sleep 1

DISPLAY=:1 ./mywm
