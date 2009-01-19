#include <stdio.h>
#include "symbol.h"
#include "parser.h"
#include "types.h"
#include "compiler.h"

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

	pointer ret = parser_parse_expression(parse, "(define (mul_add x y z) (+ (* x y) z))\n");

	print_object(ret, tbl);
	putchar('\n');

	pointer tmp = create_pair(create_symbol(tbl, "define"), 
							  create_pair(create_pair(create_symbol(tbl, "mul_add"),
													  create_pair(create_symbol(tbl,"x"),
																  create_pair(create_symbol(tbl, "y"),
																			  create_pair(create_symbol(tbl, "z"), NIL)))),
										  create_pair(create_pair(create_symbol(tbl, "+"),
																  create_pair(create_int(1),
																			  create_pair(create_int(2), NIL))), NIL)));

		

	print_object(tmp, tbl);
	putchar('\n');

	destroy_list(tmp);
	tmp = create_pair(create_int(1), create_int(2));

	print_object(tmp, tbl);
	putchar('\n');

	destroy_parser(parse);

	compiler* compile = init_compiler(tbl);

	compile_block* program = compiler_compile_expression(compile, ret);

	destroy_compiler(compile);

	destroy_symbol_table(tbl);
	return 0;
}

