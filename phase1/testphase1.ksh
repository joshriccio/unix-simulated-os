#!/bin/ksh

resultsDirectory="/home/cs452/fall16/phase1/testResults"
#resultsDirectory="/home/cs452/fall16/admin/project/phase1/testResults"
#resultsDirectory="/Users/patrick/Classes/452/project/phase1/testResults"

if [ "$#" -eq 0 ] 
then
    echo "Usage: ksh testphase1.ksh <num>"
    echo "where <num> is 00, 01, 02, ... "
    exit 1
fi

num=$1
if [ -f test${num} ]
then
    /bin/rm test${num}
fi

if  make test${num} 
then
    test${num} > test${num}.txt 2> test${num}stderr.txt;
    if [ -s test${num}stderr.txt ]
    then
        cat test${num}stderr.txt >> test${num}.txt
    fi

    /bin/rm test${num}stderr.txt

    if diff --brief test${num}.txt ${resultsDirectory}
    then
        echo
        echo test${num} passed!
    else
        echo
        diff -C 1 test${num}.txt ${resultsDirectory}
    fi
fi
echo
