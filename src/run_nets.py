import trace_gen_wrapper as tg
import os
import subprocess
import math

def run_net( acc_config,
			 mem_config,
			 topology_file       = './topologies/IR/TIR.csv',
			 net_name            = 'TIR',

			 batch               = 1,
			):

	fname  = net_name + "_avg_bw.csv"
	bw     = open(fname, 'w')

	f2name = net_name + "_max_bw.csv"
	maxbw  = open(f2name, 'w')

	f3name = net_name + "_cycles.csv"
	cycl   = open(f3name, 'w')

	f4name = net_name + "_detail.csv"
	detail = open(f4name, 'w')

	f5name = net_name + "_time_param.csv"
	time_param = open(f5name, 'w')

	f6name = net_name + "_energy_consumption.csv"
	energy_param = open(f6name, 'w')

	bw.write("IFMAP SRAM Size,"     + \
			 "Filter SRAM Size,"    + \
			 "OFMAP SRAM Size,"     + \
			 "Conv Layer Num,"      + \
			 "MEM IFMAP Read BW,"   + \
			 "MEM Filter Read BW,"  + \
			 "MEM OFMAP Write BW,"  + \
			 "SRAM Read BW,"        + \
			 "SRAM OFMAP Write BW," + \
			 "\n")
	
	maxbw.write("IFMAP SRAM Size,"         + \
				"Filter SRAM Size,"        + \
				"OFMAP SRAM Size,"         + \
				"Conv Layer Num,"          + \
				"Max MEM IFMAP Read BW,"   + \
				"Max MEM Filter Read BW,"  + \
				"Max MEM OFMAP Write BW,"  + \
				"Max SRAM Read BW,"        + \
				"Max SRAM OFMAP Write BW," + \
				"\n")

	cycl.write("Layer,Cycles,"  + \
			   "% Utilization," + \
			   "\n")

	detailed_log = "Layer,"          + \
				 "MEM_IFMAP_start,"  + \
				 "MEM_IFMAP_stop,"   + \
				 "MEM_IFMAP_bytes,"  + \
				 "MEM_Filter_start," + \
				 "MEM_Filter_stop,"  + \
				 "MEM_Filter_bytes," + \
				 "MEM_OFMAP_start,"  + \
				 "MEM_OFMAP_stop,"   + \
				 "MEM_OFMAP_bytes,"  + \
				 "SRAM_write_start," + \
				 "SRAM_write_stop,"  + \
				 "SRAM_write_bytes," + \
				 "SRAM_read_start,"  + \
				 "SRAM_read_stop,"   + \
				 "SRAM_read_bytes,"  + \
				 "\n"

	detail.write(detailed_log)
	
	time_param.write("Layer," + \
					 "total_execution_time(us)," + \
					 "total_compute_time(us),"   + \
					 "total_delayed_time(us),"   + \
					 "IFMAP_delayed_time(us),"   + \
					 "Filter_delayed_time(us),"  + \
					 "OFMAP_delayed_time(us),"   + \
					 "total_compute_ratio(%),"   + \
					 "total_delayed_ratio(%),"   + \
					 "IFMAP_delayed_ratio(%),"   + \
					 "Filter_delayed_ratio(%),"  + \
					 "OFMAP_delayed_ratio(%),"   + \
					 "\n")


	energy_param.write("Layer," + \
					   "total_computation_cnt,"      + \
					   "IFMAP_SRAM_read_bytes,"      + \
					   "FILTER_SRAM_read_bytes,"     + \
					   "OFMAP_SRAM_write_bytes,"     + \
					   "IFMAP_MEM_read_bytes,"       + \
					   "FILTER_MEM_read_bytes,"      + \
					   "OFMAP_MEM_write_bytes,"      + \
					   "total_compute_energy(nJ),"   + \
					   "IFMAP_SRAM_read_energy(nJ)," + \
					   "FILTER_SRAM_read_energy(nJ),"+ \
					   "OFMAP_SRAM_write_energy(nJ),"+ \
					   "IFMAP_MEM_read_energy(nJ),"  + \
					   "FILTER_MEM_read_energy(nJ)," + \
					   "OFMAP_MEM_write_energy(nJ)," + \
					   "total_energy_consumpt(nJ),"  + \
					   "\n")

	#### count number of DNN layer ####
	#------------------------------------------------------------------------------------------------#
	layer_file = open(topology_file, 'r')

	layer_count = 0
	for row in layer_file:
		layer_count = layer_count + 1
	layer_count -= 1 # consider the label row

