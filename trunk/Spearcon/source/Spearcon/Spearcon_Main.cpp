#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <memory.h>
#include <math.h>

#include "Common/CommonTypes.hpp"

#include "WavIO/WavIO.hpp"

#include "CircFBuf/CircFBuf.hpp"

#include "SOLAFS/SOLAFS.hpp"

const _TCHAR * VERSION_NUM = _T("0.81");

const UINT32 TS_LOG = 0; //default (must be overridden by setting args for other types)
const UINT32 TS_RATIO = 1;
const UINT32 TS_LEN_SEC = 2;

const UINT32 CS_NONE = 0; //default for no compression stepping
const UINT32 CS_LINEAR = 1; //linear compression stepping

void genSpearcon(_TCHAR* _strInFile, 
				_TCHAR* _strOutFile, 
				UINT32 _timeScaleMode,
				UINT32 _compressionStepMode,
				double _lengthSec,
				double _inOutRatio,
				double _logA,
				double _logB,
				double _stepStartRatio,
				UINT32 _WindowSize,
				UINT32 _WindowOverlap,
				UINT32 _MaxSkew
				)
{
	WAV_FILE	OutWavFile = NULL;
	WAV_FILE	InWavFile = NULL;
	double		InToOutRatio = 1.0;//_inOutRatio;
	double		RatioStep = 0.0;
	UINT32		WindowSize = _WindowSize;//300; //200
	UINT32		WindowOverlap = _WindowOverlap;//WindowSize / 2;
	UINT32		MaxSkew = _MaxSkew;//2 * WindowSize;
	UINT32		CrossCorrDecimationFactor = 1; //8
	UINT32		SkewDecimationFactor = 1; //2

	_TCHAR*		strInFile = _strInFile;
	_TCHAR*		strOutFile = _strOutFile;

	enum WAV_ERR werr;

	werr = wavOpenForRead(strInFile, &InWavFile);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), strInFile, werr);
		goto GEN_SPEARCON_CLEANUP;

	} //if


	double fileLen = 0.0;
	double newLen = 0.0;
	switch(_timeScaleMode)
	{
		case TS_LOG:
			fileLen = (InWavFile->wavInfo->numSamples / (double)InWavFile->wavInfo->samplesPerSec);
			newLen = _logA * log10(fileLen + 1.0) + _logB;
			InToOutRatio  = fileLen / newLen;
			break;
		case TS_RATIO:
			InToOutRatio = _inOutRatio;
			break;
		case TS_LEN_SEC:
			InToOutRatio = (InWavFile->wavInfo->numSamples / (double)InWavFile->wavInfo->samplesPerSec) / _lengthSec;
			break;
		default:
			(void)_tprintf_s(_T("_timeScaleMode is invalid\n"));
			goto GEN_SPEARCON_CLEANUP;
	}


	int SD, inFileNumSamples, outFileNumSamples, N;
	double startRatio, m;

	switch(_compressionStepMode)
	{
	case CS_NONE:

		break;
	case CS_LINEAR:
		SD = WindowSize - WindowOverlap; //synthesisDistance (see SOLAFS alg)
		inFileNumSamples = InWavFile->wavInfo->numSamples;  //samples from in file
		outFileNumSamples = (int)(inFileNumSamples/InToOutRatio); //samples that should be in the outFile
														//based on calculated InToOutRatio

		//number of synthesis iterations (with SOLAFS this is FIXED, hence the name)
		N = (int)(outFileNumSamples / (double)SD)-2; 
		startRatio = _stepStartRatio; //what compression ratio are we starting with?

		//the ratio step/slope is calculated by solving a summation of synthesis iterations
		//such that the sum is equal to the total number of samples of the output file length
		m = (inFileNumSamples/(double)SD - N*startRatio)*2/( N *(N+1) );

		InToOutRatio = startRatio;
		RatioStep = m;
		break;
	default:
		(void)_tprintf_s(_T("_compressionStepMode is invalid\n"));
		goto GEN_SPEARCON_CLEANUP;
	}

	werr = wavOpenForWrite(strOutFile, &OutWavFile, WAV_FORMAT_PCM, 1, InWavFile->wavInfo->samplesPerSec, 16);


	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), strOutFile, werr);
		goto GEN_SPEARCON_CLEANUP;

	} //if

	if(_compressionStepMode == CS_LINEAR)
	{
		if(!SOLAFS(OutWavFile,InWavFile,InToOutRatio, RatioStep, WindowSize,WindowOverlap, MaxSkew, 
			CrossCorrDecimationFactor, SkewDecimationFactor))

		{
			(void)_tprintf_s(_T("SOLAFS Failed!\n"));
			goto GEN_SPEARCON_CLEANUP;
		}
	}
	else
	{
		if(!SOLAFS(OutWavFile,InWavFile,InToOutRatio, WindowSize,WindowOverlap, MaxSkew, 
			CrossCorrDecimationFactor, SkewDecimationFactor))

		{
			(void)_tprintf_s(_T("SOLAFS Failed!\n"));
			goto GEN_SPEARCON_CLEANUP;
		}

	}


	werr = wavWriteFinalize(OutWavFile);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to finalize output wav file (%s) with err: %d\n"), strOutFile, werr);
		goto GEN_SPEARCON_CLEANUP;

	} //if


