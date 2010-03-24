// Copyright (c) 2009, Nicholas "Indy" Ray. All rights reserved.
// See the LICENSE file for usage, modification, and distribution terms.
#ifndef _ERROR_H_
#define _ERROR_H_

typedef void* pointer;

void compiler_error(const char* Error, ...);
void compiler_error(pointer P, const char* Error, ...);

#define assert_cerror(pred, P, error, args...)	\
	if(!(pred)) compiler_error(P, error , ## args);

#endif //_ERROR_H
