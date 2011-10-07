
//-------------------------------------------------------------------------------------------------
///
/// @file WavIO_MAIN.c
///
/// @brief Test entry point for WavIO library.  Demonstates use of library.
///
/// @author Jeff Wilson (jeff@imtc.gatech.edu)
/// 
//-------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <tchar.h>


#include "common\System.hpp"
#include "common\CommonTypes.hpp"
#include "wavio\WavIO.hpp"

#include "Common/HelpDebug.hpp"

/// Enables/disables a "soft" comparison of two files in the test routines
#define USE_SOFT_COMPARE
/// When using the "soft" comparison, what is the max allowed error?
#define MAX_ALLOWED_ERROR (0.00004f)

/// Size of a char buffer for WAV_FILE info output
#define OUTSTRSIZE (512)
/// Frame size (for float buffer)
#define FBUFSIZE (2048)
/// maximum number of channels in float buffer
#define MAXCHAN (2)


//-------------------------------------------------------------------------------------------------
// testWavInOut
//-------------------------------------------------------------------------------------------------
///
/// @brief test reading a file in to a float buffer, then writing back to another file
///
/// @param[in]		inFile		relative or absolute path to a WAV file to be read in
/// @param[in]		outFile		relative or absolute path to where a WAV file should be written
/// @param[in]		stereoOut	determines whether to use one channel, or two
///
/// @pre inFile, outFile must be non-null, point to valid memory, point to available files for read/write (for successful operation)
/// @pre inFile must have at least two channels if stereoOut == TRUE
///
/// @post read in inFile in frames of normalized floats, then copy them to outFile
/// @post if stereoOut is false, will only read 1 channel from inFile and copy 1 channel to outFile
/// @post if stereoOut is true, will only read 2 channels from inFile and copy 2 channels to outFile
/// @post attempts to catch any error and print a warning/error message
/// @post undefined behavior if inFile, outFile point to invalid memory
///
void testWavInOut(_TCHAR* inFile, _TCHAR *outFile, BOOL stereoOut) {

	enum WAV_ERR werr;
	
	WAV_FILE wf = NULL;
	WAV_FILE writeWF = NULL;

	_TCHAR outStr[OUTSTRSIZE];

	//float fbuf1[MAXCHAN][FBUFSIZE];
	float * fbuf1 = NULL;

	UINT32 floatsRead;
	UINT32 floatsReadTotal;
	

	UINT16 numChan;


	if(stereoOut)
		numChan = 2;
	else
		numChan = 1;


	fbuf1 = (float*)malloc((size_t) ( numChan * FBUFSIZE * sizeof(float) ));

	if(fbuf1 == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() fbuf1.\n"));
		return;
	}



	werr = wavOpenForRead(inFile, &wf);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), inFile, werr);
		return;

	} //if

	werr = toString(wf, outStr, (size_t)OUTSTRSIZE);

	if(werr == WAV_NO_ERR) {
		(void)_tprintf_s(_T("%s\n"), inFile);
		(void)_tprintf_s(_T("%s\n"), outStr);
	} else {
		(void)_tprintf_s(_T("Failed to create string for: %s\n"), inFile);
	} //else

	if(wf->wavInfo->numChannels < numChan) {
		(void)_tprintf_s(_T("%s must be stereo for the stereoOut test config!\n"), inFile);
		return;
	} //if


	werr = wavOpenForWrite(outFile, &writeWF, WAV_FORMAT_PCM, numChan, 44100, 16);


	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), outFile, werr);
		return;

	} //if


	werr = WAV_NO_ERR;

	floatsReadTotal = 0;

	while(floatsReadTotal < wf->wavInfo->numSamples) {

		werr = wavRead(wf, (float*)fbuf1, (UINT32)FBUFSIZE, numChan, &floatsRead);
		
		if(werr != WAV_NO_ERR) {
			(void)_tprintf_s(_T("Unexpected error doing wavRead()!\n"));
			return;
		} //if

		floatsReadTotal += floatsRead;

//		if(floatsRead < FBUFSIZE)
//			(void)_tprintf_s(_T("floatsRead less than FBUFSIZE (%lu < %lu) @ %lu out of %lu\n"), floatsRead, FBUFSIZE,
//			floatsReadTotal, wf->wavInfo->numSamples);

		werr =  waveWrite(writeWF, (float*)fbuf1, (UINT32)FBUFSIZE, numChan, floatsRead);

		if(werr != WAV_NO_ERR) {
			(void)_tprintf_s(_T("Unexpected error doing wavWrite()!\n"));
			return;
		}//if


	} //while


	(void)_tprintf_s(_T("\nRead/Write Total: %lu\n\n"), floatsReadTotal);

	//correct/finalize the file size
	werr = wavWriteFinalize(writeWF);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to finalize output wav file (%s) with err: %d\n"), outFile, werr);

	} //if

	werr = toString(writeWF, outStr, (size_t)OUTSTRSIZE);

	if(werr == WAV_NO_ERR) {
		(void)_tprintf_s(_T("%s\n"), outFile);
		(void)_tprintf_s(_T("%s\n"), outStr);
	} else {
		(void)_tprintf_s(_T("Failed to create string for: %s\n"), outFile);
	} //else


	closeWavFile(wf);
	closeWavFile(writeWF);
	wf = NULL;
	writeWF = NULL;

	free((void*)fbuf1);
	
} //testWavInOut()



