CC = gcc
CFLAGS = -Wall -Wextra
LIBS = cmocka

TOP_DIR = ..
SRC_DIR = ../src
OUT_DIR = ../bin

$(SRC_DIR)/yaml_reader.o: $(SRC_DIR)/yaml_reader.h $(SRC_DIR)/yaml_reader.c
	@cd $(SRC_DIR); $(MAKE) yaml_reader.o

$(SRC_DIR)/read_xml.o: \
		$(SRC_DIR)/read_xml.c \
		$(SRC_DIR)/paths.h $(SRC_DIR)/query.h \
		$(SRC_DIR)/yaml_reader.h
	@cd $(SRC_DIR); $(MAKE) read_xml.o

$(OUT_DIR)/read_xml: \
		$(SRC_DIR)/read_xml.o $(SRC_DIR)/paths.o \
		$(SRC_DIR)/query.o $(SRC_DIR)/yaml_reader.o
	@cd $(SRC_DIR); $(MAKE) $(OUT_DIR)/read_xml

test_yaml_reader: test_yaml_reader.c $(SRC_DIR)/yaml_reader.o
	$(CC) $(CFLAGS) -l$(LIBS) $^ -o $@
	./$@

test_xml_reader: test_xml_reader_argument_parsing.bats test_xml_reader_output.bats \
		 $(OUT_DIR)/read_xml
	./test_xml_reader_argument_parsing.bats
	./test_xml_reader_output.bats

test: test_yaml_reader test_xml_reader

clean:
	-rm test_yaml_reader
