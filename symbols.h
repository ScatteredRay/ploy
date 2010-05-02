// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

#ifndef DECLARE_SYMBOL
#include "symbol.h"
#define DECLARE_SYMBOL(sym) extern symbol symbol_##sym;

void init_symbols(symbol_table* tbl);

#endif //DECLARE_SYMBOL

DECLARE_SYMBOL(mangle_name)

#endif //_SYMBOLS_H_
