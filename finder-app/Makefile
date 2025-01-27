ifeq ($(origin CROSS_COMPILE), undefined)
    CC = gcc
else ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
    CC = $(CROSS_COMPILE)gcc
else
    CC = gcc
endif

FILE_RESULT_PATH=/home/bhakti/AESD/assignment-1-BhaktiRamani/assignments/assignment2/fileresult.txt

CXX = $(CROSS_COMPILE)g++

# Source and target files

SRC = writer.c
TARGET = writer

# Compilation flags
CFLAGS = -Wall -O2
LDFLAGS =

# Build Targets
all : $(TARGET)
	echo "Current tool chain : $(CC)" > $(FILE_RESULT_PATH)
	file ./$(TARGET) >> $(FILE_RESULT_PATH)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

	
.PHONY: clean

clean:
	rm -f *.o $(TARGET)


