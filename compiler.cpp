// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#include "compiler_private.h"
#include "types.h"
#include "typeinfo.h"
#include "error.h"
#include <stdint.h>
#include <llvm/CodeGen/FileWriters.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <fstream>

using namespace llvm;

extern symbol_table* sym_tbl;

compiler* init_compiler(symbol_table* table)
{
	compiler* compile = new compiler();
	compile->sym_table = table;
	init_function_table(compile);
	return compile;
}

void destroy_compiler(compiler* compile)
{
	delete compile;
}

void compiler_error(const char* Error, ...)
{
	va_list ap;
	va_start(ap, Error);
	printf("Error: Compiling: ");
	vprintf(Error, ap);
	putc('\n', stdout);
	va_end(ap);
}

void compiler_error(pointer P, const char* Error, ...)
{
	va_list ap;
	va_start(ap, Error);
	printf("Error Compiling at:\n");
	print_object(P, sym_tbl);
	printf("\nError: ");
	putc('\n', stdout);
	vprintf(Error, ap);
	putc('\n', stdout);
	va_end(ap);
}

llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P);
llvm::Value* compiler_resolve_expression_list(compiler* compile, compile_block* block, pointer P);

compiler_scope* compiler_create_empty_scope(compiler_scope* parent_scope)
{
	compiler_scope* scope = new compiler_scope();
	scope->parent_scope = parent_scope;
	return scope;
}

void compiler_destroy_scope(compiler_scope* scope)
{
	delete scope;
}

void compiler_add_to_scope(compile_block* block, symbol sym, llvm::Value* val)
{
	// Error on redef of variable at same scope... not dynamically bound in compiliation mode.
	block->current_scope->scope_map[sym] = val;
}

llvm::Value* compiler_find_in_scope(compile_block* block, symbol sym)
{
	compiler_scope* scope = block->current_scope;
	while(scope != NULL)
	{
		std::map<symbol, llvm::Value*>::iterator it = scope->scope_map.find(sym);
		if(it != scope->scope_map.end())
			return it->second;
		scope = scope->parent_scope;
	}
	return NULL;
}

std::vector<const Type*> compiler_populate_param_types(compiler* compile, pointer P)
{
	std::vector<const Type*> ParamTypes;

	while(P != NIL)
	{
		assert_cerror(is_type(P, DT_Pair) && is_type(pair_car(P), DT_Symbol) && is_type(cadr(P), DT_TypeInfo),
					  P, "Parameter list not well formed.");

		ParamTypes.push_back(typeinfo_get_llvm_type(cadr(P)));
		P = cddr(P);
	}	

	return ParamTypes;
}

compile_block* compiler_create_function_block(compiler* compile, const char* Name, const llvm::Type* RetType, pointer Params, compile_block* parent_block)
{

	std::vector<const Type*> ParamTypes = compiler_populate_param_types(compile, Params);

	compile_block* block = new compile_block();

    LLVMContext* Context;
    if(parent_block)
        Context = &parent_block->function->getContext();
    else
        Context = &compile->module->getContext();

    if(!RetType)
        RetType = llvm::Type::getVoidTy(*Context);

	FunctionType* ft = FunctionType::get(RetType, ParamTypes, false);
	
	Function* f;
	Constant* c = compile->module->getFunction(Name);
	if(c)
	{
		assert_cerror(isa<Function>(c), Params, "Constant '%s' already declared.", Name);
		f = cast<Function>(c);
		assert_cerror(f->arg_size() == ParamTypes.size(), Params, "Function '%s' already declared, mismatched params.", Name);
	}
	else
		f  = Function::Create(ft, Function::ExternalLinkage, Name, compile->module);

	block->function = f;
	block->function->setCallingConv(CallingConv::C);

	compiler_scope* parent_scope = NULL;
	if(parent_block)
		parent_scope = parent_block->current_scope;
	block->current_scope = compiler_create_empty_scope(parent_scope);

	Function::arg_iterator args = block->function->arg_begin();

	pointer P = Params;
	while(P != NIL)
	{
		Value* v = args++;
		assert_cerror(is_type(P, DT_Pair) && is_type(pair_car(P), DT_Symbol) && is_type(cadr(P), DT_TypeInfo),
					  P, "Parameter list for function '%s' not well formed.", Name);

		symbol S = *get_symbol(pair_car(P));
		v->setName(string_from_symbol(compile->sym_table, S));
		compiler_add_to_scope(block, S, v);
		P = cddr(P);
	}

	f->deleteBody();
	block->block = BasicBlock::Create(*Context, "entry", block->function);
	block->builder = new IRBuilder<>(block->block);

	return block;

	
}

void compiler_destroy_function_block(compile_block* block)
{
	// llvm should maintain it's own types, we just want to get rid of the block we pass around to build it.
	compiler_destroy_scope(block->current_scope);
    delete block->builder;
	delete block;
}

