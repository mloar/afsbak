tarvol: tarvol.o storage.o
	gcc -o $@ $^ -L/usr/lib/afs -lcmd -lafsutil -lstdc++

.c.o:
	gcc -c -Wall -g -DAFS_LARGEFILE_ENV -Iinternal $<

.cpp.o:
	g++ -c -Wall -g -Iinternal $<

clean:
	-rm tarvol *.o
