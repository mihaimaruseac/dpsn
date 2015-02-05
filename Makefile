.PHONY: all clean

TARGET = ./dpsn
NPTARGET = ./dpsn
CC = gcc
CFLAGS = -Wall -Wextra -g -O0
LDFLAGS = -lm
OBJS = sn.o globals.o

all: $(TARGET) $(NPTARGET)

$(TARGET): $(OBJS)

$(NPTARGET): $(OBJS)

clean:
	@$(RM) $(OBJS) $(TARGET) $(NPTARGET)
