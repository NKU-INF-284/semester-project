OUTFILE=server.out
OPTS=-Wall -Wpedantic -Wextra
FILES=main.cpp server.cpp

main:
	g++ $(FILES) -o $(OUTFILE) $(OPTS) 

run: main
	./server.out

clean:
	rm -f $(OUTFILE)