#	print("total layer num = " + str(layer_count) + '\n')
	layer_file.close()
	#------------------------------------------------------------------------------------------------#

	first     = True
	
	layer_num = 0
	
	is_first_layer = False
	is_last_layer  = False
	
	param_file = open(topology_file, 'r')

	# DNN processing layer by layer
	#------------------------------------------------------------------------------------------------#
	for row in param_file:
		# skip the label line
		if first:
			first = False
			continue

		layer_num = layer_num + 1

		if layer_num == 1:
			is_first_layer = True

		else:
			is_first_layer = False

		if layer_num == layer_count:
			is_last_layer = True

		# network topology elements
		elems = row.strip().split(',')
		
		# elems[0] = 'Layer name'
		# elems[1] = 'IFMAP Height'
		# elems[2] = 'IFMAP Width'
		# elems[3] = 'Filter Height'
		# elems[4] = 'Filter Width'
		# elems[5] = 'Channels'
		# elems[6] = 'Num Filter'
		# elems[7] = 'Strides'
		# elems[9] = ''
		if len(elems) < 9:
			continue

		name = elems[0]
		
		## LOG for each layer start ##
		#---------------------------------------------------------#
		layer_str = "LAST_LAYER"
		if is_last_layer is True:
			layer_str = "LAST_LAYER"
		if is_first_layer is True:
			layer_str = "FIRST_LAYER"
		if not is_last_layer and not is_first_layer:
			layer_str = "HIDDEN LAYER"

		print("")
		print("Commencing run for " + name + " ("+ layer_str +")")
		#---------------------------------------------------------#
		
		ifmap_h = int(elems[1])*int(batch) # consider inference input batch size
		ifmap_w = int(elems[2])

		filt_h = int(elems[3])
		filt_w = int(elems[4])

		num_channels = int(elems[5])
		num_filters  = int(elems[6])

		strides      = int(elems[7])
		
		ifmap_base  = acc_config.ifmap_offset
		filter_base = acc_config.filter_offset
		ofmap_base  = acc_config.ofmap_offset

		bw_log = str(acc_config.isram_min*1024)  + "," + \
				 str(acc_config.fsram_min*1024) + "," +  \
				 str(acc_config.osram_min*1024)  + "," + \
				 name + ","

		max_bw_log = bw_log
		detailed_log = name + ","

		bw_str, detailed_str, util, clk, time_param_str, energy_log =  \
			tg.gen_all_traces(  # cox configuration
								acc_config = acc_config,
								mem_config = mem_config,

								# network toplogy
								ifmap_h               = ifmap_h,
								ifmap_w               = ifmap_w,
								filt_h                = filt_h,
								filt_w                = filt_w,
								num_channels          = num_channels,
								num_filt              = num_filters,
								strides               = strides,
								
								word_size_bytes       = 1,				# don't have to consider(don't change!!!)
								
								# sram trace file name
								sram_read_trace_file       = net_name + "_" + name + "_sram_read.csv",
								sram_write_trace_file      = net_name + "_" + name + "_sram_write.csv",
								sram_integrated_trace_file = net_name + "_" + name + "_sram_write.csv",

								# external memory trace file name
								mem_filter_trace_file = net_name + "_" + name + "_mem_filter_read.csv",
								mem_ifmap_trace_file  = net_name + "_" + name + "_mem_ifmap_read.csv",
								mem_ofmap_trace_file  = net_name + "_" + name + "_mem_ofmap_write.csv",

								# external memory read delay time trace file name
								mem_delay_read_trace_file  = net_name + "_" + name + "_mem_delayed_read.csv",

								# flags for current layer state
								is_first_layer = is_first_layer,
								is_last_layer  = is_last_layer,
							)

		bw_log += bw_str
		bw.write(bw_log + "\n")

		detailed_log += detailed_str
		detail.write(detailed_log + "\n")

		max_bw_log += tg.gen_max_bw_numbers(
								sram_read_trace_file   = net_name + "_" + name + "_sram_read.csv",
								sram_write_trace_file  = net_name + "_" + name + "_sram_write.csv",
								mem_filter_trace_file  = net_name + "_" + name + "_mem_filter_read.csv",
								mem_ifmap_trace_file   = net_name + "_" + name + "_mem_ifmap_read.csv",
								mem_ofmap_trace_file   = net_name + "_" + name + "_mem_ofmap_write.csv",
								precision = acc_config.precision
								)

		maxbw.write(max_bw_log + "\n")

		util_str = str(util)
		line = name + "," + clk +"," + util_str +",\n"
		cycl.write(line)
		
		time_param.write(name + ',' + time_param_str + ',\n')
		energy_param.write(name + ',' + energy_log + ',\n')

	bw.close()
	maxbw.close()
	cycl.close()
	time_param.close()
	energy_param.close()
	param_file.close()

