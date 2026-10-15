
//-------------------------------------------------------------------------------------------------
///
/// @file WavIO.h
///
/// @brief Library for reading and writing WAV files.
///
/// Currently supports:
/// WAV_FORMAT_PCM
/// 16 bits, any valid number of channels, any valid sampling rate
///
/// Excellent WAV file format resource
/// http://www.borg.com/~jglatt/tech/wave.htm
///
/// @author Jeff Wilson
/// 
/// @see WavIO.c
///
//-------------------------------------------------------------------------------------------------



#ifndef WAV_IO_H
#define WAV_IO_H



///@name Format Tag Codes
///@brief tag codes used in the WAV format.
///@{

/// @def WAV_FORMAT_UNKNOWN
/// code for unknown wav format tag.
#define	WAV_FORMAT_UNKNOWN		(0x0000)
/// @def WAV_FORMAT_PCM
/// most common WAV data format.
#define	WAV_FORMAT_PCM			(0x0001) 

///@}


#ifdef __cplusplus
extern "C" {
#endif

/// Error codes for return from WavIO routines
typedef enum WAV_ERR {
	/// successful operation
	WAV_NO_ERR					= 1, 
	/// undetermined error
	WAV_ERR_UNDETERMINED		= 0, 
	/// out of memory
	WAV_ERR_OUT_OF_MEM			= -1, 
	/// input option not implemented
	WAV_ERR_OPT_NOT_IMPL		= -2,
	/// file load error
	WAV_ERR_FILE_LOAD			= -3,
	/// unrecognized file
	WAV_ERR_FILE_UNRECOGNIZED	= -4, 
	/// unsupported variant of an otherwise valid wav file
	WAV_ERR_FILE_UNSUPPORTED	= -5, 
	/// bits per second unsupported
	WAV_ERR_FILE_BPS_UNSUP		= -6, 
	/// bad argument
	WAV_ERR_BAD_ARG				= -7, 
	/// unexpected end of file
	WAV_ERR_UNEXPECTED_EOF		= -8,
	/// file write error
	WAV_ERR_FILE_WRITE			= -9 
};

/// Specifiers for wav file access
typedef enum WAV_FILE_ACCESS {
	/// set file for read
	WAV_FILE_READ=0,
	/// set file for write
	WAV_FILE_WRITE, 
	/// set file for read/write
	WAV_FILE_READWRITE 
	//add append modes?
}; 



/// A container for attributes from a WAV file header
typedef struct WAV_INFO_STRUCT {        
	/// The ID of the type of wav data (e.g. WAV_FORMAT_PCM)
  UINT16 formatTag; 
  ///number of channels such as (LEFT, RIGHT) or (LEFT, RIGHT, CENTER)
  UINT16 numChannels;
  ///samples per sec (per channel)
  UINT32 samplesPerSec; 
  ///avgBytesPerSec = samplesPerSec * blockAlign
  UINT32 avgBytesPerSec;
  ///raw frame size,  which is: blockAlign = bytesPerSample * numChannels
  UINT16 blockAlign; 
  ///bits per sample (per channel)
  UINT16 bitsPerSample; 
  ///bytes per sample: byte aligned (unpacked and zero-padded) (bitsPerSample + 7) / 8
  UINT16 bytesPerSample;
  ///in bytes: dataLength = numSamples * blockAlign
  UINT32 dataLength; 
  ///number of sample frames. channels not counted separately.
  UINT32 numSamples; 
} WAV_INFO_STRUCT, * WAV_INFO;

/// @typedef WAV_INFO
/// @brief A pointer to a WAV_INFO_STRUCT


/// A container for attributes of a WAV file, including the header and the file pointer
typedef struct WAV_FILE_STRUCT {
	/// wav file header info
	WAV_INFO wavInfo;
	/// file pointer
	FILE * fp; 
	/// count bytes written so we no how big a file is
	UINT32 byteWriteCount; 
} WAV_FILE_STRUCT, * WAV_FILE;

/// @typedef WAV_FILE
/// @brief A pointer a WAV_FILE_STRUCT


///@name WAV File I/O Routines
///@brief Routines for opening/closing/reading/writing to/from WAV files

//-------------------------------------------------------------------------------------------------
// wavOpenForRead
//-------------------------------------------------------------------------------------------------
///
/// @brief Open a new WAV file for reading.
///
/// @param[in]		filePath	_TCHAR* relative or absolute path to a WAV file.
/// @param[out]		wf			pointer to WAV_FILE that will be created and returned, upon success
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	filepath, wf must be non-null and point to valid memory
/// @pre	filepath must be a valid file path to a file available for reading (for successful operation)
/// @pre	enough memory must be available for successful creation of WAV_FILE
///
/// @post	returns WAV_NO_ERR if successful, and wf points to a WAV_FILE open and ready for use reading data
/// @post	returns WAV_ERR_BAD_ARG if inputs are NULL
/// @post	returns WAV_ERR_OUT_OF_MEM if cannot allocate needed memory
/// @post	returns WAV_ERR_FILE_LOAD if there are any errors loading or reading file (possibly due to file permissions or truncation)
/// @post	returns WAV_ERR_FILE_UNRECOGNIZED if the file is clearly not a WAV file (i.e. no RIFF header)
/// @post	returns WAV_ERR_FILE_UNSUPPORTED if WAV file is of a type not supported (see file documentation for supported types)
/// @post	returns WAV_ERR_BPS_UNSUP if an unsupported bits per sample is found in the WAV file header (see file docs for supported types)
/// @post	undefined behavior if filepath, wf point to invalid memory
/// @post	no memory is malloc'ed (and unfreed) if a return other than WAV_NO_ERR
///
enum WAV_ERR wavOpenForRead(_TCHAR * filePath, WAV_FILE * wf);

//-------------------------------------------------------------------------------------------------
// wavOpenForWrite
//-------------------------------------------------------------------------------------------------
///
/// @brief Open a new WAV file for writing.
///
/// @param[in]		filePath		_TCHAR* relative or absolute path to a WAV file.
/// @param[out]		wf				pointer to WAV_FILE that will be created and returned, upon success
/// @param[in]		formatTag		type of WAV file to create (see file documentation for supported types)
/// @param[in]		numChannels		The number of channels to use in this WAV (should be at least 1)
/// @param[in]		samplesPerSec	The number of samples per sec to use in this WAV (should be at least 1)
/// @param[in]		bitsPerSample	The bits per sample to use in this WAV (see file documentation for supported types)
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	filepath, wf must be non-null and point to valid memory
/// @pre	filepath must be a valid file path to a file available for writing (for successful operation)
/// @pre	enough memory must be available for successful creation of WAV_FILE
/// @pre	formatTag, numChannels, samplesPerSec, bitsPerSample must all be valid for WAV files and supported by the WavIO library
///
/// @post	returns WAV_NO_ERR if successful, and wf points to a WAV_FILE open and ready for use writing data
/// @post	returns WAV_ERR_BAD_ARG if inputs are NULL
/// @post	returns WAV_ERR_BAD_ARG if numChannels, samplesPerSec less than one
/// @post	returns WAV_ERR_FILE_UNSUPPORTED if formatTag is of a type not supported (see file documentation for supported types)
/// @post	returns WAV_ERR_BPS_UNSUP if an unsupported bitsPerSample is selected (see file docs for supported types)
/// @post	returns WAV_ERR_OUT_OF_MEM if cannot allocate needed memory
/// @post	returns WAV_ERR_FILE_WRITE if there are any errors loading or writing file header (possibly due to file permissions or truncation)
/// @post	undefined behavior if filepath, wf point to invalid memory
/// @post	no memory is malloc'ed (and unfreed) if a return other than WAV_NO_ERR
///
enum WAV_ERR wavOpenForWrite(_TCHAR * filePath, WAV_FILE * wf, UINT16 formatTag, UINT16 numChannels, UINT32 samplesPerSec, 
							 UINT16 bitsPerSample);



//-------------------------------------------------------------------------------------------------
// wavRead_
//-------------------------------------------------------------------------------------------------
///
/// @brief Move read head w/o overhead of conversions, etc.
enum WAV_ERR wavSeekRead(WAV_FILE w, UINT32 numSamples, UINT32 *numSkipped);



//-------------------------------------------------------------------------------------------------
// wavRead_
//-------------------------------------------------------------------------------------------------
///
/// @brief Reads a block of normalized ([-1.0f, 1.0f]) floats from a 16bps WAVE_FILE
///
/// @param[in,out]		w		Pointer to WAV_FILE, must be open for reading
/// @param[out]		fbuf1	2D array of floats of dimensions float[numChan][size]
/// @param[in]		size	the maximum desired number of samples to read (matches the size of 2nd dimension of fbuf1)
/// @param[in]		numChan	the number of channels to read into fbuf (matches the size of 1st dimension of fbuf1)
/// @param[out]		numRead	the actual number of samples read
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	w, numRead, fbuf1 must be non-null and point to valid memory, and w must be open for reading
/// @pre	fbuf1 must point to allocated memory of size sizeof(float)*numChan*size
/// @pre	for successful operation, numChan must be >0 and <= the number of channels of the WAV file being read from
/// @pre	for successful operation, size must be >0
/// @pre	for successful operation, w should be a 16bps file
///
/// @post	returns WAV_NO_ERR if successful, and fbuf1 contains normalized floats. numRead specifies how many samples (across channels) actually read
/// @post	So long as there is at least one sample (across all desired channels) left in w, WAV_NO_ERR is returned
/// @post	returns WAV_ERR_UNEXPECTED_EOF if there is a file read error on attempt to read first float from w (i.e. no samples left)
/// @post	returns WAV_ERR_BAD_ARG if w,fbuf1, numRead == NULL, size==0, numChan<1, numChan > w->wavInfo->numChannels
/// @post	returns WAV_ERR_FILE_UNSUPPORTED if w->wavInfo->bitsPerSample != 16
/// @post	undefined behavior if w,fbu1, or numRead points to invalid memory
/// @post	w's FILE file pointer is at unknown location on failure
///
enum WAV_ERR wavRead(WAV_FILE w, float * fbuf1, UINT32 size, UINT16 numChan, UINT32 *numRead);


//-------------------------------------------------------------------------------------------------
// wavWrite
//-------------------------------------------------------------------------------------------------
///
/// @brief Writes a block of normalized ([-1.0f, 1.0f]) floats to a 16bps WAVE_FILE
///
/// @param[in,out]		w		Pointer to WAV_FILE must be open for writing
/// @param[out]		fbuf1	2D array of floats of dimensions float[numChan][rowsize]
/// @param[in]		rowSize	the rowsize of fbuf1 (matches the size of 2nd dimension of fbuf1)
/// @param[in]		numChan	the number of channels to write to w (matches the size of 1st dimension of fbuf1)
/// @param[in]		size	the actual number of samples to write from fbuf1 into w
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	w, fbuf1 must be non-null and point to valid memory, and w must be open for writing
/// @pre	fbuf1 must point to allocated memory of size sizeof(float)*numChan*rowSize
/// @pre	for successful operation, numChan must be >0 and <= the number of channels of the WAV file being written to
/// @pre	for successful operation, size must be >0
/// @pre	for successful operation, w should be a 16bps file
///
/// @post	returns WAV_NO_ERR if successful, and w's FILE is written normalized floats. 
/// @post	returns WAV_ERR_FILE_WRITE if there is a file write error
/// @post	returns WAV_ERR_BAD_ARG if w,fbuf1, == NULL, size, rowSize==0, numChan<1, numChan > w->wavInfo->numChannels
/// @post	returns WAV_ERR_FILE_UNSUPPORTED if w->wavInfo->bitsPerSample != 16
/// @post	undefined behavior if w or fbuf1 points to invalid memory
/// @post	w's FILE file pointer is at unknown location on failure
///
enum WAV_ERR waveWrite(WAV_FILE w, float * fbuf1, UINT32 rowSize, UINT16 numChan, UINT32 size);


//-------------------------------------------------------------------------------------------------
// wavWriteFinalize
//-------------------------------------------------------------------------------------------------
///
/// @brief Corrects the file size of a WAV that has been written to
///
/// @param[in,out]		w		Pointer to WAV_FILE
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	w must be non-null and point to valid memory, and w must be open for writing
/// @pre	FILE in w must be seekable
///
/// @post	returns WAV_NO_ERR if successful, and w's size in WAV file header is corrected (seek to end of file before return)
/// @post	returns WAV_ERR_FILE_WRITE if there is a file write error (will attempt to seek to the end of the file before return)
/// @post	undefined behavior if w points to invalid memory
///
enum WAV_ERR wavWriteFinalize(WAV_FILE w);

//-------------------------------------------------------------------------------------------------
// closeWavFile
//-------------------------------------------------------------------------------------------------
///
/// @brief Close the FILE within a WAV_FILE and free the WAV_FILE
///
/// @param[in,out]		wf	Pointer to WAV_FILE that contains open FILE
///
/// @pre	wf must be NULL, or point to valid memory on the heap (originally created with createWavFile())
///
/// @post	if an open FILE exists, it is closed
/// @post	wf (and its WAV_INFO) are freed and must not be used again
/// @post	if wf is NULL, does nothing
/// @post	undefined behavior if wf points to invalid memory
///
void closeWavFile(WAV_FILE wf);

///@}

///@name Miscellaneous Routines
///@brief Various useful routines for WAV files



//-------------------------------------------------------------------------------------------------
// toString
//-------------------------------------------------------------------------------------------------
///
/// @brief Copies string output about a WAV_FILE to a string
///
/// @param[in]		wf		Pointer to WAV_FILE
/// @param[out]		outStr	String to write info about wf to
/// @param[in]		size	The size of the string
///
/// @return        WAV_ERR enum of error codes identifying success or failure of operation
///
/// @pre	w must be non-null and point to valid memory, and w must be open for writing
/// @pre	FILE in w must be seekable
///
/// @post	returns WAV_NO_ERR if successful, 
/// @post	will truncate down to string of size if outStr is too small for output (always leaves room for null terminator)
/// @post	returns WAV_ERR_BAD_ARG if(wf == NULL || outStr == NULL || size < 1)
/// @post	returns WAV_ERR_UNDETERMINED if string print fails
/// @post	undefined behavior if w, outStr points to invalid memory
///
enum WAV_ERR toString(WAV_FILE wf, _TCHAR * outStr, size_t size);

///@}



#ifdef __cplusplus
};
#endif



#endif //#ifndef WAV_IO_H






