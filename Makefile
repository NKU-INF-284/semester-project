OUTDIR  := build
OUTFILE := $(OUTDIR)/server.out
OPTS    := -Wall -Wpedantic -Wextra -O2 -pthread
SRC     := src
FILES   := $(shell find $(SRC) -name *.cpp)


main:
	mkdir -p $(OUTDIR)
	g++ $(FILES) -o $(OUTFILE) $(OPTS) 

run: main
	./$(OUTFILE)

clean:
	rm -rf $(OUTDIR)

reload: main
	sudo systemctl daemon-reload && sudo systemctl restart tcp-server.service