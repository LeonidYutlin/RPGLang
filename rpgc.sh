#!/bin/bash

set -xe
AST=".temp/ast.txt"

./bin/rpgc-frontend $1 -o $AST
sleep 1
./bin/rpgc-middleend $AST -o ".temp/ast_opt.txt"
