afstar: afstar.o create.o storage.o
	gcc -o $@ $^ -lstdc++

.c.o:
	gcc -c -Wall -g -DAFS_LARGEFILE_ENV -Iinternal $<

.cpp.o:
	g++ -c -Wall -g -Iinternal $<

clean:
	-rm afstar *.o
