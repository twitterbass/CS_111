#! /usr/local/cs/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists
#   lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","
BILLION = 1000000000

# GRAPH 1
set title "Graph-1: Throughput vs Number of Threads for Mutex & Spin locks"
set xlabel "Number of Threads"
set logscale x 10
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

plot \
	"< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'mutex' with linespoints lc rgb 'green', \
	"< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'spin locks' with linespoints lc rgb 'red'
	
# GRAPH 2
set title "Graph-2: Wait-for-lock time & Avg Time for Operation vs Number of Threads for Mutex"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time"
set logscale y 10
set output 'lab2b_2.png'

plot \
	"< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
		title 'avg wait time' with linespoints lc rgb 'green', \
	"< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
		title 'avg time per op' with linespoints lc rgb 'red'
	
# GRAPH 3
set title "Graph-3: Successful Iterations vs Number of Threads"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
	"< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
		title 'no lock' with points lc rgb 'blue', \
	"< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
		title 'mutex' with points lc rgb 'green', \
	"< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
		title 'spin locks' with points lc rgb 'red'

	
# GRAPH 4
set title "Graph-4: Total Operations per second vs Number of Threads for mutex"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_4.png'

plot \
	"< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=1' with linespoints lc rgb 'blue', \
	"< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=4' with linespoints lc rgb 'green', \
	"< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=8' with linespoints lc rgb 'red', \
	"< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=16' with linespoints lc rgb 'violet'
	
	
# GRAPH 5
set title "Graph-5: Total Operations per second vs Number of Threads for spin locks"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_5.png'

plot \
	"< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=1' with linespoints lc rgb 'blue', \
	"< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=4' with linespoints lc rgb 'green', \
	"< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=8' with linespoints lc rgb 'red', \
	"< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(BILLION / ($7)) \
		title 'lists=16' with linespoints lc rgb 'violet'



