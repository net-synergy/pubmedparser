CC = gcc
CFLAGS = -Wall -Werror -Wextra

TOP_DIR = ..
OUT_DIR = ../bin
CACHE_DIR = ../cache
LIBS = z
READXML := $(OUT_DIR)/read_xml

debug: CFLAGS += -g
profile: CFLAGS += -pg

.PHONY: debug profile run all clean
read_xml.o: read_xml.c paths.h query.h
	$(CC) -fopenmp -c $*.c
$(READXML): read_xml.o paths.o query.o
	@mkdir -p $(CACHE_DIR)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -l$(LIBS) -fopenmp -O3 $^ -o $@
debug: $(READXML)
profile: $(READXML)
	$^
	gprof -b $^
run: $(READXML)
	$^
all: $(READXML)
clean:
	-rm *.o
	-rm -rf $(OUT_DIR) $(CACHE_DIR)
