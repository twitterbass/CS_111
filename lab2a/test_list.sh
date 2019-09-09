# /bin/bash

# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 3

rm -f lab2_list.csv


for i in 10 100 1000 10000 20000
do
    echo "./lab2_list --threads=1 --iterations=$i >> lab2_list.csv"
    ./lab2_list --threads=1 --iterations=$i >> lab2_list.csv
done

#######################################
for t in 2 4 8 12
do
    for i in 1 10 100 1000
    do
	echo "./lab2_list --threads=$t --iterations=$i >> lab2_list.csv"
	./lab2_list --threads=$t --iterations=$i >> lab2_list.csv
	done
done

##################################
for t in 2 4 8 12
do
    for i in 1 2 4 8 16 32
    do
	for y in i d l id il dl
	do
	    echo "./lab2_list --threads=$t --iterations=$i --yield=$y  >> lab2_list.csv" 
	    ./lab2_list --threads=$t --iterations=$i --yield=$y  >> lab2_list.csv
	    done
	done
done

#################################
for y in i d l id il dl
do
    for s in m s
    do
	echo "./lab2_list --threads=12 --iterations=32 --yield=$y   --sync=$s >> lab2_list.csv"
	./lab2_list --threads=12 --iterations=32 --yield=$y   --sync=$s >> lab2_list.csv
	done
done

###############################
for t in 1 2 4 8 12 16 24
do
    echo "./lab2_list --threads=$t --iterations=1000          >> lab2_list.csv"
    ./lab2_list --threads=$t --iterations=1000          >> lab2_list.csv

    for s in m s
    do
	echo "./lab2_list --threads=$t --iterations=1000 --sync=$s >> lab2_list.csv"
	./lab2_list --threads=$t --iterations=1000 --sync=$s >> lab2_list.csv
	done

done






