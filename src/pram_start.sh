acc_config=$1
mem_config=$2

#echo ${acc_config}

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=1 > "1_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=1 > "1_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=1 > "1_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=1 > "1_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=8 > "8_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=8 > "8_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=8 > "8_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=8 > "8_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=16 > "16_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=16 > "16_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=16 > "16_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=16 > "16_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=32 > "32_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=32 > "32_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=32 > "32_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=32 > "32_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=64 > "64_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=64 > "64_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=64 > "64_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=64 > "64_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=128 > "128_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=128 > "128_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=128 > "128_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=128 > "128_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=256 > "256_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=256 > "256_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=256 > "256_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=256 > "256_${acc_config}_TIR_log.txt"

python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ReId.csv -batch=512 > "512_${acc_config}_ReId_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/ESTP.csv -batch=512 > "512_${acc_config}_ESTP_log.txt" 
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/MIR.csv  -batch=512 > "512_${acc_config}_MIR_log.txt"
python3 cox.py -acc_arch_config="./configs/acc_configs/${acc_config}.cfg" -mem_arch_config="./configs/mem_configs/${mem_config}.cfg" -network_config=./configs/network_configs/IR/TIR.csv  -batch=512 > "512_${acc_config}_TIR_log.txt"


mv ./*.txt ./logs

