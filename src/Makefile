CFLAGS += -I$(INCLUDE_DIR)

.PHONY: all
all: $(BIN_DIR)/read_xml

.PHNOY: lib
lib: $(LIB_DIR)/libpubmedparser.so

error.o: error.c error.h $(INCLUDE_DIR)/read_xml.h
paths.o: paths.c paths.h query.h error.h
nodes.o: nodes.c nodes.h paths.h query.h error.h $(INCLUDE_DIR)/structure.h
yaml_reader.o: yaml_reader.c yaml_reader.h $(INCLUDE_DIR)/read_xml.h
read_structure_file.o: read_structure_file.c yaml_reader.h error.h \
		       $(INCLUDE_DIR)/structure.h $(INCLUDE_DIR)/read_xml.h

read_xml_core.o: read_xml_core.c paths.h query.h nodes.h error.h \
		 $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -pthread -c $*.c

read_xml.o: read_xml.c read_xml_core.o $(INCLUDE_DIR)/read_xml.h \
	    $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -pthread -c $*.c

$(BIN_DIR)/read_xml: read_xml.o read_xml_core.o read_structure_file.o nodes.o \
                     paths.o error.o query.o yaml_reader.o error.o
	-[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)
	$(CC) $(CFLAGS) -pthread $^ -o $@ -lz

$(LIB_DIR)/libpubmedparser.so: read_xml_core.o read_structure_file.o nodes.o \
			       paths.o error.o query.o yaml_reader.o
	-[ -d $(LIB_DIR) ] || mkdir $(LIB_DIR)
	$(CC) -shared -o $@ $^ -lz

.PHONY: clean
clean:
	-rm -f *.o

.PHONY: clean-dist
clean-dist: clean
	-rm -rf $(BIN_DIR) $(LIB_DIR)
