#!/bin/bash

case "$OSTYPE" in
  darwin*)  
	echo "OSX"
	TESTS=`find dbuild/tests -perm +0111 -type f`	
 	;;
  linux*)   
	echo "LINUX"
	TESTS=`find dbuild/tests -type f -executable -print`
	;;
  bsd*)     
	echo "BSD"
	TESTS=`find dbuild/tests -type f -executable -print`
	;;
  msys*)    
	echo "WINDOWS"
	TESTS=`find dbuild/tests -type f -executable -print`
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
