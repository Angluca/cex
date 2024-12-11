.PHONY: clean run test all cex build

all: cex build

cex:
	cex process --fakedir=tests/fake/ lib/
	cex process --fakedir=tests/fake/ src/

build:
	cex app build .

test:
	cex test run all

clean:
	cex app clean .


