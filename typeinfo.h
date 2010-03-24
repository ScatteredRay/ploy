// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

#include <cstdio>
#include <map>
#include "symbol.h"

typedef void* pointer;
struct symbol_table;
class typeinfo;

extern const size_t type_info_size;
typedef std::map<symbol, pointer, sym_cmp> type_map;

namespace llvm
{
	class Type;
    class LLVMContext;
}

void print_typeinfo(pointer P, symbol_table* tbl, FILE* out);
void typeinfo_finish(pointer P);
const llvm::Type* typeinfo_get_llvm_type(pointer type);

void transform_tree_gen_typedef(pointer P, symbol_table* tbl, type_map* type_define_map);
void transform_tree_gen_typeinfo(pointer P, symbol_table* tbl, type_map* type_define_map);

typeinfo* get_typeinfo(pointer P);
pointer get_int_typeinfo(llvm::LLVMContext&);
pointer get_float_typeinfo(llvm::LLVMContext&);


#endif //_TYPEINFO_H_
