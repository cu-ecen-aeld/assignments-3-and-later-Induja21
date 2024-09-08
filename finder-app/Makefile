# Makefile
# To build and clean the writer utility
# Author: Induja Narayanan <Induja.Narayanan@colorado.edu>

#If CROSS_COMPILE is defined only as aarch64-none-linux-gnu- do cross compilation else do native compilation
ifdef CROSS_COMPILE
ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
CC = $(CROSS_COMPILE)gcc
else
CC = gcc
endif
endif

#Target executable writer utility
TARGET=writer

OBJS = $(SRC:.c=.o)
SRC  = writer.c
CFLAGS = -Werror

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@
clean: 
	rm -f $(OBJS) $(TARGET)
