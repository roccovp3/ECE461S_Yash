CC = gcc
CFLAGS = -Wall -g -Iinc
LDFLAGS = -lreadline -lc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

TARGET = yash

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) 

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<
	
clean:
	rm -f $(TARGET) $(OBJS)

format:
	$(shell clang-format -i --style=Microsoft *.c)

.PHONY: all clean
