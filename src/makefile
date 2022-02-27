CC = gcc
OUT_DIR = ../bin
LIBS = z

read_xml.o: read_xml.c query.h paths.h
	$(CC) -fopenmp -c read_xml.c
read_xml: read_xml.o query.o paths.o
	$(CC) -l$(LIBS) -fopenmp -O3 query.o paths.o read_xml.o -o $(OUT_DIR)/read_xml
debug: read_xml.o query.o paths.o
	$(CC) -g -l$(LIBS) -fopenmp query.o paths.o read_xml.o -o $(OUT_DIR)/read_xml
profile: read_xml.o query.o paths.o
	$(CC) -pg -l$(LIBS) -fopenmp query.o paths.o read_xml.o -o $(OUT_DIR)/read_xml
	$(OUT_DIR)/read_xml
	gprof -b $(OUT_DIR)/read_xml
run: read_xml
	$(OUT_DIR)/read_xml
clean:
	rm -r *.o
