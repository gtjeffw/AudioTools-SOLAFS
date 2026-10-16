
//-------------------------------------------------------------------------------------------------
///
/// @file WavIO.c
/// 
/// @brief Description: Library for reading and writing WAV files.  See WavIO.h for more documentation.
///
/// @author Jeff Wilson
///
/// @see WavIO.h
///
//-------------------------------------------------------------------------------------------------


#include <stdio.h>
#include <tchar.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "Common/System.hpp"
#include "Common/CommonTypes.hpp"
#include "Common/HelperTools.hpp"

#include "Common/HelpDebug.hpp"

#include "FileIO.hpp"
#include "WavIO.hpp"



//#define NORMALIZE

/// @def USE_DOUBLES_FOR_NORMALIZATION
/// if defined, doubles are used for normalization before converting to floats (for numerical accuracy consideration)
//#define USE_DOUBLES_FOR_NORMALIZATION

/// @def HEADER_1ST_PART_LEN
/// size of the first couple block headers in a WAV file
#define HEADER_1ST_PART_LEN		(8) //4 + 4

/// @def HEADER_2ND_PART_LEN
/// size of a WAV header after the first size indicator
#define HEADER_2ND_PART_LEN		(36) //3*4 + (2*2+2*4+2*2) + 2*4

/// @def WAV_MIN_INT16
/// the minimum size int16 to assume for use in WAV file headers
#define WAV_MIN_INT16 (-MIN_INT16_TWOS_COMP)//(-MAX_INT16)

/// @def WAV_MAX_INT16
/// the max size int16 to assume for use in WAV file headers
#define WAV_MAX_INT16 (MAX_INT16)


//This code depends on system attributes such as big endian versus little endian
//we might want to reconsider how this is handled as most wav's are made on little endian systems.
//the


//TODO: if we decide to switch to procedural C++, change type-cast literals to use the U and L suffix (when needed)


void freeWavFile(WAV_FILE wf);


///@name Initialization/Deinitialization Routines
///@brief Routines for creating and destroying WAV files
///@{

//-------------------------------------------------------------------------------------------------
// createWavInfo
//-------------------------------------------------------------------------------------------------
///
/// @brief Create a zeroed WAVE_INFO on the heap, returns pointer to it
///
/// @param[out]		wiOut	Pointer to WAV_INFO for assignment of a ptr to allocated WAV_INFO_STRUCT
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	wiOut must be non-null and point to valid memory
/// @pre	enough memory must be available for successful creation of WAV_INFO (and struct)
///
/// @post	returns WAV_NO_ERR if successful, and wiOut points to a WAV_INFO that points to malloc'ed WAV_INFO_STRUCT, all values zeroed
/// @post	returns WAV_ERR_OUT_OF_MEM if cannot allocate needed memory
/// @post	undefined behavior if wiOut points to invalid memory
///
enum WAV_ERR createWavInfo(WAV_INFO * wiOut) {

	WAV_INFO wi = NULL;

	if(wiOut == NULL)
		return WAV_ERR_BAD_ARG;

	wi = (WAV_INFO)malloc(sizeof(WAV_INFO_STRUCT));

	if(wi == NULL) {
		return WAV_ERR_OUT_OF_MEM;;
	} //if

	(void)memset((void*)wi, 0, sizeof(WAV_INFO_STRUCT));


	*wiOut = wi;

	return WAV_NO_ERR;

} //createWavInfo()



//-------------------------------------------------------------------------------------------------
// createWavFile
//-------------------------------------------------------------------------------------------------
///
/// @brief Create a zeroed WAVE_FILE on the heap, returns pointer to it
///
/// @param[out]		wfOut	Pointer to WAV_FILE for assignment of a ptr to allocated WAV_FILE_STRUCT
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	wiOut must be non-null and point to valid memory
/// @pre	enough memory must be available for successful creation of WAV_FILE (and struct)
///
/// @post	returns WAV_NO_ERR if successful, and wfOut points to a WAV_FILE that points to malloc'ed WAV_FILE_STRUCT, all values zeroed
/// @post	returns WAV_ERR_OUT_OF_MEM if cannot allocate needed memory
/// @post	undefined behavior if wfOut points to invalid memory
///
enum WAV_ERR createWavFile(WAV_FILE * wfOut) {

