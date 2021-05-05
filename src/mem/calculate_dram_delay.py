import os
import math
from tqdm import tqdm

# delete white spaces in sram trace
###########################################################################################################################
def prune(input_list):
	l = []

	for e in input_list:
		e = e.strip()
		if e != '' and e != ' ':
			l.append(e)

	return l
###########################################################################################################################


###########################################################################################################################
def calculate_dram_read_io_time(
		sram_sz		    = 64,		# ifmap/ofmap/filter buffer size
		word_sz_bytes   = 1,
		min_addr        = 0,		# ifmap/ofmap/filter base start address
		max_addr        = 1000000,	# ifmap/ofmap/filter base end address
		
		precision       = 4,
		clock_speed     = 500000,
		read_bw         = 20.0,		# ifmap/ofmap/filter DRAM read bandwidth
        
		sram_trace_file        = "sram_log.csv",
		mem_delayed_trace_file = "mem_delayed.csv",
		is_from_mem            = True
	):

	# Change sram size considering precision
	sram_sz *= 1024
	sram_sz /= precision
#	sram_sz /= 2

	# clock speed(KHz -> Hz)
	clock_speed   *= 1000

	# seconds per cycle(us)
	time_per_cycle = (1/clock_speed)*1000*1000

	# Change memory bandwidth (bytes/sec -> bytes/cycle)
	# bandwidth(bytes per sec) / ( precision(byte) * clock speed(Hz = clocks per sec) )
	read_bw *= (1024*1024*1024)	# GB -> bytes
	read_bw /= precision
	read_bw /= clock_speed
	
	print("----------------------------------------------------------")
	print("SRAM size: \t" + str(int(sram_sz*precision)) + " bytes")
	print("read_bw  : \t" + str(round(read_bw*precision,6)) + " bytes/cycle")
	print("----------------------------------------------------------")
	
	# Initialize fill and drain cycle pointer
	t_fill_start	=  0
	t_drain_start   =  0

	# Initialize io delay time and compute time
	t_delayed_io    = 0
	t_sram_read     = 0

	sram = set()

	sram_requests = open(sram_trace_file, 'r')
	temp_log      = open('temp_log.csv',  'w')

	# SRAM_FOR_LOOP
	# Read sram elements(data) cycle by cycle
	#---------------------------------------------------------------------------------------------------------------------#
	for entry in sram_requests:
		# Refine sram trace
		elems = entry.strip().split(',')
		elems = prune(elems)
		elems = [float(x) for x in elems]

		clk   = elems[0]	# First column of sram trace is the clock cycle
		t_sram_read += time_per_cycle
		
		# IS_FROM_MEMORY
		#-----------------------------------------------------------------------------------------------------------------#
		if(is_from_mem == True):
			# ELEM_FOR_LOOP
			# Read sram elements(data) one by one
			#-------------------------------------------------------------------------------------------------------------#
			for e in range(1, len(elems)):

				# IF_SRAM_RANGE 
				# 1. Check whether there are sram trace in the row
				# 2. Check wheter the sram trace is out of the base address
				#---------------------------------------------------------------------------------------------------------#
				if (elems[e] not in sram) and (elems[e] >= min_addr) and (elems[e] < max_addr):

					# IF_SRAM_FULL
					# Used up all the unique data in the SRAM?
					#-----------------------------------------------------------------------------------------------------#
					if len(sram) + word_sz_bytes > sram_sz:
	
						# Generate the filling trace from time t_fill_start to t_drain_start
						cycles_needed   = t_drain_start - t_fill_start
	
						# t_available        : available time between filling previous sram buffer and next sram buffer
						# t_fill_sram_buffer : time to fill the sram buffer considering external memory bandwidth
						t_available        = cycles_needed * time_per_cycle
						t_fill_sram_buffer = (len(sram)/read_bw) * time_per_cycle
	
						# if the filling the sram buffer time is larger than available time, add io delayed time
						temp_time = 0
						if t_fill_sram_buffer > t_available:
							temp_time     = t_fill_sram_buffer - t_available
							t_delayed_io += temp_time
							
							delayed_trace = str(t_drain_start) + ',' + str(temp_time) + ',\n'
							temp_log.write(delayed_trace)
					
						'''
						print("clock : %6d"               %(int(t_drain_start)) + "\t " + \
							  "t_available = %10f"        %(t_available)        + "\t " + \
							  "t_fill_sram_buffer = %10f" %(t_fill_sram_buffer) + "\t " + \
							  "delayed_time = %10f"       %(temp_time)
							  )
						'''

						# drain all the elements in the sram buffer
						sram.clear()

						t_fill_start	= t_drain_start
						t_drain_start   = clk
					#-----------------------------------------------------------------------------------------------------#
					# END IF_SRAM_FULL

					# Add the new element to sram
					sram.add(elems[e])
				#---------------------------------------------------------------------------------------------------------#
				# END IF_SRAM_RANGE
			#-------------------------------------------------------------------------------------------------------------#
			# END ELEM_FOR_LOOP
		#-----------------------------------------------------------------------------------------------------------------#
		# END IS_FROM_MEMORY
	#---------------------------------------------------------------------------------------------------------------------#
	# END SRAM_FOR_LOOP

	# IF_SRAM_REMAINED
	# Drain all the remained elements in the sram buffer
	#---------------------------------------------------------------------------------------------------------------------#
	if len(sram) > 0:

		# Generate the filling trace from time t_fill_start to t_drain_start
		cycles_needed = t_drain_start - t_fill_start

		t_available        = cycles_needed * time_per_cycle
		t_fill_sram_buffer = (len(sram)/read_bw) * time_per_cycle

		# if the filling the sram buffer time is larger than available time, add io delayed time
		temp_time = 0
		if t_fill_sram_buffer > t_available:
			temp_time     = t_fill_sram_buffer - t_available
			t_delayed_io += temp_time
			
			delayed_trace = str(t_drain_start) + ',' + str(temp_time) + ',\n'
			temp_log.write(delayed_trace)
	
			'''
			print("clock : %6d"               %(int(t_drain_start)) + "\t " + \
				  "t_available = %10f"        %(t_available)        + "\t " + \
				  "t_fill_sram_buffer = %10f" %(t_fill_sram_buffer) + "\t " + \
				  "delayed_time = %10f"       %(temp_time)
			)
			'''

		sram.clear()
	#---------------------------------------------------------------------------------------------------------------------#
	# END IF_SRAM_REMAINED

	sram_requests.close()
	temp_log.close()

	# Generate off-chip memory delay time trace file
	#---------------------------------------------------------------------------------------------------------------------#
	temp_log2       = open('temp_log.csv', "r")
	mem_delayed_log = open(mem_delayed_trace_file, "w")

	delayed_clk  = -1
	delayed_time = 0
	
	mem_delayed_trace = temp_log2.readline()

	if mem_delayed_trace:
		delayed_clk  = float(mem_delayed_trace.split(',')[0])
		delayed_time = float(mem_delayed_trace.split(',')[1])
 
	# GEN_TRACE_LOOP
	#---------------------------------------------------------------------------------------------------------------------#
	for c in range(0, int(clk) + 1):
		temp_clk     = float(c)
		temp_delayed = 0

		if(delayed_clk == temp_clk):
			temp_delayed += delayed_time
			
			mem_delayed_trace = temp_log2.readline()

			if mem_delayed_trace:
				delayed_clk  = float(mem_delayed_trace.split(',')[0])
				delayed_time = float(mem_delayed_trace.split(',')[1])
 
		trace = str(temp_clk) + ',' + str(temp_delayed) + ',\n'
		mem_delayed_log.write(trace)
	#---------------------------------------------------------------------------------------------------------------------#
	# END GEN_TRACE_LOOP
	
	temp_log2.close()
	mem_delayed_log.close()

	# delete temporary logging file
	cmd = "rm temp_log.csv"
	os.system(cmd)
	#---------------------------------------------------------------------------------------------------------------------#
	
	## Return value ##
	# t_sram_read : DNN calculation time
	# t_delayed_io: extra execution delay time due to off-chip memory bandwidth
	return t_sram_read, t_delayed_io
