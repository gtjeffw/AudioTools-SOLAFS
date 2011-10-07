#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <memory.h>
#include <math.h>


#include "Common\CommonTypes.hpp"
#include "WavIO\WavIO.hpp"
#include "CircFBuf\CircFBuf.hpp"


#include "SOLAFS.hpp"


double gInToOutRatio = 1.0;
UINT32 gWindowSize = 400;
UINT32 gWindowOverlap = 200;
UINT32 gMaxSkew = 200;
UINT32 gCrossCorrDecimationFactor = 8;
UINT32 gSkewDecimationFactor = 2;

UINT32 gSynthesisDistance = gWindowSize - gWindowOverlap; 
UINT32 gAnalysisDistance = gSynthesisDistance; //need method for tracking fractions
double gAnalysisFraction = 0.0;
INT32 gProcessDiff = 0;

UINT32 gOutPos = 0;
UINT32 gInPos = 0;
UINT32 gNoSkewInPos = 0;
UINT32 gPrevInPos = 0;

UINT32 gSkew = 0;
UINT32 gSkewPred = 0; //predicted skew


float * gCrossFade = NULL;







UINT32 SOLAFS_FindSkew(CIRCULAR_FLOAT_BUFFER X, UINT32 X_VirtPos,
					 CIRCULAR_FLOAT_BUFFER Y, UINT32 Y_VirtPos,
					 UINT32 CompareSize, UINT32 MaxSkew,
					 UINT32 CrossCorrDecimationFactor, UINT32 SkewDecimationFactor)
{

	UINT32 vpsX = CFG_virtualPosStart(X);
	UINT32 vpsY = CFG_virtualPosStart(Y);
	if(
		X == NULL || Y == NULL || CrossCorrDecimationFactor == 0 || SkewDecimationFactor == 0 ||
		X_VirtPos < vpsX ||
		(X_VirtPos + CompareSize) > X->virtSize ||
		Y_VirtPos < vpsY ||
		(Y_VirtPos + CompareSize) > Y->virtSize 
		)
	{
		return false;
	}

	UINT32 bestScoringSkew = 0;
	double best_R_xy = 0.0;

	double rxy = 0.0;
	double norm_rxy = 0.0;
	double rxx = 0.0;
	//double ryy = 0.0;
	double R_xy = 0.0;
	double xy;

	UINT32 k;
	UINT32 n;
	UINT32 xInd;
	UINT32 yInd;

	float xSample;
	float ySample;



	for(k=0 ; k<MaxSkew ; k+=SkewDecimationFactor)
	{
		rxy = 0.0;
		norm_rxy = 0.0;
		rxx = 0.0;
		//ryy = 0.0;


		for(n=0, xInd=(X_VirtPos+k), yInd=Y_VirtPos ; 
			n<CompareSize ; 
			n+=CrossCorrDecimationFactor, xInd+=CrossCorrDecimationFactor, yInd+=CrossCorrDecimationFactor)
		{
			xSample = X->buf[xInd & X->mask];
			ySample = Y->buf[yInd & Y->mask];

			xy = (double)xSample * (double)ySample;
			rxy += xy;
			norm_rxy += xy*xy;
			rxx += (double)xSample * (double)xSample;
			//ryy += (double)ySample * (double)ySample;

		} //for

		norm_rxy = sqrt(norm_rxy);

		if(rxx == 0.0 )
			//|| ryy == 0.0)
			R_xy = 0.0;
		else
			R_xy = (rxy * norm_rxy) / rxx; //(rxx * ryy);

		if(R_xy > best_R_xy)
		{
			best_R_xy = R_xy;
			bestScoringSkew = k;
		}

	} //for

	return bestScoringSkew;
}


bool SOLAFS_PerformCrossFade(CIRCULAR_FLOAT_BUFFER OutBuf, UINT32 OutBuf_VirtPos,
							   CIRCULAR_FLOAT_BUFFER InBuf, UINT32 InBuf_VirtPos,
							   float * XFade, UINT32 XFadeSize)
