.PHONY: all
all: $(BIN_DIR)/read_xml

.PHNOY: lib
lib: $(LIB_DIR)/libpubmedparser.so

paths.o: paths.c paths.h query.h error.h
	$(CC) $(CFLAGS) -c $*.c

nodes.o: nodes.c nodes.h paths.h query.h error.h $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -c $*.c -I$(INCLUDE_DIR)

read_structure_file.o: read_structure_file.c yaml_reader.h error.h \
		       $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -c $*.c -I$(INCLUDE_DIR)

read_xml_core.o: read_xml_core.c paths.h query.h nodes.h error.h \
		 $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -pthread -c $*.c -I$(INCLUDE_DIR)

read_xml.o: read_xml.c read_xml_core.o $(INCLUDE_DIR)/read_xml.h \
	    $(INCLUDE_DIR)/structure.h
	$(CC) $(CFLAGS) -pthread -c $*.c -I$(INCLUDE_DIR)

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
	-rm -rf $(BIN_DIR) $(LIB_DIR)
