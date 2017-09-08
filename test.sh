# test.sh

# trap 'echo "Segmentation Fault" > ${testdir}/${i}' SIGSEGV 


testdir=$(echo testdir$(date -Iminutes))
mkdir ${testdir}

for i in `seq 1 100`;
do
	echo "${i} Start"
	./producer_consumer.exe > ${testdir}/${i}.out
	echo "${i} Finish"

	sleep 5
done


result=$(cat ${testdir}/* | grep main | wc -l)

echo "${result} tests passed for this run"