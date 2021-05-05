import os
import configparser as cp
from beautifultable import BeautifulTable

#-----------------------------------------------------------------------------------------------#
## acclerator configuration class ##
# description:
#		define systolic array accelerator configuration
#		(1) systolic array
#		(2) dataflow mapping scheme
#		(3) clock speed & precision
#		(4) sram buffer size for each data(input, output, weight)
#       (5) data paths for each data
#-----------------------------------------------------------------------------------------------#
class AccConfig:
	## class initialization function ##
	#############################################################################################
	def __init__(self):
		self.acc_config_filename = ''
		self.run_name = ''

		## array of systolic array 
		self.ar_h_min = 0
		self.ar_w_min = 0
		
		## sram buffer size for each data(input, output, weight)
		self.isram_min  = 0
		self.fsram_min  = 0
		self.osram_min  = 0

		## base address for each data
		self.ifmap_offset  = 0
		self.filter_offset = 0
		self.ofmap_offset  = 0

		## dataflow mapping scheme inside the accelerator
		# (1) WS(Weight Stationary)
		# (2) OS(Ouptut Stationary)
		# (3) IS(Input Stationary)
		self.dataflow = ''

		## precision of data (Byte)
		# ex) 32bit-floating point -> 4
		self.precision = 0

		## clock speed(KHz)
		# PE in systolic array considered as one cycle calculation(multiply and accumulation in one cycle)
		self.clock_speed = 0

		## off-chip memory data path for each data
		self.ifmap_mem       = ''
		self.filter_mem      = ''
		self.ofmap_mem       = ''
		self.inter_ofmap_mem = ''

		self.ifmap_mem_flash_dense       = ''
		self.filter_mem_flash_dense      = ''
		self.ofmap_mem_flash_dense       = ''
		self.inter_ofmap_mem_flash_dense = ''
	#############################################################################################
	
	## Parsing accelerator configuration function ##
	#############################################################################################
	def parse_configs(self, acc_config_filename):
		general = 'general'
		acc_arch_sec = 'accelerator_architecture_presets'

		print("\nUsing accelerator architechture from: \t",acc_config_filename)
		
		self.acc_config_filename = acc_config_filename
		
		acc_config = cp.ConfigParser()
		acc_config.read(self.acc_config_filename)

		self.run_name = acc_config.get(general, 'run_name')

		## Read the accelerator architecture_presets

		## Set systolice array height min, max
		ar_h = acc_config.get(acc_arch_sec, 'ArrayHeight').split(',')
		self.ar_h_min = int(ar_h[0].strip())

		if len(ar_h) > 1:
			self.ar_h_max = int(ar_h[1].strip())

		## Set systolic array width min, max
		ar_w = acc_config.get(acc_arch_sec, 'ArrayWidth').split(',')
		self.ar_w_min = int(ar_w[0].strip())

		if len(ar_w) > 1:
			self.ar_w_max = int(ar_w[1].strip())

		## Set IFMAP SRAM buffer min, max
		ifmap_sram = acc_config.get(acc_arch_sec, 'IfmapSramSz').split(',')
		self.isram_min = float(ifmap_sram[0].strip())

		if len(ifmap_sram) > 1:
			self.isram_max = float(ifmap_sram[1].strip())

		## Set FILTER SRAM buffer min, max
		filter_sram = acc_config.get(acc_arch_sec, 'FilterSramSz').split(',')
		self.fsram_min = float(filter_sram[0].strip())
	
		if len(filter_sram) > 1:
			self.fsram_max = float(filter_sram[1].strip())

		## Set OFMAP SRAM buffer min, max
		ofmap_sram = acc_config.get(acc_arch_sec, 'OfmapSramSz').split(',')
		self.osram_min = float(ofmap_sram[0].strip())

		if len(ofmap_sram) > 1:
			self.osram_max = float(ofmap_sram[1].strip())

		## Set dataflow (os, ws, is)
		self.dataflow= acc_config.get(acc_arch_sec, 'Dataflow')

		## Set base address of input feature vector
		ifmap_offset = acc_config.get(acc_arch_sec, 'IfmapOffset')
		self.ifmap_offset = int(ifmap_offset.strip())

		## Set base address of filter
		filter_offset = acc_config.get(acc_arch_sec, 'FilterOffset')
		self.filter_offset = int(filter_offset.strip())

		## Set base address of output feature vector
		ofmap_offset = acc_config.get(acc_arch_sec, 'OfmapOffset')
		self.ofmap_offset = int(ofmap_offset.strip())

		## Set precision(byte)
		precision = acc_config.get(acc_arch_sec, 'Precision')
		self.precision = int(precision.strip())
	
		## Set clock speed(KHz)
		clock_speed = acc_config.get(acc_arch_sec, 'ClockSpeed')
		self.clock_speed = int(clock_speed.strip())

		## Set external memory for input feature vector
		self.ifmap_mem = acc_config.get(acc_arch_sec, 'IfmapMEM')

		## Set external memory for intermediate ofmap
		self.inter_ofmap_mem = acc_config.get(acc_arch_sec, 'InterOfmapMEM')
		
		## Set external memory for filter
		self.filter_mem = acc_config.get(acc_arch_sec, 'FilterMEM')

		## Set external memory for output feature vector
		self.ofmap_mem = acc_config.get(acc_arch_sec, 'OfmapMEM')

		## Set external flash dense for input feature vector
		if self.ifmap_mem == 'FLASH':
			self.ifmap_mem_flash_dense = acc_config.get(acc_arch_sec, 'IfmapMEMFlashDense')

		## Set external flash dense for input feature vector
		if self.inter_ofmap_mem == 'FLASH':
			self.inter_ofmap_mem_flash_dense = acc_config.get(acc_arch_sec, 'InterOfmapMEMFlashDense')

		## Set external flash dense for input feature vector
		if self.filter_mem == 'FLASH':
			self.filter_mem_flash_dense = acc_config.get(acc_arch_sec, 'FilterMEMFlashDense')

		## Set external flash dense for input feature vector
		if self.ofmap_mem == 'FLASH':
			self.ofmap_mem_flash_dense = acc_config.get(acc_arch_sec, 'OfmapMEMFlashDense')
	#############################################################################################
	
	## Printing accelerator configuration function ##
	#############################################################################################	
	def print_configs(self):

		df_string = "Output Stationary"
		if self.dataflow == 'ws':
			df_string = "Weight Stationary"
		elif self.dataflow == 'is':
			df_string = "Input Stationary"
		
		ifmap_mem_string = self.ifmap_mem
		if ifmap_mem_string == 'FLASH':
			ifmap_mem_string = ifmap_mem_string + '(' + self.ifmap_mem_flash_dense + ')'
		
		inter_ofmap_mem_string = self.inter_ofmap_mem
		if inter_ofmap_mem_string == 'FLASH':
			inter_ofmap_mem_string = inter_ofmap_mem_string + '(' + self.inter_ofmap_mem_flash_dense + ')'
	
		filter_mem_string = self.filter_mem
		if filter_mem_string == 'FLASH':
			filter_mem_string = filter_mem_string + '(' + self.filter_mem_flash_dense + ')'
		
		ofmap_mem_string = self.ofmap_mem
		if ofmap_mem_string == 'FLASH':
			ofmap_mem_string = ofmap_mem_string + '(' + self.ofmap_mem_flash_dense + ')'
		
		table = BeautifulTable()

		table.columns.header = ["Parameter", "Value"]
		table.rows.append(["Array Size", str(str(self.ar_h_min) + "x" + str(self.ar_w_min))])
		table.rows.append(["IFMAP  SRAM Size",                  str(self.isram_min) + " KB"])
		table.rows.append(["Filter SRAM Size",                  str(self.fsram_min) + " KB"])
		table.rows.append(["OFMAP  SRAM Size",                  str(self.osram_min) + " KB"])
		table.rows.append(["Dataflow",                                            df_string])
		table.rows.append(["Precision",                      str(self.precision) + " bytes"])
		table.rows.append(["Clock Speed",                    str(self.clock_speed) + " KHz"])
		table.rows.append(["IFMAP External Memory",                        ifmap_mem_string])
		table.rows.append(["Filter External Memory",                      filter_mem_string])
		table.rows.append(["OFMAP External Memory",                        ofmap_mem_string])
		table.rows.append(["Infer-Ofmap External Memory",            inter_ofmap_mem_string])
		table.rows.append(["IFMAP Base Address",                     str(self.ifmap_offset)])
		table.rows.append(["Filter Base Address",                   str(self.filter_offset)])
		table.rows.append(["OFMAP Base Address",                     str(self.ofmap_offset)])
	
		print('            [ACCELERATOR CONFIGURATION]')
		table.set_style(BeautifulTable.STYLE_GRID)
		table.columns.alignment["Parameter"] = BeautifulTable.ALIGN_LEFT
		table.columns.alignment["Value"]     = BeautifulTable.ALIGN_RIGHT
		table.columns.header.alignment = BeautifulTable.ALIGN_CENTER
		print(table)
	#############################################################################################
