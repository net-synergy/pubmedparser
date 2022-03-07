CC = gcc
SRC_DIR = src
TEST_DIR = tests
LIBS = z

.PHONY: all clean run test
all:
	@cd $(SRC_DIR); $(MAKE) all

clean:
	@cd $(SRC_DIR); $(MAKE) clean

# run: all
# 	./process.sh --cache-dir=cache

test:
	@cd $(TEST_DIR); $(MAKE) test
