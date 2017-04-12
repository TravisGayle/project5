#!/bin/sh
#test script to test conditions of virtmem
#tests each program, and algorithm for 100 pages and 3-100 frames
#output is appended to results.txt file
#command ./virtmem $pages(100) $frames $sort $prog

if [ -f "results.txt" ]
then
	rm results.txt
fi

touch results.txt

page=100

for prog in scan \sort focus; do
	for sort in rand fifo custom; do
		for frame in $(seq 3 1 100); do
			if [ $frame -ge 4 ]
			then
				echo >>results.txt
			fi
			echo "virtmem $page $frame $sort $prog" | tee -a results.txt
			echo >>results.txt
			./virtmem $page $frame $sort $prog>>results.txt
		done
	done
done