GEN_SPEARCON_CLEANUP:

	if(OutWavFile != NULL)
		closeWavFile(OutWavFile);
	
	if(InWavFile != NULL)
		closeWavFile(InWavFile);

}


bool processArgs(int argc, _TCHAR* argv[],
					_TCHAR ** _inFile,
					_TCHAR ** _outFile,
					UINT32* _timeScaleMode,
					UINT32* _compStepMode,
					double* _lengthSec,
					double* _inOutRatio,
					double* _logA,
					double* _logB,
					double* _compStepStartRatio,
					UINT32* _WindowSize,
					UINT32* _WindowOverlap,
					UINT32* _MaxSkew			 
				 )
{
	_TCHAR * inFile = NULL;
	_TCHAR * outFile = NULL;
	double lengthSec = 0.25;
	double inOutRatio = 1.0;
	double compStepStartRatio = 1.0;
	//A*log10(lengthSec+1.0) + B
	double logA = 1.0;
	double logB = 0.25;
	UINT32 timeScaleMode = TS_LOG;
	UINT32 compStepMode = CS_NONE;
	UINT32 WindowSize = 300;
	UINT32 WindowOverlap = WindowSize / 2;
	UINT32 MaxSkew = WindowSize * 2;

	bool timeModeSet = false;
	bool compModeSet = false;

	_TCHAR * tokContext = NULL;

	for(int i = 1 ; i<argc ; i++)
	{
		_TCHAR * param;
		_TCHAR * strVal;

		param = _tcstok_s(argv[i], _T(":"), &tokContext);
		//strVal = _tcstok_s(NULL, _T(":"), &tokContext);

		if(param == NULL)// || strVal == NULL)
			return false;

		//quick hack to deal with possibility of a ":" in file path for in/out files
		//strVal = &(argv[i][strVal - param]);
		strVal = param+_tcslen(param)+1; //_tcslen(argv[i])-param]);

		if(0 == _tcscmp(_T("INFILE"), param))
		{
			inFile = strVal;
		}
		else if(0 == _tcscmp(_T("OUTFILE"), param))
		{
			outFile = strVal;
		}
		else if(0 == _tcscmp(_T("SOLAFS_WINDOWSIZE"), param))
		{
			WindowSize = _tstoi(strVal);
		}
		else if(0 == _tcscmp(_T("SOLAFS_OVERLAP"), param))
		{
			WindowOverlap = _tstoi(strVal);
		}
		else if(0 == _tcscmp(_T("SOLAFS_MAXSKEW"), param))
		{
			MaxSkew = _tstoi(strVal);
		}
		else if(0 == _tcscmp(_T("TIMESCALE"), param))
		{
			UINT32 tsMode = TS_LOG;

			if(0 == _tcscmp(_T("LEN_SEC"), strVal))
				tsMode = TS_LEN_SEC;
			else if(0 == _tcscmp(_T("IN_TO_OUT_RATIO"), strVal))
				tsMode = TS_RATIO;
			else if(0 == _tcscmp(_T("LOG"), strVal))
				tsMode = TS_LOG;
			else
				return false;

			if(timeModeSet && timeScaleMode != tsMode)
				return false;

			timeScaleMode = tsMode;
			timeModeSet = true;
			
		}
		else if(0 == _tcscmp(_T("COMPSTEPMODE"), param))
		{
			UINT32 csMode = CS_NONE;

			if(0 == _tcscmp(_T("NONE"), strVal))
				csMode = CS_NONE;
			else if(0 == _tcscmp(_T("LINEAR"), strVal))
				csMode = CS_LINEAR;	
			else
				return false;

			compStepMode = csMode;
			compModeSet = true;
		}
		else if(0 == _tcscmp(_T("LENGTH_SEC"), param))
		{
			if(timeModeSet && timeScaleMode != TS_LEN_SEC)
				return false;

			lengthSec = _tstof(strVal);

			timeModeSet = true;
			timeScaleMode = TS_LEN_SEC;
		}
		else if(0 == _tcscmp(_T("IN_TO_OUT_RATIO"), param))
		{
			if(timeModeSet && timeScaleMode != TS_RATIO)
				return false;

			inOutRatio = _tstof(strVal);

			timeModeSet = true;
			timeScaleMode = TS_RATIO;
		}
		else if(0 == _tcscmp(_T("LOG_TIME_A"), param))
		{
			if(timeModeSet && timeScaleMode != TS_LOG)
				return false;

			logA = _tstof(strVal);

			timeModeSet = true;
			timeScaleMode = TS_LOG;
		}
		else if(0 == _tcscmp(_T("LOG_TIME_B"), param))
		{
			if(timeModeSet && timeScaleMode != TS_LOG)
				return false;

			logB = _tstof(strVal);

			timeModeSet = true;
			timeScaleMode = TS_LOG;
		}
		else if(0 == _tcscmp(_T("CS_START_RAT"), param))
		{
			if(compModeSet && compStepMode != CS_LINEAR)
				return false;

			compStepStartRatio = _tstof(strVal);

			compModeSet = true;
			compStepMode = CS_LINEAR;
		}
		else
		{
			_tprintf_s(_T("ERROR: Unrecognized parameter: %s\n"), param);
			return false;
		}
	} //for


	if(inFile == NULL || outFile == NULL)
		return false;

	*_inFile = inFile;
	*_outFile = outFile;
	*_timeScaleMode = timeScaleMode;
	*_compStepMode = compStepMode;
	*_lengthSec = lengthSec;
	*_inOutRatio = inOutRatio;
	*_compStepStartRatio = compStepStartRatio;
	*_logA = logA;
	*_logB = logB;
	*_WindowSize = WindowSize;
	*_WindowOverlap = WindowOverlap;
	*_MaxSkew = MaxSkew;		 

	return true;
}



