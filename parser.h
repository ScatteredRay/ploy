#ifndef _PARSER_H_
#define _PARSER_H_

struct parser;
struct symbol_table;

typedef void* pointer;

parser* init_parser(symbol_table* table);
void destroy_parser(parser* parse);
pointer parser_parse_expression(parser* parse, const char* string);

#endif //_PARSER_H_
