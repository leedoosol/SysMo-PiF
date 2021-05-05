import os
import math
from beautifultable import BeautifulTable
from mem import dram_trace as dram

from mem import calculate_dram_delay as dram_delay
from mem import calculate_flash_delay as flash_delay

from sram import sram_traffic_os as sram_os
from sram import sram_traffic_ws as sram_ws
from sram import sram_traffic_is as sram_is

import utils as ut

global_is_mem_write = True

###################################################################################################################
def gen_all_traces(
		mem_config,
		acc_config,

		ifmap_h      = 7,
		ifmap_w      = 7,
		filt_h       = 3,
		filt_w       = 3,
		num_channels = 3,
		num_filt     = 8,
		strides      = 1,

		word_size_bytes = 1,
		
		sram_read_trace_file   = "sram_read.csv",
		sram_write_trace_file  = "sram_write.csv",
		sram_integrated_trace_file = "sram_integrated.csv",
		
		mem_filter_trace_file = "mem_filter_read.csv",
		mem_ifmap_trace_file  = "mem_ifmap_read.csv",
		mem_ofmap_trace_file  = "mem_ofmap_write.csv",

		mem_delay_read_trace_file  = "mem_delayed_read.csv",

		is_first_layer = True,
		is_last_layer = False,
	):

	global global_is_mem_write

	####  Initialize local variables   ####
	sram_cycles = 0 # SRAM starting cycle
	util		= 0 # Utilization of PEs

	#### Initialize timing parameters #####
	t_ifmap_sram_read      = 0
	t_ifmap_delayed_io     = 0

	t_filter_sram_read     = 0
	t_filter_delayed_io    = 0

	t_ofmap_sram_write     = 0
	t_ofmap_delayed_io     = 0

	ifmap_sram_read_bytes  = 0
	filter_sram_read_bytes = 0
	ofmap_sram_write_bytes = 0

	ifmap_mem_read_bytes   = 0
	filter_mem_read_bytes  = 0
	ofmap_mem_write_bytes  = 0

	# GEN_SRAM_TRACE
	#### Generate SRAM trace using dataflow(OS, WS, IS) ####
	print("Generating traces and bw numbers")
	#-------------------------------------------------------------------------------------------------------------#

	#### Output Stationary ####
	if acc_config.dataflow == 'os':
		sram_cycles, util = \
			sram_os.sram_traffic(
				dimension_rows        = acc_config.ar_h_min,
				dimension_cols        = acc_config.ar_w_min,

				ifmap_h               = ifmap_h,
				ifmap_w               = ifmap_w,
				filt_h                = filt_h,
				filt_w                = filt_w,
				num_channels          = num_channels,
				strides               = strides,
				num_filt              = num_filt,
				
				filt_base             = acc_config.filter_offset,
				ifmap_base            = acc_config.ifmap_offset,
				ofmap_base            = acc_config.ofmap_offset,

				sram_read_trace_file  = sram_read_trace_file,
				sram_write_trace_file = sram_write_trace_file
			)


	#### Weight Stationary ####
	elif acc_config.dataflow == 'ws':
		sram_cycles, util = \
			sram_ws.sram_traffic(
				dimension_rows        = acc_config.ar_h_min,
				dimension_cols        = acc_config.ar_w_min,
				
				ifmap_h               = ifmap_h,
				ifmap_w               = ifmap_w,
				filt_h                = filt_h,
				filt_w                = filt_w,
				num_channels          = num_channels,
				strides               = strides,
				num_filt              = num_filt,
				
				ofmap_base            = acc_config.ofmap_offset,
				filt_base             = acc_config.filter_offset,
				ifmap_base            = acc_config.ifmap_offset,
				
				sram_read_trace_file  = sram_read_trace_file,
				sram_write_trace_file = sram_write_trace_file
			)


	#### Input Stationary ####
	elif acc_config.dataflow == 'is':
		sram_cycles, util = \
			sram_is.sram_traffic(
				dimension_rows        = acc_config.ar_h_min,
				dimension_cols        = acc_config.ar_w_min,

				ifmap_h               = ifmap_h,
				ifmap_w               = ifmap_w,
				filt_h                = filt_h,
				filt_w                = filt_w,
				num_channels          = num_channels,
				strides               = strides,
				num_filt              = num_filt,
				
				ofmap_base            = acc_config.ofmap_offset,
				filt_base             = acc_config.filter_offset,
				ifmap_base            = acc_config.ifmap_offset,

				sram_read_trace_file  = sram_read_trace_file,
				sram_write_trace_file = sram_write_trace_file
			)


	'''
	# Integrate SRAM read/write trace into one file considering cycles
	
	ut.integrate_sram_read_write(
				dimension_rows = acc_config.ar_h_min,
				dimension_cols = acc_config.ar_w_min,

				sram_read_trace_file  = sram_read_trace_file,
				sram_write_trace_file = sram_write_trace_file,

				sram_integrated_file = sram_integrated_trace_file
	)
	'''

	#-------------------------------------------------------------------------------------------------------------#
	# END GEN_SRAM_TRACE



	# GEN_MEMORY_TRACE
	#### Generating DRAM trace ####
	#    -  Considering idle required bandwidth for DNN processing with configured accelerator
	#       (No off-chip memory access delay considered)
	#-------------------------------------------------------------------------------------------------------------#
	
	# off-chip memory trace for input feature map
	ifmap_sram_read_bytes, ifmap_mem_read_bytes = \
		dram.dram_trace_read_v2(
			sram_sz               = acc_config.isram_min,
			min_addr              = acc_config.ifmap_offset,
			max_addr              = acc_config.filter_offset,
			
			precision             = acc_config.precision,
		
			sram_trace_file       = sram_read_trace_file,
			mem_trace_file        = mem_ifmap_trace_file
		)
	
	# off-chip memory trace for filter
	filter_sram_read_bytes, filter_mem_read_bytes = \
		dram.dram_trace_read_v2(
			sram_sz               = acc_config.fsram_min,
			min_addr              = acc_config.filter_offset,
			max_addr              = acc_config.ofmap_offset,
	
			precision             = acc_config.precision,
		
			sram_trace_file       = sram_read_trace_file,
			mem_trace_file        = mem_filter_trace_file
		)

	# off-chip trace for output feature map
	ofmap_sram_write_bytes, ofmap_mem_write_bytes = \
		dram.dram_trace_write(
			ofmap_sram_size       = acc_config.osram_min,
			precision             = acc_config.precision,

			sram_write_trace_file = sram_write_trace_file,
			mem_write_trace_file  = mem_ofmap_trace_file
		)
	#-------------------------------------------------------------------------------------------------------------#
	# END GEN_MEMORY_TRACE


	# GEN_MEMORY_DELAY_TRACE
	##### Calculate MEM delayed time(DRAM, NAND interface, flash) #####
	#-------------------------------------------------------------------------------------------------------------#
	
	# memory delay trace for input feature map
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	ifmap_memory         = acc_config.ifmap_mem
	ifmap_flash_dense    = acc_config.ifmap_mem_flash_dense

	## Consider the previous layer intermediate output data path
	if is_first_layer is False:
		ifmap_memory      = acc_config.inter_ofmap_mem
		ifmap_flash_dense = acc_config.inter_ofmap_mem_flash_dense

	## if the previous layer output was written to external memory,
	if global_is_mem_write:
		print("(1) generating ifmap off-chip memory delay time....\t[ " + str(ifmap_memory) + " ]", end="")
		if ifmap_memory == "FLASH":
			print("[ " + ifmap_flash_dense + " ]")
		else:
			print("")
	## if the not,(written in the external memory)
	else:
		print("(1) generating ifmap off-chip memory delay time....\t[ SRAM Buffer ]: NO EXTERNAL MEMORY ACCESS")
		ifmap_mem_read_bytes = 0 # no external memory read
	

	if ifmap_memory == 'DRAM':
		t_ifmap_sram_read, t_ifmap_delayed_io = \
			dram_delay.calculate_dram_read_io_time(
					sram_sz                = acc_config.isram_min,
					word_sz_bytes          = word_size_bytes,

					min_addr               = acc_config.ifmap_offset,
					max_addr               = acc_config.filter_offset,
			
					precision              = acc_config.precision,
					clock_speed            = acc_config.clock_speed,
					read_bw                = mem_config.dram_bw,
			
					sram_trace_file        = sram_read_trace_file,
					mem_delayed_trace_file = "temp_log1.csv",

					is_from_mem            = global_is_mem_write
			)
	elif ifmap_memory == 'NANDINTERFACE':
		t_ifmap_sram_read, t_ifmap_delayed_io = \
			dram_delay.calculate_dram_read_io_time(
					sram_sz                = acc_config.isram_min,
					word_sz_bytes          = word_size_bytes,
					
					min_addr               = acc_config.ifmap_offset,
					max_addr               = acc_config.filter_offset,
	
					precision              = acc_config.precision,
					clock_speed            = acc_config.clock_speed,
					read_bw                = mem_config.nand_interface_bw,
			
					sram_trace_file        = sram_read_trace_file,
					mem_delayed_trace_file = "temp_log1.csv",

					is_from_mem            = global_is_mem_write
			)
	elif ifmap_memory == 'FLASH':
		t_ifmap_sram_read, t_ifmap_delayed_io = \
			flash_delay.calculate_flash_read_io_time(
					sram_sz                = acc_config.isram_min,
					word_sz_bytes          = word_size_bytes,
					min_addr               = acc_config.ifmap_offset,
					max_addr               = acc_config.filter_offset,
	
					precision              = acc_config.precision,
					clock_speed            = acc_config.clock_speed,
			
					mem_config             = mem_config,
					flash_dense            = ifmap_flash_dense,
#					flash_page_size        = mem_config.flash_page_size,
#					flash_plane_num        = mem_config.flash_plane_num,

					sram_trace_file        = sram_read_trace_file,
					mem_delayed_trace_file ="temp_log1.csv",

					is_from_mem            = global_is_mem_write
			)
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	
	# memory delay trace for filter
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	filter_memory      = acc_config.filter_mem
	filter_flash_dense = acc_config.filter_mem_flash_dense

	print("(2) generating filter off-chip memory delay time...\t[ " + str(filter_memory) + " ]", end="")
	if filter_memory == "FLASH":
		print("[ " + filter_flash_dense + " ]")
	else:
		print("")


	if filter_memory == 'DRAM':
		t_filter_sram_read, t_filter_delayed_io = \
			dram_delay.calculate_dram_read_io_time(
					sram_sz         = acc_config.fsram_min,
					word_sz_bytes   = word_size_bytes,
					
					min_addr        = acc_config.filter_offset,
					max_addr        = acc_config.ofmap_offset,
	
					precision       = acc_config.precision,
					clock_speed     = acc_config.clock_speed,
					read_bw         = mem_config.dram_bw,

					sram_trace_file = sram_read_trace_file,
					mem_delayed_trace_file = "temp_log2.csv"
			)
	elif filter_memory == 'NANDINTERFACE':
		t_filter_sram_read, t_filter_delayed_io = \
			dram_delay.calculate_dram_read_io_time(
					sram_sz         = acc_config.fsram_min,
					word_sz_bytes   = word_size_bytes,
					
					min_addr        = acc_config.filter_offset,
					max_addr        = acc_config.ofmap_offset,
			
					precision       = acc_config.precision,
					clock_speed     = acc_config.clock_speed,		
					read_bw         = mem_config.nand_interface_bw,

					sram_trace_file = sram_read_trace_file,
					mem_delayed_trace_file = "temp_log2.csv"
			)
	elif filter_memory == 'FLASH':
		t_filter_sram_read, t_filter_delayed_io = \
			flash_delay.calculate_flash_read_io_time(
					sram_sz         = acc_config.fsram_min,
					word_sz_bytes   = word_size_bytes,
					
					min_addr        = acc_config.filter_offset,
					max_addr        = acc_config.ofmap_offset,

					precision       = acc_config.precision,
					clock_speed     = acc_config.clock_speed,
			
					mem_config      = mem_config,
					flash_dense     = filter_flash_dense,
			
					sram_trace_file = sram_read_trace_file,
					mem_delayed_trace_file = "temp_log2.csv"
			)
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#

	'''
	# LOG: ifmap/filter external memory access delayed time
	print("t_ifmap_delayed_io : \t" + str(t_ifmap_delayed_io))
	print("t_filter_delayed_io: \t" + str(t_filter_delayed_io))
	'''

	## generate total external memory read delay time trace
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	t_ifmap_delayed_io, t_filter_delayed_io = \
		ut.merge_two_delay_log_file(
				mem_delayed_file_a = "temp_log1.csv",
				mem_delayed_file_b = "temp_log2.csv",
				result_file = mem_delay_read_trace_file
		)

	## delete temporary read delay log file
	cmd = "rm temp_log1.csv"
	os.system(cmd)

	cmd = "rm temp_log2.csv"
	os.system(cmd)
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	

	# memory delay trace for output feature map
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	ofmap_memory      = acc_config.ofmap_mem
	ofmap_flash_dense = acc_config.ofmap_mem_flash_dense

	if is_last_layer is False:
		ofmap_memory = acc_config.inter_ofmap_mem
		ofmap_flash_dense = acc_config.inter_ofmap_mem_flash_dense

	print("(3) generating ofmap off-chip memory delay time....\t[ " + str(ofmap_memory) + " ]", end="")
	if ofmap_memory == "FLASH":
		print("[ " + ofmap_flash_dense + " ]")
	else:
		print("")

	if ofmap_memory == 'DRAM':
		t_ofmap_sram_write, t_ofmap_delayed_io, global_is_mem_write = \
			dram_delay.calculate_dram_write_io_time(
					ofmap_sram_size       = acc_config.osram_min,
					data_width_bytes      = word_size_bytes,
					
					write_bw              = mem_config.dram_bw,
					
					precision             = acc_config.precision,
					clock_speed           = acc_config.clock_speed,
					
					sram_write_trace_file = sram_write_trace_file,
					mem_delayed_read_trace_file = mem_delay_read_trace_file,

					is_last_layer = is_last_layer
			)
	elif ofmap_memory == 'NANDINTERFACE':
		t_ofmap_sram_write, t_ofmap_delayed_io, global_is_mem_write = \
			dram_delay.calculate_dram_write_io_time(
					ofmap_sram_size       = acc_config.osram_min,
					data_width_bytes      = word_size_bytes,
					
					write_bw              = mem_config.nand_interface_bw,
					
					precision             = acc_config.precision,
					clock_speed           = acc_config.clock_speed,
			
					sram_write_trace_file = sram_write_trace_file,
					mem_delayed_read_trace_file = mem_delay_read_trace_file,

					is_last_layer = is_last_layer
			)
	elif ofmap_memory == 'FLASH':
		t_ofmap_sram_write, t_ofmap_delayed_io, global_is_mem_write = \
			flash_delay.calculate_flash_write_io_time(
					ofmap_sram_size       = acc_config.osram_min,
					data_width_bytes      = word_size_bytes,
			
					precision             = acc_config.precision,
					clock_speed           = acc_config.clock_speed,
					
					mem_config            = mem_config,
					flash_dense           = ofmap_flash_dense,

					sram_write_trace_file = sram_write_trace_file,
					mem_delayed_read_trace_file = mem_delay_read_trace_file,

					is_last_layer = is_last_layer
		)

	## if the ofmap is not written to external memory
	if (not global_is_mem_write) and (not is_last_layer):
		ofmap_mem_write_bytes = 0
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
	
	'''
	ifmap_sram_read_bytes  *= acc_config.precision
	filter_sram_read_bytes *= acc_config.precision
	ofmap_sram_write_bytes *= acc_config.precision

	ifmap_mem_read_bytes   *= acc_config.precision
	filter_mem_read_bytes  *= acc_config.precision
	ofmap_mem_write_bytes  *= acc_config.precision
	'''
	
	#-------------------------------------------------------------------------------------------------------------#
	# END GEN_MEORY_DELAY_TRACE

	# util and sram_cycles are return values of sram_traffic module
	print("\n---------------------------ACCELRATOR REULTS---------------------------------")
	print("Average utilization: %10f"  %(util) + " %")
	print("Cycles for compute : %10d"  %(int(sram_cycles)) + " cycles")
	print("-----------------------------------------------------------------------------\n")
	# Get detail_logs and required bandwidth
	bw_numbers, detailed_log  = gen_bw_numbers(
									 mem_ifmap_trace_file,
									 mem_filter_trace_file,
									 mem_ofmap_trace_file,
									 sram_write_trace_file,
									 sram_read_trace_file,
									 precision = acc_config.precision
								 )
	
	# Get energy consumption(compute, sram access, external memory access)
	energy_log = gen_energy_consumption(
				dimension_rows        = acc_config.ar_h_min,
				dimension_cols        = acc_config.ar_w_min,

				ifmap_sram_sz         = acc_config.isram_min,
				filter_sram_sz        = acc_config.fsram_min,
				ofmap_sram_sz         = acc_config.osram_min,

				ifmap_sram_read_bytes  = ifmap_sram_read_bytes,
				filter_sram_read_bytes = filter_sram_read_bytes,
				ofmap_sram_write_bytes = ofmap_sram_write_bytes,

				ifmap_mem_read_bytes   = ifmap_mem_read_bytes,
				filter_mem_read_bytes  = filter_mem_read_bytes,
				ofmap_mem_write_bytes  = ofmap_mem_write_bytes,

				ifmap_mem              = ifmap_memory,
				ifmap_flash_dense      = ifmap_flash_dense,
	
				filter_mem             = filter_memory,
				filter_flash_dense     = filter_flash_dense,
	
				ofmap_mem              = ofmap_memory,
				ofmap_flash_dense      = ofmap_flash_dense,

				flash_page_size        = mem_config.flash_page_size,
	
				util        = float(util),
				sram_cycles = int(sram_cycles),
	
				precision   = acc_config.precision
			)

	# Get detail time parameter
	time_param_log = gen_time_parameters(
						sram_cycles = sram_cycles,
			
						t_ifmap_delayed_io  = t_ifmap_delayed_io,	
						t_filter_delayed_io = t_filter_delayed_io,
						t_ofmap_delayed_io  = t_ofmap_delayed_io,

						acc_config = acc_config,
					 )

	#### return values ####
	# util		    : utilization of PEs for DNN inference				     (return from sram_traffic module)
	# sram_cycles   : total number of sram cycles(sram_read+sram_write)	     (return from sram_traffic module)
	# bw_numbers    : bandwidth for DRAM, SRAM access (in/output map+ filter)(return from gen_bw_numbers module)
	# detailed_log  : detailed logs(ex. DRAM start cycle, total read bytes..)(return from gen_bw_numbers module)
	# time_param_log: time logs(computation, off-chip memory delayed time,..)(return from gen_time_parameters)
	return bw_numbers, detailed_log, util, sram_cycles, time_param_log, energy_log
