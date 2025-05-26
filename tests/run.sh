#! /bin/sh

CC="cobalt"

CFLAGS="-nocol"

cp ../cobalt ./cobalt

if ! test -d "output"
then
    mkdir output
else
    rm -rf output/*
fi

for n in $(seq 1000)
do
    paddedn=$(printf "%05d" "$n")
    file="./single-exec/$paddedn.c"

    if test -f "$file"
    then
        ./$CC $CFLAGS $file -o "output/$paddedn.bin" &> "output/$paddedn.ccout"

        if ! [ -s "output/$paddedn.ccout" ]
        then
            rm "output/$paddedn.ccout"
        fi

        if ! test -f "output/$paddedn.bin"
        then 
            echo "Test $n failed: Failed to compile $file"
            continue
        fi

        if ! "./output/$paddedn.bin" > "output/$paddedn.output" 2>&1
        then
            echo "Test $n failed: Failed to run $file"
            continue
        fi

        if ! diff -u "single-exec/$paddedn.c.expected" "output/$paddedn.output"
        then
            echo "Test $n failed: Did not match expected value"
            continue
        fi 
    else
        break
    fi
done