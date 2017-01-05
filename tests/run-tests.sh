#!/bin/sh

TESTBIN_DIR=./dbuild/

TESTS=`find dbuild/tests -type f -executable -print`
for f in $TESTS;
do
    echo "testing $f"
    $f > /dev/null
    if [ $? -eq 0 ]; then
        echo "OK"
    else
        echo "$? tests FAILED"
    fi
done
