
//-------------------------------------------------------------------------------------------------
///
/// @file System.h
///
/// @brief Specifies hardware specific attributes that affect code.
///
/// @author Jeff Wilson
/// 
//-------------------------------------------------------------------------------------------------

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

//byte order for multi byte data formats?
//(prefixed so they don't collide with the LITTLE_ENDIAN/BIG_ENDIAN macros that system headers define
//on macOS and Linux, where BIG_ENDIAN is always defined and tripped the check in FileIO.hpp)
#define SYSTEM_LITTLE_ENDIAN
//#define SYSTEM_BIG_ENDIAN

//what integer format is used on this system?
#define TWOS_COMPLEMENT
//#define ONES_COMPLEMENT
//#define SIGN_AND_MAGNITUDE

#endif