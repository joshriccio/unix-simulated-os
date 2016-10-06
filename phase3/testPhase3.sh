#!/bin/bash

for i in $(seq $1 $2)
do
    if [ $i -gt 9 ]
    then
        number=$i
    else
        number="0$i"
    fi

    echo "----------- test0$number -----------"
#   make $(echo "test$number") 
#   echo
    ./$(echo "test$number") &> diffTests/$(echo "test$number""diff")
    echo
    diff $(echo "diffTests/test$number""diff") $(echo "testResults/test$number"".txt")
    echo
done
