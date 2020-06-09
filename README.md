# Implementing LLVM frontend for Kaleidoscope language

This repository implements an LLVM frontend for the Kaleidoscope language it follows the tutorial
"My First Language Frontend with LLVM Tutorial" on the official LLVM website (https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) </br>

I am using this as a way to teach myself LLVM.</br>

## What is kaleidoscope

Kaleidoscope is a simple language with only one datatype which is double.
It supports binary operations : +,-,*,/,<,>
It supports functions, control flow: for loops and if/then/else statements.
You can write programs like:</br>

<code>
//Compute the x'th fibonacci number.
def fib(x)<br>
  if x < 3 then<br>
    1<br>
  else<br>
    fib(x-1)+fib(x-2)<br>
</code>


## The different stages

### Lexer
Implement a lexer to convert the source program to tokens

### Parser
Implement a parser to convert the tokens to AST

### Codegen to LLVM IR
Codegen to convert the AST to LLVM IR

### Add support for mutable variables
Add support for defining new variables and modifying existing variables using LLVM stack variables or AllocaInst 

### Add support for compiling to object files
Add support to convert the generated LLVM IR to an onject file : "output.o" which can be linked with other file by using:
clang++ main.cpp output.o -o main