//-------------------------------------------------------------------------------------------------
// compareWavFiles
//-------------------------------------------------------------------------------------------------
///
/// @brief compares two wav files sample by sample for mono or stereo channels and prints output to user regarding match.
///
/// @param[in]		inFile1		relative or absolute path to a WAV file to be read in
/// @param[in]		inFile2		relative or absolute path to where a WAV file should be written
/// @param[in]		stereoOut	determines whether to use one channel, or two
///
/// @pre inFile, inFile2 must be non-null, point to valid memory, point to available files for read/write (for successful operation)
/// @pre inFile, inFile2 must have at least two channels if stereoOut == TRUE
///
/// @post read in inFile and inFile2 in frames of normalized floats, then compare samples
/// @post if streoOut is false, will only read 1 channel from inFile, inFile2
/// @post if streoOut is true, will only read 2 channels from inFile, inFile2
/// @post attempts to catch any error and print a warning/error message
/// @post undefined behavior if inFile, inFile2 point to invalid memory
///
void compareWavFiles(_TCHAR * inFile1, _TCHAR * inFile2, BOOL stereoOut) {
	//Now, let's reopen the two files in read mode and compare them sample by sample!
	//Note, that the file written to may have less channels than the file read from
	//We only compare the channels of the written file to same channels of read (ignoring any extras)

	enum WAV_ERR werr;
	enum WAV_ERR werr2;
	WAV_FILE wf1 = NULL;
	WAV_FILE wf2 = NULL;

	//float fbuf1[MAXCHAN][FBUFSIZE];
	//float fbuf2[MAXCHAN][FBUFSIZE];

	float * fbuf1 = (float*)NULL;
	float * fbuf2 = (float*)NULL;

	float minValue;
	float maxValue;

	float floatSum;

	UINT32 floatsRead;
	UINT32 floatsRead2;
	UINT32 floatsReadTotal;

	UINT32 negCount;

	UINT32 i;
	UINT32 j;

	BOOL isMatch = TRUE;


	UINT16 numChan;


	if(stereoOut)
		numChan = 2;
	else
		numChan = 1;


	fbuf1 = (float*)malloc((size_t) ( numChan * FBUFSIZE * sizeof(float) ));

	if(fbuf1 == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() fbuf1.\n"));
		return;
	}

	fbuf2 = (float*)malloc((size_t) ( numChan * FBUFSIZE * sizeof(float) ));

	if(fbuf2 == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() fbuf2.\n"));
		return;
	}



	werr = wavOpenForRead(inFile1, &wf1);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), inFile1, werr);
		return;

	} //if

	werr = wavOpenForRead(inFile2, &wf2);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), inFile2, werr);
		return;

	} //if


	if(wf1->wavInfo->numChannels < numChan) {
		(void)_tprintf_s(_T("%s must be stereo for the stereoOut test config!\n"), inFile1);
		return;
	} //if

	if(wf2->wavInfo->numChannels < numChan) {
		(void)_tprintf_s(_T("%s must be stereo for the stereoOut test config!\n"), inFile2);
		return;
	} //if



	isMatch = TRUE;
	werr = WAV_NO_ERR;
	werr2 = WAV_NO_ERR;

	floatsReadTotal = 0;

	negCount = 0;


	minValue = 2.0f;
	maxValue = -2.0f;

	floatSum = 0.0f;

	while(floatsReadTotal < wf1->wavInfo->numSamples && isMatch) {

		werr = wavRead(wf1, (float *)fbuf1, (UINT32)FBUFSIZE, numChan, &floatsRead);
		werr2 =  wavRead(wf2, (float *)fbuf2, (UINT32)FBUFSIZE, numChan, &floatsRead2);

		if(werr != WAV_NO_ERR || werr2 != WAV_NO_ERR) {
			(void)_tprintf_s(_T("Unexpected EOF!\n"));
			isMatch = FALSE;
			break;
		} else if(floatsRead != floatsRead2) {
			(void)_tprintf_s(_T("floats read not same size\n"));
			isMatch = FALSE;
			break;
		} else {
			
			float fb1val = 0.0f;
			float fb2val = 0.0f;

			for(i=0 ; i < floatsRead ; i++) {

				for(j=0 ; j < numChan ; j++) {

					fb1val = fbuf1[j*FBUFSIZE+i];
					fb2val = fbuf2[j*FBUFSIZE+i];
				
					P_ASSERT(fb1val >= -1.0f && fb1val <= 1.0f, _T("Floats (fbuf1) not normalized!!\n"));
					
					P_ASSERT(fb2val >= -1.0f && fb2val <= 1.0f, _T("Floats (fbuf2) not normalized!!\n"));

					//P_ASSERT(fbuf1[j][i] >= -1.0f && fbuf1[j][i] <= 1.0f, _T("Floats (fbuf1) not normalized!!\n"));
					//
					//P_ASSERT(fbuf2[j][i] >= -1.0f && fbuf2[j][i] <= 1.0f, _T("Floats (fbuf2) not normalized!!\n"));

#ifdef USE_SOFT_COMPARE

					if(fb1val - fb2val > MAX_ALLOWED_ERROR ) {
						(void)_tprintf_s(_T("float vals don't match within tolerance!\n"));
						(void)_tprintf_s(_T("(fbuf1[%lu][%lu](=%f)) - (fbuf2[%lu][%lu](=%f)) (=%f) > MAX_ALLOWED_ERROR(=%f)\n"), 
							j,i,fb1val, j,i,fb2val, fb1val - fb2val, MAX_ALLOWED_ERROR);

					//if(fbuf1[j][i] - fbuf2[j][i] > MAX_ALLOWED_ERROR ) {
					//	(void)_tprintf_s(_T("float vals don't match within tolerance!\n"));
					//	(void)_tprintf_s(_T("(fbuf1[%lu][%lu](=%f)) - (fbuf2[%lu][%lu](=%f)) (=%f) > MAX_ALLOWED_ERROR(=%f)\n"), 
					//		j,i,fbuf1[j][i], j,i,fbuf2[j][i], fbuf1[j][i] - fbuf2[j][i], MAX_ALLOWED_ERROR);

#else
					if(fb1val != fb2val ) {
						(void)_tprintf_s(_T("float vals don't match!\n"));
						(void)_tprintf_s(_T("fbuf1[%lu][%lu](=%f) != fbuf2[%lu][%lu](=%f)\n"), j,i,fb1val, j,i,fb2val);

					//if(fbuf1[j][i] != fbuf2[j][i] ) {
					//	(void)_tprintf_s(_T("float vals don't match!\n"));
					//	(void)_tprintf_s(_T("fbuf1[%lu][%lu](=%f) != fbuf2[%lu][%lu](=%f)\n"), j,i,fbuf1[j][i], j,i,fbuf2[j][i]);
#endif
						isMatch = FALSE;
						break;
					} //if

				} //for(j)

				if(!isMatch)
					break;

				floatsReadTotal++;

				//use LEFT channel only for some stats


				fb1val = fbuf1[0*FBUFSIZE + i];

				floatSum += fb1val;

				if(fb1val < minValue)
					minValue = fb1val;

				if(fb1val > maxValue)
					maxValue = fb1val;

				if(fb1val < 0.0f)
					negCount++;

				//floatSum += fbuf1[0][i];

				//if(fbuf1[0][i] < minValue)
				//	minValue = fbuf1[0][i];

				//if(fbuf1[0][i] > maxValue)
				//	maxValue = fbuf1[0][i];

				//if(fbuf1[0][i] < 0.0f)
				//	negCount++;

			} //for

		} //else

	} //while

	if(!isMatch)
		(void)_tprintf_s(_T("File data DOES NOT match at: %lu sample!!!\n"), floatsReadTotal);
	else
		(void)_tprintf_s(_T("File data matches!!! Compared %lu samples\n"), floatsReadTotal);


	(void)_tprintf_s(_T("AVG sample val observed: %f\n"), floatSum / floatsReadTotal);

	(void)_tprintf_s(_T("Negative samples observed: %lu\n"), negCount);

	(void)_tprintf_s(_T("Min and max values observed: [%f, %f]\n"), minValue, maxValue);

	closeWavFile(wf1);
	closeWavFile(wf2);
	wf1 = NULL;
	wf2 = NULL;

	free(fbuf1);
	free(fbuf2);

} //compareWavFiles()