compile_block* compiler_init_module(compiler* compile)
{
	compile->module = new Module("", getGlobalContext());

	return compiler_create_function_block(compile, "main");
}

llvm::Value* compiler_resolve_variable(compiler* compile, compile_block* block, symbol S)
{
	Value* var = compiler_find_in_scope(block, S);
	if(!var)
		compiler_error("Undefined variable '%s'.", string_from_symbol(compile->sym_table, S));
	return var;
}

llvm::Value* compiler_resolve_expression(compiler* compile, compile_block* block, pointer P)
{

	switch(get_type_id(P))
	{
	case DT_Pair:
	{
		Function* function = NULL;
		if(is_type(pair_car(P), DT_Symbol))
		{
			// Try to resolve special forms first!
			symbol S = *get_symbol(pair_car(P));
			compiler_special_form* fun = &compile->form_table[S];
			if(fun && fun->special_form)
				return (*fun->special_form)(compile, block, P);
		}

		Value* var = compiler_resolve_expression(compile, block, pair_car(P));
		if(var && isa<Function>(var))
			function = cast<Function>(var);
		else
			compiler_error(P, "Function not defined.");

		SmallVector<Value*, 4> Params;
		P = pair_cdr(P);
		while(P != NIL)
		{
			Params.push_back(compiler_resolve_expression(compile, block, pair_car(P)));
			if(is_type(P, DT_Pair))
				P = pair_cdr(P);
			else
				P = NIL;
		}
		
		return block->builder->CreateCall(function, Params.begin(), Params.end());
	}
	case DT_Symbol:
	{
		// Resolve variables
		Value* var = compiler_resolve_variable(compile, block, *get_symbol(P));
		assert_cerror(var, P, "Undefined variable");
		return var;
	}
	case DT_Int:
		return ConstantInt::get(Type::getInt32Ty(block->function->getContext()), APInt(32, get_int(P), true));
	case DT_Real:
		return ConstantFP::get(block->function->getContext(), APFloat(get_real(P)));
	case DT_String:
    {
        // The Parser currentlly only supports ASCII, but use UTF-8.
        const char* Curr = get_string(P);
        size_t strlength = strlen(Curr);
        std::vector<Constant*> StringList;
        StringList.reserve(strlength+1);
        do
        {
            // TODO: Either convert from extended ascii to UTF-8, or use UTF-8 all through
            // the parser.40
            assert(*Curr >= 0 && "Conversion from extended ascii and UTF-8 not supported");
            StringList.push_back(
                ConstantInt::get(Type::getInt8Ty(block->function->getContext()), (uint64_t)*Curr, false));
        }
        while(*(Curr++) != '\0');

        llvm::ArrayType* ArrType = ArrayType::get(Type::getInt8Ty(block->function->getContext()),
                                               StringList.size());

        Constant* Arr = ConstantArray::get(
            ArrType,
            StringList);
        
        GlobalVariable* gv = new GlobalVariable(*compile->module,
                                                ArrType,
                                                true,
                                                GlobalValue::InternalLinkage,
                                                Arr,
                                                llvm::Twine(""),
                                                0,
                                                false);

        llvm::Value* V = block->builder->CreateConstInBoundsGEP2_64(gv, 0, 0);
        
        V->getType()->dump();

        return V;
    }
	case DT_Char:
		return ConstantInt::get(Type::getInt8Ty(block->function->getContext()), APInt(8, get_char(P), false));
	default:
		compiler_error(P, "Uncompilable expression in AST.");
		break;
	}

}

llvm::Value* compiler_resolve_expression_list(compiler* compile, compile_block* block, pointer P)
{
	llvm::Value* LastExp = NULL;
	while(P != NIL)
	{

		if(is_type(P, DT_Pair))
		{
			LastExp = compiler_resolve_expression(compile, block, pair_car(P));
			P = pair_cdr(P);
		}
		else
			P = NIL;
	}
	return LastExp;
}

void compiler_compile_expression(compiler* compile, pointer P)
{
	compile_block* block = compiler_init_module(compile);

	compiler_resolve_expression_list(compile, block, P);
	block->builder->CreateRetVoid();

	compiler_destroy_function_block(block);

	// Seperate the rest of this into pass and run modules.
	
	verifyModule(*compile->module, PrintMessageAction);
}

void compiler_print_module(compiler* compile)
{
	PassManager PM;
	PM.add(createPrintModulePass(&llvm::outs()));
	PM.run(*compile->module);
}

void compiler_write_asm_file(compiler* compile, const char* output_filename)
{
	std::ofstream out_file(output_filename);
	llvm::raw_os_ostream raw_out(out_file);
	PassManager PM;
	PM.add(createPrintModulePass(&raw_out));
	PM.run(*compile->module);
}
