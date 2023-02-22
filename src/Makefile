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

read_xml.o: read_xml.c paths.h query.h yaml_reader.h
	$(CC) -fopenmp -c $*.c

$(BIN_DIR)/read_xml: read_xml.o paths.o query.o yaml_reader.o
	-[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $^ -o $@ -lz

yaml_get_key_component.o: yaml_get_key_component.c yaml_reader.h
	$(CC) -c $*.c

$(BIN_DIR)/yaml_get_key_component: yaml_get_key_component.o yaml_reader.o
	-[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

debug: $(BIN_DIR)/read_xml

profile: $(BIN_DIR)/read_xml
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ~/data/pubmed/pubmed21n0001.xml.gz
	gprof -b $^

run: $(BIN_DIR)/read_xml
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ../data/pubmed21n0001.xml.gz

run-get-content: $(BIN_DIR)/yaml_get_key_component
	$^ --structure-file=../example/structure.yml nodes

all: $(BIN_DIR)/read_xml $(BIN_DIR)/yaml_get_key_component

clean:
	-rm *.o
	-rm -rf $(BIN_DIR) $(CACHE_DIR)
