ifeq ($(origin CROSS_COMPILE), undefined)
    CC = gcc
else ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
    CC = $(CROSS_COMPILE)gcc
else
    CC = gcc
endif

CXX = $(CROSS_COMPILE)g++

# Source and target files

SRC = writer.c
TARGET = writer

# Compilation flags
CFLAGS = -Wall -O2
LDFLAGS =

# Build Targets
all : $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

	
.PHONY: clean

clean:
	rm -f *.o $(TARGET)


