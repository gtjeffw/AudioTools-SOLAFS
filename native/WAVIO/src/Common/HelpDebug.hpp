
//-------------------------------------------------------------------------------------------------
///
/// @file HelpDebug.h
///
/// @brief Common debug helper routines.
///
/// @author Jeff Wilson
/// 
/// @see HelpDebug.c
/// 
//-------------------------------------------------------------------------------------------------


#ifndef _HELP_DEBUG_H_
#define _HELP_DEBUG_H_

#ifdef _DEBUG
#define PDEBUG
#endif

/// @def P_ERR_OUT
/// FILE alias for printing from P_ASSERT() (usually stderr)
#define P_ERR_OUT (stderr)
//#define P_ERR_OUT (stdout)



//-------------------------------------------------------------------------------------------------
// exitHelper
//-------------------------------------------------------------------------------------------------
///
/// @brief Wrapper for stdlib.h : exit() to avoid "unreachable code" warnings in P_ASSERT macro
///
/// @pre None.
///	@post Exit program.
///
void exitHelper();


//-------------------------------------------------------------------------------------------------
// P_ASSERT
//-------------------------------------------------------------------------------------------------
///
/// @def P_ASSERT(test, tstring)
/// @brief Macro Assertion for debugging.
/// @param[in] test Boolean expression to test.
/// @param[in] tstring _TCHAR* string for printing if assertion fails
///
/// @pre P_ERR_OUT must be valid file open for writing
/// @pre PDEBUG must be declared.
///
///	@post if not test, then print message and exit program.  Otherwise do nothing.
/// @post code only exists when PDEBUG is declared.
///
#ifdef PDEBUG 
/// @def P_ASSERT(test, tstring)
/// Macro assertion for debugging
#define P_ASSERT(test, tstring) if(!(test)) {(void)_ftprintf_s(P_ERR_OUT, (tstring)); exitHelper();}
#else
#define P_ASSERT(test, tstring);
#endif



#endif //#ifndef _HELP_DEBUG_H_