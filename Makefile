default: yash

yash.o: yash.c
	gcc -c yash.c -o yash.o

format: yash.o 
	$(shell clang-format -i *.c)
	gcc yash.o -o yash

yash: yash.o
	gcc yash.o -o yash

clean:
	-rm -f yash.o
	-rm -f yash