//-------------------------------------------------------------------------------------------------
// _tmain
//-------------------------------------------------------------------------------------------------
///
/// @brief A test main() for the WavIO library.
///
/// @param[in]		argc	not used
/// @param[in]		argv	not used
///
/// @return		int	Always returns 0
///
/// @pre	Test wav files must exist in ./audio off of working directory
///
/// @post	Several tests run, errors printed on failure.
/// @post	For some errors, program may exit abruptly without cleanup (shouldn't be a problem on win2k or above)
///
int _tmain(int argc, _TCHAR* argv[])
{
	
	_TCHAR* testFileMono = _T("./audio/natures_miracle.wav");
	//_TCHAR* testFileMono2 = _T("./audio/test1_16_1_44_PCM.wav");
	//_TCHAR* testFileMono3 = _T("./audio/test2_16_1_44_PCM.wav");	
	//_TCHAR* testFileStereo = _T("./audio/test3_16_2_44_PCM.wav");
	

	_TCHAR* OUTTestFile0 = _T("./audio/out0.wav");
	//_TCHAR* OUTTestFile1 = _T("./audio/out1.wav");
	//_TCHAR* OUTTestFile2 = _T("./audio/out2.wav");
	//_TCHAR* OUTTestFile3 = _T("./audio/out3.wav");
	//_TCHAR* OUTTestFile4 = _T("./audio/out4.wav");

	//handle WARNING C4100 (unref'ed formals)
	(void)argc;
	(void)argv;



	(void)_tprintf_s(_T("TEST MONO (16 bit 44_1) IN AND MONO (16 bit 44_1) OUT:\n"));
	testWavInOut(testFileMono, OUTTestFile0, FALSE);
	compareWavFiles(testFileMono, OUTTestFile0, FALSE);


	//(void)_tprintf_s(_T("\n\nTEST STEREO (16 bit 44_1) IN AND MONO (16 bit 44_1) OUT:\n"));
	//testWavInOut(testFileStereo, OUTTestFile1, FALSE);
	//compareWavFiles(testFileStereo, OUTTestFile1, FALSE);
	//(void)_tprintf_s(_T("\n\nTEST STEREO (16 bit 44_1) IN AND STEREO (16 bit 44_1) OUT:\n"));
	//testWavInOut(testFileStereo, OUTTestFile2, TRUE);
	//compareWavFiles(testFileStereo, OUTTestFile2, TRUE);

	//(void)_tprintf_s(_T("TEST MONO ALT (16 bit 44_1) IN AND MONO (16 bit 44_1) OUT:\n"));
	//testWavInOut(testFileMono2, OUTTestFile3, FALSE);
	//compareWavFiles(testFileMono2, OUTTestFile3, FALSE);

	//(void)_tprintf_s(_T("TEST MONO ALT-2 (16 bit 44_1) IN AND MONO (16 bit 44_1) OUT:\n"));
	//testWavInOut(testFileMono3, OUTTestFile4, FALSE);
	//compareWavFiles(testFileMono3, OUTTestFile4, FALSE);

	(void)_tprintf_s(_T("Press enter to exit.\n"));

	(void)getc(stdin);

	return 0;

} //_tmain

