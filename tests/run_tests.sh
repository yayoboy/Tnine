#!/usr/bin/env bash
# Compila ed esegue i test nativi (nessun hardware richiesto).
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
FLAGS="-std=c++17 -Wall -Wextra -Werror -I shim -I ../src"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

$CXX $FLAGS test_t9.cpp ../src/T9Engine.cpp -o "$OUT/test_t9"
"$OUT/test_t9"

$CXX $FLAGS test_meshproto.cpp \
    ../src/mesh/ProtoWriter.cpp \
    ../src/mesh/ProtoReader.cpp \
    ../src/mesh/MeshtasticCodec.cpp \
    -o "$OUT/test_meshproto"
"$OUT/test_meshproto"

echo "Tutti i test nativi superati."