###################################################################################################################


###################################################################################################################
def get_sram_energy(
		sram_size = 0
#		precision = 4
	):

	## SRAM Energy Model
	## Reference:DNN Dataflow Choice is Overrated, Stanford, arXiv preprint arXiv:1809.94979 (2018)
	## energy consumption for 1 byte sram access(nJ)
	energy_sram = 0.003 #default SRAM Size = 32KB
	
	if   sram_size == 32:
		energy_sram = 0.003
	elif sram_size == 64:
		energy_sram = 0.0045
	elif sram_size == 128:
		energy_sram = 0.00675
	elif sram_size == 256:
		energy_sram = 0.010125
	elif sram_size == 512:
		energy_sram = 0.0151875
	else:
		# just temporary sram energy consideration
		# f(x) = 2*e^(0.4055)(x/32) //x=sram_size
		e            = math.exp(1)
		energy_sram  = pow(e,( 0.4055*(sram_size/32)))
		energy_sram *= 2
		energy_sram /= 1000 # pJ -> nJ

#	energy_sram *= precision

	return energy_sram
###################################################################################################################
def get_mem_energy(
		mem         = 'DRAM',
		flash_dense = 'MLC',
		access_type = 'R',
	):
	
	## External Memory(DRAM, FLASH) Access Power Model
	## DRAM  power reference: DNN Dataflow Choice is Overrated, Standford, arXiv preprint arXiv:1809.04070, 2018
	## Flash power reference: Modeling Power Consumption of NAND Flash Memories Using FlashPower,
	##                        IEEE Transactions on Computer-Aided Design of Intergrated Circuits and Systems, 2013
	## energy consumption for 1 byte access

	# default energy(DRAM)
	energy_mem        = (0.02) * 8
	energy_dram       = (0.02) * 8

	# flash power consumption for different density

	# SLC
	energy_flash_slc_read  = 0.24
	energy_flash_slc_write = 3.47

	# MLC
	energy_flash_mlc_read  = 0.48
	energy_flash_mlc_write = 8.78

	# TLC
	energy_flash_tlc_read  = 0.97
	energy_flash_tlc_write = 17.57


	if mem == 'DRAM':
		energy_mem = energy_dram
	elif mem == 'NANDINTERFACE' or mem == 'FLASH':

		if flash_dense == 'SLC':
			if access_type == 'R':
				energy_mem = energy_flash_slc_read
			elif access_type == 'W':
				energy_mem = energy_flash_slc_write
	
		elif flash_dense == 'MLC':
			if access_type == 'R':
				energy_mem = energy_flash_mlc_read
			elif access_type == 'W':
				energy_mem = energy_flash_mlc_write

		elif flash_dense == 'TLC':
			if access_type == 'R':
				energy_mem = energy_flash_tlc_read
			elif access_type == 'W':
				energy_mem = energy_flash_tlc_write


	return energy_mem
