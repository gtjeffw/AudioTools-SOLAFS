
//-------------------------------------------------------------------------------------------------
///
/// @file FileIO.c
///
/// @brief Common file read/write routines.
///
/// @author Jeff Wilson
/// 
/// @see FileIO.h
///
//-------------------------------------------------------------------------------------------------

#include <stdio.h>

#include "Common/CommonTypes.hpp"
#include "FileIO.hpp"

//-------------------------------------------------------------------------------------------------
BOOL fReadUINT16_LE(UINT16 * v, FILE * fp)
{
	int c1;
	int c2;


	if(v == NULL || fp == NULL)
		return FALSE;

	c1 = getc(fp);
	c2 = getc(fp);

	if(c1 == EOF || c2 == EOF)
		return FALSE;

	*v = ((UINT16)c2 << 8) | (UINT16)c1;

	return TRUE;

} //fReadUINT16_LE()

//-------------------------------------------------------------------------------------------------
BOOL fReadUINT32_LE(UINT32 * v, FILE * fp)
{

	int b1;
	int b2;
	int b3;
	int b4;

	if(v == NULL || fp == NULL)
		return FALSE;	

	*v = (UINT32)0;

	if(fp == NULL)
		return FALSE;

	b1 = fgetc(fp);
	b2 = fgetc(fp);
	b3 = fgetc(fp);
	b4 = fgetc(fp);

	if(b1 == EOF || b2 == EOF || b3 == EOF || b4 == EOF)
		return FALSE;

	*v = ((UINT32)b1) | (((UINT32)b2)<<8) | (((UINT32)b3)<<16) | (((UINT32)b4)<<24);
	
	return TRUE;

} //readUINT32_LE

//-------------------------------------------------------------------------------------------------
BOOL fWriteUINT16_LE(UINT16 v, FILE * fp)
{

	if(fp == NULL)
		return FALSE;


	//I'm not masking bytes.  It appears putc() only uses L-byte
	if(putc((int)v, fp) < 0)
		return FALSE;

	if(putc( ((int)v) >> 8, fp) < 0)
		return FALSE;

	return TRUE;

} //fWriteUINT16_LE()

//-------------------------------------------------------------------------------------------------
BOOL fWriteUINT32_LE(UINT32 v, FILE * fp)
{

	if(fp == NULL)
		return FALSE;

	//I'm not masking bytes.  It appears putc() only uses L-byte
	if(putc((int)(v ), fp) < 0)
		return FALSE;

	if(putc((int)((v >> 8)), fp) < 0)
		return FALSE;

	if(putc((int)((v >> 16)), fp) < 0)
		return FALSE;

	if(putc((int)((v >> 24)), fp) < 0)
		return FALSE;

	return TRUE;

} //fWriteUINT32_LE()


