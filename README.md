# Toy Compiler for `Mina` Programming Language

## This project implements a complete but small compiler pipeline for a hypothetical language named **Mina**.

# Summary file

Refer to summary file for maore details of the project. Find Summary.pdf file in codebase.

## Language shape

The syntax is intentionally unlike C:

- `proc` defines a function
- `let` declares a variable
- `set x <- expr` assigns a value
- `when expr { ... } otherwise { ... }` is conditional control flow
- `repeat expr { ... }` is looping
- `give expr;` returns from a function
- `show(expr);` prints a value
- `ask()` reads a numeric value from standard input

## Build

```bash
rm -f parser.tab.* lex.yy.c toycompiler

bison -d parser.y
flex lexer.l
gcc -std=c11 -Wall -Wextra -g -o toycompiler parser.tab.c lex.yy.c ast.c symbol_table.c semantic.c ir.c optimize.c codegen.c runtime.c main.c -lm
```

## Run

```bash
./toycompiler sample.mina
```
