# /bin/bash

# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495
# SLIPDAYS: 3

rm -f lab2_add.csv

###################################
for t in 2 4 8 12
do
    for i in 100 1000 10000 100000
    do
	echo "./lab2_add --threads=$t --iterations=$i >> lab2_add.csv"
	./lab2_add --threads=$t --iterations=$i >> lab2_add.csv
	done
done

###########################################
for t in 2 4 8 12
do
    for i in 10 20 40 80 100 1000 10000 100000
    do
	echo "./lab2_add --threads=$t --iterations=$i --yield >> lab2_add.csv"
	./lab2_add --threads=$t --iterations=$i --yield >> lab2_add.csv
	done
done

################################
for t in 2 8
do
    for i in 100 1000 10000 100000
    do
	echo "./lab2_add --threads=$t --iterations=$i          >> lab2_add.csv"
	./lab2_add --threads=$t --iterations=$i          >> lab2_add.csv
	echo "./lab2_add --threads=$t --iterations=$i  --yield >> lab2_add.csv"
	./lab2_add --threads=$t --iterations=$i  --yield >> lab2_add.csv
	done
done

##############################
for i in 10 20 40 80 100 1000 10000 100000 1000000
do
    echo "./lab2_add --threads=1 --iterations=$i >> lab2_add.csv"
    ./lab2_add --threads=1 --iterations=$i >> lab2_add.csv
done

############################
for t in 2 4 8 12
do
    echo "./lab2_add --threads=$t --iterations=10000  --yield --sync=m >> lab2_add.csv"
    ./lab2_add --threads=$t --iterations=10000  --yield --sync=m >> lab2_add.csv
    echo "./lab2_add --threads=$t --iterations=10000  --yield --sync=c >> lab2_add.csv"
    ./lab2_add --threads=$t --iterations=10000  --yield --sync=c >> lab2_add.csv
    echo "./lab2_add --threads=$t --iterations=1000   --yield --sync=s >> lab2_add.csv"
    ./lab2_add --threads=$t --iterations=1000   --yield --sync=s >> lab2_add.csv
done

#########################
for t in 1 2 4 8 12
do
    echo "./lab2_add --threads=$t --iterations=10000           >> lab2_add.csv"
    ./lab2_add --threads=$t --iterations=10000           >> lab2_add.csv

    for s in m c s
    do
	echo "./lab2_add --threads=$t --iterations=10000  --sync=$s >> lab2_add.csv"
	./lab2_add --threads=$t --iterations=10000  --sync=$s >> lab2_add.csv
	done
done