	WAV_FILE wf = NULL;
	enum WAV_ERR wavErr;

	if(wfOut == NULL)
		return WAV_ERR_BAD_ARG;



	wf = (WAV_FILE)malloc(sizeof(WAV_FILE_STRUCT));

	if(wf == NULL) {

		return WAV_ERR_OUT_OF_MEM;

	} //if

	(void)memset((void*)wf, 0, sizeof(WAV_FILE_STRUCT));

	wavErr = createWavInfo(&(wf->wavInfo));

	if(wavErr != WAV_NO_ERR || wf->wavInfo == NULL) {
		freeWavFile(wf);
		return WAV_ERR_OUT_OF_MEM;
	} //if

	*wfOut = wf;

	return WAV_NO_ERR;

} //createWavFile()



//-------------------------------------------------------------------------------------------------
// freeWavInfo
//-------------------------------------------------------------------------------------------------
///
/// @brief Free a WAV_INFO_STRUCT from the heap
///
/// @param[in,out]		wi	Pointer to WAV_INFO for freeing from heap
///
/// @pre	wi must be non-null and point to valid memory on the heap (originally created with createWavInfo())
///
/// @post	frees memory if wi is valid
/// @post	if wi in NULL, does nothing
/// @post	undefined behavior if wi points to invalid memory
///
void freeWavInfo(WAV_INFO wi) {

	if(wi != NULL)
		free((void*)wi);

} //freeWavInfo()

//-------------------------------------------------------------------------------------------------

void closeWavFile(WAV_FILE wf) {
	freeWavFile(wf);
} //closeWavFile()

//-------------------------------------------------------------------------------------------------
// freeWavFile
//-------------------------------------------------------------------------------------------------
///
/// @brief Free a WAV_FILE_STRUCT from the heap (recursively)
///
/// @param[in,out]		wf	Pointer to WAV_FILE for freeing from heap
///
/// @pre	wf must be non-null and point to valid memory on the heap (originally created with createWavFile())
///
/// @post	frees memory if wi is valid
/// @post	if wf in NULL, does nothing
/// @post	any other malloc'ed structs pointed to by WAV_FILE_STRUCT are freed (e.g. WAV_INFO)
/// @post	if a FILE is open within the WAV_FILE_STRUCT, it is first closed before freeing memory
/// @post	undefined behavior if wf points to invalid memory
///
void freeWavFile(WAV_FILE wf)
{
	if(wf != NULL) {
		if(wf->wavInfo != NULL)
			freeWavInfo(wf->wavInfo);

		//close open file, if needed
		if(wf->fp != NULL)
			(void)fclose(wf->fp);

		wf->fp = NULL;

		free( (void *)wf);
	} //if
} //freeWavFile



///@}


