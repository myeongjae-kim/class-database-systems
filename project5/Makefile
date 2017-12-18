.SUFFIXES: .c .o

CC=gcc

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.c
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
# SRCS_FOR_LIB:=$(SRCDIR)bpt.c
SRCS_FOR_LIB:=$(wildcard src/*.c)
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

CFLAGS+= -g -fPIC -I $(INC) -std=c11

TARGET=main

all: $(TARGET)

$(TARGET): $(TARGET_SRC) $(SRCS_FOR_LIB)
	$(CC) $(CFLAGS) -o $(SRCDIR)main.o -c $(SRCDIR)main.c
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_ext.o -c $(SRCDIR)bpt_ext.c
	$(CC) $(CFLAGS) -o $(SRCDIR)buf.o\
		-c $(SRCDIR)buf.c
	$(CC) $(CFLAGS) -o $(SRCDIR)conn.o\
		-c $(SRCDIR)conn.c
	$(CC) $(CFLAGS) -o $(SRCDIR)delete.o\
		-c $(SRCDIR)delete.c
	$(CC) $(CFLAGS) -o $(SRCDIR)disk.o\
		-c $(SRCDIR)disk.c
	$(CC) $(CFLAGS) -o $(SRCDIR)helper.o\
		-c $(SRCDIR)helper.c
	$(CC) $(CFLAGS) -o $(SRCDIR)insert.o\
		-c $(SRCDIR)insert.c
	$(CC) $(CFLAGS) -o $(SRCDIR)table.o\
		-c $(SRCDIR)table.c
	# $(CC) $(CFLAGS) -o $(OBJS_FOR_LIB) -c $(SRCS_FOR_LIB)
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

library:
	gcc -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
