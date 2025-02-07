
# Author: Bhakti Ramani
# Date: 2025-01-26
# University: University of Colorado Boulder
# Subject: ECEN 5713 Advanced Embedded Linux Development
#
# File Description:
# Simple Makefile, generates executable for writer.c file using native
# build and supports cross compilation of aarch64-none-linux-gnu
# to run normally, type make
# to use cross compilation, type make CROSS_COMPILATION=aarch64-none-linux-gnu-
# Executable will generate using this perticular cross compilation but will
# not be able to run on x86 pc. 
# Logs status on fileresult.txt file
#
# File Name: Makefile

CC = $(CROSS_COMPILE)gcc

FILE_RESULT_PATH=/home/bakri/Work/1_CU_Boulder/AESD/assignments-3-and-later-BhaktiRamani/assignments/assignment2/fileresult.txt

# Source and target files

SRC = writer.c
TARGET = writer

# Compilation flags
CFLAGS = -Wall -O2
LDFLAGS =

# Build Targets
all : $(TARGET)
	@echo "Current tool chain : $(CC)" > $(FILE_RESULT_PATH)
	@file ./$(TARGET) >> $(FILE_RESULT_PATH)
	@if [ "$(CROSS_COMPILE)" = "aarch64-none-linux-gnu-" ]; then \
	echo "Cross Compilation Successfull" >> $(FILE_RESULT_PATH); \
	echo "Cross Compilation Successfull";\
    	fi
        	
	

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

	
.PHONY: clean

clean:
	rm -f *.o $(TARGET)


