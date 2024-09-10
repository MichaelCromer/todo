CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g

SRCDIR = src
BLDDIR = build
TARGET = todo
TODO_INSTALL_DIR ?= /usr/local/bin/

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(SRC:%.c=$(BLDDIR)/%.o)

$(BLDDIR)/$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

$(BLDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install
install:
	echo "$(TODO_INSTALL_DIR)"
	cp $(BLDDIR)/$(TARGET) $(TODO_INSTALL_DIR)

.PHONY: clean
clean:
	rm -r $(BLDDIR)

