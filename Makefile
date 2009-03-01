tarvol: create.o storage.o tarvol.o
	gcc -o $@ $^ -lstdc++

aestar: aestar.o
	gcc -o $@ $^

.c.o:
	gcc -c -Wall -g -DAFS_LARGEFILE_ENV -Iinternal $<

.cpp.o:
	g++ -c -Wall -g -Iinternal $<

clean:
	-rm tarvol *.o
