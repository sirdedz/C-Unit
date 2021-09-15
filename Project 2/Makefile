# A makefile for building the mergetars project

# Target : Dependencies
#  (TAB) Actions

#VARIABLES
PROJECT = mergetars
HEADERS = $(PROJECT).h
OBJECTS = mergetars.o globals.o processes.o filelist.o


C99 = cc -std=c99
CFLAGS = -Wall -pedantic -Werror



$(PROJECT) : $(OBJECTS)
	$(C99) $(CFLAGS) -o $(PROJECT) $(OBJECTS) -lm


%.o : %.c $(HEADERS)
	$(C99) $(CFLAGS) -c $<


clean:
	rm -f $(PROJECT) $(OBJECTS)
