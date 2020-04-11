#!/bin/bash

wget https://bootstrap.pypa.io/get-pip.py && python3 get-pip.py --user
$HOME/.local/bin/pip3 install wget --user
$HOME/.local/bin/pip3 install bs4 --user
rm -rf get-pip.py
