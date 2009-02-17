#include <stdio.h>
#include "symbol.h"
#include "parser.h"
#include "types.h"
#include "compiler.h"
#include "typeinfo.h"

symbol_table* sym_tbl;

int main(int argc, char** argv)
{
	symbol_table* tbl = sym_tbl = init_symbol_table();

	printf("Hello:%d\n", symbol_from_string(tbl, "Hello").Id);
	printf("hello:%d\n", symbol_from_string(tbl, "hello").Id);
	printf("heyme:%d\n", symbol_from_string(tbl, "heyme").Id);
	printf("_why:%d\n", symbol_from_string(tbl, "_why").Id);
	printf("_why:%d\n", symbol_from_string(tbl, "_why").Id);
	printf("_Why:%d\n", symbol_from_string(tbl, "_Why").Id);

	parser* parse = init_parser(tbl);

	pointer ret = parser_parse_expression(parse, "(define (div_add x :float y : float z :float) :(float) (+ (/ x y) z))\n(div_add 1.0 2.0 3.0)\n");

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

