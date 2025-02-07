#!/bin/bash
CURRENT_TIME=$(date +%s.%N)
DESIRED_TIME=$1
SEC_IN_MS=1000
#echo "$CURRENT_TIME"
RESULT=$(echo "scale=3 ; $DESIRED_TIME/$SEC_IN_MS" | bc)
START_TIME=$CURRENT_TIME
while [ $(echo "$CURRENT_TIME - $START_TIME < $RESULT" | bc -l) -eq 1 ]; do
    #echo Hey
    CURRENT_TIME=$(date +%s.%N)
    #echo $CURRENT_TIME
done
#echo $CURRENT_TIME
echo "Delay of $DESIRED_TIME ms is complete"
