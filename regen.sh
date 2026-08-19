#!/bin/sh
# regenerate crypto ports and game DB from the MAME tree

set -euf

MAMEDIR="${MAMEDIR:-ref/mame}"

python3 tools/port_crypto.py "${MAMEDIR}"/src/devices/bus/neogeo src/crypt
python3 tools/gendb.py "${MAMEDIR}"/src/mame/snk/neogeo.cpp src/games_db.c
python3 tools/gen_meta.py data/known_good.tsv -o src/meta_db.c
