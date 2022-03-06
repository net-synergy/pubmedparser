CC = gcc
CFLAGS = -Wall -Wextra

TOP_DIR = ..
OUT_DIR = ../bin
CACHE_DIR = ../cache
LIBS = z
READXML := $(OUT_DIR)/read_xml

debug: CFLAGS += -g
profile: CFLAGS += -pg

.PHONY: debug profile run all clean
paths.o: paths.c paths.h yaml_reader.h
	$(CC) -c $*.c

read_xml.o: read_xml.c paths.h query.h yaml_reader.h
	$(CC) -fopenmp -c $*.c

$(READXML): read_xml.o paths.o query.o yaml_reader.o
	@mkdir -p $(CACHE_DIR)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -l$(LIBS) -fopenmp -O3 $^ -o $@

debug: $(READXML)

profile: $(READXML)
	$^
	gprof -b $^

run: $(READXML)
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ../data/pubmed21n0001.xml.gz

all: $(READXML)

clean:
	-rm *.o
	-rm -rf $(OUT_DIR) $(CACHE_DIR)
