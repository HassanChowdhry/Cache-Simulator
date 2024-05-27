#!/bin/sh

echo ======================================================
echo ====================== BENCH $1 ======================
echo ======================================================
if timeout 10 ./$2/$3 < tests/bench.$1.in > tests/bench.$1.out; then 
  grep -v Cache tests/bench.$1.out > tests/bench.$1.check
  grep Cache tests/bench.$1.out > tests/bench.$1.stats
  if diff -w tests/bench.$1.check tests/bench.$1.expected > /dev/null; then
    cat tests/bench.$1.stats
    cat tests/bench.$1.rate
  else
    echo FAILED
    echo ======
    echo ____Your_output____ __Expected_Output___
   #echo +++++++++++++++++++ ++++++++++++++++++++
    diff -y -W 80 tests/bench.$1.out tests/bench.$1.expected 
    echo =====================
    echo Your Output:
    echo ++++++++++++++++
    cat tests/bench.$1.out
    echo =====================
    echo Expected Output:
    echo ++++++++++++++++
    cat tests/bench.$1.expected
    exit 1
  fi
elif [ $? -eq 124 ]; then
  echo TIMEOUT
  exit 1
else 
  echo Abnormal program termination: the program crashed
  echo Exit code $?
  exit 1
fi
