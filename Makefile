default: yash

yash.o: yash.c
	gcc -c yash.c -o yash.o

format: yash.o 
# must have clang-format installed
	$(shell clang-format -i --style=Microsoft *.c)
	gcc yash.o -lreadline -o yash

yash: yash.o
	gcc yash.o -lreadline -o yash

clean:
	-rm -f yash.o
	-rm -f yash