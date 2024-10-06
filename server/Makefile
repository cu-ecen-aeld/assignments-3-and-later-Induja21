# Makefile
# To build and clean the aesdsocket utility
# Author: Induja Narayanan <Induja.Narayanan@colorado.edu>

#If CROSS_COMPILE is defined only as aarch64-none-linux-gnu- do cross compilation else do native compilation
ifdef CROSS_COMPILE
ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
CC = $(CROSS_COMPILE)gcc
else
CC = gcc
endif
endif

#Target executable aesdsocket utility
TARGET=aesdsocket

OBJS = $(SRC:.c=.o)
SRC  = aesdsocket.c
CFLAGS = -Werror -Wall -Wunused -Wunused-variable -Wextra

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@
clean: 
	rm -f $(OBJS) $(TARGET)