//-------------------------------------------------------------------------------------------------
enum WAV_ERR wavOpenForWrite(_TCHAR * filePath, WAV_FILE * wf, UINT16 formatTag, UINT16 numChannels, UINT32 samplesPerSec, 
							 UINT16 bitsPerSample)
{


	_TCHAR FM_WRITE[] = _T("wb");  //The b for binary mode is important!!!!

	errno_t err;

	enum WAV_ERR wavErr;

	WAV_FILE ret = NULL;

	if(filePath == NULL || wf == NULL)
		return WAV_ERR_BAD_ARG;


	//TODO determine minimum samples per sec allowed

	if(numChannels < 1 || samplesPerSec < 1) {
		return WAV_ERR_BAD_ARG;
	} //if

	if(bitsPerSample != 16) {
		return WAV_ERR_FILE_BPS_UNSUP;
	}//if

	switch(formatTag) {
		case WAV_FORMAT_PCM:
		break;
		default:
			return WAV_ERR_FILE_UNSUPPORTED;
	} //switch()


	wavErr = createWavFile(&ret);

	if(wavErr != WAV_NO_ERR || ret == NULL) {

		return wavErr;

	} //if

	ret->byteWriteCount = (UINT32)0;

	//TODO: next line MIGHT be giving:
	//First-chance exception at 0x00000000 in WavIO.exe: 0xC0000005: Access violation reading location 0x00000000.
	//It prints to stderr, but doesn't interrupt execution of the program.  Can't figure it out...
	if( (err = _tfopen_s(&(ret->fp), filePath, FM_WRITE)) != 0 ) {

		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if


	ret->wavInfo->formatTag = formatTag;
	ret->wavInfo->numChannels = numChannels;
	ret->wavInfo->samplesPerSec = samplesPerSec;
	ret->wavInfo->bitsPerSample = bitsPerSample;
	

	//this will need to change if 8bit is added
	if(ret->wavInfo->bitsPerSample <= 8) {

		freeWavFile(ret);
		return WAV_ERR_FILE_BPS_UNSUP;

	} //if
		

	ret->wavInfo->bytesPerSample = (ret->wavInfo->bitsPerSample + 7) / 8;

	ret->wavInfo->blockAlign = ret->wavInfo->numChannels * ret->wavInfo->bytesPerSample;
	ret->wavInfo->avgBytesPerSec = ret->wavInfo->samplesPerSec * ret->wavInfo->blockAlign;

	//guess max size (will not work on files systems that can't go greater than 2GB files,
	//but only a problem if file is actually filled all the way up!
	//the minus HEADER_1ST_PART_LEN only needed for 4GB limitation filesystems
	ret->wavInfo->dataLength = (UINT32)MAX_UINT32 - HEADER_1ST_PART_LEN - HEADER_2ND_PART_LEN;

	if(fputs("RIFF", ret->fp) < 0) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT32_LE((UINT32)(ret->wavInfo->dataLength + HEADER_2ND_PART_LEN), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(fputs("WAVE", ret->fp) < 0) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(fputs("fmt ", ret->fp) < 0) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT32_LE((UINT32)(16), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT16_LE(ret->wavInfo->formatTag, ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT16_LE(ret->wavInfo->numChannels, ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT32_LE((UINT32)(ret->wavInfo->samplesPerSec), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT32_LE((UINT32)(ret->wavInfo->avgBytesPerSec), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if


	if(!fWriteUINT16_LE(ret->wavInfo->blockAlign, ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT16_LE(ret->wavInfo->bitsPerSample, ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(fputs("data", ret->fp) < 0) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(!fWriteUINT32_LE((UINT32)(ret->wavInfo->dataLength), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_WRITE;
	} //if

	
	*wf = ret;

	return WAV_NO_ERR;

} //wavOpenForWrite()


//-------------------------------------------------------------------------------------------------
enum WAV_ERR wavOpenForRead(_TCHAR * filePath, WAV_FILE * wf)
{

	
	_TCHAR FM_READ[] = _T("rb");  //The b for binary mode is important!!!!

	//_TCHAR * fileMode = (_TCHAR*)NULL;

	errno_t err;

	enum WAV_ERR wavErr;

	WAV_FILE ret = NULL;

	char headerBuf[4];

	UINT32 len = (UINT32)0;

	int devNullInt;

	BOOL keepLooping;

	UINT32 bytesRead = (UINT32)0;

	if(filePath == NULL || wf == NULL)
		return WAV_ERR_BAD_ARG;


	wavErr = createWavFile(&ret);

	if(wavErr != WAV_NO_ERR || ret == NULL) {

		return wavErr;

	} //if

	//TODO: next line is giving:
	//First-chance exception at 0x00000000 in WavIO.exe: 0xC0000005: Access violation reading location 0x00000000.
	//It prints to stderr, but doesn't interrupt execution of the program.  Can't figure it out...
	if( (err = _tfopen_s(&(ret->fp), filePath, FM_READ)) != 0 ) {

		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if


	//not using _tchar funcs/types is intentional b/c wav file headers NOT unicode
	if( ((size_t)4 != fread(headerBuf, sizeof(char), (size_t)4, ret->fp)) || strncmp("RIFF", headerBuf, (size_t)4))
	{
		freeWavFile(ret);
		return WAV_ERR_FILE_UNRECOGNIZED;;
	} //if

	bytesRead += (UINT32)4;

	if(!fReadUINT32_LE(&len, ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)4;
	//(void)_tprintf_s(_T("Size of block %lu\n"), len);

	if( (size_t)4 != fread(headerBuf, sizeof(char), (size_t)4, ret->fp) || strncmp("WAVE", headerBuf, (size_t)4))
	{
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;;
	} //if

	bytesRead += (UINT32)4;

	keepLooping = TRUE;

	while(keepLooping) {

		if( (size_t)4 != fread(headerBuf, sizeof(char), (size_t)4, ret->fp) )
		{
			freeWavFile(ret);
			return WAV_ERR_FILE_LOAD;;
		} //if

		bytesRead += (UINT32)4;

		if(!fReadUINT32_LE(&len, ret->fp)) {
			freeWavFile(ret);
			return WAV_ERR_FILE_LOAD;
		} //if

		bytesRead += (UINT32)4;

		//(void)_tprintf_s(_T("Size of block %lu\n"), len);

		if(strncmp("fmt ", headerBuf, 4) == 0) {
			keepLooping = FALSE;
			break;
		} //if

		while(len > 0) {

			devNullInt = fgetc(ret->fp);

			bytesRead += (UINT32)1;

			len--;

			if(devNullInt == EOF) {
				freeWavFile(ret);
				return WAV_ERR_FILE_LOAD;
			} //if

		} //while


	} //while()


	if(len < 16) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if



	if(!fReadUINT16_LE(&(ret->wavInfo->formatTag), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)2;

	switch(ret->wavInfo->formatTag) {
		case WAV_FORMAT_PCM:
			//supported format
			break;
		default:
			freeWavFile(ret);
			return WAV_ERR_FILE_UNSUPPORTED;
	} //switch()

	if(!fReadUINT16_LE(&(ret->wavInfo->numChannels), ret->fp) || (ret->wavInfo->numChannels==0)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)2;

	if(!fReadUINT32_LE(&(ret->wavInfo->samplesPerSec), ret->fp) || (ret->wavInfo->samplesPerSec==0)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)4;

	if(!fReadUINT32_LE(&(ret->wavInfo->avgBytesPerSec), ret->fp) || (ret->wavInfo->avgBytesPerSec==0)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)4;

	if(!fReadUINT16_LE(&(ret->wavInfo->blockAlign), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)2;

	if(!fReadUINT16_LE(&(ret->wavInfo->bitsPerSample), ret->fp)) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if

	bytesRead += (UINT32)2;



	//this will need to change if support for 8bit is added
	if(ret->wavInfo->bitsPerSample <= 8) {

		freeWavFile(ret);
		return WAV_ERR_FILE_BPS_UNSUP;

	} //if
		
	ret->wavInfo->bytesPerSample = (ret->wavInfo->bitsPerSample + 7) / 8;


	len -= 16; //remove fmt size from length

	while(len > 0) {

		devNullInt = fgetc(ret->fp);

		bytesRead += (UINT32)1;

		len--;

		if(devNullInt == EOF) {
			freeWavFile(ret);
			return WAV_ERR_FILE_LOAD;
		} //if

	} //while


	
	//find WAVE data
	keepLooping = TRUE;
	while(keepLooping) {

		if( (size_t)4 != fread(headerBuf, sizeof(char), (size_t)4, ret->fp) )
		{
			freeWavFile(ret);
			return WAV_ERR_FILE_LOAD;
		} //if

		bytesRead += (UINT32)4;

		if(!fReadUINT32_LE(&len, ret->fp)) {
			freeWavFile(ret);
			return WAV_ERR_FILE_LOAD;
		} //if

		bytesRead += (UINT32)4;

		//(void)_tprintf_s(_T("Size of block %lu\n"), len);

		if(strncmp("data", headerBuf, 4) == 0) {
			keepLooping = FALSE;
			break;
		} //if

		while(len > 0) {

			devNullInt = fgetc(ret->fp);

			bytesRead += (UINT32)1;

			len--;

			if(devNullInt == EOF) {
				freeWavFile(ret);
				return WAV_ERR_FILE_LOAD;
			} //if

		} //while


	} //while()


	ret->wavInfo->dataLength = len;

	ret->wavInfo->numSamples = ret->wavInfo->dataLength / (ret->wavInfo->bytesPerSample * ret->wavInfo->numChannels);

	//make sure dataLength % bytesPerSample is 0 (e.g. dataLength is multiple of bytesPerSample)

	if( ((ret->wavInfo->dataLength) % (ret->wavInfo->bytesPerSample)) != 0 ) {
		freeWavFile(ret);
		return WAV_ERR_FILE_LOAD;
	} //if


	*wf = ret;

	return WAV_NO_ERR;

} //wavOpenForRead()


//-------------------------------------------------------------------------------------------------

enum WAV_ERR waveWrite(WAV_FILE w, float * fbuf1, UINT32 rowSize, UINT16 numChan, UINT32 size) {

	UINT32 i;
	UINT32 j;
	float v;
	UINT16 writeVal;

#ifdef USE_DOUBLES_FOR_NORMALIZATION
	double d;
#endif


	if(w == NULL || fbuf1 == NULL || rowSize == 0 || size == 0 || numChan < 1 || numChan > w->wavInfo->numChannels)
		return WAV_ERR_BAD_ARG;

	if(w->wavInfo->bitsPerSample != 16)
		return WAV_ERR_FILE_UNSUPPORTED;


	for(i=0 ; i<size ; i++) {

		for(j=0 ; j<numChan ; j++) {
			
			v = fbuf1[j*rowSize + i];

#ifdef NORMALIZE

			P_ASSERT( v >= -1.0f && v <= 1.0f, _T("waveWrite_fn_TO_16b(): float is not normalized!\n"));

#ifdef USE_DOUBLES_FOR_NORMALIZATION
			d = (double)v;
			
			writeVal = (UINT16)normalizeDouble(d, -1.0, 1.0, (double)WAV_MIN_INT16, (double)WAV_MAX_INT16);
#else
			
			//writeVal = (UINT16)normalizeFloat(v, -1.0, 1.0, (float)WAV_MIN_INT16, (float)WAV_MAX_INT16);
			writeVal = (UINT16)lrintf(normalizeFloat(v, -1.0, 1.0, (float)WAV_MIN_INT16, (float)WAV_MAX_INT16));
#endif
#else
			//clamp, then go through INT16: casting a negative float straight to an unsigned type
			//is undefined (it happens to work with MSVC on x86, but gives 0 on ARM).
			//For in-range values this truncates toward zero, same as the old cast did on x86.
			if(v > 32767.0f)
				v = 32767.0f;
			else if(v < -32768.0f)
				v = -32768.0f;

			writeVal = (UINT16)(INT16)v;
#endif

			if(!fWriteUINT16_LE(writeVal, w->fp))
				return WAV_ERR_FILE_WRITE;

			w->byteWriteCount += 2;
		} //for

	} //for
	
	return WAV_NO_ERR;

} //wavWrite()


//-------------------------------------------------------------------------------------------------

//TODO: should probably add a call to wavWriteFinalize() in wavClose() (when closing a write-file)
enum WAV_ERR wavWriteFinalize(WAV_FILE w) {

	if(w == NULL)
		return WAV_ERR_BAD_ARG;


	w->wavInfo->dataLength = w->byteWriteCount;
	w->wavInfo->numSamples = w->wavInfo->dataLength / (w->wavInfo->blockAlign);

	
	P_ASSERT( (w->wavInfo->dataLength) % (UINT32)(w->wavInfo->blockAlign) == 0Ui32, _T("wavWriteFinalize(): dataLength must be multiple of blockAlign!\n"));


	//TODO: fseek() to SEEK_END may not work in binary file mode under windows
	//http://cboard.cprogramming.com/c-programming/118839-determine-file-size.html
	//if so, use ftell() or fgetpos() or _stat()


	if(0 != fseek(w->fp, (long)4, SEEK_SET)) {
		(void)fseek(w->fp, (long)0, SEEK_END);
		return WAV_ERR_FILE_WRITE;
	}//if

	if(!fWriteUINT32_LE((UINT32)(w->wavInfo->dataLength + HEADER_2ND_PART_LEN), w->fp)) {
		(void)fseek(w->fp, (long)0, SEEK_END);
		return WAV_ERR_FILE_WRITE;
	} //if

	if(0 != fseek(w->fp, (long)40, SEEK_SET)) {
		(void)fseek(w->fp, (long)0, SEEK_END);
		return WAV_ERR_FILE_WRITE;
	}//if

	if(!fWriteUINT32_LE((UINT32)(w->wavInfo->dataLength), w->fp)) {
		(void)fseek(w->fp, (long)0, SEEK_END);
		return WAV_ERR_FILE_WRITE;
	} //if

	(void)fseek(w->fp, (long)0, SEEK_END);
	return WAV_NO_ERR;
} //wavWriteFinalize()



//-------------------------------------------------------------------------------------------------

enum WAV_ERR wavSeekRead(WAV_FILE w, UINT32 numSamples, UINT32 *numSkipped)
{

	if(w == NULL || numSamples == 0 || numSkipped == NULL)
		return WAV_ERR_BAD_ARG;

	if(w->wavInfo->bitsPerSample != 16) //this code probably works w/o the 16 bps req
		return WAV_ERR_FILE_UNSUPPORTED;


	//if this fails, then probably file is too small.  if a failure is seen, we might
	//next try to seek to SEEK_END.  If that is successful, then the file is indeed smaller than
	//expected.  This can be the documented behavior for what this function does
	if(0 == fseek(w->fp, (long)numSamples*w->wavInfo->numChannels*w->wavInfo->bytesPerSample, SEEK_CUR))
	{
		*numSkipped = numSamples;
		return WAV_NO_ERR;
	}
	else
	{
		//go to end (assume failure due to file too small)
		//TODO: determine if SEEK_END actually works or not under Windows (for files in binary mode)
		if(0 == fseek(w->fp, (long)0, SEEK_END))
		{
			long tmp = ftell(w->fp);

			if(tmp >= 0)
			{
				*numSkipped = tmp / (w->wavInfo->numChannels * w->wavInfo->bytesPerSample);
			}
		}
		else
		{
			*numSkipped = 0;
		}

		return WAV_ERR_UNEXPECTED_EOF;
	}
}


//-------------------------------------------------------------------------------------------------

enum WAV_ERR wavRead(WAV_FILE w, float * fbuf1, UINT32 size, UINT16 numChan, UINT32 *numRead) {

	UINT32 i;
	UINT32 j;
	BOOL success = FALSE;
	BOOL oneGoodRead = FALSE;

	UINT16 val;
	INT16 sval;

#ifdef NORMALIZE
#ifdef USE_DOUBLES_FOR_NORMALIZATION
	double dval;
	double normdval;
#else
	float fval;
#endif
#endif
	float normfval;

	if(w == NULL || fbuf1 == NULL || size == 0 || numRead == NULL || numChan < 1 || numChan > w->wavInfo->numChannels)
		return WAV_ERR_BAD_ARG;

	if(w->wavInfo->bitsPerSample != 16)
		return WAV_ERR_FILE_UNSUPPORTED;

	*numRead = 0UL;

	for(i=0 ; i<size ; i++) {

		for(j=0 ; j<numChan ; j++) {

			//will be signed for PCM 16 bit
			//read as unsigned, then cast should work
			success = fReadUINT16_LE(&val, w->fp);

			if(!success) {
				if(j != 0)
					oneGoodRead = FALSE; //truncated (unequal number of samples per channel!)
				break;
			} //if

			sval = (INT16)val;

#ifdef NORMALIZE
#ifdef USE_DOUBLES_FOR_NORMALIZATION

			dval = (double)sval;

			normdval = normalizeDouble(dval, (double)WAV_MIN_INT16, (double)WAV_MAX_INT16, -1.0, 1.0);
			normfval = (float)normdval;

#else

			//convert to normalized float and write to buffer
			fval = (float)sval;	

			normfval = normalizeFloat(fval, (float)WAV_MIN_INT16, (float)WAV_MAX_INT16, -1.0, 1.0);

#endif

			if(!(normfval >= -1.0f && normfval <= 1.0f)) {
				(void)_tprintf_s(_T("fval (%f) not in norm range[%f, %f]!\n"), fval, (float)WAV_MIN_INT16, (float)WAV_MAX_INT16);
				(void)_tprintf_s(_T("fval (%f) not in norm range[%f, %f]!\n"), normfval, -1.0f, 1.0f);

			} //if

			P_ASSERT(normfval >= -1.0f && normfval <= 1.0f, _T("wavRead_16b_TO_fn(): float is not normalized!\n"));
#else
			normfval = sval;
#endif

			fbuf1[j*size + i] = normfval;
		} //for(j)


		if(!success) {
			break;
		}  //if

		//only incr if desired num of chan incr successfully
		(*numRead)++;

		oneGoodRead = TRUE;



		//exhaust any channels we are not reading and copying into output buffer
		//Do we care if read is unsuccessful?  Probably means truncated file, but user will probably
		//figure that out on next read call (which will fail)
		for( ; j < w->wavInfo->numChannels ; j++) 
			success = fReadUINT16_LE(&val, w->fp);

	} //for


	if(!oneGoodRead)
		return WAV_ERR_UNEXPECTED_EOF;

	return WAV_NO_ERR;

} //waveRead


//-------------------------------------------------------------------------------------------------

enum WAV_ERR toString(WAV_FILE wf, _TCHAR * outStr, size_t size) {

	int ret;

	if(wf == NULL || outStr == NULL || size < 1)
		return WAV_ERR_BAD_ARG;



	ret=_sntprintf_s(outStr, size, _TRUNCATE, 
		_T("\
		 NumChannels:\t\t%u\n\
		 SamplesPerSec:\t\t%lu\n\
		 BitsPerSample:\t\t%u\n\
		 BytesPerSample:\t%u\n\
		 AvgBytesPerSec\t\t%lu\n\
		 BlockAlign:\t\t%u\n\
		 NumSamples:\t\t%u\n\
		 DataLength:\t\t%u\n\
		 FormatTag:\t\t%u\n"),
		 wf->wavInfo->numChannels,
		 wf->wavInfo->samplesPerSec,
		 wf->wavInfo->bitsPerSample,
		 wf->wavInfo->bytesPerSample,
		 wf->wavInfo->avgBytesPerSec,
		 wf->wavInfo->blockAlign,
		 wf->wavInfo->numSamples,
		 wf->wavInfo->dataLength,
		 wf->wavInfo->formatTag);



	if(ret < 0)
		return WAV_ERR_UNDETERMINED;

	return WAV_NO_ERR;

} //toString()

