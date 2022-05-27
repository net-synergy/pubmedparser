CC = gcc
SRC_DIR = src
TEST_DIR = tests

.PHONY: all clean check
all:
	@cd $(SRC_DIR); $(MAKE) all

clean:
	@cd $(SRC_DIR); $(MAKE) clean

check:
	@cd $(TEST_DIR); $(MAKE) check
