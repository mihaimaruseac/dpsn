.PHONY: all clean

TARGET = ./dpsn
CC = gcc
CFLAGS = -Wall -Wextra -g -O0
LDFLAGS = -lm
OBJS = sn.o globals.o sanitization.o test.o

all: $(TARGET) $(NPTARGET)

$(TARGET): $(OBJS)

clean:
	@$(RM) $(OBJS) $(TARGET) $(NPTARGET)
