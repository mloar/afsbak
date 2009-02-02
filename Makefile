tarvol: tarvol.o storage.o
	gcc -o $@ $^ -L/usr/lib/afs -lcmd -lafsutil -lstdc++

.c.o:
	gcc -c -g -Iinternal $<

.cpp.o:
	g++ -c -g -Iinternal $<

clean:
	-rm tarvol *.o
