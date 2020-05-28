#!/bin/bash

### CONFIG ###
FILES=("./test_list1" "./test_list2" "./test_list3" "./test_list4"
       "./test_list1_output.txt" "./test_list2_output.txt"
       "./test_list3_output.txt" "./test_list4_output.txt")
##############

#init
res=0

#check files exist
for fi in "${FILES[@]}"; do
    if [ ! -f "$fi" ]; then
        echo "$fi does not exist"
        exit 1
    fi
done

OPTIONS="-w -i -B"
TIME="30"

echo "-- linked_list"$(( $1 + 1 ))" --"

# === testing linkedlist ===
valgrind -q --leak-check=full --show-reachable=yes --errors-for-leak-kinds=all --error-exitcode=1 ./test_list$(( $1 + 1 )) >/tmp/test_list$(( $1 + 1 )).txt
if [ $? -eq 1 ]; then
    res=1
    echo -e "valgrind test_list"$(( $1 + 1 ))" reported memory leaks\n"
else
    echo "valgrind test_list"$(( $1 + 1 ))" ok"
fi
diff $OPTIONS /tmp/test_list$(( $1 + 1 )).txt test_list$(( $1 + 1 ))_output.txt
if [ $? -eq 0 ]; then
    echo "diff test_list"$(( $1 + 1 ))" ok"
else
    res=$(( $res + 2 ))
fi
echo -en "\n"

# === results ===
echo -e "\tTEST\t\tVALGRIND\tDIFF"
echo -en "TEST test_list"$(( $1 + 1 ))"\t\t"
if [ $res -eq 0 ]; then
    echo -e "correct\t\tcorrect\n"
    exit 0
elif [ $res -eq 1 ]; then
    echo -e "FAILED\t\tcorrect\n"
    exit 0 #TODO: change this to other error code and pass it throught to "collect_results.py"
elif [ $res -eq 2 ]; then
    echo -e "correct\t\tFAILED\n"
else
    echo -e "FAILED\t\tFAILED\n"
fi

exit 1
