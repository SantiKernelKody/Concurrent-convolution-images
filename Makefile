# Define the compiler
CC=gcc

# Define any compile-time flags
CFLAGS=-Wall -pthread -g

# Define any directories containing header files other than /usr/include
#
INCLUDES=

# Define the C source files
SRCS=image_processor.c pgmconv33abs.c

# Define the C header files
HDRS=pgm.h

# Define the C object files 
#
OBJS=$(SRCS:.c=.o)

# Define the executable file 
MAIN=image_processor

#
# The following part of the Makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)
	@echo  Simple compiler named $(MAIN) has been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS)

# This is a suffix replacement rule for building .o's from .c's
# It uses automatic variables $<: the name of the prerequisite of 
# the rule(a .c file) and $@: the name of the target of the rule (a .o file)
# (see the GNU make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
