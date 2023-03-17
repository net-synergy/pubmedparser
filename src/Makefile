CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native

TOP_DIR = ..
BIN_DIR = ../bin
CACHE_DIR = ../cache

debug: CFLAGS += -g
profile: CFLAGS += -pg

.PHONY: debug profile run all clean
paths.o: paths.c paths.h yaml_reader.h
	$(CC) -c $*.c

nodes.o: nodes.c nodes.h paths.h structure.h error.h
	$(CC) -c $*.c

read_structure.o: read_structure.c structure.h
	$(CC) -c $*.c

read_xml_core.o: read_xml_core.c paths.h query.h read_xml.h error.h
	$(CC) -fopenmp -c $*.c

read_xml.o: read_xml.c paths.h query.h nodes.h read_xml.h error.h yaml_reader.h structure.h
	$(CC) -fopenmp -c $*.c

$(BIN_DIR)/read_xml: read_xml.o paths.o query.o nodes.o error.o yaml_reader.o read_xml_core.o read_structure_file.o
	-[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $^ -o $@ -lz

debug: $(BIN_DIR)/read_xml

profile: $(BIN_DIR)/read_xml
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ~/data/pubmed/pubmed21n0001.xml.gz
	gprof -b $^

run: $(BIN_DIR)/read_xml
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ../data/pubmed21n0001.xml.gz

all: $(BIN_DIR)/read_xml

clean:
	-rm *.o
	-rm -rf $(BIN_DIR) $(CACHE_DIR)