//XFade must contain XFadeSize samples in the range [0.0f, 1.0f] (generally assumed lowest to highest)
//OutBuf[OutBuf_VirtPos+i] = OutBuf[OutBuf_VirtPos+i]*XFade[XFadeSize-1-i] + 
//								InBuf[InBuf_VirtPos+i]*XFade[i]
//where i=[0,XFadeSize-1]
{
	UINT32 vpsOut = CFG_virtualPosStart(OutBuf);
	UINT32 vpsIn = CFG_virtualPosStart(InBuf);
	if(
		OutBuf == NULL || InBuf == NULL || XFade == NULL ||
		OutBuf_VirtPos < vpsOut ||
		(OutBuf_VirtPos + XFadeSize) > OutBuf->virtSize ||
		InBuf_VirtPos < vpsIn ||
		(InBuf_VirtPos + XFadeSize) > InBuf->virtSize 
		)
	{
		return false;
	}

	UINT32 i;
	UINT32 j;
	UINT32 k;


	for(i=0, j=OutBuf_VirtPos, k=InBuf_VirtPos ; i<XFadeSize ; i++, j++, k++)
	{
		UINT32 jmask = j&OutBuf->mask;
		UINT32 kmask = k&InBuf->mask;
		float highXFade = XFade[XFadeSize - 1 - i];
		float lowXFade = XFade[i];

		float inVal = InBuf->buf[kmask];
		float outVal = OutBuf->buf[jmask];

		//float nextinVal = InBuf->buf[(j+1)&OutBuf->mask];

		OutBuf->buf[jmask] = outVal * highXFade + inVal  * lowXFade;
	}

	return true;
}

 bool SOLAFS_BeginStream(
			CIRCULAR_FLOAT_BUFFER	OutBuf, //the output buffer
			CIRCULAR_FLOAT_BUFFER	InBuf, //the input buffer
			double					InToOutRatio, //How much scaling up or down? (len(X)=Ratio*len(Y))
			UINT32					WindowSize,  //window size in units of samples 
			UINT32					WindowOverlap, //what portion of Window overlaps with previous output?
			UINT32					MaxSkew, //max skew allowed for optimization of similarity match 
			UINT32					CrossCorrDecimationFactor, //number of samples to skip when doing CrossCorr
			UINT32					SkewDecimationFactor, //number of samples to skip when iterating through possible skew values
										//during the CrossCorr
			UINT32 *				NextPredInPos, //out param for what the next smallest position
													//could be in InBuf
			UINT32 *				CurrOutPos
			)
{
	//InBuf and OutBuf must have particular minimum sizes
	//furthermore, InBuf must have a virtSize > WindowOverlap (esp for the priming step below)
	//		oh, and virt size 0 must be avail
	if(NextPredInPos == NULL || CurrOutPos == NULL || OutBuf == NULL || InBuf == NULL)
		return false;

	gSynthesisDistance = WindowSize - WindowOverlap;

	if(WindowSize == 0 || WindowOverlap == 0 || MaxSkew == 0 || InToOutRatio == 0.0 ||
		CrossCorrDecimationFactor == 0 || SkewDecimationFactor == 0 ||
		(2*WindowOverlap) > WindowSize || 
		gSynthesisDistance < WindowOverlap)
		return false;
	
	gInToOutRatio = InToOutRatio;
	gWindowSize = WindowSize;
	gWindowOverlap = WindowOverlap;
	gMaxSkew = MaxSkew;
	gCrossCorrDecimationFactor = CrossCorrDecimationFactor;
	gSkewDecimationFactor = SkewDecimationFactor;
									
	double analysis = gAnalysisFraction + (double)(gInToOutRatio * gSynthesisDistance);
	double analWholeNum = floor(analysis);
	gAnalysisFraction = analysis - analWholeNum;
	gAnalysisDistance = (UINT32)analWholeNum;
	gProcessDiff = (INT32)gSynthesisDistance - (INT32)gAnalysisDistance;

	//gAnalysisDistance = (UINT32)floor((double)(gInToOutRatio * gSynthesisDistance));

	//Deal with return here
	if(!CFB_write(OutBuf, InBuf, (UINT32)0, WindowSize))//WindowOverlap);
	{
		return false;
	}

	gOutPos = gSynthesisDistance;//was WindowOverlap;
	gNoSkewInPos = gAnalysisDistance;
	gInPos = gNoSkewInPos;

	gPrevInPos = 0;
	*NextPredInPos = gInPos;
	*CurrOutPos = 0; //position where the write occured

	gSkew = 0;
	gSkewPred = 0; //predicted skew


	if(gCrossFade != NULL)
	{
		free(gCrossFade);
		gCrossFade = NULL;
	}

	gCrossFade = (float*)malloc((size_t)gWindowOverlap * sizeof(float));

	if(gCrossFade == NULL)
		return false;

	//linear cross-fade
	for(UINT32 i=0 ; i<gWindowOverlap ; i++)
		gCrossFade[i] = (i) / (float)gWindowOverlap;


	return true;
}


