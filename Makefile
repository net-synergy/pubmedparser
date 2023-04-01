CC = gcc
SRC_DIR = src
TEST_DIR = tests
LIB_DIR = lib
PYTHON_MODULE = pubmedparser
ARCH = x86_64
CPYTHON = cpython-310

.PHONY: all clean check cli
all: $(PYTHON_MODULE)/_readxml.so cli

cli:
	@cd $(SRC_DIR); $(MAKE) all

clean:
	-[ -f $(PYTHON_MODULE)/_readxml.so ] && rm $(PYTHON_MODULE)/_readxml.so
	@rm -rf build
	@cd $(SRC_DIR); $(MAKE) clean

check:
	@cd $(TEST_DIR); $(MAKE) check

$(LIB_DIR)/libpubmedparser.so:
	@cd $(SRC_DIR); $(MAKE) $@

$(SRC_DIR)/read_xml_core.o:
	@cd $(SRC_DIR); $(MAKE) $@

$(PYTHON_MODULE)/_readxml.so: $(SRC_DIR)/read_xml_core.o $(PYTHON_MODULE)/_readxml.c
	poetry build
	-[ -L $@ ] || ln -s \
	../build/lib.linux-$(ARCH)-$(CPYTHON)/pubmedparser/_readxml.$(CPYTHON)-$(ARCH)-linux-gnu.so $@
