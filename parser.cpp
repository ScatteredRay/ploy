#include "parser.h"
#include "types.h"
#include "symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


struct parser
{
	struct symbol_table* symbols;
	const char* curr;
	int curr_line_num;
	bool bErrorFree;
};

pointer parser_error(parser* parse, const char* Error, ...)
{
	va_list ap;
	va_start(ap, Error);
	parse->bErrorFree = false;
	printf("Error: Parsing Error line, %d: ", parse->curr_line_num);
	vprintf(Error, ap);
	putc('\n', stdout);
	va_end(ap);
	return NIL;
}

bool is_newline(char c)
{
	return (c == '\r') || ( c == '\n');
}

bool is_whitespace(char c)
{
	return (c == ' ') || (c == '\t') || is_newline(c);
}

bool is_delimiter(char c)
{
	return is_whitespace(c) || (c == '(') || (c == ')') || (c == '\0');
}

bool is_symbol_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || strchr("!$%&*+-./:<=>?@^_~#", c);
}

bool is_number_char(char c)
{
	return (c >= '0' && c <= '9') || strchr(".+-", c);
}

bool is_extended_hex_char(char c)
{
	return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

void eat_whitespace(parser* parse)
{
	while(is_whitespace(*parse->curr))
	{
		if(*parse->curr == '\n')
			parse->curr_line_num++;
		parse->curr++;
	}
}

pointer parse_expr(parser* parse);

pointer parse_string(parser* parse)
{
	parse->curr++;

	pointer string_buf;

	{
		const char* C = parse->curr;
		size_t string_len = 0;
		
		while(*C != '"')
		{
			if(*C == '\\')
				C++;
			C++;
			string_len ++;
			if(*C == '\0')
				return parser_error(parse, "Unexpected EOF in string literal");
		}
		string_buf = alloc_string(string_len);
	}

	char* C = get_string_ref(string_buf);

	while(*parse->curr != '"')
	{
		if(*parse->curr == '\\')
		{
			parse->curr++;
			switch(*parse->curr)
			{
			case 'n':
				*C++ = '\n';
				break;
			case 't':
				*C++ = '\t';
				break;
			default:
				*C++ = *parse->curr;
			}
			parse->curr++;
		}
		else
			*C++ = *parse->curr++;
	}
	*C = '\0';
	parse->curr++;

	return string_buf;
}

pointer parse_symbol(parser* parse)
{
	const char* C = parse->curr;

	while(!is_delimiter(*parse->curr))
	{
		if(is_symbol_char(*parse->curr))
			parse->curr++;
		else
			return parser_error(parse, "Unexpected char '%c' in symbol.", *parse->curr);
	}

	return create_symbol(parse->symbols, C, parse->curr-C);
}

pointer parse_number(parser* parse)
{
	size_t len = 0;
	bool isFloat = false;
	bool isHex = false;
	bool isBinary = false;
	
	const char* Start = parse->curr;

	while(!is_delimiter(*parse->curr))
	{
		if(*parse->curr == '.')
			isFloat = true;

		if(is_number_char(*parse->curr) ||
		   (isHex && is_extended_hex_char(*parse->curr)))
			len++;
		else if(len == 1 &&
				*parse->curr == 'x' &&
				*Start == '0')
		{
			len++;
			isHex = true;
		}
		else if(len == 0 && *parse->curr == 'b')
			isBinary = true;
		else
			return parser_error(parse, "Unexpected char '%c' in number literal.", *parse->curr);
		parse->curr++;
	}

	{
		int TotalIs = isHex + isBinary + isFloat;
		if(TotalIs > true)
		{
			char* buffer = new char[len+1];
			
			strncpy(buffer, Start, len);
			buffer[len] = '\0';
			parser_error(parse, "Unexpected number literal: %s.", buffer);
			delete buffer;
			return NIL;

		}
	}

	if(isFloat)
	{
		char* buffer = new char[len+1];
	
		strncpy(buffer, Start, len);
		buffer[len] = '\0';

		float ret = atof(buffer);
		delete buffer;
		
		return create_real(ret);
	}
	else
	{
		// Might be smart to use a buffer here, in case strtol doesn't see all delimiters as we do.
		int ret;
		if(isHex)
			ret  = strtol(Start + 2, NULL,  16);
		else if(isBinary)
			ret = strtol(Start + 1, NULL, 2);
		else
			ret = strtol(Start, NULL, 10);

		return create_int(ret);
	}

	
}

pointer parse_number_or_symbol(parser* parse)
{
	// The scheme spec doesn't explicitlly call for symbols like ++, but we're going to allow them.
	
	char next = (*(parse->curr+1));
	if(next >= '0' && next <= '9')
		return parse_number(parse);
	else
		return parse_symbol(parse);
		
}

pointer parse_number_or_pair(parser* parse)
{
	char next = (*(parse->curr+1));

	pointer ret_car;
	pointer ret_cdr;

	if(is_whitespace(next))
	{
		parse->curr++;
		return parse_expr(parse);
	}
	else if(next >= '0' && next <= '9')
	{
		ret_car = parse_number(parse);
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	}
	else
	{
		ret_car = parse_symbol(parse);
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	}
}

pointer parse_quote(parser* parse)
{
	if(is_whitespace(*parse->curr))
		return parser_error(parse, "unexpected whitespace after quote.");

	switch(*parse->curr)
	{
	case '(':
		return create_pair(create_symbol(parse->symbols, "QUOTE"), parse_expr(parse));
	default:
		if(is_symbol_char(*parse->curr) && !is_number_char(*parse->curr))
			return create_pair(create_symbol(parse->symbols, "QUOTE"), parse_symbol(parse));
		else
			return parser_error(parse, "Unexpected token after quote.");

	}
	
}

pointer parse_expr(parser* parse)
{
	eat_whitespace(parse);
	pointer ret_car;
	pointer ret_cdr;
	switch(*parse->curr)
	{
	case '(':
		parse->curr++;
		ret_car = parse_expr(parse);
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	case '"':
		ret_car = parse_string(parse);
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	case '\'':
		parse->curr++;
		ret_car = parse_quote(parse);
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	case ')':
		parse->curr++;
		return NIL;
	case '+': case '-': case 'b':
		ret_car = parse_number_or_symbol(parse);
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	case '.':
		return parse_number_or_pair(parse);
	case '\\':
		parse->curr++;
		ret_car = create_char(*(parse->curr++));
		ret_cdr = parse_expr(parse);
		return create_pair(ret_car, ret_cdr);
	case ';':
		while(!is_newline(*parse->curr))
			parse->curr++;
		return parse_expr(parse);
	case 0:
		return NIL;
	default:
		if(is_number_char(*parse->curr))
		{
			ret_car = parse_number(parse);
			ret_cdr = parse_expr(parse);
			return create_pair(ret_car, ret_cdr);
		}
		else if(is_symbol_char(*parse->curr))
		{
			ret_car = parse_symbol(parse);
			ret_cdr = parse_expr(parse);
			return create_pair(ret_car, ret_cdr);
		}
		else
			return parser_error(parse, "Unexpected char in expression.");
		
	}
	parse->curr++;
}


pointer parser_parse_expression(parser* parse, const char* string)
{
	parse->curr = string;
	parse->curr_line_num = 0;
	parse->bErrorFree = true;
	
	pointer ret = parse_expr(parse);
	if(parse->bErrorFree)
		return ret;
	else
	{
		destroy_list(ret);
		return NIL;
	}
}

parser* init_parser(symbol_table* table)
{
	parser* parse = new parser();
	parse->symbols = table;
	parse->curr = NULL;
}

void destroy_parser(parser* parse)
{
	delete parse;
}
