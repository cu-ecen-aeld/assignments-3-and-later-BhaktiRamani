#!/bin/bash


#checking if 2 inputs are provided or not
if [[ $# -ne 2 ]]; then
	echo "ERROR : writefile/writestr not provided"
	exit 1
fi

GIVEN_PATH="$1"
CONTENT="$2"


#Extracting the path and removing the file name
NEW_PATH=$(dirname "$GIVEN_PATH")


#Checking if matching directory does not exists
if [ ! -d "$NEW_PATH" ]; then
    mkdir -p "$NEW_PATH"
fi


#Extracting file name from original path
FILE_NAME=$(basename "$GIVEN_PATH")

#Goind to a desired directory
cd $NEW_PATH

#Generating a file with extracted file name and specified content
echo $CONTENT > $FILE_NAME


#Checking if file is properly generated or not in desired directory
ls | grep $FILE_NAME


#Reading the file contents
#echo "File name : $FILE_NAME and contents : $(cat $FILE_NAME)" 



