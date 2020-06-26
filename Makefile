all: jobExecutor

jobExecutor: jobExecutor.o connections.o searchMode.o trie.o counts.o
	gcc jobExecutor.c connections.c trie.c searchMode.c counts.c -o jobExecutor

jobExecutor.o: jobExecutor.c
	gcc -c jobExecutor.c

connections.o: connections.c
	gcc -c connections.c

searchMode.o: searchMode.c
	gcc -c searchMode.c

counts.o: counts.c
	gcc -c counts.c

trie.o: trie.c
	gcc -c trie.c

clean:
	rm -rf *o jobExecutor