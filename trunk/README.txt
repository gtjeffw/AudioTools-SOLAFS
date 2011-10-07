Several projects are present, however all the code is really in support of the "Spearcon" project.

You can find a VStudio solution that will load everything here:

.\SOLAFS\project\SOLAFS\SOLAFS.sln   (That's not the best place for it, but that's where it is right now)

Compile everything and then either ./bin/Release/ or ./bin/Debug/ will contain all the binaries.

spearcon.exe is the most useful.  The other executables are really just for quick unit testing of the
various support libs.

./bin/audio/ contains some test audio.

./bin/testSpearcon.bat and ./bin/testSpearcon_DEBUG.bat can be used to test spearcon.exe.
Running the tests will output files into ./bin/audio/

Run spearcon.exe with no args to see instructions for how to use it.