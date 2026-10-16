
//-------------------------------------------------------------------------------------------------

///
/// @author Jeff Wilson (jeff@imtc.gatech.edu)
/// 
//-------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <tchar.h>
#include <math.h>

#include <dirent.h>

#include "common\System.hpp"
#include "common\CommonTypes.hpp"
#include "wavio\WavIO.hpp"

#include "Common/HelpDebug.hpp"


/// Size of a char buffer for WAV_FILE info output
#define OUTSTRSIZE (512)
/// Frame size (for float buffer)
//#define FBUFSIZE (2048)
/// maximum number of channels in float buffer
#define MAXCHAN (2)

#define SILENT_THRESH (0.01f)

void silentStripper(_TCHAR* inFile, _TCHAR *outFile, float silenceThresh, UINT32 envRampSize_ms, UINT32 beginPad_ms, UINT32 endPad_ms) {

	enum WAV_ERR werr;
	
	WAV_FILE wf = NULL;
	WAV_FILE writeWF = NULL;

	_TCHAR outStr[OUTSTRSIZE];

	//float fbuf1[MAXCHAN][FBUFSIZE];
	float * fbuf1 = NULL;

	UINT32 floatsRead;
	//UINT32 floatsReadTotal;
	

	UINT16 numChan;

	UINT32 fBufSize = 0;


	float * fbufPadBegin = NULL;
	float * fbufPadEnd = NULL;

	UINT32 fBufPadBeginSize = 0;
	UINT32 fBufPadEndSize = 0;

	//declared up here so the goto's below don't skip their initialization
	UINT32 startPos = 0;
	UINT32 endPos = 0;
	UINT32 rampSize = 0;


	werr = wavOpenForRead(inFile, &wf);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), inFile, werr);
		goto SILENT_STRIPPER_CLEANUP;

	} //if


	werr = toString(wf, outStr, (size_t)OUTSTRSIZE);

	if(werr == WAV_NO_ERR) {
		(void)_tprintf_s(_T("%s\n"), inFile);
		(void)_tprintf_s(_T("%s\n"), outStr);
	} else {
		(void)_tprintf_s(_T("Failed to create string for: %s\n"), inFile);
	} //else



	numChan = wf->wavInfo->numChannels;

	if(numChan != 1)
	{
		(void)_tprintf_s(_T("Only mono files supported for now.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}


	fBufSize = wf->wavInfo->numSamples;

	//numSamples comes straight from the file header, so make sure the malloc() size can't overflow
	if(fBufSize == 0 || fBufSize > ((size_t)-1) / (numChan * sizeof(float)))
	{
		(void)_tprintf_s(_T("Bad sample count in file header.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}

	fbuf1 = (float*)malloc((size_t)numChan * fBufSize * sizeof(float));

	if(fbuf1 == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() fbuf1.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}


	fBufPadBeginSize =  (UINT32)(wf->wavInfo->samplesPerSec * (beginPad_ms/(float)1000)); 

	if(fBufPadBeginSize > ((size_t)-1) / (numChan * sizeof(float)))
	{
		(void)_tprintf_s(_T("beginPad_ms is too big.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}

	fbufPadBegin = (float*)malloc((size_t)numChan * fBufPadBeginSize * sizeof(float));

	if(fbufPadBegin == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() fbufPadBegin.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}


	for(UINT32 i=0 ; i<fBufPadBeginSize ; ++i)
		fbufPadBegin[i] = 0.0f;

	fBufPadEndSize =  (UINT32)(wf->wavInfo->samplesPerSec * (endPad_ms/(float)1000)); 

	if(fBufPadEndSize > ((size_t)-1) / (numChan * sizeof(float)))
	{
		(void)_tprintf_s(_T("endPad_ms is too big.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}

	fbufPadEnd = (float*)malloc((size_t)numChan * fBufPadEndSize * sizeof(float));

	if(fbufPadEnd == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() fbufPadBegin.\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}

	for(UINT32 i=0 ; i<fBufPadEndSize ; ++i)
		fbufPadEnd[i] = 0.0f;

	werr = wavOpenForWrite(outFile, &writeWF, WAV_FORMAT_PCM, numChan, wf->wavInfo->samplesPerSec, wf->wavInfo->bitsPerSample); //44100, 16


	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), outFile, werr);
		goto SILENT_STRIPPER_CLEANUP;

	} //if


	werr = WAV_NO_ERR;

	werr = wavRead(wf, (float*)fbuf1, (UINT32)fBufSize, numChan, &floatsRead);

	if(werr != WAV_NO_ERR) {
		(void)_tprintf_s(_T("Unexpected error doing wavRead()!\n"));
		goto SILENT_STRIPPER_CLEANUP;
	} //if


	if(fBufSize != floatsRead)
		(void)_tprintf_s(_T("WARNING: floatsRead was not the same as fBufSize!\n"));

	startPos = 0;

	switch(numChan)
	{
	case 1:
		for(UINT32 i=0 ; i<floatsRead ; ++i)
		{
			//(void)_tprintf_s(_T("%f\n"),fbuf1[i]);
			if(fabs(fbuf1[i]) <= silenceThresh)
				startPos = i;
			else
				break;
		}
		break;
	case 2:
		for(UINT32 i=0 ; i<floatsRead*2 ; i+=2)
		{
			if(fabs(fbuf1[i]) <= silenceThresh)
				startPos = i;
			else
				break;
		}
		break;
	default:
		(void)_tprintf_s(_T("Unsupported channels!\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}


	endPos = floatsRead-1;

	//i is unsigned, so count down to 1 and use i-1 (i>=0 would always be true)
	switch(numChan)
	{
	case 1:
		for(UINT32 i=floatsRead ; i>0 ; --i)
		{
			//(void)_tprintf_s(_T("%f\n"),fbuf1[i-1]);
			if(fabs(fbuf1[i-1]) <= silenceThresh)
				endPos = i-1;
			else
				break;
		}
		break;
	case 2:
		for(UINT32 i=floatsRead ; i>0 ; --i)
		{
			if(fabs(fbuf1[(i-1)*2]) <= silenceThresh)
				endPos = (i-1)*2;
			else
				break;
		}
		break;
	default:
		(void)_tprintf_s(_T("Unsupported channels!\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}


	(void)_tprintf_s(_T("Trimmed Silence: start: %d stop: %d\n"), startPos, endPos);




	//ramp the sound (attack/sustain) from 0

	rampSize =  (UINT32)(wf->wavInfo->samplesPerSec * (envRampSize_ms/(float)1000)); //40;


	//written to avoid unsigned wrap-around (e.g. endPos < startPos when the whole file is silent)
	if( (endPos < startPos) || (rampSize > (endPos - startPos)/2) )
	{
		(void)_tprintf_s(_T("ERROR: Audio trimmed so small that beginning and end cannot be enveloped! (Or maybe ramp size is too big.)\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}


	for(UINT32 i=startPos ; i<(startPos + rampSize) ; ++i)
	{
		float ratio = (i-startPos)/(float)rampSize;

		if(ratio > 1.0f)
		{
			(void)_tprintf_s(_T("ERROR: rampUp envelope scalar greater than 1.0f (==%f)!\n"), ratio);
			goto SILENT_STRIPPER_CLEANUP;
		}

		fbuf1[i] *= ratio;
	}

	for(UINT32 n=0 ; n<=rampSize ; ++n) //i runs from endPos down to (endPos - rampSize)
	{
		UINT32 i = endPos - n;
		float ratio = ((endPos-i))/(float)rampSize;

		if(ratio > 1.0f)
		{
			(void)_tprintf_s(_T("ERROR: rampDown envelope scalar greater than 1.0f (==%f)!\n"), ratio);
			goto SILENT_STRIPPER_CLEANUP;
		}

		fbuf1[i] *= ratio;
	}



	(void)_tprintf_s(_T("Padding beginning!\n"));

	werr =  waveWrite(writeWF, (float*)fbufPadBegin, fBufPadBeginSize, numChan, fBufPadBeginSize);

	if(werr != WAV_NO_ERR) {
		(void)_tprintf_s(_T("Unexpected error doing wavWrite()!\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}//if


		(void)_tprintf_s(_T("Writing cropped and enveloped data!\n"));

	werr =  waveWrite(writeWF, (float*)(&(fbuf1[startPos])), (UINT32)(endPos-startPos), numChan, (endPos-startPos));

	if(werr != WAV_NO_ERR) {
		(void)_tprintf_s(_T("Unexpected error doing wavWrite()!\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}//if


	(void)_tprintf_s(_T("Padding ending!\n"));

	werr =  waveWrite(writeWF, (float*)fbufPadEnd, fBufPadEndSize, numChan, fBufPadEndSize);

	if(werr != WAV_NO_ERR) {
		(void)_tprintf_s(_T("Unexpected error doing wavWrite()!\n"));
		goto SILENT_STRIPPER_CLEANUP;
	}//if


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


SILENT_STRIPPER_CLEANUP:

	//closeWavFile() and free() both do nothing for NULL
	closeWavFile(wf);
	closeWavFile(writeWF);
	wf = NULL;
	writeWF = NULL;

	free((void*)fbuf1);

	free((void*)fbufPadBegin);
	free((void*)fbufPadEnd);

} //testWavInOut()



char * wstrdup(_TCHAR * wSrc)
{
	char * l_nstr = NULL;


	if (sizeof(TCHAR) == sizeof(wchar_t))
	{
		int l_len = _tcslen(wSrc);
		l_nstr = (char*)malloc(l_len+1);

		//copy at most l_len chars (narrowing each one), so an empty string can't overrun l_nstr
		if (l_nstr) {
			for(int l_idx=0 ; l_idx<l_len ; l_idx++)
				l_nstr[l_idx] = (char)wSrc[l_idx];
			l_nstr[l_len] = 0;
		}
	}
	else //must be 8 bit
	{
		l_nstr = (char*)malloc(strlen((char*)wSrc)+1);

		if(l_nstr)
			strcpy_s(l_nstr, strlen((char*)wSrc)+1, (char*)wSrc);

	}
	return l_nstr;
}



_TCHAR * strdup(char * src)
{
	_TCHAR * l_nstr = NULL;


	if (sizeof(TCHAR) == sizeof(wchar_t))
	{
		int l_len = strlen(src);
		l_nstr = (_TCHAR*)malloc(sizeof(wchar_t)*(l_len+1));
		mbstowcs((wchar_t*)l_nstr, src ,_TRUNCATE); 
		//int l_idx=0;
		//int l_len = strlen(src);
		//l_nstr = (_TCHAR*)malloc(sizeof(wchar_t)*(l_len+1));
		//if (l_nstr) {
		//	do {
		//		l_nstr[l_idx] = (TCHAR)src[l_idx];
		//		l_idx++;
		//	} while ((char)src[l_idx]!=0);
		//}
		//l_nstr[l_idx] = 0;
	}
	else //must be 8 bit
	{
		l_nstr = (_TCHAR*)malloc(strlen(src)+1);

		if(l_nstr)
			strcpy_s((char*)l_nstr, strlen(src)+1, src);

	}
	return l_nstr;
}

//-------------------------------------------------------------------------------------------------
// _tmain
//-------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	_TCHAR* inFile;// = _T("./audio/allegra_sidner.wav");
	//_TCHAR* testFileMono = _T("./audio/natures_miracle.wav");
	//_TCHAR* testFileMono2 = _T("./audio/test1_16_1_44_PCM.wav");
	//_TCHAR* testFileMono3 = _T("./audio/test2_16_1_44_PCM.wav");	
	//_TCHAR* testFileStereo = _T("./audio/test3_16_2_44_PCM.wav");
	

	_TCHAR outFile[1024] = {0};// = _T("./audio/out0.wav");
	//_TCHAR* OUTTestFile1 = _T("./audio/out1.wav");
	//_TCHAR* OUTTestFile2 = _T("./audio/out2.wav");
	//_TCHAR* OUTTestFile3 = _T("./audio/out3.wav");
	//_TCHAR* OUTTestFile4 = _T("./audio/out4.wav");




	if(argc != 7)
	{
		(void)_tprintf_s(_T("Bad args!\n"));
		(void)_tprintf_s(_T("Usage: SilentStripper inFile outFile silenceThresh envRampSize_ms beginPad_ms endPad_ms\n"));
		(void)_tprintf_s(_T("inFile can be wildcard with WAV ext assumed (<path>*), in which case outFile is treated as an output path where outFiles same name as the inFiles are written to\n"));
		return -1;
	}

	//the code below indexes the last char of both, and outFile is a fixed size buffer
	if(_tcslen(argv[1]) == 0 || _tcslen(argv[2]) == 0 || _tcslen(argv[2]) >= 1024)
	{
		(void)_tprintf_s(_T("Bad args! inFile and outFile must not be empty, and outFile must be under 1024 chars.\n"));
		return -1;
	}

	inFile = argv[1];
	_tcscpy_s( outFile, argv[2]);





	float sThresh = (float)_tstof(argv[3]);
	UINT32 env_ms = _tstoi(argv[4]);
	UINT32 beginPad_ms = _tstoi(argv[5]);
	UINT32 endPad_ms = _tstoi(argv[6]);




	char * ch_inFile = NULL;



	ch_inFile = wstrdup(inFile);

	if(ch_inFile == NULL)
	{
		(void)_tprintf_s(_T("Failed to malloc() ch_inFile.\n"));
		return -1;
	}



	BOOL wildcard = false;


	printf("last char: %c\n", ch_inFile[strlen(ch_inFile)-1]);

	//a lot of assumptions made in this code. not very robust
	if(ch_inFile[strlen(ch_inFile)-1] == '*')
	{
		wildcard = true;
		ch_inFile[strlen(ch_inFile)-1] = '\0';

		inFile[_tcslen(inFile)-1] = '\0';

		//hackish way to make sure outFile is a directory
		if( outFile[_tcslen(outFile)-1] != '/' &&
			outFile[_tcslen(outFile)-1] != '\\' )
		{
			outFile[_tcslen(outFile)-1] = '/';
		}
	}

	
	_TCHAR incrInFile[1024] = {0};
	_TCHAR incrOutFile[1024] = {0};


	DIR *dir;
    errno = 0;


	printf("parsing dir: %s<END>\n", ch_inFile);

    if((dir = opendir(ch_inFile)) != 0)
    {
        struct dirent *entry;

        while((entry = readdir(dir)) != 0)
        {

			//puts(&(entry->d_name[strlen(entry->d_name)-4]));
			

			if( strlen(entry->d_name) > 4 &&
				((0 == strncmp(".wav", &(entry->d_name[strlen(entry->d_name)-4]), 4)) ||
				(0 == strncmp(".WAV", &(entry->d_name[strlen(entry->d_name)-4]), 4))))
			{

				puts(entry->d_name);


				_TCHAR * tc_d_name = strdup(entry->d_name);

				if(tc_d_name != NULL)
				{
					if(_tcslen(inFile) + _tcslen(tc_d_name) >= 1024 ||
						_tcslen(outFile) + _tcslen(tc_d_name) >= 1024)
					{
						(void)_tprintf_s(_T("Path too long, skipping: %s\n"), tc_d_name);
					}
					else
					{
						_tcscpy_s(incrInFile, 1024, inFile);
						_tcscat_s(incrInFile, 1024, tc_d_name);

						_tcscpy_s(incrOutFile, 1024, outFile);
						_tcscat_s(incrOutFile, 1024, tc_d_name);

						(void)_tprintf_s(_T("attempting: %s to %s "), incrInFile, incrOutFile);

						silentStripper(incrInFile, incrOutFile, sThresh, env_ms, beginPad_ms, endPad_ms);
					}
					free(tc_d_name);
				}
			}


        }

        errno = 0;
        closedir(dir);
    }

    if(errno != 0)
    {
		(void)_tprintf_s(_T("Could not parse dir: "));
		printf("%s\n", ch_inFile);
		free(ch_inFile);
		return -1;
    }




	free(ch_inFile);




	(void)_tprintf_s(_T("Press enter to exit.\n"));

	(void)getc(stdin);
	return 1;







	//strchr(
	//void testWavInOut(_TCHAR* inFile, _TCHAR *outFile, float silenceThresh, UINT32 envRampSize_ms, UINT32 beginPad_ms, UINT32 endPad_ms)
	silentStripper(inFile, outFile, sThresh, env_ms, beginPad_ms, endPad_ms);


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