void printUsage()
{
	(void)_tprintf_s(_T("VERSION: %s\n\n"), VERSION_NUM);
	(void)_tprintf_s(_T("Description:\n\n"));
	(void)_tprintf_s(_T("The spearcon program performs timescaling on 16-bit PCM WAV files\n"));
	(void)_tprintf_s(_T("with an emphasis on the generation of spearcons.\n"));
	(void)_tprintf_s(_T("WAV files must be 16-bit PCM and should be single channel.\n"));
	(void)_tprintf_s(_T("The program will attempt to only use the first channel if multi-channel\n"));
	(void)_tprintf_s(_T("is detected (outputting a mono 16-bit file with the same sampling rate)\n"));
	(void)_tprintf_s(_T("The time compression algorithm used is SOLAFS [Hejna&Musicus'91].\n"));

	(void)_tprintf_s(_T("\nAuthor: Jeff Wilson (jeff.wilson@imtc.gatech.edu)\n"));
	(void)_tprintf_s(_T("Copyright 2009 Georgia Institute of Technology\n\n"));
	(void)_tprintf_s(_T("More info on spearcons: http://www.sonify.psych.gatech.edu\n"));
	(void)_tprintf_s(_T("\nUSAGE:\nspearcon [[parameter1:value1] [parameter2:value2] ...] \n\n"));
	(void)_tprintf_s(_T("\nList of possible parameters:\n\n"));
	(void)_tprintf_s(_T("MANDATORY:\n"));
	(void)_tprintf_s(_T("INFILE - the input WAV file path (string).\n"));
	(void)_tprintf_s(_T("OUTFILE - the outupt WAV file path (string).\n"));
	(void)_tprintf_s(_T("\nOPTIONAL:\n"));
	(void)_tprintf_s(_T("TIMESCALE - specifies time scale method (string). Defaults to 'LOG'.\n"));
	(void)_tprintf_s(_T("   Timescaling allows compress-/stretching of the input WAV.  The following\n"));
	(void)_tprintf_s(_T("   are the available means of specifying time scaling.\n"));
	(void)_tprintf_s(_T("   LEN_SEC - scale based on a desired output length.  Assoc'ed with LENGTH_SEC.\n"));
	(void)_tprintf_s(_T("   IN_TO_OUT_RATIO - scale base on a ratio. Associated with IN_TO_OUT_RATIO\n"));
	(void)_tprintf_s(_T("   LOG - LOG10 based scaling.  Associated with LOG_TIME_A and LOG_TIME_B\n"));
	(void)_tprintf_s(_T("      Used to determine length in sec of scaled output.\n"));
	(void)_tprintf_s(_T("      Of the form: outputLen = LOG_TIME_A*log10(inputLen) + LOG_TIME_B\n"));
	(void)_tprintf_s(_T("COMPSTEPMODE - compression stepping mode (string). Defaults to 'NONE'\n"));
	(void)_tprintf_s(_T("   Compression stepping allows for spearcons that start slow but speed up\n"));
	(void)_tprintf_s(_T("   as they get closer to the end.  The comp. step rate is determ'ed internally\n"));
	(void)_tprintf_s(_T("   and does not affect the overall timescaling (output length).\n"));
	(void)_tprintf_s(_T("   NONE - no compression stepping.\n"));
	(void)_tprintf_s(_T("   LINEAR - linear compression stepping.  Associated with CS_START_RAT.\n"));
	(void)_tprintf_s(_T("LENGTH_SEC - length in sec for TIMESCALE:LEN_SEC (float).  Defaults to 0.25s.\n"));
	(void)_tprintf_s(_T("IN_TO_OUT_RATIO - scaling ratio for TIMESCALE:IN_TO_OUT_RATIO (float).\n"));
	(void)_tprintf_s(_T("   Defaults to 1.0.\n"));
	(void)_tprintf_s(_T("LOG_TIME_A - log equation coeff (float).  See TIMESCALE:LOG. Defaults to 1.0.\n"));
	(void)_tprintf_s(_T("LOG_TIME_B - log equation coeff (float).  See TIMESCALE:LOG. Defaults to 0.25.\n"));
	(void)_tprintf_s(_T("CS_START_RAT - the init. scaling ratio for COMPSTEPMODE:LINEAR (float).\n"));
	(void)_tprintf_s(_T("   Defaults to 1.0\n"));
	(void)_tprintf_s(_T("\nThe following params are specific to the SOLAFS algorithm.\n"));
	(void)_tprintf_s(_T("SOLAFS_WINDOWSIZE - size of window (pos integer). Defaults to 300.\n"));
	(void)_tprintf_s(_T("SOLAFS_OVERLAP - size of overlap (pos integer). Defaults to 150.\n"));
	(void)_tprintf_s(_T("SOLAFS_MAXSKEW - max allowed skew (pos integer). Defaults to 600.\n"));
	(void)_tprintf_s(_T("\nNOTES:\n"));
	(void)_tprintf_s(_T("If a TIMESCALE is not specified, the program will implicitly determine\n"));
	(void)_tprintf_s(_T("   the appropriate mode based on other assigned params.  However, if params\n"));
	(void)_tprintf_s(_T("   are conflicting, the program will error out.\n"));
	(void)_tprintf_s(_T("   Ex: Specifying 'LENGTH_SEC:0.3' will be detected as TIMESCALE:LEN_SEC,\n"));
	(void)_tprintf_s(_T("   but setting 'LENGTH_SEC:0.3' and 'LOG_TIME_A:2.0' will error out.\n"));
	(void)_tprintf_s(_T("\nEXAMPLES:\n"));
	(void)_tprintf_s(_T("Basic: (Create spearcon with default parameters)\n"));
	(void)_tprintf_s(_T("   spearcon INFILE:in.wav OUTFILE:out.wav\n"));
	(void)_tprintf_s(_T("Scale based on desired output length:\n"));
	(void)_tprintf_s(_T("   spearcon INFILE:in.wav OUTFILE:out.wav LENGTH_SEC:0.3\n"));
	(void)_tprintf_s(_T("Scale plus compression stepping:\n"));
	(void)_tprintf_s(_T("   spearcon INFILE:in.wav OUTFILE:out.wav LENGTH_SEC:0.3\n"));
	(void)_tprintf_s(_T("      COMPSTEPMODE:LINEAR CS_START_RAT:0.1\n"));
	(void)_tprintf_s(_T("\n\n"));


}



