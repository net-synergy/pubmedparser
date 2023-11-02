CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O3

TOP_DIR = ..
BIN_DIR = ../bin
INCLUDE_DIR = ../include
LIB_DIR = ../lib
PYTHON_MODULE = ../pubmedparser

debug: CFLAGS += -g3 -O0

.PHONY: debug run all clean lib core
paths.o: paths.c paths.h query.h error.h
	$(CC) $(CFLAGS) -c $*.c

nodes.o: nodes.c nodes.h paths.h query.h error.h $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -c $*.c -I$(INCLUDE_DIR)

read_structure_file.o: read_structure_file.c yaml_reader.h error.h \
		       $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -c $*.c -I$(INCLUDE_DIR)

read_xml_core.o: read_xml_core.c paths.h query.h nodes.h error.h \
		 $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -fopenmp -c $*.c -I$(INCLUDE_DIR)

read_xml.o: read_xml.c read_xml_core.o $(INCLUDE_DIR)/read_xml.h \
	    $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -fopenmp -c $*.c -I$(INCLUDE_DIR)

$(BIN_DIR)/read_xml: read_xml.o read_xml_core.o read_structure_file.o nodes.o \
                     paths.o error.o query.o yaml_reader.o error.o
	-[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $^ -o $@ -lz

$(LIB_DIR)/libpubmedparser.so: read_xml_core.o read_structure_file.o nodes.o \
			       paths.o error.o query.o yaml_reader.o
	-[ -d $(LIB_DIR) ] || mkdir $(LIB_DIR)
	$(CC) -shared -o $@ $^ -lz

debug: $(BIN_DIR)/read_xml

lib: $(LIB_DIR)/libpubmedparser.so

core: read_xml_core.o $(INCLUDE_DIR)/read_xml.h

all: $(BIN_DIR)/read_xml lib

clean:
	-rm *.o
	-rm -rf $(BIN_DIR) $(CACHE_DIR) $(LIB_DIR)
