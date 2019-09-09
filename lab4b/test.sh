#!/bin/bash

# NAME: John Tran
# EMAIL: johntran627@gmail.com
# UID: 005183495

let errors=0


################################################################
# check --log
echo "... checking LOG"
./lab4b --log=LOGFILE > STDOUT <<-EOF
OFF
EOF

if [ ! -s LOGFILE ]; then
	errors+=1
fi


################################################################
# check SCALE
echo "... checking SCALE"
./lab4b --log=LOGFILE > STDOUT <<-EOF
SCALE=F
SCALE=C
OFF
EOF

grep "SCALE=F" LOGFILE > /dev/null
if [ $? -ne 0 ]; then
	errors+=1
fi

grep "SCALE=C" LOGFILE > /dev/null
if [ $? -ne 0 ]; then
	errors+=1
fi


################################################################
# check PERIOD=5
echo "... checking PERIOD=5"
./lab4b --log=LOGFILE > STDOUT <<-EOF
PERIOD=5
OFF
EOF

grep "PERIOD=5" LOGFILE > /dev/null
if [ $? -ne 0 ]; then
	errors+=1
fi


################################################################
# check STOP / START
echo "... checking STOP / START"
./lab4b --log=LOGFILE > STDOUT <<-EOF
STOP
START
OFF
EOF

grep "STOP" LOGFILE > /dev/null
if [ $? -ne 0 ]; then
	errors+=1
fi

grep "START" LOGFILE > /dev/null
if [ $? -ne 0 ]; then
	errors+=1
fi


################################################################
# check LOG
echo "... checking LOG"
./lab4b --log=LOGFILE > STDOUT <<-EOF
LOG MEOW MEOW
OFF
EOF

grep "MEOW MEOW" LOGFILE > /dev/null
if [ $? -ne 0 ]; then
	errors+=1
fi

################################################################
# report the result of smoke test
if [ $errors -eq 0 ]; then
	echo "... PASSED ALL CHECKS"
else
	echo "... FAILED THE TEST WITH $errors ERRORS"
fi

# cleanup
rm STDOUT LOGFILE