int _tmain(int argc, _TCHAR* argv[])
{
	_TCHAR * inFile;
	_TCHAR * outFile;
	UINT32 timeScaleMode = TS_LOG;
	UINT32 compStepMode = CS_NONE;
	double lengthSec = 0.25;
	double inOutRatio = 1.0;
	double logA = 1.0;
	double logB = 0.25;
	double compStepStartRatio = 1.0;
	UINT32 WindowSize = 300;
	UINT32 WindowOverlap = WindowSize / 2;
	UINT32 MaxSkew = WindowSize * 2;

	//some examples: 
	//INFILE:./audio/bruce_walker.wav OUTFILE:./audio/spearcon_bruce.wav TIMESCALE:LOG LOG_TIME_A:0.5 LOG_TIME_B:0.15
	//INFILE:./audio/bruce_walker.wav OUTFILE:./audio/spearcon_bruce.wav LENGTH_SEC:0.4
	//INFILE:./audio/natures_miracle_med.wav OUTFILE:./audio/spearcon_miracle_med.wav LENGTH_SEC:0.7


	if(!processArgs(argc,argv,
					&inFile,
					&outFile,
					&timeScaleMode,
					&compStepMode,
					&lengthSec,
					&inOutRatio,
					&logA,
					&logB,
					&compStepStartRatio,
					&WindowSize,
					&WindowOverlap,
					&MaxSkew))
	{
		(void)_tprintf_s(_T("Invalid args!\n\n"));
		printUsage();
		return 0;
	}
	else
	{
		(void)_tprintf_s(_T("-=Spearcon Generator=-\n"));
		(void)_tprintf_s(_T("VERSION: %s\n"), VERSION_NUM);
		(void)_tprintf_s(_T("\nAuthor: Jeff Wilson (jeff.wilson@imtc.gatech.edu)\n"));
		(void)_tprintf_s(_T("Copyright 2009 Georgia Institute of Technology\n"));	
		(void)_tprintf_s(_T("\nWorking....\n"));
	}


	genSpearcon(inFile, outFile, timeScaleMode, compStepMode, lengthSec, inOutRatio, logA, logB, compStepStartRatio, WindowSize, WindowOverlap, MaxSkew);
	

	(void)_tprintf_s(_T("Spearcon generated.\n"));


	return 0;
}