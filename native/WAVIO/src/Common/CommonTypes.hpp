
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

#include "system.hpp"

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

	#define UINT32 unsigned long int
	#define INT32 long int
	#define UINT8 unsigned char
	#define UINT16 unsigned short int
	#define INT16 short int

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
	#define TRUE (1ui8)
	/// @def FALSE
	/// value of false for BOOL type
	#define FALSE (0ui8) 

	///@}
#endif

///@name Limits
///@brief Various constant values related to limits of portable types.
///@{

#define MAX_UINT32	(0xFFFFFFFFui32)
#define MAX_UINT16	(0xFFFFui16)
#define MAX_UINT8	(0xFFui8)

#define MAX_INT8	(127i8)
#define MAX_INT16	(32767i16)
#define MAX_INT32	(2147483647i32)


#define MIN_INT8_TWOS_COMP	(-128i8)
#define MIN_INT16_TWOS_COMP	(-32768i16)
#define MIN_INT32_TWOS_COMP	(-2147483648i32)

#define MIN_INT8_ONES_COMP	(-127i8)
#define MIN_INT16_ONES_COMP	(-32767i16)
#define MIN_INT32_ONES_COMP	(-2147483647i32)

#define MIN_INT8_SIGN_MAG	(-127i8)
#define MIN_INT16_SIGN_MAG	(-32767i16)
#define MIN_INT32_SIGN_MAG	(-2147483647i32)

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