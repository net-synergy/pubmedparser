CC = gcc
CFLAGS = -Wall -Wextra
LIBS = cmocka

TOP_DIR = ..
SRC_DIR = ../src

$(SRC_DIR)/yaml_reader.o: $(SRC_DIR)/yaml_reader.h $(SRC_DIR)/yaml_reader.c
	@cd $(SRC_DIR); $(MAKE) yaml_reader.o

test_yaml_reader: test_yaml_reader.c $(SRC_DIR)/yaml_reader.o
	$(CC) $(CFLAGS) -l$(LIBS) $^ -o $@

test: test_yaml_reader
	./$^

clean:
	-rm test_yaml_reader
