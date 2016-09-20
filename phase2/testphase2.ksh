#!/bin/ksh
dir=/home/cs452/fall16/phase2/testResults
dir=/home/cs452/fall16/admin/project/phase2/testResults
#dir=/Users/patrick/Classes/452/project/phase2/testResults

if [ "$#" -eq 0 ] 
then
    echo "Usage: ksh testphase2.ksh <num>"
    echo "where <num> is 00, 01, 02, ... or 26"
    exit 1
fi

num=$1
if [ -f test${num} ]
then
    /bin/rm test${num}
fi

if  make test${num} 
then

    # Copy term1 input for test 14
    if (( num == 14 )); then
        cp testcases/term1.in .
    fi

    # Copy all 4 term input files for test 32
    if (( num == 32 )); then
        cp testcases/term0.in .
        cp testcases/term1.in .
        cp testcases/term2.in .
        cp testcases/term3.in .
    fi

    test${num} > test${num}.txt 2> test${num}stderr.txt;
    if [ -s test${num}stderr.txt ]
    then
        cat test${num}stderr.txt >> test${num}.txt
    fi

    /bin/rm test${num}stderr.txt

    if diff --brief test${num}.txt ${dir}
    then
        echo
        echo test${num} passed!
    else
        echo
        echo test${num} failed!
        echo
        diff -C 1 test${num}.txt ${dir}
    fi
fi
echo