bool SOLAFS_UpdateStream(
			CIRCULAR_FLOAT_BUFFER	OutBuf, //the output buffer
			CIRCULAR_FLOAT_BUFFER	InBuf, //the input buffer
			double					InToOutRatio, //How much scaling up or down? (len(X)=Ratio*len(Y))
									//NOTE: new ratios only affect the NEXT call to SOLAFS_UpdateStream()
			UINT32 *				NextPredInPos, //out param for what the next smallest position
													//could be in InBuf
			UINT32 *				CurrOutPos
			)
{
	//NOTE: an attempt at testing for (Sa < Kmax) as detailed in Hejna's and Musicus's "The
	//SOLAFS Time-Scale Modification Algorithm" section "5 Parameter Choices" did not work.
	//Tests resulted in a degenerate state where the skew became "pegged" against the MaxSkew
	//boundary.  Therefore, I believe that allowing for the possibility of going backwards is
	//ok.

	if(InToOutRatio == 0 || OutBuf == NULL || InBuf == NULL || NextPredInPos == NULL ||
		CurrOutPos == NULL)
		return false;

	gInToOutRatio = InToOutRatio;


	if((UINT32)abs(gProcessDiff) < gMaxSkew)
	{
		
		gSkewPred = gSkew + gProcessDiff;

		//seems like it might be a good thing to make sure gSkewPred is a multiple
		//of gSkewDecimationFactor
		gSkewPred = (gSkewPred / gSkewDecimationFactor)*gSkewDecimationFactor;

		//for testing! (forces predication failure)
		//gSkewPred = gMaxSkew + 100;
	}


	if((UINT32)abs(gProcessDiff) < gMaxSkew &&  //is successful prediction ever possible?
		gSkewPred < gMaxSkew &&  //check for a valid skew predication range
		gSkewPred >=0)
	{
		gSkew = gSkewPred;
		gInPos += gSkew;
	}
	else
	{

		gSkew = SOLAFS_FindSkew(InBuf, gInPos, 
								OutBuf, gOutPos, 
								gWindowOverlap, gMaxSkew,
								gCrossCorrDecimationFactor, gSkewDecimationFactor);

		//for testing! force no skew
		//gSkew = 0;

		gInPos += gSkew;

		
		if(!SOLAFS_PerformCrossFade( OutBuf, gOutPos,
									InBuf, gInPos,
									gCrossFade, gWindowOverlap))
			return false;

	}


	
	if(!CFB_write(OutBuf, InBuf, gInPos + gWindowOverlap, gSynthesisDistance)) //size is: WindowSize - WindowOverlap
		return false;

	*CurrOutPos = gOutPos; //position where the write occured

	//updates necessary for the next call to SOLAFS_UpdateStream()

	double analysis = gAnalysisFraction + (double)(gInToOutRatio * gSynthesisDistance);
	double analWholeNum = floor(analysis);
	gAnalysisFraction = analysis - analWholeNum;
	gAnalysisDistance = (UINT32)analWholeNum;

	//gAnalysisDistance = (UINT32)floor((double)(gInToOutRatio * gSynthesisDistance));
	
	gProcessDiff = (INT32)gSynthesisDistance - (INT32)gAnalysisDistance;

	gOutPos += gSynthesisDistance;
	gPrevInPos = gInPos;

	gNoSkewInPos += gAnalysisDistance;
	gInPos = gNoSkewInPos;

	
	//(void)_tprintf_s(_T("gInPos: %lu gPrevInPos: %lu gAnalysisDistance: %lu ALT: %lu\n"), gInPos, gPrevInPos, gAnalysisDistance, (UINT32)floor(InToOutRatio*(double)gOutPos));


	*NextPredInPos = gInPos;
	
	return true;
}


