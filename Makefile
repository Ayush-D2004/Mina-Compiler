CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -g
OBJS=parser.tab.c lex.yy.c ast.c symbol_table.c semantic.c ir.c optimize.c codegen.c runtime.c main.c
TARGET=toycompiler

all: $(TARGET)

parser.tab.c parser.tab.h: parser.y ast.h
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

$(TARGET): parser.tab.c lex.yy.c ast.c symbol_table.c semantic.c ir.c optimize.c codegen.c runtime.c main.c
	$(CC) $(CFLAGS) -o $(TARGET) parser.tab.c lex.yy.c ast.c symbol_table.c semantic.c ir.c optimize.c codegen.c runtime.c main.c -lm

clean:
	rm -f parser.tab.c parser.tab.h lex.yy.c $(TARGET)

.PHONY: all clean
