SRCS = aesdsocket.c
OBJS = $(SRCS:.c=.o)

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt -Wl,--hash-style=gnu  # Add GNU_HASH flag

.PHONY: all
all: $(TARGET)

$(OBJS): %.o: %.c 
	$(CC) $(CFLAGS) -c $^ -o $@ $(INCLUDES)  # Remove LDFLAGS from this line

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(INCLUDES) $(LDFLAGS)  # Keep LDFLAGS here

.PHONY: clean
clean: 
	rm -f $(OBJS) $(TARGET)

