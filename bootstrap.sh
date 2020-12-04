#!/bin/bash

if [ -z "$PYTHONUSERBASE" ]; then
    PYTHONUSERBASE=$HOME/.local
fi

wget https://bootstrap.pypa.io/get-pip.py && python3 get-pip.py --user
$PYTHONUSERBASE/bin/pip3 install wget --user
$PYTHONUSERBASE/bin/pip3 install bs4 --user
rm -rf get-pip.py
