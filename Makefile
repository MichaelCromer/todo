CC = gcc
CFLAGS = -Wall -Wextra -pedantic

DIR_SRC = src
DIR_BLD = build
DIR_OBJ = $(DIR_BLD)/obj
TARGET = todo

SRC = $(wildcard $(DIR_SRC)/*.c)
OBJ = $(SRC:$(DIR_SRC)/%.c=$(DIR_OBJ)/%.o)


$(DIR_BLD)/$(TARGET): $(OBJ) | $(DIR_BLD)
	$(CC) $(CFLAGS) $(OBJ) -o $@


$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@


$(DIR_BLD) $(DIR_OBJ): ; mkdir -p $@


.PHONY: dev
dev : CFLAGS += -g -fsanitize=address,leak,undefined
dev : clean $(DIR_BLD)/$(TARGET)


.PHONY: tags
tags: ; ctags $(SRC)


.PHONY: clean
clean: ; rm -rf $(DIR_BLD)
