#!/bin/bash

HERE=$(cd `dirname $0`; pwd)
SPDZROOT=$HERE/..

bits=${2:-128}
g=${3:-0}
mem=${4:-empty}

. $HERE/run-common.sh

run_player mascot-party.x ${1:-test_all} -lgp ${bits} -lg2 ${g} -m ${mem} || exit 1
