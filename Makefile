CC = gcc
CFLAGS = -Wall -Wextra -pedantic

SRCDIR = src
BLDDIR = build
TARGET = todo

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(SRC:%.c=$(BLDDIR)/%.o)

$(BLDDIR)/$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

$(BLDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BLDDIR)

