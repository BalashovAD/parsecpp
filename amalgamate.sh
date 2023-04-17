#!/bin/sh
python3 thirdparty/amalgamate/amalgamate.py -c tools/amalgamate_config_full.json -s . -v=yes
python3 thirdparty/amalgamate/amalgamate.py -c tools/amalgamate_config_all.json -s . -v=yes
python3 thirdparty/amalgamate/amalgamate.py -c tools/amalgamate_config_fwd.json -s . -v=yes
