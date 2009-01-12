#include "lib.h"

#include "types.h"
#include "function.h"
#include "environment.h"
#include <stdio.h>
#include <assert.h>

pointer ploy_display(environment* env, pointer p)
{
	
}

pointer ploy_display_list(environment* env, pointer p)
{
	if(p != NIL)
	{
		assert(is_type(p, DT_Pair));
		putc('(', env->out);
		ploy_display(env, car(p));
		ploy_display_list(env, cdr(p));
	}
	else
		putc(')', env->out);
	return NIL;
}

pointer ploy_display_int(environment* env, pointer p)
{
	assert(is_type(p, DT_Int));
	fprintf(env->out, "%d", get_int(p));
	return NIL;
}

pointer ploy_display_real(environment* env, pointer p)
{
	assert(is_type(p, DT_Real));
	fprintf(env->out, "%f", get_real(p));
	return NIL;
}

pointer ploy_display_char(environment* env, pointer p)
{
	assert(is_type(p, DT_Char));
	putc(get_char(p), env->out);
	return NIL;
}

pointer ploy_display_string(environment* env, pointer p)
{
	assert(is_type(p, DT_String));
	fputs(get_string(p), env->out);
	fputc('\n', env->out);
	return NIL;
}
