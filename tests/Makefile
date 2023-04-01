CC = gcc
CFLAGS = -Wall -Wextra

TOP_DIR = ..

test_xml_reader: test_xml_reader_argument_parsing.bats test_xml_reader_output.bats
	@cd $(TOP_DIR); $(MAKE) cli
	bats ./test_xml_reader_argument_parsing.bats
	bats ./test_xml_reader_output.bats

test_sort_performance: test_sort_performance.bats read_xml
	@cd $(TOP_DIR); $(MAKE) cli
	bats ./test_sort_performance.bats

check: test_xml_reader
