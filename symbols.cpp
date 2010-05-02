// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#include "symbols.h"

#undef _SYMBOLS_H_
#undef DECLARE_SYMBOL
#define DECLARE_SYMBOL(sym) symbol symbol_##sym;
#include "symbols.h"

void init_symbols(symbol_table* tbl)
{

#undef _SYMBOLS_H_
#undef DECLARE_SYMBOL
#define DECLARE_SYMBOL(sym) symbol_##sym = symbol_from_string(tbl, #sym);
#include "symbols.h"

}
