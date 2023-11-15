CC = gcc
SRC_DIR = src
TEST_DIR = tests
LIB_DIR = lib
PYTHON_MODULE = pubmedparser
ARCH = x86_64
CPYTHON = cpython-310

.PHONY: all cli lib python clean check debug core
all: python cli lib

cli:
	@cd $(SRC_DIR); $(MAKE) all

lib:
	@cd $(SRC_DIR); $(MAKE) lib

python: $(PYTHON_MODULE)/_readxml.so

clean:
	-[ -L $(PYTHON_MODULE)/_readxml.so ] && rm $(PYTHON_MODULE)/_readxml.so
	@rm -rf build
	@cd $(SRC_DIR); $(MAKE) clean

check:
	@cd $(TEST_DIR); $(MAKE) check

debug:
	@cd $(SRC_DIR); $(MAKE) debug

core:
	@cd $(SRC_DIR); $(MAKE) core

$(PYTHON_MODULE)/_readxml.so: core $(PYTHON_MODULE)/_readxml.c
	-[ -L $@ ] && rm $@
	-[ -d build/ ] && rm -rf build
	poetry build
	ln -s \
	  ../build/lib.linux-$(ARCH)-$(CPYTHON)/pubmedparser/_readxml.$(CPYTHON)-$(ARCH)-linux-gnu.so $@
