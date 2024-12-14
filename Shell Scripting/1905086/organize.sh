#!/bin/bash 
#submissions-$1 targets-$2 tests-$3 answers-$4

if [ $# -lt 4 ]; then
    	echo "Usage:
./organize.sh <submission folder> <target folder> <answer folder> [-v] [-noexecute]

-v: verbose
-noexecute: do not execute code files"
    kill -INT $$
fi

mkdir -p $2/dump
mkdir -p $2/C; mkdir -p $2/Java; mkdir -p $2/Python

if [ "$5" != "-noexecute" ] && [ "$6" != "-noexecute" ]
then
    echo "student_id,type,matched,unmatched" > $2/result.csv # generating csv file
fi

for file in $1/*.zip
do
    #echo $file
    unzip -q "$file" -d $2/dump
    id=${file: -11: -4}
    #echo "$id"
    verbose=0
    if [ "$5" = "-v" ] || [ "$6" = "-v" ]
    then
        verbose=1
        echo "Organizing files for $id"
    fi

    codefile=$(find $2/dump/ -name *.c)
    ext=${codefile: -1}
    if [ "$ext" = "c" ]
	then
    # organizing
        destdir="$2/C/$id"
		mkdir -p "$destdir"
		mv "$codefile" "$destdir/main.c"
    
    # executing
        if [ "$5" != "-noexecute" ] && [ "$6" != "-noexecute" ]
        then
            if [ $verbose -eq 1 ]
            then
                echo "Executing files for $id"
            fi

            cfile=$destdir/main.c
            gcc $cfile -o $destdir/main.out
            answerdir=$4
            i=1
            no_match_count=1
            for testcase in $3/*
            do
                $destdir/main.out < $testcase > $destdir/out$i.txt
                ansfile=$answerdir/ans$i.txt
                compresult=$(diff --brief $destdir/out$i.txt $ansfile)
                compresult=${compresult: -6}
                if [ "$compresult" = "differ" ]
                then
                        no_match_count=$((no_match_count + 1))
                fi
                i=$((i + 1))
            done
            echo "$id,C,$(($i - $no_match_count)),$((no_match_count - 1))" >> $2/result.csv
        fi
    fi

    codefile=$(find $2/dump/ -name *.java)
    ext=${codefile: -1}
    if [ "$ext" = "a" ]
    then
    # organizing
        destdir="$2/Java/$id"
		mkdir -p "$destdir"
		mv "$codefile" "$destdir/Main.java"

    #executing
        if [ "$5" != "-noexecute" ] && [ "$6" != "-noexecute" ]
        then
            if [ $verbose -eq 1 ]
            then
                echo "Executing files for $id"
            fi

            jfile=$destdir/Main.java
            javac "$jfile"
            answerdir=$4
            i=1
            no_match_count=1
            for testcase in $3/*
            do
                java -cp "$destdir" Main < $testcase > $destdir/out$i.txt
                ansfile=$answerdir/ans$i.txt
                compresult=$(diff --brief $destdir/out$i.txt $ansfile)
                compresult=${compresult: -6}
                if [ "$compresult" = "differ" ]
                then
                        no_match_count=$((no_match_count + 1))
                fi
                i=$((i + 1))
            done
            echo "$id,Java,$(($i - $no_match_count)),$((no_match_count - 1))" >> $2/result.csv
        fi
    fi

    codefile=$(find $2/dump/ -name *.py)
    ext=${codefile: -1}
    if [ "$ext" = "y" ]
    then
    # organizing
        destdir="$2/Python/$id"
		mkdir -p "$destdir"
		mv "$codefile" "$destdir/main.py"
    #executing
        if [ "$5" != "-noexecute" ] && [ "$6" != "-noexecute" ]
        then
            if [ $verbose -eq 1 ]
            then
                echo "Executing files for $id"
            fi

            i=1
            no_match_count=1
            for testcase in $3/*
            do
                python3 "$destdir"/main.py < $testcase > $destdir/out$i.txt
                ansfile=$answerdir/ans$i.txt
                compresult=$(diff --brief $destdir/out$i.txt $ansfile)
                compresult=${compresult: -6}
                if [ "$compresult" = "differ" ]
                then
                        no_match_count=$((no_match_count + 1))
                fi
                i=$((i + 1))
            done
            echo "$id,Python,$(($i - $no_match_count)),$((no_match_count - 1))" >> $2/result.csv
        fi
    fi
    rm -Rf $2/dump/*
done
rm -R $2/dump
