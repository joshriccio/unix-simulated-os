#!/bin/ksh
#dir=/home/cs452/fall16/phase5/testResults
dir=/home/cs452/fall16/admin/project/phase5/testResults
#dir=/Users/patrick/Classes/452/project/phase5/testResults

if [ "$#" -eq 0 ] 
then
    echo "Usage: ksh testphase5.ksh <num>"
    echo "where <num> is 1, 2, ... or 8"
    exit 1
fi

num=$1
if [ -f simple${num} ]
then
    /bin/rm simple${num}
fi

# Copy disk and terminal files
cp testcases/disk0.orig disk0
cp testcases/disk1.orig disk1

if  make simple${num} 
then

    ./simple${num} > simple${num}.txt 2> simple${num}stderr.txt;

    if [ -s simple${num}stderr.txt ]
    then
        cat simple${num}stderr.txt >> simple${num}.txt
    fi

    /bin/rm simple${num}stderr.txt

#    if [ "${num}" -eq 14 ]; then
#        cmp disk1 testResults/disk14
#        diskCompare=$?
#        echo "diskCompare = "${diskCompare}
#        if diff --brief simple${num}.txt ${dir}
#        then
#            if [ ${diskCompare} -eq 0 ] ; then
#                echo
#                echo simple${num} passed!
#            else
#                echo
#                echo simple${num} failed!
#                echo incorrect contents in disk1
#            fi
#        else
#            echo
#            diff -C 1 simple${num}.txt ${dir}
#        fi
#        exit
#    fi

    if diff --brief simple${num}.txt ${dir}
    then
        echo
        echo simple${num} passed!
    else
        echo
        diff -C 1 simple${num}.txt ${dir}
    fi
fi
echo
