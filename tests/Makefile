CC = gcc
CFLAGS = -Wall -Wextra

TOP_DIR = ..
SRC_DIR = ../src
BIN_DIR = ../bin

$(SRC_DIR)/read_xml.o: \
		$(SRC_DIR)/read_xml.c \
		$(SRC_DIR)/paths.h $(SRC_DIR)/query.h \
		$(SRC_DIR)/yaml_reader.h
	@cd $(SRC_DIR); $(MAKE) read_xml.o

$(BIN_DIR)/read_xml: \
		$(SRC_DIR)/read_xml.o $(SRC_DIR)/paths.o \
		$(SRC_DIR)/query.o $(SRC_DIR)/yaml_reader.o
	@cd $(SRC_DIR); $(MAKE) $(BIN_DIR)/read_xml

test_xml_reader: test_xml_reader_argument_parsing.bats test_xml_reader_output.bats \
		 $(BIN_DIR)/read_xml
	bats ./test_xml_reader_argument_parsing.bats
	bats ./test_xml_reader_output.bats

test_sort_performance: test_sort_performance.bats read_xml
	bats ./test_sort_performance.bats

check: test_xml_reader
