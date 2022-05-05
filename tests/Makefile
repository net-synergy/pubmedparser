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

$(OUT_DIR)/overlap: $(SRC_DIR)/overlap.c
	@cd $(SRC_DIR); $(MAKE) $(OUT_DIR)/overlap

test_yaml_reader: test_yaml_reader.c $(SRC_DIR)/yaml_reader.o
	$(CC) $(CFLAGS) $^ -o $@ -l$(LIBS)
	./$@

test_xml_reader: test_xml_reader_argument_parsing.bats test_xml_reader_output.bats \
		 $(OUT_DIR)/read_xml
	./test_xml_reader_argument_parsing.bats
	./test_xml_reader_output.bats

test_sort_performance: test_sort_performance.bats $(OUT_DIR)/read_xml
	./test_sort_performance.bats

test_overlap: test_overlap_calculator.bats $(OUT_DIR)/overlap
	./test_overlap_calculator.bats

test_overlap_performance: test_overlap_performance.bats $(OUT_DIR)/overlap
	./test_overlap_performance.bats

test: test_yaml_reader test_xml_reader test_overlap

clean:
	-rm test_yaml_reader
