ROOT := $(PWD)
export SRC_DIR := $(ROOT)/src
export TEST_DIR := $(ROOT)/tests
export LIB_DIR := $(ROOT)/lib
export BIN_DIR := $(ROOT)/bin
export INCLUDE_DIR := $(ROOT)/include
export PYTHON_MODULE := $(ROOT)/pubmedparser
export CFLAGS += -fPIC -Wall -Wextra -O3

FILES ?= $(wildcard  data/*)
NTHREADS ?= 1

example := $(BIN_DIR)/read_xml --cache=cache \
	--structure-file=example/structure.yml \
	--overwrite-cache \
	--num-threads $(NTHREADS) \
	$(FILES)

.PHONY: all
all: python cli lib

.PHONY: cli
cli:
	@cd $(SRC_DIR); $(MAKE) all

.PHONY: lib
lib:
	@cd $(SRC_DIR); $(MAKE) lib

.PHONY: python
python:
	python setup.py develop --editable -b build

.PHONY: clean
clean:
	rm -f $(PYTHON_MODULE)/_*.so
	rm -rf build/temp*
	rm -f vgcore.*
	@cd $(SRC_DIR); $(MAKE) clean

.PHONY: clean-dist
clean-dist: clean
	rm -rf dist
	rm -rf build
	rm -rf cache

.PHONY: check
check: cli
	@cd $(TEST_DIR); $(MAKE) check

.PHONY: compile_commands
compile_commands: clean-dist
	bear -- $(MAKE) all

.PHONY: debug
debug: CFLAGS += -g3 -O0
debug: cli

.PHONY: valgrind
valgrind: debug
	@rm -f vgcore.*
	valgrind --track-origins=yes --leak-check=full $(example)

.PHONY: asan
asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: debug
	$(example)

.PHONY: time
time: cli
	time $(example)

.PHONY: run
run: cli
	$(example)
