// Experiment driver for SOLAFS: the file loop of SOLAFS() in SOLAFS.cpp, unchanged except for what
// the options below add. With only -r it produces the same output as SOLAFS() (and so as spearcon.exe
// and the web app's engine) for the same settings.
//
// usage: solafs_exp IN.wav OUT.wav W WOV KMAX [options]       (W, WOV, KMAX in samples)
//   -r RATIO      constant input-to-output speed ratio (default 1)
//   -s FILE       ratio schedule instead: first value = frame hop in input samples, then one ratio per
//                 frame. The ratio for each step is looked up at the step's predicted input position.
//   -a FILE       anchors: input positions a step must land on (the jump to each is shortened to reach it)
//   -f SHAPE      cross-fade shape: linear (default, as published) or hann (sin^2; with WOV = W/2 this
//                 is WSOLA). Applied by overwriting SOLAFS's fade table after SOLAFS_BeginStream(), so
//                 the library itself is unchanged; wsola/solafs-crossfade-option.patch does the same
//                 inside SOLAFS.cpp.
//   -l FILE       step log: one line per step, "<input position copied> <output position>"; the input
//                 copied by a step is [pos, pos+W)
//
// Built by build.sh. Cross-correlation and skew decimation are 1, as spearcon.exe uses.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tchar.h>
#include <vector>
#include "Common/CommonTypes.hpp"
#include "WavIO/WavIO.hpp"
#include "CircFBuf/CircFBuf.hpp"
#include "SOLAFS/SOLAFS.hpp"

//SOLAFS.cpp globals
extern UINT32 gSynthesisDistance, gPrevInPos, gWindowOverlap;
extern float * gCrossFade;

static std::vector<double> gSched;
static double gHop = 1;

static double ratioAt(UINT32 pos, double constant)
{
	if(gSched.empty())
		return constant;
	size_t i = (size_t)(pos / gHop);
	return gSched[i < gSched.size() ? i : gSched.size() - 1];
}

static int usage()
{
	fprintf(stderr, "usage: solafs_exp IN.wav OUT.wav W WOV KMAX [-r ratio] [-s schedule] [-a anchors] [-f linear|hann] [-l steplog]\n");
	return 2;
}

