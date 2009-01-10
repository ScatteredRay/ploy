#include <stdio.h>
#include "symbol.h"
#include "parser.h"

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

	pointer ret = parser_parse_expression(parse, "(print (cons (\"Hello\" 0.0) sym))\n");

	destroy_parser(parse);
	destroy_symbol_table(tbl);
	return 0;
}
