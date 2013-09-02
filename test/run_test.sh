#!/bin/bash

# TODO(dkorolev): Get rid of the Makefile and compile from this script with various options.
# TODO(dkorolev): report.txt -> report.html

TEST_SECONDS=10

cat >/dev/null <<EOF
SAVE_IFS="$IFS"
COMPILERS="g++ clang++"
OPTIONS="-O0 -O3"
CMDLINES=""

for compiler in $COMPILERS ; do
  for options in $OPTIONS ; do
    CMDLINES+=$compiler' '$options':'
  done
done

IFS=':'
for cmdline in $CMDLINES ; do
  echo $cmdline
done
IFS="$SAVE_IFS"
EOF

for function in $(ls functions/ | cut -f1 -d.) ; do 
  echo "Function: "$function
  data=''
  for action in generate evaluate intermediate_evaluate compiled_evaluate; do
    data+=$(./test_binary $function $action $TEST_SECONDS)':'
  done
  echo $function':'$data | awk -F: '{ printf "  %20s EvalNative %10.2f kqps, EvalSlow %10.2f kqps, EvalFast %10.2f kqps, %.2fs compilation time.\n", $1, 0.001/(1/$3-1/$2), 0.001/(1/$4-1/$2), 0.001/(1/$5-1/$2), $5 }'
  unset times
done
