#!/bin/bash -x

# SCR cleanup script
# Usage:
# ./cleanup.sh [file regex ...]
# where file contains a list of items to remove

rm -rf /tmp/$USER/scr.*/
rm -rf .scr/
while [ $1 ]; do
	rm -rf $1
	shift
done

exit 0