bool SOLAFS_readFile(WAV_FILE InWavFile,
					 CIRCULAR_FLOAT_BUFFER OutBuf,
					 UINT32 TargetVirtStartPos,
					 UINT32 Size,
					 UINT32* trueSize) //returns the true virtual size of OutBuf (less the padding)
{
	bool ret = true;

	if(InWavFile == NULL || OutBuf == NULL || OutBuf->size == 0 || trueSize == NULL)
		return false;

	if(OutBuf->size < Size)
		return false; //we can never hold the range [TargetVirtStartPos, TargetVirtStartPos+Size], so fail

	UINT32 buf_virtStartPos = CFG_virtualPosStart(OutBuf);

	if(buf_virtStartPos > TargetVirtStartPos)
		return false;  //we've already overshot somehow!!!


	//now read until buf_virtStartPos == TargetVirtStartPos

	UINT32 samplesToRead = 0;

	//deal with possibility of partially full buffer
	//(we will go ahead and try to fill to cap, even if we don't need to)
	if(OutBuf->virtSize < OutBuf->size)
		samplesToRead += OutBuf->size - OutBuf->virtSize;

	//how far to read to move the OutBuf's start pos to where we want
	samplesToRead += (TargetVirtStartPos - buf_virtStartPos);

	if(samplesToRead == 0)
	{
		*trueSize = OutBuf->virtSize;
		return true; //already where we need to be
	}
	
	//lets figure out how many samples will be overwritten (if any) and instead seek for efficiency

	if(samplesToRead > OutBuf->size)
	{
		UINT32 seekNumSamples = (samplesToRead - OutBuf->size);


		//seek by seekNumSamples
		UINT32 samplesSeeked = 0;
		enum WAV_ERR wavErr;

		wavErr = wavSeekRead(InWavFile, seekNumSamples, &samplesSeeked);

		if( (wavErr == WAV_NO_ERR && samplesSeeked != seekNumSamples) ||
			(wavErr != WAV_NO_ERR) )
		{	
			//we've hit the end of the file, but we still need to update the buffer
			//size and pad zero's
			ret = false;
		}


		samplesToRead -= seekNumSamples;
		OutBuf->virtSize += seekNumSamples;  //pretend we actually copied to buffer


	}

	enum WAV_ERR wavErr;


	//BEGIN NEW READ

	UINT32 numRead1 = 0;
	UINT32 numRead2 = 0;
	//UINT32 virtPosStart = CFG_virtualPosStart(OutBuf);
	UINT32 bufPos = OutBuf->virtSize & OutBuf->mask;
	UINT32 partAReadSize = OutBuf->size - bufPos;
	UINT32 samplesFaked = 0;

	partAReadSize = (partAReadSize > samplesToRead) ? samplesToRead : partAReadSize;

	wavErr = wavRead(InWavFile, &(OutBuf->buf[bufPos]), partAReadSize, 1, &numRead1);

	if(wavErr != WAV_NO_ERR)
	{	
		ret = false;
		numRead1 = 0;
	}
	
	if(numRead1 < partAReadSize)
	{
		samplesFaked = partAReadSize - numRead1;

		//pad 0's
		for(UINT32 i = numRead1 ; i<partAReadSize ; i++)
		{
			OutBuf->buf[bufPos + i] = 0.0f;
		} //for
	}


	if(numRead1 < samplesToRead)
	{
		UINT32 partBReadSize = samplesToRead - numRead1;

		wavErr = wavRead(InWavFile, OutBuf->buf, partBReadSize, 1, &numRead2);

		if(wavErr != WAV_NO_ERR)
		{	
			if(numRead1 <= 0) //only fail if we haven't been able to read anything (including part A)
				ret = false;
			numRead2 = 0;
		}

		if(numRead2 < partBReadSize)
		{
			samplesFaked += partBReadSize - numRead2;

			//pad 0's
			for(UINT32 i = numRead2 ; i<partBReadSize ; i++)
			{
				OutBuf->buf[i] = 0.0f;
			} //for
		}

	}

	OutBuf->virtSize += samplesToRead;

	*trueSize = OutBuf->virtSize - samplesFaked;


	//END NEW READ


	return ret;
}



