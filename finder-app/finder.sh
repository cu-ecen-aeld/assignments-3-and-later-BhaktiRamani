#!/bin/bash

echo Writer Shell file

#checking if 2 inputs are provided or not
if [[ $# -ne 2 ]]; then
	echo "ERROR : INVALID INPUT"
	exit 1
fi

filesdir="$1"
searchstr="$2"


#Checking if directory exists
if [ ! -d "$filesdir" ]; then
	echo "$filesdir directory does not exist"
	exit 1
fi


#Going to a Given directory path
echo "Current directory : $(pwd)"
cd $filesdir
echo "Changed directory to : $(pwd)"
echo " "


#Counting the number of files in the directory
number_of_files=0
number_of_occurance=0

for i in *;
do
	#Checking if its a regular file
	if [[ -f "$i" ]];
	then
		#checking if it has searchstr or not
		if [[ "$i" == *"$searchstr"* ]];
		then 
			echo "Found $searchstr"
			number_of_occurance=$((number_of_occurance+1));
		
		fi
	fi
	number_of_files=$((number_of_files+1));
	
done

echo "Total number of files : $number_of_files"
echo "Total occurance of $searchstr : $number_of_occurance"
echo "Program end"	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

