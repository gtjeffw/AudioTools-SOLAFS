#!/bin/sh
# Builds build/solafs_exp from the native SOLAFS sources with clang (macOS or Linux).
# -ffp-contract=off keeps clang from fusing multiply-adds, so the output matches the web app's
# WebAssembly engine and spearcon.exe bit for bit.
set -e
E=$(cd "$(dirname "$0")" && pwd)
N=$E/../..
R=$N/..
mkdir -p "$E/build/obj"
INC="-I$R/web/wasm/compat -I$E/compat -I$N/WAVIO/src -I$N/CIRCULAR_BUFFER/src -I$N/SOLAFS/src"
for f in "$N/WAVIO/src/WavIO/WavIO.cpp" "$N/WAVIO/src/WavIO/FileIO.cpp" "$N/WAVIO/src/Common/HelpDebug.cpp" \
         "$N/CIRCULAR_BUFFER/src/CircFBuf/CircFBuf.cpp" "$N/SOLAFS/src/SOLAFS/SOLAFS.cpp" "$E/solafs_exp.cpp"; do
  b=$(basename "$f" .cpp)
  clang++ -std=c++14 -O2 -ffp-contract=off -w $INC -c "$f" -o "$E/build/obj/$b.o"
done
clang++ -o "$E/build/solafs_exp" "$E"/build/obj/*.o
echo "built $E/build/solafs_exp"
