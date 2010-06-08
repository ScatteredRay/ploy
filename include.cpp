// Copyright (c) 2010, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.

#include "include.h"
#include "symbols.h"
#include "error.h"
#include "parser.h"
#include "types.h"

pointer* ref_car(pointer P);
pointer* ref_cdr(pointer P);

void set_car(pointer P, pointer V);
void set_cdr(pointer P, pointer V);
void clear_car(pointer P);
void clear_cdr(pointer P);

void append_in_place(pointer P, pointer V);

void ploy_free(pointer P);

void materialize_includes(pointer* P, symbol_table* table)
{
    if(is_type(*P, DT_Pair))
    {
        if(is_type(car(*P), DT_Pair) &&
            is_type(caar(*P), DT_Symbol) &&
           *get_symbol(caar(*P)) == symbol_include)
        {
            assert_cerror(is_type(cdar(*P), DT_Pair) &&
                          is_type(cadar(*P), DT_String),
                          car(*P),
                          "include directive expects string.");

            pointer ret = parse_file_to_tree(get_string(cadar(*P)), table);

            append_in_place(ret, cdr(*P));
            clear_cdr(*P);
            set_car(*P, car(ret));
            set_cdr(*P, cdr(ret));
            ploy_free(*P);
            *P = ret;

            materialize_includes(P, table);
        }
        else
        {
            materialize_includes(ref_car(*P), table);
            materialize_includes(ref_cdr(*P), table);
        }
    }
}
