#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 RUN" >&2
  exit 1
fi

RUN="$1"

# Hard-coded paths for use after copying this script into a data directory.
JAEASORTIO="/home/js/OtherCodes/JAEASortTechno/bin/JAEASortTechnoAP"
RUN_PREFIX="run"
DATA_DIR="/path/to/data"
INFOFILE="/home/js/OtherCodes/JAEASortTechno/ExampleFiles/Example.info"

INFILE=""

shopt -s nullglob

matches=( "$DATA_DIR/${RUN_PREFIX}${RUN}"*.bin )
if (( ${#matches[@]} > 0 )); then
  INFILE="$DATA_DIR/${RUN_PREFIX}${RUN}"
else
  matches=( "$DATA_DIR/${RUN}"*.bin )
  if (( ${#matches[@]} > 0 )); then
    INFILE="$DATA_DIR/${RUN}"
  else
    matches=( "${RUN}"*.bin )
    if (( ${#matches[@]} > 0 )); then
      INFILE="$RUN"
    fi
  fi
fi

shopt -u nullglob

if [[ -z "$INFILE" ]]; then
  echo "Error: no input .bin files found for run '$RUN'." >&2
  exit 1
fi

"$JAEASORTIO" "$INFILE" "$INFOFILE"
