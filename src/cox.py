import os
import time
import get_configs as gc
import run_nets as r
from absl import flags
from absl import app

## flags ##
#------------------------------------------------------------------------------------------------------------------------------------------------------------#
FLAGS = flags.FLAGS
#description: name of flag | default | explanation
flags.DEFINE_string ("acc_arch_config", "./configs/acc_configs/16_cox_os.cfg",                 "file where we are getting our accelerator architechture from")
flags.DEFINE_string ("mem_arch_config", "./configs/mem_configs/dramfull_nand12_flash8plane.cfg",     "file where we are getting our memory architecture from")
flags.DEFINE_string ("network_config",  "./configs/network_configs/IR/ESTP.csv",                                                "network that we are reading")
flags.DEFINE_boolean("save",            True,                                                              "decide whether to save the layerwise file or not")
flags.DEFINE_integer("batch",           1,                                                             "decide how many ifmaps the one DNN inference exceute")
#------------------------------------------------------------------------------------------------------------------------------------------------------------#

#----------------------------------------------------------------------------------------#
## cox class ##
# description:
#     define simulation configurations(accelerator, memory, network model, etc)
#     and start the simulation, then store the simulation results
#----------------------------------------------------------------------------------------#
class cox:
	## class initialization function ##
	######################################################################################
	def __init__(self, save = False):
		self.save_space = save	# save = decide whether to save the layerwise file or not
	######################################################################################
	
	## parsing configuration function ##
	######################################################################################
	def parse_config(self):
		# Get accelerator configurations
		acc_config_filename = FLAGS.acc_arch_config
		self.acc_config = gc.AccConfig()
		self.acc_config.parse_configs(acc_config_filename)

		# Get memory configurations
		mem_config_filename = FLAGS.mem_arch_config
		self.mem_config = gc.MemConfig()
		self.mem_config.parse_configs(mem_config_filename)
		
		# Get network topology file
		self.topology_file= FLAGS.network_config
		print("\nUsing network topology from: \t\t", self.topology_file, "\n")

		## Read the config name
		self.net_name = self.topology_file.split('/')[-1].split('.')[0]
	######################################################################################
	

	## cox run function ##
	######################################################################################
	def run_cox(self):
		self.parse_config()
			
		self.run_once()
	######################################################################################


	## cox execution function ##
	######################################################################################
	def run_once(self):
		## Print total configurations(accelerator, memory, network configurations)
		print("====================================================")
		print("|********************** COX SIM *******************|")
		print("====================================================")
		
		self.acc_config.print_configs()
		print("")
		self.mem_config.print_configs()
		print('----------------------[TOPOLOGY]-------------------')
		print("Network Name               |     " + self.net_name)
		print("Batch Size                 |     " + str(FLAGS.batch))
		print('---------------------------------------------------')
		print("===================================================")
		
		r.run_net(  acc_config    = self.acc_config,
					mem_config    = self.mem_config,
					topology_file = self.topology_file,
					net_name      = self.net_name,
					batch         = FLAGS.batch,
				)
		
		self.cleanup()

		print("")
		print("====================================================")
		print("|*************** COX SIM Run Complete *************|")
		print("====================================================")
	######################################################################################
	
	######################################################################################
	def cleanup(self):
		if not os.path.exists("./outputs/"):
			os.system("mkdir ./outputs")

		path = "./output/cox_out"
		if self.acc_config.run_name == "":
			path = "./outputs/" + self.net_name +"_"+ self.acc_config.dataflow
		else:
			path = "./outputs/" + self.acc_config.run_name + "_" + self.mem_config.mem_name + '_batch_' + str(FLAGS.batch)

		if not os.path.exists(path):
			os.system("mkdir " + path)
		else:
			t = time.time()
			new_path= path + "_" + str(t)
			os.system("mv " + path + " " + new_path)
			os.system("mkdir " + path)


		cmd = "mv *.csv " + path
		os.system(cmd)

		cmd = "mkdir " + path + "/layer_wise_" + self.net_name
		os.system(cmd)

		cmd = "mv " + path +"/*sram* " + path +"/layer_wise_" + self.net_name
		os.system(cmd)

		cmd = "mv " + path +"/*mem* " + path +"/layer_wise_" + self.net_name
		os.system(cmd)

		if self.save_space == True:
			cmd = "rm -rf " + path +"/layer_wise_" + self.net_name
			os.system(cmd)
	######################################################################################
	
#----------------------------------------------------------------------------------------#
# END cox class

def main(argv):
	s = cox(save = FLAGS.save)
	s.run_cox()

if __name__ == '__main__':
  app.run(main)