###########################################################################################################################



###########################################################################################################################
def calculate_dram_write_io_time(
		ofmap_sram_size = 64,
		data_width_bytes = 1,
		write_bw  = 10,
		precision = 4,
		clock_speed = 500000,
		sram_write_trace_file = "sram_write.csv",
		mem_delayed_read_trace_file = "mem_delayed_read.csv",

		is_last_layer = False
	):

#	print(ofmap_sram_size)
#	print(data_width_bytes)
#	print(write_bw)
#	print(sram_write_trace_file)
#	print(mem_delayed_read_trace_file)
	is_mem_write = False

	# Change sram size considering precision
	ofmap_sram_size *= 1024
	ofmap_sram_size /= precision
#	ofmap_sram_size /= 2 #considering double buffered
	
	# clock speed(KHz -> Hz)
	clock_speed *= 1000

	# seconds per cycle(us)
	time_per_cycle = (1/clock_speed)*1000*1000
	
	# Change memory bandwidth (bytes/sec -> bytes/cycle)
	# bandwidth(bytes per sec) / ( precision(byte) * clock speed(Hz = clocks per sec) )
	write_bw *= (1024*1024*1024)	# GB -> bytes
	write_bw /= precision
	write_bw /= clock_speed
	
	# Initialize io delay time and compute time
	t_delayed_io  = 0
	t_sram_write  = 0

	print("----------------------------------------------------------")
	print("SRAM size: \t" + str(int(ofmap_sram_size*precision)) + " bytes")
	print("write_bw : \t" + str(round(write_bw*precision,6)) + " bytes/cycle")
	print("----------------------------------------------------------")
	

	traffic = open(sram_write_trace_file, 'r')
	mem_delayed_log = open(mem_delayed_read_trace_file, 'r')

	delayed_clk  = 0
	delayed_time = 0
	last_clk     = 0
	clk          = 0

	sram_buffer  = [set(), set()]
	
	filling_buf	 = 0
	draining_buf = 1
	
	read_delay_trace = mem_delayed_log.readline()

	# SRAM_FOR_LOOP
	# read sram elements(data) cycle by cycle
	#---------------------------------------------------------------------------------------------------------------------#
	for row in traffic:
		elems = row.strip().split(',')
		elems = prune(elems)
		elems = [float(x) for x in elems]
		
		clk = elems[0]
		
		t_sram_write += time_per_cycle


		# If enough space is in the filling buffer
		# Keep filling the buffer
		#-----------------------------------------------------------------------------------------------------------------#
		if (len(sram_buffer[filling_buf]) + (len(elems) - 1) * data_width_bytes ) <= ofmap_sram_size:
			for i in range(1,len(elems)):
				sram_buffer[filling_buf].add(elems[i])
		#-----------------------------------------------------------------------------------------------------------------#
		

		# IF_FILLING_BUF_FULL
		# Filling buffer is full, spill the data to the other buffer
		#-----------------------------------------------------------------------------------------------------------------#
		else:
#			print("haha")
			# IF_DATA_IN_DRAIN_BUF
			# If there is data in the draining buffer
			# drain it
			#-------------------------------------------------------------------------------------------------------------#
			if len(sram_buffer[draining_buf]) > 0:
				# Calculate available time for draining the draining buffer
				delta_clks = clk - last_clk

				# ADD_READ_DELAY_TIME
				# Check more available time due to off-chip memory read delay time
				#---------------------------------------------------------------------------------------------------------#
				idle_delay_time = 0
				while read_delay_trace:
					
					delayed_elems = read_delay_trace.strip().split(',')
					delayed_elems = prune(delayed_elems)
					delayed_elems = [float(x) for x in delayed_elems]

					delayed_clk  = delayed_elems[0]
					delayed_time = delayed_elems[1]
						
					idle_delay_time += delayed_time
						
					if delayed_clk == clk:
						break

					read_delay_trace = mem_delayed_log.readline()
				#---------------------------------------------------------------------------------------------------------#
				# END ADD_READ_DELAY_TIME
							
				t_available  = delta_clks * time_per_cycle
				t_drain_sram_buffer = (len(sram_buffer[draining_buf]) / write_bw) * time_per_cycle
				
				'''
				print("last_clk = %7d"             %(last_clk)        + "\t " + \
					  "clk = %7d"                  %(clk)             + "\t " + \
					  "delayed_clk %7d"            %(delayed_clk)     + "\t " + \
					  "idle_delay_time = %10f"     %(idle_delay_time) + "\t " + \
					  "t_available = %10f"         %(idle_delay_time) + "\t " + \
					  "t_drain_sram_buffer = %10f" %(t_drain_sram_buffer)
				)
				'''

				t_available += idle_delay_time

				if t_drain_sram_buffer > t_available:
					temp_time     = t_drain_sram_buffer - t_available
					t_delayed_io += temp_time 
				
				# Drain the data
				sram_buffer[draining_buf].clear()

				is_mem_write = True
			#-------------------------------------------------------------------------------------------------------------#
			# END IF_DATA_IN_DRAIN_BUF			

			#Swap the ids for drain buffer and fill buffer
			tmp			 = draining_buf
			draining_buf = filling_buf
			filling_buf	 = tmp

			#Set the last clk value
			last_clk = clk
			
			#Fill the new data now
			for i in range(1,len(elems)):
				sram_buffer[filling_buf].add(elems[i])
		#-----------------------------------------------------------------------------------------------------------------#
		# END IF_FILLING_BUF_FULL
	#---------------------------------------------------------------------------------------------------------------------#
	# END SRAM_FOR_LOOP
	
	#Drain the last fill buffer
	#---------------------------------------------------------------------------------------------------------------------#
	if (len(sram_buffer[draining_buf]) > 0) and ((is_mem_write is True) or (is_last_layer is True)):
		delta_clks = clk - last_clk

		t_available = delta_clks * time_per_cycle
		t_drain_sram_buffer = (len(sram_buffer[draining_buf]) / write_bw) * time_per_cycle
		
		t_delayed_io += t_drain_sram_buffer
		
		# Drain the data
		sram_buffer[draining_buf].clear()
	#---------------------------------------------------------------------------------------------------------------------#

	#Drain the last drain buffer
	#---------------------------------------------------------------------------------------------------------------------#
	if (len(sram_buffer[filling_buf]) > 0) and ((is_mem_write is True) or (is_last_layer is True)):
		delta_clks = clk - last_clk
		
		t_available = delta_clks * time_per_cycle
		t_drain_sram_buffer = (len(sram_buffer[filling_buf]) / write_bw) * time_per_cycle
		
		t_delayed_io += t_drain_sram_buffer
	
		# Drain the data
		sram_buffer[filling_buf].clear()
	#---------------------------------------------------------------------------------------------------------------------#

#	print("sram_write_time = \t" + str(t_sram_write))
#	print("t_delayed_io    = \t" + str(t_delayed_io))
	
	traffic.close()
	mem_delayed_log.close()

	## Return value ##
	# t_sram_write : time consumed to store ofmap in the sram buffer
	# t_delayed_io : time consumed to move ofmap to off-chip memory
	# is_mem_write : check whether there was intermediate ofmap off-chip memory write
	return t_sram_write, t_delayed_io, is_mem_write
###########################################################################################################################

