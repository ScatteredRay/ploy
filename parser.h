// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _PARSER_H_
#define _PARSER_H_

struct parser;
struct symbol_table;

typedef void* pointer;

parser* init_parser(symbol_table* table);
void destroy_parser(parser* parse);
pointer parser_parse_expression(parser* parse, const char* string);

#endif //_PARSER_H_
