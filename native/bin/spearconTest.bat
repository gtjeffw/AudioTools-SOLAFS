release\spearcon.exe INFILE:"C:\jeff\projects\sonlab\SOLA\bin\audio\natures_miracle_med.wav" OUTFILE:"C:\jeff\projects\sonlab\SOLA\bin\audio\natures_miracle_ABS_med.wav" LENGTH_SEC:0.7

release\spearcon.exe INFILE:"./audio/natures_miracle_med.wav" OUTFILE:"./audio/spearcon_norm_miracle_med.wav" LENGTH_SEC:0.7

release\spearcon.exe INFILE:./audio/natures_miracle_med.wav OUTFILE:./audio/spearcon_step_miracle_med.wav LENGTH_SEC:2.0 COMPSTEPMODE:LINEAR CS_START_RAT:0.01

release\spearcon.exe INFILE:./audio/bruce_walker.wav OUTFILE:./audio/spearcon_norm_bruce.wav LENGTH_SEC:0.2

release\spearcon.exe INFILE:./audio/bruce_walker.wav OUTFILE:./audio/spearcon_ramp_bruce.wav LENGTH_SEC:0.2 COMPSTEPMODE:LINEAR CS_START_RAT:0.1