ifdef CROSS_COMPILE
CC = $(CROSS_COMPILE)gcc
else
CC = gcc
endif


TARGET=writer

OBJS = $(SRC:.c=.o)
SRC  = writer.c

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@
clean: 
	rm -f $(OBJS) $(TARGET)
