import os.path
import math

def merge_two_delay_log_file(
	mem_delayed_file_a = "mem_delayed1.csv",
	mem_delayed_file_b = "mem_delayed2.csv",
	result_file = "merged_file.csv"
	):
	
	f1 = open(mem_delayed_file_a, 'r')
	f2 = open(mem_delayed_file_b, 'r')
	f3 = open(result_file, 'w')
	
	f1_trace = f1.readline()
	f2_trace = f2.readline()

	f1_total_time = 0
	f2_total_time = 0

	while f1_trace:
		temp_time = 0

		f1_clk  = 0.0
		f1_time = 0.0

		f2_clk  = 0.0
		f2_time = 0.0

		if f1_trace:
			f1_clk  = float(f1_trace.split(',')[0])
			f1_time = float(f1_trace.split(',')[1])

		if f2_trace:
			f2_clk  = float(f2_trace.split(',')[0])
			f2_time = float(f2_trace.split(',')[1])
		
			
		if(f1_time > f2_time):
			temp_time = f1_time
			f1_total_time += f1_time
			
		else:
			temp_time = f2_time
			f2_total_time += f2_time

		trace = str(f1_clk) + ',' + str(temp_time) + ',\n'
		f3.write(trace)
	
		f1_trace = f1.readline()
		f2_trace = f2.readline()

	f1.close()
	f2.close()
	f3.close()

	return f1_total_time, f2_total_time

def extract_clk_and_trace(trace):
	clk       = 0
	trace_str = ''
	is_executed = True


	clk       = int(trace.split(',')[0])
	trace_str = ','.join(trace.split(',')[1:-1]) + ','

	return clk, trace_str

def integrate_sram_read_write(
		dimension_rows = 16,
		dimension_cols = 64,

		sram_read_trace_file  = "sram_read.csv",
		sram_write_trace_file = "sram_write.csv",

		sram_integrated_file = "sram_integrated.csv"
		):

	sram_read_requests  = open(sram_read_trace_file, 'r')
	sram_write_requests = open(sram_write_trace_file, 'r')
	
	sram_integrated_file = open(sram_integrated_file, 'w')

	read_elems  = ''
	write_elems = ''

	read_clk  = 0
	write_clk = 0
	last_clk  = 0

	read_trace  = ''
	write_trace = ''
	integrated_trace = ''
	dump_write_trace = ''
	dump_read_trace  = ''

	for _ in range(dimension_cols):
		dump_write_trace = dump_write_trace + ','
	
	for _ in range(dimension_cols + dimension_rows):
		dump_read_trace = dump_read_trace + ','

	write_elems = sram_write_requests.readline()
	if write_elems is not False:
		write_clk, write_trace = extract_clk_and_trace(write_elems)
	
	req_count = 0
	for read_entry in sram_read_requests:
		req_count  = req_count + 1

		read_clk, read_trace = extract_clk_and_trace(read_entry)

		last_clk = read_clk

			
		if (read_clk == write_clk) and write_elems:
			integrated_trace = str(read_clk) + ',' + str(read_trace) + str(write_trace)
			
#			print(integrated_trace)
			write_elems = sram_write_requests.readline()
			if not write_elems:
				continue
			else:
				write_clk, write_trace = extract_clk_and_trace(write_elems)
		else:
			integrated_trace = str(read_clk) + ',' + str(read_trace) + dump_write_trace
		
		sram_integrated_file.write(integrated_trace + '\n')
	
	last_clk = last_clk + 1
#	print(last_clk)
	while True:
		if write_elems:
			if last_clk == write_clk:
				integrated_trace = str(last_clk) + ',' + dump_read_trace + write_trace
				sram_integrated_file.write(integrated_trace)
				
				last_clk = last_clk + 1
#				print(integrated_trace)
				
				write_elems = sram_write_requests.readline()
				
				if not write_elems:
					continue
				else:
					write_clk, write_trace = extract_clk_and_trace(write_elems)
				
			else:
				integrated_trace = str(last_clk) + ',' + dump_read_trace + dump_write_trace
				
				sram_integrated_file.write(integrated_trace + '\n')
				last_clk = last_clk + 1
		else:
			break


	sram_read_requests.close()
	sram_write_requests.close()

	sram_integrated_file.close()

if __name__== "__main__":
	integrate_sram_read_write(
			sram_read_trace_file  = "TIR_FC1_sram_read.csv",
			sram_write_trace_file = "TIR_FC1_sram_write.csv"
			)

