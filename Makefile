CC = gcc
CFLAGS = -Wall -Werror -g

# LIBS += -lpthread -lrt

SOURCES  = $(wildcard *.c)
INCLUDES = $(wildcard *.h)
OBJECTS  = $(SOURCES:.c=.o)

PROJ = demo

all: clean $(PROJ)

$(PROJ):$(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROJ) $(OBJECTS) $(LIBS)

$(OBJECTS):$(SOURCES) $(INCLUDES)
	$(CC) $(CFLAGS) -c $(SOURCES)

run: all
	sudo ./$(PROJ)

.PHONY: clean
clean:
	-rm *.o 2>> /dev/null;
	-rm *.so 2>> /dev/null;
	-rm core.* 2>> /dev/null;
