#include <stdio.h>
#include "symbol.h"
#include "parser.h"
#include "types.h"
#include "compiler.h"
#include "typeinfo.h"

int main(int argc, char** argv)
{
	symbol_table* tbl = init_symbol_table();

	printf("Hello:%d\n", symbol_from_string(tbl, "Hello").Id);
	printf("hello:%d\n", symbol_from_string(tbl, "hello").Id);
	printf("heyme:%d\n", symbol_from_string(tbl, "heyme").Id);
	printf("_why:%d\n", symbol_from_string(tbl, "_why").Id);
	printf("_why:%d\n", symbol_from_string(tbl, "_why").Id);
	printf("_Why:%d\n", symbol_from_string(tbl, "_Why").Id);

	parser* parse = init_parser(tbl);

	pointer ret = parser_parse_expression(parse, "(define (mul_add x :int y : int z :int) :(int) (+ (* x y) z))\n(mul_add 1 2 3)\n");

	print_object(ret, tbl);
	putchar('\n');

	destroy_parser(parse);

	transform_tree_gen_typeinfo(ret, tbl);

	print_object(ret, tbl);
	putchar('\n');

	compiler* compile = init_compiler(tbl);

	compile_block* program = compiler_compile_expression(compile, ret);

	destroy_compiler(compile);

	destroy_symbol_table(tbl);
	return 0;
}

