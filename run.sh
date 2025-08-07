#!/bin/bash
sudo ./build/brainco_hand --id 126 --serial /dev/ttyUSB1 &
sudo ./build/brainco_hand --id 127 --serial /dev/ttyUSB2 &
wait
