#!/bin/bash

TEST_DIR=${1:-dbuild/tests}

case "$OSTYPE" in
  darwin*)  
	echo "OSX"
	TESTS=`find $TEST_DIR -perm +0111 -type f`	
 	;;
  linux*)   
	echo "LINUX"
	TESTS=`find $TEST_DIR -type f -executable -print`
	;;
  bsd*)     
	echo "BSD"
	TESTS=`find $TEST_DIR -type f -executable -print`
	;;
  msys*)    
	echo "WINDOWS"
	TESTS=`find $TEST_DIR -name "*.exe" -print`
	;;
  *)        echo "unknown: $OSTYPE" ;;
esac

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
