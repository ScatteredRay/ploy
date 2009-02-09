#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

typedef void* pointer;
struct symbol_table;

void transform_tree_gen_typeinfo(pointer P, symbol_table* tbl);

#endif //_TYPEINFO_H_
