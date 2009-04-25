#ifndef _compiler_private_h_
#define _compiler_private_h_

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "types.h"
#include "compiler.h"
#include <map>
#include <vector>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/PassManager.h>
#include <llvm/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ValueSymbolTable.h>

namespace llvm
{
	class Type;
}

struct compiler_scope
{
	compiler_scope* parent_scope;
	std::map<symbol, llvm::Value*, sym_cmp> scope_map;
};

struct compile_block
{
	llvm::Function* function;
	llvm::BasicBlock* block;
	llvm::IRBuilder<> builder;
	llvm::Value* last_exp;
	compiler_scope* current_scope;
};

typedef llvm::Value* (*form_compile_func)(compiler*, compile_block*, pointer);

struct compiler_special_form
{
	form_compile_func special_form;
	compiler_special_form()
	{
		special_form = NULL;
	}
};

struct compiler
{
	symbol_table* sym_table;
	llvm::Module* module;
	std::map<symbol, compiler_special_form, sym_cmp> form_table;
};

void init_function_table(compiler* compile);

compile_block* compiler_create_function_block(compiler* compile, const char* Name="", const llvm::Type* RetType = llvm::Type::VoidTy, pointer Params=NIL, compile_block* parent_block = NULL);
void compiler_destroy_function_block(compile_block* block);
void compiler_add_to_scope(compile_block* block, symbol sym, llvm::Value* val);
llvm::Value* compiler_find_in_scope(compile_block* block, symbol sym);
llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P);
llvm::Value* compiler_resolve_expression_list(compiler* compile, compile_block* block, pointer P);
std::vector<const llvm::Type*> compiler_populate_param_types(pointer P);

#endif //_compiler_private_h_