#-----------------------------------------------------------------------------------------------#


#-----------------------------------------------------------------------------------------------#
## off-chip memory configuration class ##
# description:
#		define off-chip memories' configurations in SSD
#		(1) SSD DRAM
#		(2) NAND interface
#       (3) Flash Memory
#-----------------------------------------------------------------------------------------------#
class MemConfig:
	## class intializing function ##
	#############################################################################################
	def __init__(self):
		self.mem_name = ''
		self.mem_config_filename = ''

		# dram configuration
		self.dram_bw = 0.0

		# flash configurations
		self.nand_interface_bw = 0.0
		self.flash_page_size   = 0
		self.flash_plane_num   = 0

		# flash read/write latency(us)
		self.t_slc_read  = 0.0
		self.t_mlc_read  = 0.0
		self.t_tlc_read  = 0.0

		self.t_slc_write = 0.0
		self.t_mlc_write = 0.0
		self.t_tlc_write = 0.0
	#############################################################################################
	
	## parsing memory configuration function ##
	#############################################################################################
	def parse_configs(self, mem_config_filename):
		memory_name  = 'memory_name'
		mem_arch_sec = 'memory_architecture_presets'

		print("\nUsing memory architecture from: \t", mem_config_filename)

		self.mem_config_filename = mem_config_filename
		
		mem_config = cp.ConfigParser()
		mem_config.read(mem_config_filename)

		self.mem_name = mem_config.get(memory_name, 'mem_name')

		## Read the memory architecture_presets

		## Set DRAM bandwidth (GB/sec)
		dram_bw = mem_config.get(mem_arch_sec, 'DramBW')
		self.dram_bw = float(dram_bw.strip())

		#------------------------FLASH CONFIGURATION-------------------------------#
		## Set NAND interface bandwidth (GB/sec)
		nand_interface_bw = mem_config.get(mem_arch_sec, 'NANDInterfaceBW')
		self.nand_interface_bw = float(nand_interface_bw.strip())

		## Set the flash page size(KB)
		flash_page_size = mem_config.get(mem_arch_sec, 'FlashPageSz')
		self.flash_page_size = int(flash_page_size.strip())

		## Set number of plane (unit of mutiplane operation)
		flash_plane_num = mem_config.get(mem_arch_sec, 'FlashPlaneNum')
		self.flash_plane_num = int(flash_plane_num.strip())

		## Set SLC read latency
		t_slc_read = mem_config.get(mem_arch_sec, 'SLCRead')
		self.t_slc_read = float(t_slc_read.strip())
		
		## Set MLC read latency
		t_mlc_read = mem_config.get(mem_arch_sec, 'MLCRead')
		self.t_mlc_read = float(t_mlc_read.strip())
	
		## Set TLC read latency
		t_tlc_read = mem_config.get(mem_arch_sec, 'TLCRead')
		self.t_tlc_read = float(t_tlc_read.strip())
	
		## Set SLC write latency
		t_slc_write = mem_config.get(mem_arch_sec, 'SLCWrite')
		self.t_slc_write = float(t_slc_write.strip())
		
		## Set MLC write latency
		t_mlc_write = mem_config.get(mem_arch_sec, 'MLCWrite')
		self.t_mlc_write = float(t_mlc_write.strip())
	
		## Set TLC write latency
		t_tlc_write = mem_config.get(mem_arch_sec, 'TLCWrite')
		self.t_tlc_write = float(t_tlc_write.strip())
		#----------------------------------------------------------------------------#
	#############################################################################################
	
	## printing memory configuration function ##
	#############################################################################################
	def print_configs(self):
		table = BeautifulTable()

		table.columns.header = ["Parameter", "Value"]
		table.rows.append(["DRAM Bandwidth",                      str(self.dram_bw) + ' GB/s'])
		table.rows.append(["NAND Interface Bandwidth",  str(self.nand_interface_bw) + ' GB/s'])
		table.rows.append(["Flash Page Size",               str(self.flash_page_size) + ' KB'])
		table.rows.append(["Flash Plane Num",                       str(self.flash_plane_num)])
		table.rows.append(['SLC page read latency',              str(self.t_slc_read) + ' us'])
		table.rows.append(['MLC page read latency',              str(self.t_mlc_read) + ' us'])
		table.rows.append(['TLC page read latency',              str(self.t_tlc_read) + ' us'])
		table.rows.append(['SLC page write latency',            str(self.t_slc_write) + ' us'])
		table.rows.append(['MLC page write latency',            str(self.t_mlc_write) + ' us'])
		table.rows.append(['TLC page write latency',            str(self.t_tlc_write) + ' us'])

		print('          [OFF-CHIP MEMORY CONFIGURATION]')
		table.set_style(BeautifulTable.STYLE_GRID)
		table.columns.padding_right['Parameter'] = 4
		table.columns.padding_left['Value']      = 9 
		table.columns.alignment["Parameter"] = BeautifulTable.ALIGN_LEFT
		table.columns.alignment["Value"]     = BeautifulTable.ALIGN_RIGHT
		table.columns.header.alignment = BeautifulTable.ALIGN_CENTER
		print(table)
	#############################################################################################
#-----------------------------------------------------------------------------------------------#

if __name__=='__main__':
	acc_config = AccConfig()
	mem_config = MemConfig()

	acc_config.parse_configs("./configs/acc_configs/chip_os_new.cfg")
	mem_config.parse_configs("./configs/mem_configs/dramfull_nand12_flash8plane_new.cfg")

	acc_config.print_configs()
	mem_config.print_configs()


