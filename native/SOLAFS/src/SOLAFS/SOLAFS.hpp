#ifndef _SOLAFS_HPP_
#define _SOLAFS_HPP_


//typedef struct TAG_PingPongBufferStructType
//{
//	//the permanent buffers
//	float * A;
//	float * B;
//	UINT32 size;
//
//	bool InIsA; //true if In buf points to A, false if points to B
//				//note that Out buf is generally pointing to diff regions
//				//of both A and B
//
//	//In and Out flip flop back and forth pointing to A or B
//	float * In;
//	//float * Out;
//
//	float * InSkewed; //the in buffer skewed from it's beginning pos to point of best cross-corr 
//
//	UINT32 In_virtSize; //sample count of all samples processed for Input
//	UINT32 In_virtStart; //virtual sample number of the first sample pointed to by In
//	UINT32 In_virtSkewPos; //virtual sample number of the skewed In buffer
//
//	float * Out_overlap; //start of overlap region
//	foat * Out_copy; //start of the copy region
//	float * Out_nextOverlap; //start of the next overlap region
//	UINT32 Out_overlapSize; //size of the overlap regions
//	UINT32 Out_copySize; //size of the copy region
//
//} PingPongBufferStructType, * PingPongBufferType;



#endif //#ifndef _SOLAFS_HPP_


UINT32 SOLAFS_FindSkew(CIRCULAR_FLOAT_BUFFER X, UINT32 X_VirtPos,
					 CIRCULAR_FLOAT_BUFFER Y, UINT32 Y_VirtPos,
					 UINT32 CompareSize, UINT32 MaxSkew,
					 UINT32 CrossCorrDecimationFactor, UINT32 SkewDecimationFactor);

bool SOLAFS_PerformCrossFade(CIRCULAR_FLOAT_BUFFER OutBuf, UINT32 OutBuf_VirtPos,
							   CIRCULAR_FLOAT_BUFFER InBuf, UINT32 InBuf_VirtPos,
							   float * XFade, UINT32 XFadeSize);

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
			);


bool SOLAFS_UpdateStream(
			CIRCULAR_FLOAT_BUFFER	OutBuf, //the output buffer
			CIRCULAR_FLOAT_BUFFER	InBuf, //the input buffer
			double					InToOutRatio, //How much scaling up or down? (len(X)=Ratio*len(Y))
									//NOTE: new ratios only affect the NEXT call to SOLAFS_UpdateStream()
			UINT32 *				NextPredInPos, //out param for what the next smallest position
													//could be in InBuf
			UINT32 *				CurrOutPos
			);

void SOLAFS_EndStream(); //frees what SOLAFS_BeginStream() allocated

bool SOLAFS_readFile(WAV_FILE InWavFile,
					 CIRCULAR_FLOAT_BUFFER OutBuf,
					 UINT32 TargetVirtStartPos,
					 UINT32 Size,
					 UINT32* amntRead);



bool SOLAFS_writeFile(WAV_FILE OutWavFile,
					 CIRCULAR_FLOAT_BUFFER Buf,
					 UINT32 TargetVirtStartPos,
					 UINT32 Size);
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
			);

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
			);