###################################################################################################################
def gen_energy_consumption(
		dimension_rows        = 0,
		dimension_cols        = 0,

		ifmap_sram_sz         = 0,
		filter_sram_sz        = 0,
		ofmap_sram_sz         = 0,

		ifmap_sram_read_bytes  = 0,
		filter_sram_read_bytes = 0,
		ofmap_sram_write_bytes = 0,

		ifmap_mem_read_bytes   = 0,
		filter_mem_read_bytes  = 0,
		ofmap_mem_write_bytes  = 0,

		ifmap_mem              = 'DRAM',
		ifmap_flash_dense      = 'MLC',

		filter_mem             = 'DRAM',
		filter_flash_dense     = 'MLC',

		ofmap_mem              = 'DRAM',
		ofmap_flash_dense      = 'MLC',

		sram_cycles = 0,
		util        = 0.0,

		flash_page_size = 16,

		precision      = 4,
		precision_type = 'float'
	):

	flash_page_size *= 1024

	## Computing Energy Model
	## Reference: 1.1 Computing's Energy Problem(and what we can do it about it), Standford, ISSCC 2014
	'''
	       [Integer Operation]                          [Floating Point Operation]
	+-----------+-----------+--------+              +-----------+-----------+--------+ 
	| Operation | Precision | Energy |              | Operation | Precision | Energy |
	+-----------+-----------+--------+              +-----------+-----------+--------+
	|           |     8 bit | 0.03pJ |              |           |    16 bit |  0.4pJ |
	|    ADD    +-----------+--------|              |    ADD    +-----------+--------|
	|           |    32 bit |  0.1pJ |              |           |    32 bit |  0.9pJ |
	+-----------+-----------+--------+              +-----------+-----------+--------+
	|           |     8 bit |  0.2pJ |              |           |    16 bit |  1.1pJ |
	|    MULT   +-----------+--------|              |    MULT   +-----------+--------|
	|           |    32 bit |  3.1pJ |              |           |    32 bit |  3.7pJ |
	+-----------+-----------+--------+              +-----------+-----------+--------+

	'''
	energy_inst_add  = 0.0009 # default FADD  32BIT(nJ)
	energy_inst_mult = 0.0037 # default FMULT 32BIT(nJ)

	if precision_type == 'float':
		# precision = 16bit
		if precision == 2:
			energy_inst_add  = 0.0004
			energy_inst_mult = 0.0011
		# precision = 32bit:
		elif precision == 4:
			energy_inst_add  = 0.0009
			energy_inst_mult = 0.0037

	elif precision_type == 'int':
		# precision = 8bit
		if precision == 1:
			energy_inst_add  = 0.00003
			energy_inst_mult = 0.0002
		# precision = 32bit
		elif precision == 4:
			energy_inst_add  = 0.0001
			energy_inst_mult = 0.0031

	energy_per_pe = energy_inst_add + energy_inst_mult

	total_compute_cnt = int((dimension_rows * dimension_cols) * (util/100) * sram_cycles)
	energy_compute    = (energy_per_pe) * total_compute_cnt

	## SRAM Access Energy Model
	energy_isram_per_data = get_sram_energy(int(ifmap_sram_sz))
	energy_fsram_per_data = get_sram_energy(int(filter_sram_sz))
	energy_osram_per_data = get_sram_energy(int(ofmap_sram_sz))

	energy_isram = energy_isram_per_data * ifmap_sram_read_bytes
	energy_fsram = energy_fsram_per_data * filter_sram_read_bytes
	energy_osram = energy_osram_per_data * ofmap_sram_write_bytes

	## External Memory Access Energy Model
	energy_dram       = (0.02) * 8

	energy_imem_read  = get_mem_energy(
			mem         = ifmap_mem,
			flash_dense = ifmap_flash_dense,
			access_type = 'R'
			)
	energy_fmem_read  = get_mem_energy(
			mem         = filter_mem,
			flash_dense = filter_flash_dense,
			access_type = 'R'
			)
	energy_omem_write = get_mem_energy(
			mem         = ofmap_mem,
			flash_dense = ofmap_flash_dense,
			access_type = 'W'

			)
	
	# Consider flash page granularity
	if ifmap_mem == 'FLASH' or ifmap_mem == 'NANDINTERFACE':
		if ifmap_mem_read_bytes % flash_page_size != 0:
			ifmap_mem_read_bytes = (ifmap_mem_read_bytes // flash_page_size) * flash_page_size
			ifmap_mem_read_bytes += flash_page_size
	if filter_mem == 'FLASH' or filter_mem == 'NANDINTERFACE':
		if filter_mem_read_bytes % flash_page_size != 0:
			filter_mem_read_bytes = (filter_mem_read_bytes // flash_page_size) * flash_page_size
			filter_mem_read_bytes += flash_page_size
	if ofmap_mem == 'FLASH' or ofmap_mem == 'NANDINTERFACE':
		if ofmap_mem_write_bytes % flash_page_size != 0:
			ofmap_mem_write_bytes = (ofmap_mem_write_bytes // flash_page_size) * flash_page_size
			ofmap_mem_write_bytes += flash_page_size

	energy_imem = energy_imem_read  * ifmap_mem_read_bytes
	energy_fmem = energy_fmem_read  * filter_mem_read_bytes
	energy_omem = energy_omem_write * ofmap_mem_write_bytes

	total_sram_energy = energy_isram + energy_fsram + energy_osram
	total_mem_energy  = energy_imem + energy_fmem + energy_omem
	total_energy      = energy_compute + total_sram_energy + total_mem_energy

	print("\n--------------------------ENERGY CONSUMPTION---------------------------------")
	print("IFMAP MEM: \t" + str(ifmap_mem))
	print("FILTER MEM:\t" + str(filter_mem))
	print("OFMAP MEM: \t" + str(ofmap_mem))
	print("Total Energy Consumption = %f" %(total_energy) + ' nJ')
	table = BeautifulTable()

	table.columns.header = ["Parameter", "Total Access", "Energy Consumption"]
	table.rows.append(["Computation", str(total_compute_cnt),                       str(energy_compute) + " nJ"])
	table.rows.append(["IFMAP  SRAM Read", str(ifmap_sram_read_bytes) + " bytes",     str(energy_isram) + " nJ"])
	table.rows.append(["FILTER SRAM Read", str(filter_sram_read_bytes) + " bytes",    str(energy_fsram) + " nJ"])
	table.rows.append(["OFMAP  SRAM Write", str(ofmap_sram_write_bytes) + " bytes",   str(energy_osram) + " nJ"])
	table.rows.append(["IFMAP  MEM Read", str(ifmap_mem_read_bytes) + " bytes",        str(energy_imem) + " nJ"])
	table.rows.append(["FILTER MEM Read", str(filter_mem_read_bytes) + " bytes",       str(energy_fmem) + " nJ"])
	table.rows.append(["OFMAP  MEM write", str(ofmap_mem_write_bytes) + " bytes",      str(energy_omem) + " nJ"])

	table.set_style(BeautifulTable.STYLE_GRID)
	table.columns.padding_right['Parameter']         = 4
	table.columns.padding_left['Total Access']       = 9
	table.columns.padding_left['Energy Consumption'] = 9
	table.columns.alignment["Parameter"]             = BeautifulTable.ALIGN_LEFT
	table.columns.alignment["Total Access"]          = BeautifulTable.ALIGN_RIGHT
	table.columns.alignment['Energy Consumption']    = BeautifulTable.ALIGN_RIGHT
	table.columns.header.alignment = BeautifulTable.ALIGN_CENTER
	print(table)
	print("-----------------------------------------------------------------------------\n")
	'''
	print("----------------------------------------------------------------------")
	print("|          parameter         |        value     | energy consumption|")
	print("----------------------------------------------------------------------")
	print("| Computation                | %10d" %(total_compute_cnt)        + "     \t|" + "%15f" %(energy_compute) + " nJ |")
	print("| IFMAP  SRAM  Read          | %10d" %(ifmap_sram_read_bytes)   + "bytes \t|" + "%15f" %(energy_isram)   + " nJ |")
	print("| FILTER SRAM  Read          | %10d" %(filter_sram_read_bytes)  + "bytes \t|" + "%15f" %(energy_fsram)   + " nJ |")
	print("| OFMAP  SRAM  Write         | %10d" %(ofmap_sram_write_bytes)  + "bytes \t|" + "%15f" %(energy_osram)   + " nJ |")
	print("| IFMAP  MEM   Read          | %10d" %(ifmap_mem_read_bytes)    + "bytes \t|" + "%15f" %(energy_imem)    + " nJ |")
	print("| FILTER MEM   Read          | %10d" %(filter_mem_read_bytes)   + "bytes \t|" + "%15f" %(energy_fmem)    + " nJ |")
	print("| OFMAP  MEM   Write         | %10d" %(ofmap_mem_write_bytes)   + "bytes \t|" + "%15f" %(energy_omem)    + " nJ |")
	print("----------------------------------------------------------------------")
	print("| Total Energy Consumption   = %f" %(total_energy) + " nJ")
	print("----------------------------------------------------------------------")
	'''
	detail_energy_log = str(total_compute_cnt)      + ',' + \
						str(ifmap_sram_read_bytes)  + ',' + \
						str(filter_sram_read_bytes) + ',' + \
						str(ofmap_sram_write_bytes) + ',' + \
						str(ifmap_mem_read_bytes)   + ',' + \
						str(filter_mem_read_bytes)  + ',' + \
						str(ofmap_mem_write_bytes)  + ',' + \
						str(energy_compute)         + ',' + \
						str(energy_isram)           + ',' + \
						str(energy_fsram)           + ',' + \
						str(energy_osram)           + ',' + \
						str(energy_imem)            + ',' + \
						str(energy_fmem)            + ',' + \
						str(energy_omem)            + ',' + \
						str(total_energy)


	return detail_energy_log
###################################################################################################################


###################################################################################################################
def gen_time_parameters(
		sram_cycles,
		
		t_ifmap_delayed_io,		
		t_filter_delayed_io,
		t_ofmap_delayed_io,

		acc_config,
	):

	# clock speed: MHz->Hz
	clock_speed = acc_config.clock_speed * 1000

	# seconds per cycle(us)
	time_per_cycle = (1/clock_speed)*1000*1000

	# total compute time
	t_total_compute = float(sram_cycles) * time_per_cycle

	# total delayed time
	t_total_delayed_io = t_ifmap_delayed_io + t_filter_delayed_io + t_ofmap_delayed_io

	# total execution time
	t_total_execute = t_total_compute + t_total_delayed_io

	# Calulate each execution time ratio
	compute_ratio = (t_total_compute    /t_total_execute)*100
	delayed_ratio = (t_total_delayed_io /t_total_execute)*100
	ifmap_ratio   = (t_ifmap_delayed_io /t_total_execute)*100
	filter_ratio  = (t_filter_delayed_io/t_total_execute)*100
	ofmap_ratio   = (t_ofmap_delayed_io /t_total_execute)*100
	
	
	table = BeautifulTable()

	table.columns.header = ["Timing Parameter", "time(us)", "ratio(%)"]
	table.rows.append(["total   execution time",     str(round(t_total_execute,6)), '100,0'])
	table.rows.append(["accel   computing time",     str(round(t_total_compute,6)), str(round(compute_ratio,2))])
	table.rows.append(["IFMAP  read delay time",  str(round(t_ifmap_delayed_io,6)),   str(round(ifmap_ratio,2))])
	table.rows.append(["FILTER read delay time", str(round(t_filter_delayed_io,6)),  str(round(filter_ratio,2))])
	table.rows.append(["OFMAP write delay time",  str(round(t_ofmap_delayed_io,6)),   str(round(ofmap_ratio,2))])

	table.set_style(BeautifulTable.STYLE_GRID)
	table.columns.padding_right['Timing Parameter'] = 4
	table.columns.padding_left['time(us)']        = 14
	table.columns.padding_left['ratio(%)']        = 14
	table.columns.alignment["Timing Parameter"] = BeautifulTable.ALIGN_LEFT
	table.columns.alignment["time(us)"]         = BeautifulTable.ALIGN_RIGHT
	table.columns.alignment['ratio(%)']         = BeautifulTable.ALIGN_RIGHT
	table.columns.header.alignment = BeautifulTable.ALIGN_CENTER

	print("\n--------------------------PERFORMANCE BREAKDOWN------------------------------")
	print(table)
	print("-----------------------------------------------------------------------------\n")
	

	'''	
	print("----------------------------------------------------------------------")
	print("|      timing parameter      |      time(us)" + "\t|      ratio(%)     |")
	print("----------------------------------------------------------------------")
	print("| total  execution  time     | %15f" %(t_total_execute)     + "\t| %15f" %(100.0)         +"   |")
	print("| accel. computing  time     | %15f" %(t_total_compute)     + "\t| %15f" %(compute_ratio) +"   |")
	print("| IFMAP  read delay time     | %15f" %(t_ifmap_delayed_io)  + "\t| %15f" %(ifmap_ratio)   +"   |")
	print("| FILTER read delay time     | %15f" %(t_filter_delayed_io) + "\t| %15f" %(filter_ratio)  +"   |")
	print("| OFMAP write delay time     | %15f" %(t_ofmap_delayed_io)  + "\t| %15f" %(ofmap_ratio)   +"   |")
	print("----------------------------------------------------------------------")
	'''
	time_param_log = str(t_total_execute)     + ',' + \
				     str(t_total_compute)     + ',' + \
				     str(t_total_delayed_io)  + ',' + \
				     str(t_ifmap_delayed_io)  + ',' + \
				     str(t_filter_delayed_io) + ',' + \
					 str(t_ofmap_delayed_io)  + ',' + \
				     str(compute_ratio)       + ',' + \
				     str(delayed_ratio)       + ',' + \
					 str(ifmap_ratio)         + ',' + \
					 str(filter_ratio)        + ',' + \
					 str(ofmap_ratio)
	

	return time_param_log
###################################################################################################################


###################################################################################################################
def gen_max_bw_numbers(
		mem_ifmap_trace_file,
		mem_filter_trace_file,
		mem_ofmap_trace_file,
		sram_write_trace_file,
		sram_read_trace_file,
		precision = 4
		):

	##### MAX IFMAP MEMORY BANDWIDTH #####
	#-------------------------------------------------------------------------------------------------------------#
	max_dram_activation_bw = 0
	num_bytes = 0
	max_dram_act_clk = ""
	f = open(mem_ifmap_trace_file, 'r')

	for row in f:
		clk = row.split(',')[0]
		num_bytes = len(row.split(',')) - 2
		
		
		if max_dram_activation_bw < num_bytes:
			max_dram_activation_bw = num_bytes
			max_dram_act_clk = clk

	#ilbo: considering precision
	max_dram_activation_bw *= precision
	
	f.close()
	#-------------------------------------------------------------------------------------------------------------#

	#### MAX OFMAP MEMORY BANDWIDTH #####
	#-------------------------------------------------------------------------------------------------------------#
	max_dram_filter_bw = 0
	num_bytes = 0
	max_dram_filt_clk = ""
	f = open(mem_filter_trace_file, 'r')

	for row in f:
		clk = row.split(',')[0]
		num_bytes = len(row.split(',')) - 2

		if max_dram_filter_bw < num_bytes:
			max_dram_filter_bw = num_bytes
			max_dram_filt_clk = clk
	
	#ilbo: considering precision
	max_dram_filter_bw *= precision
	
	f.close()
	#-------------------------------------------------------------------------------------------------------------#
	
	#### MAX OFMAP MEMORY BANDWIDTH ####
	#-------------------------------------------------------------------------------------------------------------#
	max_dram_ofmap_bw = 0
	num_bytes = 0
	max_dram_ofmap_clk = ""
	f = open(mem_ofmap_trace_file, 'r')

	for row in f:
		clk = row.split(',')[0]
		num_bytes = len(row.split(',')) - 2

		if max_dram_ofmap_bw < num_bytes:
			max_dram_ofmap_bw = num_bytes
			max_dram_ofmap_clk = clk
	
	#ilbo: considering precision
	max_dram_ofmap_bw *= precision
	
	f.close()
	#-------------------------------------------------------------------------------------------------------------#
	
	##### MAX SRAM WRITE BANDWIDTH ####
	#-------------------------------------------------------------------------------------------------------------#
	
	max_sram_ofmap_bw = 0
	num_bytes = 0
	f = open(sram_write_trace_file, 'r')

	for row in f:
		num_bytes = len(row.split(',')) - 2

		if max_sram_ofmap_bw < num_bytes:
			max_sram_ofmap_bw = num_bytes
	
	#ilbo: considering precision
	max_sram_ofmap_bw *= precision
	
	f.close()
	#-------------------------------------------------------------------------------------------------------------#
	
	#### MAX SRAM READ BANDWIDTH ####
	#-------------------------------------------------------------------------------------------------------------#
	
	max_sram_read_bw = 0
	num_bytes = 0
	f = open(sram_read_trace_file, 'r')

	for row in f:
		num_bytes = len(row.split(',')) - 2

		if max_sram_read_bw < num_bytes:
			max_sram_read_bw = num_bytes
	
	#ilbo: considering precision
	max_sram_read_bw *= precision
	
	f.close()
	#-------------------------------------------------------------------------------------------------------------#
	
	#-------------------------------------------------------------------------------------------------------------#
	#print("DRAM IFMAP Read BW, DRAM Filter Read BW, DRAM OFMAP Write BW, SRAM OFMAP Write BW")
	log  = str(max_dram_activation_bw) + "," + str(max_dram_filter_bw) + "," 
	log += str(max_dram_ofmap_bw) + "," + str(max_sram_read_bw) + ","
	log += str(max_sram_ofmap_bw)  + ","
	
	# Anand: Enable the following for debug print
	#log += str(max_dram_act_clk) + "," + str(max_dram_filt_clk) + ","
	#log += str(max_dram_ofmap_clk) + ","
	#print(log)
	
	return log
	#-------------------------------------------------------------------------------------------------------------#
###################################################################################################################


###################################################################################################################	
def gen_bw_numbers( mem_ifmap_trace_file,
					mem_filter_trace_file,
					mem_ofmap_trace_file,
					sram_write_trace_file, 
					sram_read_trace_file,
					precision = 4,
					#sram_read_trace_file,
					#array_h, array_w		# These are needed for utilization calculation
					):

	min_clk = 100000
	max_clk = -1
	detailed_log = ""

	#### IFMAP MEM DETAIL ####
	#-------------------------------------------------------------------------------------------------------------#

	num_dram_activation_bytes = 0
	f = open(mem_ifmap_trace_file, 'r')
	start_clk = 0
	first = True

	for row in f:
		num_dram_activation_bytes += len(row.split(',')) - 2
		
		elems = row.strip().split(',')
		clk = float(elems[0])

		if first:
			first = False
			start_clk = clk

		if clk < min_clk:
			min_clk = clk

	# ilbo: consider precision
	num_dram_activation_bytes *= precision

	stop_clk = clk
	detailed_log += str(start_clk) + "," + str(stop_clk) + "," + str(num_dram_activation_bytes) + ","
	f.close()
	#-------------------------------------------------------------------------------------------------------------#


	#### FILTER MEM DETAIL ####
	#-------------------------------------------------------------------------------------------------------------#
	num_dram_filter_bytes = 0
	f = open(mem_filter_trace_file, 'r')
	first = True

	for row in f:
		num_dram_filter_bytes += len(row.split(',')) - 2

		elems = row.strip().split(',')
		clk = float(elems[0])

		if first:
			first = False
			start_clk = clk

		if clk < min_clk:
			min_clk = clk

	# ilbo: consider precision
	num_dram_filter_bytes *= precision

	stop_clk = clk
	detailed_log += str(start_clk) + "," + str(stop_clk) + "," + str(num_dram_filter_bytes) + ","
	f.close()
	#-------------------------------------------------------------------------------------------------------------#


	#### OFMAP MEM DETAIL ####
	#-------------------------------------------------------------------------------------------------------------#

	num_dram_ofmap_bytes = 0
	f = open(mem_ofmap_trace_file, 'r')
	first = True

	for row in f:
		num_dram_ofmap_bytes += len(row.split(',')) - 2

		elems = row.strip().split(',')
		clk = float(elems[0])

		if first:
			first = False
			start_clk = clk

	# ilbo: consider precision
	num_dram_ofmap_bytes *= precision

	stop_clk = clk
	detailed_log += str(start_clk) + "," + str(stop_clk) + "," + str(num_dram_ofmap_bytes) + ","
	f.close()

	if clk > max_clk:
		max_clk = clk
	#-------------------------------------------------------------------------------------------------------------#

	#### SRAM WRITE DETAIL ####
	#-------------------------------------------------------------------------------------------------------------#

	num_sram_ofmap_bytes = 0
	f = open(sram_write_trace_file, 'r')
	first = True

	for row in f:
		num_sram_ofmap_bytes += len(row.split(',')) - 2
		elems = row.strip().split(',')
		clk = float(elems[0])

		if first:
			first = False
			start_clk = clk

	# ilbo: consider precision
	num_sram_ofmap_bytes *= precision

	stop_clk = clk
	detailed_log += str(start_clk) + "," + str(stop_clk) + "," + str(num_sram_ofmap_bytes) + ","
	f.close()

	if clk > max_clk:
		max_clk = clk
	#-------------------------------------------------------------------------------------------------------------#


	#### SRAM READ DETAIL #####
	#-------------------------------------------------------------------------------------------------------------#
	num_sram_read_bytes = 0
	total_util = 0
	#print("Opening " + sram_trace_file)
	f = open(sram_read_trace_file, 'r')
	first = True

	for row in f:
		#num_sram_read_bytes += len(row.split(',')) - 2
		elems = row.strip().split(',')
		clk = float(elems[0])

		if first:
			first = False
			start_clk = clk

		#util, valid_bytes = parse_sram_read_data(elems[1:-1], array_h, array_w)
		valid_bytes = parse_sram_read_data(elems[1:])
		num_sram_read_bytes += valid_bytes
		#total_util += util
		#print("Total Util " + str(total_util) + ",util " + str(util))

	# ilbo: consider precision
	num_sram_read_bytes *= precision

	stop_clk = clk
	detailed_log += str(start_clk) + "," + str(stop_clk) + "," + str(num_sram_read_bytes) + ","
	f.close()

	sram_clk = clk
	if clk > max_clk:
		max_clk = clk
	#-------------------------------------------------------------------------------------------------------------#


	#### Get Average Bandwidth ####
	#-------------------------------------------------------------------------------------------------------------#

	# ilbo: get the total numbers of clock
	delta_clk = max_clk - min_clk
	# print("Total Processing Clk: " + str(delta_clk) + " cycles")
	dram_activation_bw  = num_dram_activation_bytes / delta_clk
	dram_filter_bw	    = num_dram_filter_bytes / delta_clk
	dram_ofmap_bw	    = num_dram_ofmap_bytes / delta_clk
	sram_ofmap_bw	    = num_sram_ofmap_bytes / delta_clk
	sram_read_bw		= num_sram_read_bytes / delta_clk
	#print("total_util: " + str(total_util) + ",sram_clk: " + str(sram_clk))
	#avg_util			= total_util / sram_clk * 100

	units = " Bytes/cycle"
	table = BeautifulTable()

	table.columns.header = ["Required BW", "Value"]
	table.rows.append(["MEM IFMAP Read BW",  str(round(dram_activation_bw,3)) + units])
	table.rows.append(["MEM FILTER Read BW",     str(round(dram_filter_bw,3)) + units])
	table.rows.append(["MEM OFMAP Read BW",       str(round(dram_ofmap_bw,3)) + units])
	
	table.set_style(BeautifulTable.STYLE_GRID)
	table.columns.padding_right['Required BW'] = 18
	table.columns.padding_left['Value']        = 18
	table.columns.alignment["Required BW"] = BeautifulTable.ALIGN_LEFT
	table.columns.alignment["Value"]       = BeautifulTable.ALIGN_RIGHT
	table.columns.header.alignment = BeautifulTable.ALIGN_CENTER

	print("\n---------------------REQUIRED EXTERNAL MEMORY BW-----------------------------")
	print(table)
	print("-----------------------------------------------------------------------------\n")
	'''
	print("----------------------------------------------------------------------")
	print("|       required BW          |               value                  |")
	print("----------------------------------------------------------------------")
	print("| DRAM IFMAP Read BW         |             %10f" %(dram_activation_bw) + units + "   |")
	print("| DRAM Filter Read BW        |             %10f" %(dram_filter_bw) + units + "   |")
	print("| DRAM OFMAP Write BW        |             %10f" %(dram_ofmap_bw) + units + "   |")
	print("----------------------------------------------------------------------")
	'''

	#print("Average utilization : "  + str(avg_util) + " %")
	#print("SRAM OFMAP Write BW, Min clk, Max clk")
	
	log = str(dram_activation_bw) + "," + str(dram_filter_bw) + "," + str(dram_ofmap_bw) + "," + str(sram_read_bw) + "," + str(sram_ofmap_bw) + ","
	# Anand: Enable the following line for debug
	#log += str(min_clk) + "," + str(max_clk) + ","
	#print(log)
	#return log, avg_util
	return log, detailed_log
	#-------------------------------------------------------------------------------------------------------------#
###################################################################################################################

###################################################################################################################
#def parse_sram_read_data(elems, array_h, array_w):
def parse_sram_read_data(elems):
	#half = int(len(elems) /2)
	#nz_row = 0
	#nz_col = 0
	data = 0

	for i in range(len(elems)):
		e = elems[i]
		if e != ' ':
			data += 1
			#if i < half:
			#if i < array_h:
			#	nz_row += 1
			#else:
			#	nz_col += 1

	#util = (nz_row * nz_col) / (half * half)
	#util = (nz_row * nz_col) / (array_h * array_w)
	#data = nz_row + nz_col
	
	#return util, data
	return data
###################################################################################################################

