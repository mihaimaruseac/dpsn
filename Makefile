.PHONY: all clean

TARGET = ./dpsn
NPTARGET = ./dpsn
CC = gcc
CFLAGS = -Wall -Wextra -g -O0
LDFLAGS = -lm
OBJS = #dp2d.o fp.o globals.o rule.o histogram.o

all: $(TARGET) $(NPTARGET)

$(TARGET): $(OBJS)

$(NPTARGET): $(OBJS)

clean:
	@$(RM) $(OBJS) $(TARGET) $(NPTARGET)
