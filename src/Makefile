CC = gcc
CFLAGS = -Wall -Wextra -O3

TOP_DIR = ..
OUT_DIR = ../bin
CACHE_DIR = ../cache
LIBS = z
READXML := $(OUT_DIR)/read_xml
YAMLCONTENTS := $(OUT_DIR)/yaml_get_key_component
OVERLAP := $(OUT_DIR)/overlap

debug: CFLAGS += -g
profile_readxml: CFLAGS += -pg
profile_overlap: CFLAGS += -pg

.PHONY: debug profile run all clean
paths.o: paths.c paths.h yaml_reader.h
	$(CC) -c $*.c

read_xml.o: read_xml.c paths.h query.h yaml_reader.h
	$(CC) -fopenmp -c $*.c

$(READXML): read_xml.o paths.o query.o yaml_reader.o
	$(CC) $(CFLAGS) -fopenmp $^ -o $@ -l$(LIBS)

yaml_get_key_component.o: yaml_get_key_component.c yaml_reader.h
	$(CC) -c $*.c

$(YAMLCONTENTS): yaml_get_key_component.o yaml_reader.o
	$(CC) $(CFLAGS) $^ -o $@

$(OVERLAP): overlap.c
	$(CC) $(CFLAGS) -fopenmp -fopt-info -fopt-info-missed $^ -o $@

debug: $(READXML) $(OVERLAP)

profile_readxml: $(READXML)
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ~/data/pubmed/pubmed21n0001.xml.gz
	gprof -b $^

profile_overlap: $(OVERLAP)
	$^ ~/data/synergy/import/Publication_Author_edges.tsv > /dev/null
	gprof -b $^

run: $(READXML)
	$^ --cache-dir=$(CACHE_DIR) --structure-file=../example/structure.yml ../data/pubmed21n0001.xml.gz

run-get-content: $(YAMLCONTENTS)
	$^ --structure-file=../example/structure.yml nodes

all: $(READXML) $(YAMLCONTENTS) $(OVERLAP)

clean:
	-rm *.o
	-rm -rf $(OUT_DIR) $(CACHE_DIR)
	@mkdir -p $(CACHE_DIR)
	@mkdir -p $(OUT_DIR)
