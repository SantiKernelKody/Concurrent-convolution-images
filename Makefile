image_processor: image_processor.c pgmconv33abs.c pgm.h
	gcc -g -pthread -o image_processor image_processor.c pgmconv33abs.c
