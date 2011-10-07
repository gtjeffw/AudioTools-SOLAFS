#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <memory.h>
#include <math.h>

#include "Common/CommonTypes.hpp"

#include "WavIO/WavIO.hpp"

#include "CircFBuf/CircFBuf.hpp"

#include "SOLAFS/SOLAFS.hpp"



void simpleTest(_TCHAR* _strInFile, _TCHAR* _strOutFile, double _inOutRatio)
{
	WAV_FILE	OutWavFile = NULL;
	WAV_FILE	InWavFile = NULL;
	double		InToOutRatio = _inOutRatio;
	UINT32		WindowSize = 300; //200
	UINT32		WindowOverlap = WindowSize / 2;
	UINT32		MaxSkew = 2 * WindowSize;
	UINT32		CrossCorrDecimationFactor = 8; //8
	UINT32		SkewDecimationFactor = 2; //2

	_TCHAR*		strInFile = _strInFile;
	_TCHAR*		strOutFile = _strOutFile;

	enum WAV_ERR werr;

	werr = wavOpenForRead(strInFile, &InWavFile);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), strInFile, werr);
		goto SOLAFS_SIMPLETEST_CLEANUP;

	} //if

	werr = wavOpenForWrite(strOutFile, &OutWavFile, WAV_FORMAT_PCM, 1, InWavFile->wavInfo->samplesPerSec, 16);


	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to load %s with err val: %d\n"), strOutFile, werr);
		goto SOLAFS_SIMPLETEST_CLEANUP;

	} //if


	if(!SOLAFS(OutWavFile,InWavFile,InToOutRatio, WindowSize,WindowOverlap, MaxSkew, 
				CrossCorrDecimationFactor, SkewDecimationFactor))

	{
		(void)_tprintf_s(_T("SOLAFS Failed!\n"));
		goto SOLAFS_SIMPLETEST_CLEANUP;
	}


	werr = wavWriteFinalize(OutWavFile);

	if(werr != WAV_NO_ERR) {

		(void)_tprintf_s(_T("Failed to finalize output wav file (%s) with err: %d\n"), strOutFile, werr);
		goto SOLAFS_SIMPLETEST_CLEANUP;

	} //if


SOLAFS_SIMPLETEST_CLEANUP:

	if(OutWavFile != NULL)
		closeWavFile(OutWavFile);
	
	if(InWavFile != NULL)
		closeWavFile(InWavFile);

}

void crazyTest(_TCHAR* _strInFile, _TCHAR* _strOutFile)
{
	_TCHAR* tmp = _T("./audio/tmp.wav");
	_TCHAR* tmp2 = _T("./audio/tmp2.wav");
	const double highVal = 2.0;
	const double lowVal = 1.0/highVal;
	//const double normalVal = 1.0;

	simpleTest(_strInFile, tmp, highVal);

	for(UINT32 i=0 ; i<20 ; i++)
	{
		simpleTest(tmp, tmp2, lowVal);
		simpleTest(tmp2, tmp, highVal);
	}

	simpleTest(tmp, _strOutFile, lowVal);
}


int _tmain(int argc, _TCHAR* argv[])
{
	(void)argc;
	(void)argv;

	(void)_tprintf_s(_T("SOLAFS Test\n"));

	simpleTest(_T("./audio/natures_miracle.wav"),_T("./audio/out_test1_1x.wav"),1.0);
	simpleTest(_T("./audio/natures_miracle.wav"),_T("./audio/out_test1_2x.wav"),2.0);
	simpleTest(_T("./audio/natures_miracle.wav"),_T("./audio/out_test1_0.5x.wav"),0.5);

	double spearconRate = 5.0;
	simpleTest(_T("./audio/allegra_sidner.wav"),_T("./audio/spearcon_allegra.wav"),spearconRate);
	simpleTest(_T("./audio/anne_drum.wav"),_T("./audio/spearcon_anne.wav"),spearconRate);
	simpleTest(_T("./audio/natures_miracle.wav"),_T("./audio/spearcon_nature.wav"),spearconRate);
	simpleTest(_T("./audio/jeff_wilson.wav"),_T("./audio/spearcon_jeffw.wav"),spearconRate);
	simpleTest(_T("./audio/bruce_walker.wav"),_T("./audio/spearcon_bruce.wav"),spearconRate);

	(void)_tprintf_s(_T("Press enter to exit.\n"));

	(void)getc(stdin);

	return 0;
}