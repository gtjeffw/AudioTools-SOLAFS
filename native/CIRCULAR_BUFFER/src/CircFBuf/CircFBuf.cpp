#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <memory.h>
#include <math.h>

#include "CircFBuf.hpp"



CIRCULAR_FLOAT_BUFFER CFB_create(UINT32 minimumSize)
//minimumSize is used to determine the smallest 2^m sized buffer that can hold that amount
//2^m size allows us to do determine the actual index from virtual:
//	index = convertToActual(virtualIndex BITWISE_AND 2^m-1)
{
	UINT32 pow2;
	CIRCULAR_FLOAT_BUFFER ret = NULL;

	if(minimumSize <= 0)
		return NULL;

	ret = (CIRCULAR_FLOAT_BUFFER)malloc( sizeof(CIRCULAR_FLOAT_BUFFER_STRUCT) );

	if(ret == NULL)
		return NULL;

	memset((void*)ret,0,sizeof(CIRCULAR_FLOAT_BUFFER_STRUCT));


	//find smallest power of 2 >= minimumSize
	pow2 = (UINT32)ceil(log10((float)minimumSize)/log10(2.0f));

	ret->size = (UINT32)pow(2.0f,(float)pow2);
	ret->mask = ret->size - 1;

	ret->buf = (float*)malloc( sizeof(float) * ret->size );

	if(ret->buf == NULL)
	{
		free(ret);
		return NULL;
	}

	for(UINT32 i=0 ; i<ret->size ; i++)
		ret->buf[i] = 0.0f;

	return ret;
}



bool CFB_delete(CIRCULAR_FLOAT_BUFFER cfb)
{
	if(cfb == NULL)
		return false;

	if(cfb->buf != NULL)
		free(cfb->buf);

	free(cfb);

	return true;
}

UINT32 CFG_virtualPosStart(CIRCULAR_FLOAT_BUFFER b)
{
	if(b == NULL)
		return (UINT32)0;

	if(b->virtSize < b->size)
		return (UINT32)0;

	return (b->virtSize - b->size);
}



bool CFB_write(CIRCULAR_FLOAT_BUFFER fout, CIRCULAR_FLOAT_BUFFER fin, 
			   UINT32 size)
//copies the LAST size samples from fin to fout
{
	if(fout == NULL || fin == NULL || fout->size == 0 || fin->size == 0 || 
		fin->virtSize < size)
		return false;

	bool ret = false;

	ret = CFB_write(fout, fin, fin->virtSize - size, fin->virtSize);

	return ret;
}

bool CFB_write(CIRCULAR_FLOAT_BUFFER fout, CIRCULAR_FLOAT_BUFFER fin, 
			   UINT32 virtPosStart, UINT32 size)
{
	if(fout == NULL || fin == NULL || fout->size == 0 || fin->size == 0 || 
		(virtPosStart + size) > fin->virtSize)
		return false;

	UINT32 smallestAllowedPosStart = CFG_virtualPosStart(fin);

	if(virtPosStart < smallestAllowedPosStart)
		return false;

	if(fin->virtSize == 0) //nothing to copy
		return true;

	UINT32 trueStartIndex = virtPosStart & fin->mask;
	UINT32 partASize = fin->size - trueStartIndex;
	UINT32 partBSize = 0;
	 

	partASize = size < partASize ? size : partASize;

	
	if(!CFB_write(fout, &(fin->buf[trueStartIndex]), partASize))
		return false;

	if(size > partASize) 
	{
		partBSize = size - partASize;
		if(!CFB_write(fout, fin->buf, partBSize)) //arg2 is same as: &(fin->fbuf[0])
			return false;
	}

	return true;
}


bool CFB_write(CIRCULAR_FLOAT_BUFFER cfb, float * fbuf, UINT32 size)
{

	if(cfb == NULL || fbuf == NULL || size == 0)
		return false;

	//if we are copying so much that we'll end up overwriting, lets not
	//and pretend we did (as an optimization and to simplify the copying code below)
	//instead, we'll only write the last bit that doesn't get overwritten
	if(size > cfb->size)
	{

		UINT32 overlap = size - cfb->size;
		
		size = cfb->size;
		fbuf = &(fbuf[overlap]); //skip ahead in fbuf

		cfb->virtSize += overlap; //pretend did all that overwriting
	}


	//break write operation in up to two parts to avoid needing to mask
	//each increment of the index into the CFB
	UINT32 trueIndex = cfb->virtSize & cfb->mask;
	UINT32 partASize = cfb->size - trueIndex;
	UINT32 partBSize = 0;
	 
	UINT32 i;
	UINT32 j;

	partASize = size < partASize ? size : partASize;


	for(i=trueIndex, j=0 ; i<(trueIndex + partASize) ; i++, j++)
		cfb->buf[i] = fbuf[j];

	if(size > partASize) 
	{
		partBSize = size - partASize;

		for(i=0, j=partASize ; i<partBSize ; i++, j++)
			cfb->buf[i] = fbuf[j];
	}

	cfb->virtSize += size;

	return true;

}


bool CFB_debugPrint(CIRCULAR_FLOAT_BUFFER cfb)
{
	if(cfb == NULL)
		return false;

	
	(void)_tprintf_s(_T("CFB: \n"));
	(void)_tprintf_s(_T("\tSize: %lu\n"), cfb->size);
	(void)_tprintf_s(_T("\tVirtSize: %lu\n"), cfb->virtSize);
	(void)_tprintf_s(_T("\tTruePos: %lu\n"), cfb->virtSize & cfb->mask);
	(void)_tprintf_s(_T("Buf: {"));

	for(UINT32 i=0 ; i<cfb->size-1 ; i++)
		(void)_tprintf_s(_T("%3.2f,"), cfb->buf[i]);

	(void)_tprintf_s(_T("%3.2f"), cfb->buf[cfb->size-1]);

	(void)_tprintf_s(_T("}\n"));


	return true;
}
