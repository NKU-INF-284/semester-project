OUTDIR  := build
OUTFILE := $(OUTDIR)/server.out
OPTS    := -Wall -Wpedantic -Wextra -O2
SRC     := src
FILES   := $(shell find $(SRC) -name *.cpp)


main:
	mkdir -p $(OUTDIR)
	g++ $(FILES) -o $(OUTFILE) $(OPTS) 

run: main
	./$(OUTFILE)

clean:
	rm -rf $(OUTDIR)