bool SOLAFS_writeFile(WAV_FILE OutWavFile,
					 CIRCULAR_FLOAT_BUFFER Buf,
					 UINT32 TargetVirtStartPos,
					 UINT32 Size)
{

	if(OutWavFile == NULL || Buf == NULL || Buf->size == 0)
		return false;

	if(Buf->size < Size)
		return false; //we can never hold the range [TargetVirtStartPos, TargetVirtStartPos+Size], so fail

	UINT32 buf_virtStartPos = CFG_virtualPosStart(Buf);

	if(buf_virtStartPos > TargetVirtStartPos)
		return false;  //we've already overshot somehow!!!

	if(Buf->virtSize < (TargetVirtStartPos + Size))
		return false;



	//develop more efficient copy method

	//BEGIN NEW WRITE

	UINT32 bufPos = TargetVirtStartPos & Buf->mask; //Buf->virtSize & Buf->mask;
	UINT32 partAWriteSize = Buf->size - bufPos;
	UINT32 partBWriteSize = 0;
	partAWriteSize = (partAWriteSize > Size) ? Size : partAWriteSize;

	//NOTE: be very careful if adding support for multi-channel audio due to the way wavWrite() works!
	if(WAV_NO_ERR != waveWrite(OutWavFile, &(Buf->buf[bufPos]), partAWriteSize, 1, partAWriteSize))
		return false;

	if(Size > partAWriteSize)
	{
		partBWriteSize = Size - partAWriteSize;
		
		//NOTE: be very careful if adding support for multi-channel audio due to the way wavWrite() works!
		if(WAV_NO_ERR != waveWrite(OutWavFile, Buf->buf, partBWriteSize, 1, partBWriteSize))
			return false;
	}


	return true;

	//END NEW WRITE

}


bool SOLAFS(
			WAV_FILE	OutWavFile,//SOLAFS_SAMPLE_TYPE *	Y, //the output buffer
			WAV_FILE	InWavFile,//SOLAFS_SAMPLE_TYPE *	X, //the input buffer
			double		InToOutRatio, //How much scaling up or down? (len(X)=Ratio*len(Y))
			UINT32		WindowSize,  //window size in units of samples 
			UINT32		WindowOverlap, //what portion of Window overlaps with previous output?
			UINT32		MaxSkew, //max skew allowed for optimization of similarity match 
			UINT32		CrossCorrDecimationFactor, //number of samples to skip when doing CrossCorr
			UINT32		SkewDecimationFactor //number of samples to skip when iterating through possible skew values
										//during the CrossCorr
			)
{
	return SOLAFS(OutWavFile,InWavFile,InToOutRatio,0.0,WindowSize,WindowOverlap,
					MaxSkew,CrossCorrDecimationFactor,SkewDecimationFactor);
}

