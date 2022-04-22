CC = gcc
CFLAGS = -Wall -Wextra

TOP_DIR = ..
OUT_DIR = ../bin
CACHE_DIR = ../cache
LIBS = z
READXML := $(OUT_DIR)/read_xml
YAMLCONTENTS := $(OUT_DIR)/yaml_get_key_component
OVERLAP := $(OUT_DIR)/overlap

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
	$(CC) $(CFLAGS) -fopenmp -O3 $^ -o $@ -l$(LIBS)

yaml_get_key_component.o: yaml_get_key_component.c yaml_reader.h
	$(CC) -c $*.c

$(YAMLCONTENTS): yaml_get_key_component.o yaml_reader.o
	$(CC) $(CFLAGS) $^ -o $@

$(OVERLAP): overlap.o
	$(CC) $(CFLAGS) -O3 $^ -o $@

debug: $(READXML)

profile: $(READXML)
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ~/data/pubmed/pubmed21n0001.xml.gz
	gprof -b $^

run: $(READXML)
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ../data/pubmed21n0001.xml.gz

run-get-content: $(YAMLCONTENTS)
	$^ --structure-file=../example/structure.yml nodes

all: $(READXML) $(YAMLCONTENTS) $(OVERLAP)

clean:
	-rm *.o
	-rm -rf $(OUT_DIR) $(CACHE_DIR)
