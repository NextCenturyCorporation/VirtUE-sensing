/**
* @file cppruntime.h 
* @version 0.1.0.1
* @copyright (2018) TwoSix Labs
* @brief manage the c++ runtime object ctor/dtor
*/
#pragma once
//
// add externally visible runtime constructs here
//

///
/// typedef of a pointer to a void function returning void
typedef void(__cdecl *PVFV)(void);

///
/// pointers to initialization sections

	PVFV __crtXia[];
	PVFV __crtXiz[];

	PVFV __crtXca[]; // c++
	PVFV __crtXcz[];

	PVFV __crtXpa[];
	PVFV __crtXpz[];

	PVFV __crtXta[];
	PVFV __crtXtz[];




typedef struct _EXIT_FUNC_LIST
{
	LIST_ENTRY  listEntry; // double linked list of exit functions
	PVFV        exitFunc;
} EXIT_FUNC_LIST, *PEXIT_FUNC_LIST;

// move the initialization vars into their own section
static LIST_ENTRY exitList;


///
/// we can call that here
int __cdecl atexit(PVFV func);
VOID CallGlobalDestructors();
VOID CallGlobalInitializers();

// move the intialization code into its own section

