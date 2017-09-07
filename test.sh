# test.sh
testdir=$(echo testdir$(date -Iminutes))
mkdir ${testdir}

for i in `seq 1 100`;
do
	echo "${i} Start"
	./producer_consumer.exe > ${testdir}/${i}.out
	echo "${i} Finish"

	sleep 5
done