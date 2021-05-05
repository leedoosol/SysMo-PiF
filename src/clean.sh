rm -rf *.csv

rm ./logs/*.txt
echo "delete log files"

if [ "$1" = "-o" ]
then
	echo "delete output folder"
	rm -rf "./outputs"
fi

if [ "$2" = "-c" ]
then
	echo "delete pycache"
	rm -rf "./__pycache__"
	rm -rf "./sram/__pycache__"
	rm -rf "./mem/__pycache__"
fi


