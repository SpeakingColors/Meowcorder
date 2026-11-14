#!/bin/sh
# Regenerates src/plugin/Layout.h and Icons.h from the approved mockup (docs/mockups/Meowcorder.html).
# Needs node with playwright installed. Run from anywhere: sh tools/gen_layout.sh
set -e
cd "$(dirname "$0")"
export OUT="$(mktemp -d)"
export NODE_PATH="${NODE_PATH:-$(npm root -g)}"
for s in extract extract2 logo; do node gen_$s.js >/dev/null; done
python3 -I gen_layout.py
echo "generated src/plugin/Layout.h and Icons.h"
