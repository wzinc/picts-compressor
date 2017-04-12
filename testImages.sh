#!/bin/zsh

FILE_PATH="/Users/wzinc/Desktop/ASD/spaceImages/"
QUALITY=$1
PICTS_PATH="./picts-compressor"
PICTS_ARGS="-c0 -q$QUALITY"

rm -rf output_$QUALITY.log

for f in $(ls $FILE_PATH)
do
# 	echo "$PICTS_PATH $PICTS_ARGS $FILE_PATH$f $FILE_PATH${f%.*}_$QUALITY.picts >> output_$QUALITY.log"
	$PICTS_PATH -c0 -q$QUALITY $FILE_PATH$f $FILE_PATH${f%.*}_$QUALITY.picts >> output_$QUALITY.log
done

# ./picts-compressor -c0 -q10 /Users/wzinc/Desktop/ASD/spaceImages/PIA07700.tif /Users/wzinc/Desktop/ASD/spaceImages/PIA07700_10.picts