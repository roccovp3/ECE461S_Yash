# default: yash

# yash.o: yash.c yash.h
# 	gcc -c yash.c -o yash.o

# format: yash.o 
# # must have clang-format installed
# 	$(shell clang-format -i --style=Microsoft *.c)
# 	gcc yash.o -lreadline -o yash

# yash: yash.o
# 	gcc yash.o -lreadline -o yash

# clean:
# 	-rm -f yash.o
# 	-rm -f yash

CC = gcc
CFLAGS = -Wall -g -Iinc
LDFLAGS = -lreadline

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
