// WebAssembly entry point for the Spearcon Factory web app.
//
// Runs the native SOLAFS code path unchanged: it does what genSpearcon() in
// native/Spearcon/source/Spearcon/Spearcon_Main.cpp does around SOLAFS(), on 16-bit WAV files
// that the web worker writes to (and reads back from) Emscripten's in-memory filesystem.
// Picking the speed ratio (log / target length / ratio, compression stepping) happens in
// TypeScript (web/src/engine/spearconMath.ts), mirroring genSpearcon().

#include <stdio.h>
#include <tchar.h>
#include <emscripten/emscripten.h>

#include "Common/CommonTypes.hpp"
#include "WavIO/WavIO.hpp"
#include "CircFBuf/CircFBuf.hpp"
#include "SOLAFS/SOLAFS.hpp"

extern "C" {

// Returns 1 on success, 0 on failure (bad input file, input too short for the window settings,
// invalid parameters, or a write error). Mono 16-bit output at the input's sample rate.
EMSCRIPTEN_KEEPALIVE
int solafs_file(const char * inPath, const char * outPath, double ratio, double ratioStep,
				int windowSize, int windowOverlap, int maxSkew)
{
	WAV_FILE in = NULL;
	WAV_FILE out = NULL;
	bool ok = false;

	if(wavOpenForRead((_TCHAR *)inPath, &in) != WAV_NO_ERR)
		return 0;

	if(wavOpenForWrite((_TCHAR *)outPath, &out, WAV_FORMAT_PCM, 1, in->wavInfo->samplesPerSec, 16) == WAV_NO_ERR)
	{
		//cross-correlation and skew decimation of 1, as spearcon.exe uses
		ok = SOLAFS(out, in, ratio, ratioStep, (UINT32)windowSize, (UINT32)windowOverlap, (UINT32)maxSkew, 1, 1);

		if(wavWriteFinalize(out) != WAV_NO_ERR)
			ok = false;
	}

	closeWavFile(out);
	closeWavFile(in);

	return ok ? 1 : 0;
}

}