int main(int argc, char ** argv)
{
	if(argc < 6)
		return usage();

	UINT32 W = (UINT32)atoi(argv[3]), Wov = (UINT32)atoi(argv[4]), Kmax = (UINT32)atoi(argv[5]);
	double ratio = 1.0;
	bool hann = false;
	std::vector<UINT32> anchors;
	FILE * log = NULL;

	for(int i = 6; i < argc; i += 2)
	{
		if(i + 1 >= argc)
			return usage();
		const char * opt = argv[i], * val = argv[i + 1];
		if(!strcmp(opt, "-r"))
			ratio = atof(val);
		else if(!strcmp(opt, "-f"))
		{
			if(!strcmp(val, "hann")) hann = true;
			else if(strcmp(val, "linear")) return usage();
		}
		else if(!strcmp(opt, "-s") || !strcmp(opt, "-a"))
		{
			FILE * f = fopen(val, "r");
			if(f == NULL) { fprintf(stderr, "can't read %s\n", val); return 2; }
			double v;
			if(!strcmp(opt, "-s"))
			{
				if(fscanf(f, "%lf", &gHop) != 1 || gHop <= 0) { fclose(f); return usage(); }
				while(fscanf(f, "%lf", &v) == 1) gSched.push_back(v);
			}
			else
				while(fscanf(f, "%lf", &v) == 1) anchors.push_back((UINT32)v);
			fclose(f);
		}
		else if(!strcmp(opt, "-l"))
		{
			log = fopen(val, "w");
			if(log == NULL) { fprintf(stderr, "can't write %s\n", val); return 2; }
		}
		else
			return usage();
	}

	WAV_FILE in = NULL, out = NULL;
	if(wavOpenForRead((_TCHAR *)argv[1], &in) != WAV_NO_ERR)
	{
		fprintf(stderr, "can't read %s\n", argv[1]);
		return 1;
	}
	if(wavOpenForWrite((_TCHAR *)argv[2], &out, WAV_FORMAT_PCM, 1, in->wavInfo->samplesPerSec, 16) != WAV_NO_ERR)
	{
		fprintf(stderr, "can't write %s\n", argv[2]);
		closeWavFile(in);
		return 1;
	}

	const UINT32 Ss = W - Wov, INPUT_BUF_SIZE = W + Kmax;
	UINT32 trueSize = 0, nextPredInPos = 0, currOutPos = 0, finalSize = 0, remSamples = 0;
	bool finalSizeFound = false, keepGoing = true, ok = false;
	size_t nextAnchor = 0;
	CIRCULAR_FLOAT_BUFFER InBuf = CFB_create(INPUT_BUF_SIZE), OutBuf = CFB_create(W);

	if(InBuf == NULL || OutBuf == NULL ||
		!SOLAFS_readFile(in, InBuf, 0, INPUT_BUF_SIZE, &trueSize) || trueSize < InBuf->virtSize ||
		!SOLAFS_BeginStream(OutBuf, InBuf, ratioAt(0, ratio), W, Wov, Kmax, 1, 1, &nextPredInPos, &currOutPos))
	{
		fprintf(stderr, "input too short for these settings, or invalid settings\n");
		goto CLEANUP;
	}

	if(hann)
	{
		//rising half of a Hann window; with the complementary 1-f weight the pair sums to 1
		for(UINT32 i = 0; i < gWindowOverlap; i++)
		{
			double s = sin(3.14159265358979323846 * (i + 0.5) / (2.0 * gWindowOverlap));
			gCrossFade[i] = (float)(s * s);
		}
	}

	if(!SOLAFS_writeFile(out, OutBuf, currOutPos, gSynthesisDistance))
		goto CLEANUP;
	if(log) fprintf(log, "0 0\n");

	while(keepGoing)
	{
		bool test = SOLAFS_readFile(in, InBuf, nextPredInPos, INPUT_BUF_SIZE, &trueSize);
		if(!test)
		{
			if(!finalSizeFound) { finalSize = InBuf->virtSize; finalSizeFound = true; }
		}
		else if(!finalSizeFound)
		{
			finalSize = trueSize;
			if(trueSize < InBuf->virtSize) finalSizeFound = true;
		}
		remSamples = finalSize > nextPredInPos ? finalSize - nextPredInPos : 0;

		//the ratio given to SOLAFS_UpdateStream() sets the jump to the NEXT step
		UINT32 predPos = nextPredInPos;
		double r = ratioAt(predPos, ratio);
		while(nextAnchor < anchors.size() && anchors[nextAnchor] <= predPos)
			nextAnchor++;
		if(nextAnchor < anchors.size() && predPos + r * Ss > anchors[nextAnchor])
			r = (anchors[nextAnchor] - predPos + 0.5) / Ss; //land the next step on the anchor

		if(!SOLAFS_UpdateStream(OutBuf, InBuf, r, &nextPredInPos, &currOutPos))
			goto CLEANUP;
		if(log) fprintf(log, "%u %u\n", gPrevInPos, currOutPos);

		UINT32 writeAmnt = remSamples < Ss ? remSamples : Ss;
		if(writeAmnt != 0)
		{
			if(writeAmnt < Wov)
				keepGoing = false;
			if(!SOLAFS_writeFile(out, OutBuf, currOutPos, writeAmnt))
				goto CLEANUP;
		}
		else
			keepGoing = false;
	}
	ok = true;

CLEANUP:
	SOLAFS_EndStream();
	if(InBuf) CFB_delete(InBuf);
	if(OutBuf) CFB_delete(OutBuf);
	if(wavWriteFinalize(out) != WAV_NO_ERR) ok = false;
	closeWavFile(out);
	closeWavFile(in);
	if(log) fclose(log);
	return ok ? 0 : 1;
}
