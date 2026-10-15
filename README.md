# AudioTools (SOLAFS / Spearcon)

C++ audio tools from IMTC, mostly in support of the **Spearcon** project (spearcons: speech sped up until it is no
longer recognized as speech, used as auditory menu cues). See `trunk/README.txt` for build and test instructions.

| Path | What |
|---|---|
| `trunk/Spearcon/` | `spearcon.exe`, the main tool |
| `trunk/SOLAFS/` | SOLAFS time-scale modification (changes speed without changing pitch); its `project/SOLAFS/SOLAFS.sln` loads everything |
| `trunk/WAVIO/`, `trunk/CIRCULAR_BUFFER/` | WAV file I/O and circular-buffer support libraries |
| `trunk/SilentStripper/` | trims silence from audio |
| `trunk/bin/` | test scripts (`spearconTest*.bat`, `SilentStripper_*.bat`) and test audio in `bin/audio/` |

Visual Studio 2008-era projects (`.vcproj`, `.sln`); they need upgrading to build with a current Visual Studio.

## Where this came from

Recovered in October 2026 from an SVN working copy of `https://gadget.ipat.gatech.edu/imtc/AudioTools` on an old Mac.
That SVN server no longer exists, so the revision history is lost. The single commit is the checked-out state: r5,
dated 2011-10-07 (files were last changed in r2–r5 on 2011-10-06/07, all by jeff). The working copy had no
uncommitted changes. SVN's `tags/` and `branches/` folders were empty, so they don't appear here.
