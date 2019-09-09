# bin/bash

# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 3

rm -f lab2b_list.csv


for th in 1 2 4 8 12 16 24
do
    for s in m s
    do 
	echo "./lab2_list --threads=$th --iterations=1000 --sync=$s >> lab2b_list.csv"
	./lab2_list --threads=$th --iterations=1000 --sync=$s >> lab2b_list.csv
    done
done



for th in 1 4 8 12 16 
do 
    for i in 1 2 4 8 16
    do 
	echo "./lab2_list --threads=$th --iterations=$i --yield=id --lists=4 >> lab2b_list.csv"
	./lab2_list --threads=$th --iterations=$i --yield=id --lists=4 >> lab2b_list.csv
    done
done



for th in 1 4 8 12 16
do 
    for i in 10 20 40 80
    do 
		for s in s m
		do
	    echo "./lab2_list --threads=$th --iterations=$i --yield=id --sync=$s --lists=4 >> lab2b_list.csv"
	    ./lab2_list --threads=$th --iterations=$i --yield=id --sync=$s --lists=4 >> lab2b_list.csv
		done
    done
done



for th in 1 2 4 8 12
do
    for s in s m
    do
		for l in 4 8 16
		do 
	    echo "./lab2_list --threads=$th --iterations=1000 --sync=$s --lists=$l >> lab2b_list.csv"
	    ./lab2_list --threads=$th --iterations=1000 --sync=$s --lists=$l >> lab2b_list.csv
		done
    done
done