bool SOLAFS(
			WAV_FILE	OutWavFile,//SOLAFS_SAMPLE_TYPE *	Y, //the output buffer
			WAV_FILE	InWavFile,//SOLAFS_SAMPLE_TYPE *	X, //the input buffer
			double		InToOutRatio, //How much scaling up or down? (len(X)=Ratio*len(Y))
			double		RatioScalingFactor,
			UINT32		WindowSize,  //window size in units of samples 
			UINT32		WindowOverlap, //what portion of Window overlaps with previous output?
			UINT32		MaxSkew, //max skew allowed for optimization of similarity match 
			UINT32		CrossCorrDecimationFactor, //number of samples to skip when doing CrossCorr
			UINT32		SkewDecimationFactor //number of samples to skip when iterating through possible skew values
										//during the CrossCorr
			)
{

	bool ret = false;

	UINT32 SynthesisDistance = WindowSize - WindowOverlap;; 

	const UINT32 INPUT_BUF_SIZE = WindowSize + MaxSkew;
	const UINT32 OUTPUT_BUF_SIZE = WindowSize;

	CIRCULAR_FLOAT_BUFFER InBuf = NULL;
	CIRCULAR_FLOAT_BUFFER OutBuf = NULL;

	UINT32 trueSize = 0;

	if(OutWavFile == NULL || InWavFile == NULL)
		return false;


	InBuf = CFB_create(INPUT_BUF_SIZE);

	if(InBuf == NULL)
	{
		ret = false;
		goto SOLAFS_CLEANUP;
	}

	OutBuf = CFB_create(OUTPUT_BUF_SIZE);

	if(OutBuf == NULL)
	{
		ret = false;
		goto SOLAFS_CLEANUP;
	}


	//prime the loop for incremental SOLAFS updates
	if(!SOLAFS_readFile(InWavFile, InBuf, 0, INPUT_BUF_SIZE, &trueSize) ||
		trueSize < InBuf->virtSize //this is an aggressive test, but indicates this file is so small, why bother?
		)
	{
		ret = false;
		goto SOLAFS_CLEANUP;
	}

	UINT32 nextPredInPos = 0;
	UINT32 currOutPos = 0;

	if(!SOLAFS_BeginStream(OutBuf, InBuf, InToOutRatio, WindowSize, WindowOverlap, 
							MaxSkew, CrossCorrDecimationFactor, SkewDecimationFactor, 
							&nextPredInPos, &currOutPos))
	{
		ret = false;
		goto SOLAFS_CLEANUP;
	}

	if(!SOLAFS_writeFile(OutWavFile, OutBuf, currOutPos, gSynthesisDistance)) //WindowSize
	{
		ret = false;
		goto SOLAFS_CLEANUP;
	}

	bool test = true;
	bool keepGoing = true;
	bool finalSizeFound = false;

	UINT32 finalSize = 0;

	UINT32 remSamples = 0;

	//main loop.  exhaust all of the InWavFile, do incremental SOLAFS, then write out
	//readFile should continue until Inbuf would have no data from the
	//nextPredInPos on.  Note that readFile() also advances the virtual size of inBuf.
	//this is necessary in order to fully exhaust all useful audio
	while(keepGoing)
	{

		//if readFile() fails, we assume InBuf
		//is unmodified (therefore trueSize should be corrected from undefined return val)
		//Note that the returned trueSize will only be accurate the FIRST time it is less than
		//InBuf's virtual size
		test = SOLAFS_readFile(InWavFile, InBuf, nextPredInPos, INPUT_BUF_SIZE, &trueSize);

		if(!test)
		{
			if(!finalSizeFound)
			{
				finalSize = InBuf->virtSize;  //this is not tested much!! (I think if it's wrong, worst case
													//is that some extra silence added to end of output
				finalSizeFound = true;
			}
		}
		else
		{
			if(!finalSizeFound)
			{	
				finalSize = trueSize;

				if(trueSize < InBuf->virtSize) //we have found the end of the data
				{
					finalSizeFound = true;
				} //if
			} //if
		} //else

 
		if(finalSize > nextPredInPos)
			remSamples = finalSize - nextPredInPos;
		else
			remSamples = 0;


		if(!SOLAFS_UpdateStream(OutBuf, InBuf, InToOutRatio, &nextPredInPos, &currOutPos))
		{
			ret = false;
			goto SOLAFS_CLEANUP;
		}

		InToOutRatio += RatioScalingFactor;
		

		UINT32 writeAmnt = remSamples < SynthesisDistance ? remSamples : SynthesisDistance;

		if(writeAmnt != 0)
		{
			if(writeAmnt < WindowOverlap)
			{
				keepGoing = false;
			}

			if(writeAmnt > 0)
			{
				if(!SOLAFS_writeFile(OutWavFile, OutBuf, currOutPos, writeAmnt))
				{
					ret = false;
					goto SOLAFS_CLEANUP;
				}
			}
		}
		else
			keepGoing = false;
		
	} //while	

	ret = true;

SOLAFS_CLEANUP:

	if(InBuf != NULL)
		(void)CFB_delete(InBuf);

	if(OutBuf != NULL)
		(void)CFB_delete(OutBuf);

	return ret;

} //SOLAFS()


