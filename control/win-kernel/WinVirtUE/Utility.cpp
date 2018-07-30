/**
* @file Utility.cpp
* @version 0.1.0.1
* @copyright (2018) Two Six Labs
* @brief Utility source file
*/
#include "Utility.h"

/**
* @brief compare two ansi strings for equality.
* @note Why must this be done?  We are required to run at IRQL == DISPATCH.  
* The ANSI string compare utilities require that IRQL be no higher than 
* PASSIVE.  I've no idea why this is true. Using this for short strings is ok,
* it is not optimal for anything more than approximtaely 64 characters.
* @param string1 First string to compare
* @param string2 Second string to compare
* @param IgnoreCase If TRUE then case is ignored else not ignored
* @return 0 if strings are the same else a negated index where the strings differ
*/
_Use_decl_annotations_
LONG CompareAnsiString(
	CONST ANSI_STRING& string1, 
	CONST ANSI_STRING& string2,
	BOOLEAN IgnoreCase)
{
	LONG retval = 0;
	LONG ndx = 0;

	if (string1.Length != string2.Length)
	{
		retval = MINLONG;
		goto ErrorExit;
	}

	for (ndx = 0; ndx < string1.Length; ndx++)
		if ((string1.Buffer[ndx] | (IgnoreCase ? (1 << 5) : 0x0))  // lower the case for the compare if ignore case is true
			!= (string2.Buffer[ndx] | (IgnoreCase ? (1 << 5) : 0x0)))
		{
			retval = -(ndx);  // show where we stopped agreeing, negate and return
			goto ErrorExit;
		}

	retval = 0; // show that there are zero differences

ErrorExit:

	return retval;
}

