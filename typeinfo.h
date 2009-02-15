#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

#include <cstdio>

typedef void* pointer;
struct symbol_table;

extern const size_t type_info_size;

namespace llvm
{
	class Type;
}

void print_typeinfo(pointer P, symbol_table* tbl, FILE* out);
void typeinfo_finish(pointer P);
const llvm::Type* typeinfo_get_llvm_type(pointer type);

void transform_tree_gen_typeinfo(pointer P, symbol_table* tbl);

#endif //_TYPEINFO_H_
