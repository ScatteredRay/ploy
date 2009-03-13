#include <stdio.h>
#include "symbol.h"
#include "parser.h"
#include "types.h"
#include "compiler.h"
#include "typeinfo.h"
#include "stdio.h"

symbol_table* sym_tbl;

int main(int argc, const char** argv)
{
	if(argc != 2)
	{
		printf("Bad Arguments, Usage is: ploy <file>\n");
		return 1;
	}
	
	const char* file_location = argv[1];

	symbol_table* tbl = sym_tbl = init_symbol_table();

	printf("Hello:%d\n", symbol_from_string(tbl, "Hello").Id);
	printf("hello:%d\n", symbol_from_string(tbl, "hello").Id);
	printf("heyme:%d\n", symbol_from_string(tbl, "heyme").Id);
	printf("_why:%d\n", symbol_from_string(tbl, "_why").Id);
	printf("_why:%d\n", symbol_from_string(tbl, "_why").Id);
	printf("_Why:%d\n", symbol_from_string(tbl, "_Why").Id);

	parser* parse = init_parser(tbl);
	
	FILE* fin = fopen(file_location, "r");
	if(!fin)
	{
		printf("Input File \"%s\" does not exist.\n", file_location);
		return 1;
	}

	fseek(fin, 0, SEEK_END);
	size_t flen = ftell(fin);
	rewind(fin);

	char* buffer = new char[(flen+1)*sizeof(char)];
	fread(buffer, sizeof(char), flen, fin);
	buffer[flen] = '\0';
	fclose(fin);

	pointer ret = parser_parse_expression(parse, buffer);

	print_object(ret, tbl);
	putchar('\n');

	destroy_parser(parse);

	transform_tree_gen_typeinfo(ret, tbl);

	print_object(ret, tbl);
	putchar('\n');

	compiler* compile = init_compiler(tbl);

	compiler_compile_expression(compile, ret);
	compiler_print_module(compile);
	compiler_write_asm_file(compile, "out.ll");

	destroy_compiler(compile);

	destroy_symbol_table(tbl);
	return 0;
}

