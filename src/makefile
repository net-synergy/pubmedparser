CC = gcc
OUT_DIR = ../bin

read_xml.o: read_xml.c query.h paths.h
	$(CC) -c read_xml.c
read_xml: read_xml.o query.o paths.o
	$(CC) -O3 query.o paths.o read_xml.o -o $(OUT_DIR)/read_xml
debug: read_xml.o query.o paths.o
	$(CC) -g query.o paths.o read_xml.o -o $(OUT_DIR)/read_xml
run: read_xml
	$(OUT_DIR)/read_xml
clean:
	rm -r *.o
