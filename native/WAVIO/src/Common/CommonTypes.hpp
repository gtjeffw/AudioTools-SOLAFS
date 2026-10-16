
//-------------------------------------------------------------------------------------------------
///
/// @file CommonTypes.h
///
/// @brief Common portable type defns, likely to be used throughout projects.
///
/// @author Jeff Wilson
/// 
//-------------------------------------------------------------------------------------------------


#ifndef _COMMON_TYPES_H_
#define _COMMON_TYPES_H_

#include "System.hpp"

// TODO: check for existing definitions

// TODO:
// we were experiencing problems seemingly from common symbols being defined both here and in
// windows.h. (ControlData poiter and reference parameters to some functions were misbehaving).
// the ifdef befow is a first atempt to address the problem, however there may be a cleaner
// approach to dealing with common types when targeting multiple platforms.
#ifdef _WIN32
	#include <windows.h>
#else
	///@name Types
	///@brief Various portable types.
	///@{

	//fixed-width types, so UINT32 is 32 bits even where long is 64 (e.g. macOS, Linux)
	#include <stdint.h>

	#define UINT32 uint32_t
	#define INT32 int32_t
	#define UINT8 uint8_t
	#define UINT16 uint16_t
	#define INT16 int16_t

	#define BOOL UINT8

	#ifndef NULL
	#define NULL	(0)
	#endif


	///@}

	///@name Constants
	///@brief Various constant values related to portable types.
	///@{

	/// @def TRUE
	/// value of true for BOOL type
	#define TRUE ((BOOL)1)
	/// @def FALSE
	/// value of false for BOOL type
	#define FALSE ((BOOL)0)

	///@}
#endif

///@name Limits
///@brief Various constant values related to limits of portable types.
///@{

//casts rather than MSVC-only literal suffixes (ui32, i16, ...), so other compilers accept them
#define MAX_UINT32	((UINT32)0xFFFFFFFFu)
#define MAX_UINT16	((UINT16)0xFFFFu)
#define MAX_UINT8	((UINT8)0xFFu)

#define MAX_INT8	((signed char)127)
#define MAX_INT16	((INT16)32767)
#define MAX_INT32	((INT32)2147483647)


#define MIN_INT8_TWOS_COMP	((signed char)-128)
#define MIN_INT16_TWOS_COMP	((INT16)-32768)
#define MIN_INT32_TWOS_COMP	((INT32)(-2147483647 - 1))

#define MIN_INT8_ONES_COMP	((signed char)-127)
#define MIN_INT16_ONES_COMP	((INT16)-32767)
#define MIN_INT32_ONES_COMP	((INT32)-2147483647)

#define MIN_INT8_SIGN_MAG	((signed char)-127)
#define MIN_INT16_SIGN_MAG	((INT16)-32767)
#define MIN_INT32_SIGN_MAG	((INT32)-2147483647)

#ifdef TWOS_COMPLEMENT
#define MIN_INT8	MIN_INT8_TWOS_COMP
#define MIN_INT16	MIN_INT16_TWOS_COMP	
#define MIN_INT32	MIN_INT32_TWOS_COMP	
#endif

#ifdef ONES_COMPLEMENT
#define MIN_INT8	MIN_INT8_ONES_COMP	
#define MIN_INT16	MIN_INT16_ONES_COMP	
#define MIN_INT32	MIN_INT32_ONES_COMP	
#endif

#ifdef SIGN_AND_MAGNITUDE
#define MIN_INT8	MIN_INT8_SIGN_MAG
#define MIN_INT16	MIN_INT16_SIGN_MAG	
#define MIN_INT32	MIN_INT32_SIGN_MAG	
#endif

//@}


#endif