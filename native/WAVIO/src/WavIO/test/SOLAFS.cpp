
//-------------------------------------------------------------------------------------------------
///
/// @file SOLAFS.c
///
/// @brief 
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


typedef float SOLAFS_SAMPLE_TYPE;

//
//function Y = solafs(X, F, W, Wov,  Kmax, Wsim, xdecim, kdecim)
//% Y = solafs(X, F, W, Wov, Kmax, Wsim, xdec, kdec)   Do SOLAFS timescale mod'n
//%   Y is X scaled to run F [times] faster.  X is added-in in windows
//%   W pts long, overlapping by Wov points with the previous output.  
//%   The similarity is calculated over the last Wsim points of output.
//%   Maximum similarity skew is Kmax pts.
//%   Each xcorr calculation is decimated by xdecim (8)
//%   The skew axis sampling is decimated by kdecim (2)
//%   Defaults (for 22k) W = 200, Wov = W/2, Kmax = 2*W, Wsim=Wov.
//%   Based on "The SOLAFS time-scale modification algorithm", 
//%   Don Hejna & Bruce Musicus, BBN, July 1991.
//% 1997may16 dpwe@icsi.berkeley.edu $Header: /homes/dpwe/matlab/dpwebox/RCS/solafs.m,v 1.3 2006/04/09 20:10:20 dpwe Exp $
//% 2006-04-08: fix to predicted step size, thanks to Andreas Tsiartas

//
//
//void zeroSOLAFSSampleBuf(SOLAFS_SAMPLE_TYPE * buf, const UINT32 size)
//{
//	for(UINT32 i=0 ; i<size ; i++)
//		buf[i] = (SOLAFS_SAMPLE_TYPE)0.0f;
//}
//
//
//
//bool SOLAFSRead(WAV_FILE f, SOLAFS_SAMPLE_TYPE * outBuf, UINT32 numSamples)
//{
//
//
//}
//
//
//void SOLAFS(
//			WAV_FILE	OutWavFile,//SOLAFS_SAMPLE_TYPE *	Y, //the output buffer
//			WAV_FILE	InWavFile,//SOLAFS_SAMPLE_TYPE *	X, //the input buffer
//			double		XtoYRatio, //How much scaling up or down? (X=Ratio*Y)
//			UINT32		WindowSize,  //window size in units of samples 
//			UINT32		WindowOverlap, //what portion of Window overlaps with previous output?
//			UINT32		MaxSkew, //max skew allowed for optimization of similarity match 
//			UINT32		CrossCorrDecimationFactor, //number of samples to skip when doing CrossCorr
//			UINT32		SkewDecimationFactor //number of samples to skip when iterating through possible skew values
//										//during the CrossCorr
//			)
//{
//
//	SOLAFS_SAMPLE_TYPE *	outBuf = NULL;
//	SOLAFS_SAMPLE_TYPE *	inBuf = NULL;
//
//	UINT32 SynthesisDistance = 0; 
//	UINT32 AnalysisDistance = 0; //TODO: need method for tracking fractions
//
//	const UINT32 INPUT_BUF_SIZE = WindowSize + MaxSkew;
//
//	//we must determine if our tentative analysis start pos (m*AnalysisDistance) overlaps with the
//	//previously read inBuf.  If so, we need to shift relevant samples to the beginning of inBuf
//	//and only read the missing samples to get us up to a full INPUT_BUF_SIZE worth of samples
//	inBuf = (SOLAFS_SAMPLE_TYPE*)malloc((size_t) (INPUT_BUF_SIZE  * sizeof(SOLAFS_SAMPLE_TYPE) ));
//
//	if(inBuf == NULL)
//	{
//		(void)_tprintf_s(_T("Failed to malloc() inBuf.\n"));
//		return;
//	}
//
//	zeroSOLAFSSampleBuf(inBuf, INPUT_BUF_SIZE);
//
//
//	SynthesisDistance = WindowSize - WindowOverlap;
//
//
//	//We only ever write SysthesisDistance out to file.  Then, copy the last WindowOverlap samples
//	//to the beginning of the buffer for next iteration to allow for cross-fading (if necessary)
//	outBuf = (SOLAFS_SAMPLE_TYPE*)malloc((size_t) (WindowSize * sizeof(SOLAFS_SAMPLE_TYPE) ));
//
//	if(outBuf == NULL)
//	{
//		(void)_tprintf_s(_T("Failed to malloc() outBuf.\n"));
//		return;
//	}
//
//	zeroSOLAFSSampleBuf(outBuf, WindowSize);
//
//	enum WAV_ERR werr = WAV_NO_ERR;
//
//	UINT32 samplesReadTotal = 0;
//	UINT32 targetInputBufSize = 0;
//
//	while(samplesReadTotal < InWavFile->wavInfo->numSamples) 
//	{
//
//		UINT32 samplesRemaining = InWavFile->wavInfo->numSamples - samplesReadTotal;
//
//		if(samplesRemaining < INPUT_BUF_SIZE)
//			targetInputBufSize = samplesRemaining;
//		else
//			targetInputBufSize = INPUT_BUF_SIZE;
//
//		UINT32 samplesRead = 0;
//		UINT32 tmpSamplesRead = 0;
//
//
//		while(samplesRead < targetInputBufSize)
//		{
//
//			werr = wavRead(wf, (SOLAFS_SAMPLE_TYPE*)&(fbuf1[samplesRead]), targetInputBufSize, 1, &tmpSamplesRead);
//			
//			if(werr != WAV_NO_ERR) {
//				(void)_tprintf_s(_T("Unexpected error doing wavRead()!\n"));
//				return;
//			} //if
//
//			SamplesRead += tmpSamplesRead;
//
//		}
//		samplesReadTotal += samplesRead;
//
//		if(samplesRead < INPUT_BUF_SIZE)
//			zeroSOLAFSSampleBuf(&(inBuf[samplesRead]), INPUT_BUF_SIZE - samplesRead);
//
//	} //while
//
//} //SOLAFS()
//
//
//




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
	

	(void)_tprintf_s(_T("Press enter to exit.\n"));

	(void)getc(stdin);

	return 0;

} //_tmain

