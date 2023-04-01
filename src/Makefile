CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native

TOP_DIR = ..
BIN_DIR = ../bin
INCLUDE_DIR = ../include
LIB_DIR = ../lib
PYTHON_MODULE = ../pubmedparser

debug: CFLAGS += -g

.PHONY: debug run all clean
paths.o: paths.c paths.h yaml_reader.h
	$(CC) $(CFLAGS) -c $*.c

nodes.o: nodes.c nodes.h paths.h $(INCLUDE_DIR)/structure.h error.h
	$(CC) $(CFLAGS) -c $*.c -I$(INCLUDE_DIR)

read_structure_file.o: read_structure_file.c $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -c $*.c -I$(INCLUDE_DIR)

read_xml_core.o: read_xml_core.c paths.h query.h nodes.h error.h \
                 $(INCLUDE_DIR)/read_xml.h
	$(CC) $(CFLAGS) -fopenmp -c $*.c -I$(INCLUDE_DIR)

read_xml.o: $(INCLUDE_DIR)/read_xml.h $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -fopenmp -c $*.c -I$(INCLUDE_DIR)

$(BIN_DIR)/read_xml: read_xml.o read_xml_core.o read_structure_file.o nodes.o \
                     paths.o error.o query.o yaml_reader.o
	-[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $^ -o $@ -lz

$(LIB_DIR)/libpubmedparser.so: read_xml_core.o read_structure_file.o nodes.o \
			       paths.o error.o query.o yaml_reader.o
	-[ -d $(LIB_DIR) ] || mkdir $(LIB_DIR)
	$(CC) -shared -o $@ $^ -lz

debug: $(BIN_DIR)/read_xml

all: $(BIN_DIR)/read_xml

clean:
	-rm *.o
	-rm -rf $(BIN_DIR) $(CACHE_DIR)
