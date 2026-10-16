
//-------------------------------------------------------------------------------------------------
///
/// @file FileIO.h
///
/// @brief Common file read/write routines.
///
/// @author Jeff Wilson
/// 
/// @see FileIO.c
///
//-------------------------------------------------------------------------------------------------

#ifndef _FILEIO_H_
#define _FILEIO_H_

#include "Common/System.hpp"
#include "Common/CommonTypes.hpp"

#ifdef SYSTEM_BIG_ENDIAN
#error Big Endian system support not implemented
#endif

///@name Read Routines
///@brief Routines for reading data from files.
///@{

//-------------------------------------------------------------------------------------------------
// fReadUINT16_LE
//-------------------------------------------------------------------------------------------------
///
/// @brief Reads a UINT16 from a FILE in LITTLE ENDIAN byte order.
///
/// @param[in]		v		Pointer to UINT16 to write to.
/// @param[in,out]	fp		File pointer where UINT16 will be read from.
///
/// @return			BOOL	Returns TRUE if successful operation.  Otherwise, FALSE.  
///
/// @pre fp must be open for reading, with file pointer aligned with desired value to read.
/// @pre v, fp must be non-NULL, valid pointers for successful operation.
///
///	@post File will have UINT16 read from it, incrementing file pointer.
/// @post *v will contain the value read from the file.
/// @post Returns TRUE upon success.
/// @post If fp or v are null returns FALSE.
/// @post If file read failure, returns FALSE.
/// @post *v is undefined when returning FALSE.
/// @post location of file pointer undefined when returning FALSE
/// @post undefined behavior for invalid, non-null pointer locations for v, fp
///
BOOL fReadUINT16_LE(UINT16 * v, FILE * fp);

//-------------------------------------------------------------------------------------------------
// fReadUINT32_LE
//-------------------------------------------------------------------------------------------------
///
/// @brief Reads a UINT32 from a FILE in LITTLE ENDIAN byte order.
///
/// @param[out]		v		Pointer to UINT32 to write to.
/// @param[in,out]	fp		File pointer where UINT32 will be read from.		
///
/// @return			BOOL	Returns TRUE if successful operation.  Otherwise, FALSE.  
///
/// @pre fp must be open for reading, with file pointer aligned with desired value to read.
/// @pre v, fp must be non-NULL, valid pointers for successful operation.
///
///	@post File will have UINT32 read from it, incrementing file pointer.
/// @post *v will contain the value read from the file.
/// @post Returns TRUE upon success.
/// @post If fp or v are null returns FALSE.
/// @post If file read failure, returns FALSE.
/// @post *v is undefined when returning FALSE.
/// @post location of file pointer undefined when returning FALSE
/// @post undefined behavior for invalid, non-null pointer locations for v, fp
BOOL fReadUINT32_LE(UINT32 * v, FILE * fp);

///@}


///@name Write Routines
///@brief Routines for writing data to files.
///@{

//-------------------------------------------------------------------------------------------------
// fWriteUINT16_LE
//-------------------------------------------------------------------------------------------------
///
/// @brief Writes a UINT16 to a FILE in LITTLE ENDIAN byte order.
///
/// @param[in]		v		UINT16 that will be written
/// @param[in,out]	fp		File pointer where UINT16 will be written to.		
///
/// @return			BOOL	Returns TRUE if successful operation.  Otherwise, FALSE.  
///
/// @pre fp must be open for writing.
/// @pre fp must be non-NULL, valid pointer for successful operation.
///
///	@post File will have UINT16 written to it, incrementing file pointer.
/// @post Returns TRUE upon success.
/// @post If fp is null returns FALSE.
/// @post If file write failure, returns FALSE.
/// @post if FALSE returned, file pointer location undefined
/// @post undefined behavior for invalid, non-null pointer location fp
///
BOOL fWriteUINT16_LE(UINT16 v, FILE * fp);

//-------------------------------------------------------------------------------------------------
// fWriteUINT32_LE
//-------------------------------------------------------------------------------------------------
///
/// @brief Writes a UINT32 to a FILE in LITTLE ENDIAN byte order.
///
/// @param[in]		v		UINT32 that will be written
/// @param[in,out]	fp		File pointer where UINT32 will be written to.		
///
/// @return		BOOL	Returns TRUE if successful operation.  If fp is null returns FALSE.
///						If file write failure, returns FALSE.
///
/// @pre fp must be open for writing.
/// @pre fp must be non-NULL, valid pointer for successful operation.
///
///	@post File will have UINT32 written to it, incrementing file pointer.
/// @post Returns TRUE upon success.
/// @post If fp is null returns FALSE.
/// @post If file write failure, returns FALSE.
/// @post if FALSE returned, file pointer location undefined
/// @post undefined behavior for invalid, non-null pointer location fp
///
BOOL fWriteUINT32_LE(UINT32 v, FILE * fp);

///@}

#endif //#ifndef _FILEIO_H_