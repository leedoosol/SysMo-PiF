#include "TSU_OutofOrder.h"
#include <assert.h>
#include "../ssd/NVM_PHY_ONFI_NVDDR2.h"
//#include "Stats.h"


FILE* free_log_file2 = fopen("free_block_log_urgent.txt", "a");
#define FREE_BLOCK_LOG2 0

extern bool gnStartPrint;




namespace SSD_Components
{

	TSU_OutOfOrder::TSU_OutOfOrder(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, unsigned int ChannelCount, unsigned int chip_no_per_channel,
		unsigned int DieNoPerChip, unsigned int PlaneNoPerDie,
		sim_time_type WriteReasonableSuspensionTimeForRead,
		sim_time_type EraseReasonableSuspensionTimeForRead,
		sim_time_type EraseReasonableSuspensionTimeForWrite, 
		bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled)
		: TSU_Base(id, ftl, NVMController, Flash_Scheduling_Type::OUT_OF_ORDER, ChannelCount, chip_no_per_channel, DieNoPerChip, PlaneNoPerDie,
			WriteReasonableSuspensionTimeForRead, EraseReasonableSuspensionTimeForRead, EraseReasonableSuspensionTimeForWrite,
			EraseSuspensionEnabled, ProgramSuspensionEnabled)
	{
		UserReadTRQueue = new Flash_Transaction_Queue*[channel_count];
		UserWriteTRQueue = new Flash_Transaction_Queue*[channel_count];
		GCReadTRQueue = new Flash_Transaction_Queue*[channel_count];
		GCWriteTRQueue = new Flash_Transaction_Queue*[channel_count];
		GCEraseTRQueue = new Flash_Transaction_Queue*[channel_count];
		MappingReadTRQueue = new Flash_Transaction_Queue*[channel_count];
		MappingWriteTRQueue = new Flash_Transaction_Queue*[channel_count];
		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
		{
			UserReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			UserWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			GCReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			GCWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			GCEraseTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			MappingReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			MappingWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
			{
				UserReadTRQueue[channelID][chip_cntr].Set_id("User_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
				UserWriteTRQueue[channelID][chip_cntr].Set_id("User_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
				GCReadTRQueue[channelID][chip_cntr].Set_id("GC_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
				MappingReadTRQueue[channelID][chip_cntr].Set_id("Mapping_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
				MappingWriteTRQueue[channelID][chip_cntr].Set_id("Mapping_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
				GCWriteTRQueue[channelID][chip_cntr].Set_id("GC_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
				GCEraseTRQueue[channelID][chip_cntr].Set_id("GC_Erase_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));					

				UrgentGCWriteSchCount[channelID][chip_cntr] = 0;
			}
		}

		_NVMController->SetTRQueue( UserReadTRQueue, 
			UserWriteTRQueue, 
			GCReadTRQueue, 
			GCWriteTRQueue,
			GCEraseTRQueue,
			MappingReadTRQueue, 
			MappingWriteTRQueue);

	}
	
	TSU_OutOfOrder::~TSU_OutOfOrder()
	{
		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
		{
			delete[] UserReadTRQueue[channelID];
			delete[] UserWriteTRQueue[channelID];
			delete[] GCReadTRQueue[channelID];
			delete[] GCWriteTRQueue[channelID];
			delete[] GCEraseTRQueue[channelID];
			delete[] MappingReadTRQueue[channelID];
			delete[] MappingWriteTRQueue[channelID];
		}
		delete[] UserReadTRQueue;
		delete[] UserWriteTRQueue;
		delete[] GCReadTRQueue;
		delete[] GCWriteTRQueue;
		delete[] GCEraseTRQueue;
		delete[] MappingReadTRQueue;
		delete[] MappingWriteTRQueue;
	}

	void TSU_OutOfOrder::Start_simulation() {}

	void TSU_OutOfOrder::Validate_simulation_config() {}

	void TSU_OutOfOrder::Execute_simulator_event(MQSimEngine::Sim_Event* event) {}

	void TSU_OutOfOrder::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
	{
		name_prefix = name_prefix + +".TSU";
		xmlwriter.Write_open_tag(name_prefix);

		TSU_Base::Report_results_in_XML(name_prefix, xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				UserReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".User_Read_TR_Queue", xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				UserWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".User_Write_TR_Queue", xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				MappingReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".Mapping_Read_TR_Queue", xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				MappingWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".Mapping_Write_TR_Queue", xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				GCReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Read_TR_Queue", xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				GCWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Write_TR_Queue", xmlwriter);

		for (unsigned int channelID = 0; channelID < channel_count; channelID++)
			for (unsigned int chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
				GCEraseTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Erase_TR_Queue", xmlwriter);
	
		xmlwriter.Write_close_tag();
	}

	inline void TSU_OutOfOrder::Prepare_for_transaction_submit()
	{
		opened_scheduling_reqs++;
		if (opened_scheduling_reqs > 1)
			return;
		transaction_receive_slots.clear();
	}

	inline void TSU_OutOfOrder::Submit_transaction(NVM_Transaction_Flash* transaction)
	{
		transaction_receive_slots.push_back(transaction);
	}

	void TSU_OutOfOrder::Schedule()
	{
		opened_scheduling_reqs--;
		if (opened_scheduling_reqs > 0)
			return;
		if (opened_scheduling_reqs < 0)
			PRINT_ERROR("TSU_OutOfOrder: Illegal status!");

		if (transaction_receive_slots.size() == 0)
			return;

		for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_receive_slots.begin();
			it != transaction_receive_slots.end(); it++)
		{

			
			if (gnStartPrint == true && (*it)->Address.ChannelID == DEBUG_CH )
//			if ((*it)->Address.ChannelID == 3 && (*it)->Address.ChipID == 3)
			{
				std::cout << "TSU: SCH: PB: "
					<< " CH_ID: " << (*it)->Address.ChannelID
					<< " CHIP_ID: " << (*it)->Address.ChipID
					<< " TSC_ADDR: " << std::hex << (void*)(*it) << std::dec
					<< " TYPE: " << (int)((*it)->Type)
					<< " SRC: " << (int)((*it)->Source)
					<< " ST: " << Simulator->Time() << std::endl;

			}
			

			switch ((*it)->Type)
			{
			case Transaction_Type::READ:
				switch ((*it)->Source)
				{
				case Transaction_Source_Type::CACHE:
				case Transaction_Source_Type::USERIO: {
					//std::cout << "This is USERIO" << std::endl;
					UserReadTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
					break;
					}
				case Transaction_Source_Type::MAPPING:
					MappingReadTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
					break;
				case Transaction_Source_Type::GC_WL:
					GCReadTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
					break;
				default:
					PRINT_ERROR("TSU_OutOfOrder: unknown source type for a read transaction!")
				}
				break;
			case Transaction_Type::WRITE:
				switch ((*it)->Source)
				{
				case Transaction_Source_Type::CACHE:
				case Transaction_Source_Type::USERIO:
					UserWriteTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
					break;
				case Transaction_Source_Type::MAPPING:
					MappingWriteTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
					break;
				case Transaction_Source_Type::GC_WL:
					GCWriteTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
					break;
				default:
					PRINT_ERROR("TSU_OutOfOrder: unknown source type for a write transaction!")
				}
				break;
			case Transaction_Type::ERASE:
				GCEraseTRQueue[(*it)->Address.ChannelID][(*it)->Address.ChipID].push_back((*it));
				break;
			default:
				break;
			}
		}

#if (1)// flag0221
		for (flash_channel_ID_type channelID = 0; channelID < channel_count; channelID++)
		{
			if (_NVMController->Get_channel_status(channelID) == BusChannelStatus::IDLE)
			{
				flash_chip_ID_type nChip_ID = Round_robin_turn_of_channel[channelID];
				flash_chip_ID_type nChip_ID_ORG = nChip_ID;				
				bool bScheudleNonUserRead = true;

				for (unsigned int i = 0; i < chip_no_per_channel; i++)
				{
					NVM::FlashMemory::Flash_Chip* chip = _NVMController->Get_chip(channelID, nChip_ID);

	#if (ADAPTIVE_RPS_SCH_ENABLE)
					if (true == service_read_transaction(chip, READ_PRIO_SCH))
	#else
					if (true == service_read_transaction(chip, true))
	#endif
					{
						if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip) && GCReadTRQueue[chip->ChannelID][chip->ChipID].size()) { //doodu debug.
							//std::cout << "UsrRQ: " << UserReadTRQueue[chip->ChannelID][chip->ChipID].size() << ", GCRQ: " << GCReadTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
						}
						assert(_NVMController->Get_channel_status(channelID) != BusChannelStatus::IDLE);
						Round_robin_turn_of_channel[channelID] = (flash_chip_ID_type)(nChip_ID_ORG + 1) % chip_no_per_channel;
						break;
					}
					nChip_ID = (flash_chip_ID_type)(nChip_ID + 1) % chip_no_per_channel;
				}

				
				for ( unsigned int i = 0; i < chip_no_per_channel; i++)
				{
					NVM::FlashMemory::Flash_Chip* chip = _NVMController->Get_chip(channelID, i);
					if ( true == check_user_read_transaction(chip) )
					{
						bScheudleNonUserRead = false;
						break;
					}
				}

	#if (ADAPTIVE_RPS_SCH_ENABLE)
				if ( (false == READ_PRIO_SCH) || (true == bScheudleNonUserRead) )
	#else
				if ( true == bScheudleNonUserRead ) // gc���� read�� queue�� �־, user read�� q�� ������ ���� �ʴ´�..
	#endif
				{
					if (_NVMController->Get_channel_status(channelID) == BusChannelStatus::IDLE)
					{
						nChip_ID = nChip_ID_ORG;
						for (unsigned int i = 0; i < chip_no_per_channel; i++)
						{
							NVM::FlashMemory::Flash_Chip* chip = _NVMController->Get_chip(channelID, nChip_ID);

							if (true == service_read_transaction(chip, false))
							{
								assert(_NVMController->Get_channel_status(channelID) != BusChannelStatus::IDLE);
								Round_robin_turn_of_channel[channelID] = (flash_chip_ID_type)(nChip_ID_ORG + 1) % chip_no_per_channel;
								break;
							}
							nChip_ID = (flash_chip_ID_type)(nChip_ID + 1) % chip_no_per_channel;
						}
					}

					if (_NVMController->Get_channel_status(channelID) == BusChannelStatus::IDLE)
					{
						for (unsigned int i = 0; i < chip_no_per_channel; i++)
						{
							NVM::FlashMemory::Flash_Chip* chip = _NVMController->Get_chip(channelID, Round_robin_turn_of_channel[channelID]);
							if (!service_write_transaction(chip))
								service_erase_transaction(chip);
							Round_robin_turn_of_channel[channelID] = (flash_chip_ID_type)(Round_robin_turn_of_channel[channelID] + 1) % chip_no_per_channel;
							if (_NVMController->Get_channel_status(chip->ChannelID) != BusChannelStatus::IDLE)
								break;
						}
					}
				}
			}
		}
#else
		for (flash_channel_ID_type channelID = 0; channelID < channel_count; channelID++)
		{
			if (_NVMController->Get_channel_status(channelID) == BusChannelStatus::IDLE)
			{
				for (unsigned int i = 0; i < chip_no_per_channel; i++) {
					NVM::FlashMemory::Flash_Chip* chip = _NVMController->Get_chip(channelID, Round_robin_turn_of_channel[channelID]);
					//The TSU does not check if the chip is idle or not since it is possible to suspend a busy chip and issue a new command
					if (!service_read_transaction(chip, false))
						if (!service_write_transaction(chip))
							service_erase_transaction(chip);
					Round_robin_turn_of_channel[channelID] = (flash_chip_ID_type)(Round_robin_turn_of_channel[channelID] + 1) % chip_no_per_channel;
					if (_NVMController->Get_channel_status(chip->ChannelID) != BusChannelStatus::IDLE)
						break;
				}
			}
		}
#endif
	}

	
	bool TSU_OutOfOrder::check_user_read_transaction(NVM::FlashMemory::Flash_Chip* chip)
	{
		if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			return true;

		return false;
	}

	unsigned int snGCReadSchCount = 0;
	unsigned int cnt_mappingRead = 0; //doodu
	bool TSU_OutOfOrder::service_read_transaction(NVM::FlashMemory::Flash_Chip* chip, bool bUserReadOnlySchedule)
	{
		bool srcQ1isUsrQ=false;
		Flash_Transaction_Queue *sourceQueue1 = NULL, *sourceQueue2 = NULL;
#if FREE_BLOCK_LOG2 
		fprintf(free_log_file2, "%d %d %d %d %d %d\n", (Simulator->Time() / SIM_TIME_TO_MICROSECONDS_COEFF), free_block_pool_size, plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID);
#endif

		if (MappingReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)//Flash transactions that are related to FTL mapping data have the highest priority
		{
			cnt_mappingRead++;
			std::cout << "Read for FTL mapping: " << cnt_mappingRead << std::endl;
			sourceQueue1 = &MappingReadTRQueue[chip->ChannelID][chip->ChipID];
			if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip) && GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				sourceQueue2 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
			else if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				sourceQueue2 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
		}
		else if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip))//If flash transactions related to GC are prioritzed (non-preemptive execution mode of GC), then GC queues are checked first
		{
			//std::cout << "GC is in urgent mode!!!" << std::endl;
			
#if SCHEDULE_NORM


	
			if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{

				if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip) && GCReadTRQueue[chip->ChannelID][chip->ChipID].size()) { //doodu debug.
					std::cout << "(urgent)UsrRQ: " << UserReadTRQueue[chip->ChannelID][chip->ChipID].size() << ", GCRQ: " << GCReadTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
				}
#if ADAPTIVE_GCUSR  //made by doodu
				if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() < 50) {
					sourceQueue1 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
					Stats::serve_GC_read++;
					if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
						sourceQueue2 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
				}
				else {
					if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
						sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
					sourceQueue2 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
					Stats::serve_GC_read++;
				}
#else 
				sourceQueue1 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
				Stats::serve_GC_read++;
				if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
					sourceQueue2 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
#endif

			}
			else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				Stats::serve_nothing_R++;
				return false;
			}
			else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				Stats::serve_nothing_R++;
				return false;
			}
			else if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
			}
			else {
				Stats::serve_nothing_R++;
				return false;
			}
#elif SCHEDULE_ATC
			if ( bUserReadOnlySchedule == false)
			{

				if ( GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0 )
				{
					if ( (snGCReadSchCount % 3 == 0) && (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0) ) 
					{
						//std::cout << "flag 1" << std::endl;
						srcQ1isUsrQ = true;
						sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
						//std::cout << "not an error. check service_read_transaction [doodu]" << std::endl;  exit(1);
					}
					else
					{
						//std::cout << "flag 2" << std::endl;////
						sourceQueue1 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
						Stats::serve_GC_read++;
						
					}
					snGCReadSchCount++;
				}
				
				else if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				{ // User Read Prior Service
					//std::cout << "flag 3" << std::endl;
					srcQ1isUsrQ = true;
					sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
				}
				
				else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
					//std::cout << "flag 4" << std::endl;////
					Stats::Total_R_unserviced++;
					Stats::serve_nothing_R++;
					return false;
				}
				else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
					//std::cout << "flag 5" << std::endl;
					Stats::Total_R_unserviced++;
					Stats::serve_nothing_R++;
					return false;
				}
				/*
else if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
{ // User Read Prior Service
	//std::cout << "flag 3" << std::endl;
	srcQ1isUsrQ = true;
	sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
}*/
				else {
					//std::cout << "flag 6" << std::endl;////
					Stats::Total_R_unserviced++;
					Stats::serve_nothing_R++;
					return false;

				}
			}

			else
			{

				if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				{ // User Read Prior Service
					//std::cout << "flag7" << std::endl;
					//std::cout << "GCRQ: " << GCReadTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
					srcQ1isUsrQ = true;
					sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
				}
				else 
				{
					//std::cout << "flag8" << std::endl;///
					Stats::Total_R_unserviced++;
					Stats::serve_nothing_R++;
					return false;
				}

			}
			 //flag 0225 doodu
#endif
		} 

		else //If GC is currently executed in the preemptive mode, then user IO transaction queues are checked first
		{
			if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				srcQ1isUsrQ = true;
				sourceQueue1 = &UserReadTRQueue[chip->ChannelID][chip->ChipID];
				if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
					sourceQueue2 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
			}
			else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {//write barrier? doodu
				Stats::Total_unserviced_R_barrier++;
				//std::cout << "hey" << std::endl;
				Stats::Total_R_unserviced++;
				return false;
			}
			else if (GCReadTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {/////////////////////////////////////////////////////////////////////
				Stats::Total_only_GC_read++;
				sourceQueue1 = &GCReadTRQueue[chip->ChannelID][chip->ChipID];
				//std::cout << "GCReadTRQ size: " << GCReadTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
			}
			else {
				Stats::Total_R_unserviced++;
				return false;
			}
		}

		bool suspensionRequired = false;		
		ChipStatus cs = _NVMController->GetChipStatus(chip);

		switch (cs)
		{
		case ChipStatus::IDLE:
			break;
		case ChipStatus::WRITING:
			Stats::Total_reads_while_writing++;
			if (PGM_SUS_ENABLE == false)
			{
				Stats::Total_R_unserviced++;
				return false;
			}
			else
			{
				if (_NVMController->HasSuspendedCommand(chip)) {
					Stats::Total_R_unserviced++;
					return false;
				}
				if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() == 0)
				{
					Stats::Total_R_unserviced++;
					return false;
				} 
				if ( (_NVMController->Expected_finish_time(chip) > Simulator->Time()) && 
					(_NVMController->Expected_finish_time(chip) - Simulator->Time() < writeReasonableSuspensionTimeForRead) )
				{
					Stats::Total_R_unserviced++;
					return false;
				}

				if ( true == _NVMController->Check_PGM_suspend_threshold( chip->ChannelID, chip->ChipID, 0) )
				{
					Stats::Total_R_unserviced++;
					return false;
				}
				

				suspensionRequired = true;
				break;
			}
		case ChipStatus::ERASING:
			Stats::Total_reads_while_erasing++;
			if (srcQ1isUsrQ == true) Stats::Total_USER_reads_while_erasing++;




#if (ERS_CANCEL_ENABLE == false)
			return false;
#endif

			//std::cout << "flag1[subers]" << std::endl;
			if (!eraseSuspensionEnabled || _NVMController->HasSuspendedCommand(chip)) {
				Stats::Total_R_unserviced++;
				return false;
			}

			if (UserReadTRQueue[chip->ChannelID][chip->ChipID].size() == 0)
			{ // the ERS operation must do not suspend by GC Read 
				Stats::Total_R_unserviced++;
				return false;
			}

			
#if (ERS_IDEAL_SUS_ENABLE)			
			eraseReasonableSuspensionTimeForRead = 0;
#endif



			if ( (_NVMController->Expected_finish_time(chip) > Simulator->Time()) && 
				(_NVMController->Expected_finish_time(chip) - Simulator->Time() < eraseReasonableSuspensionTimeForRead) )
			{
				Stats::Total_R_unserviced++;
				return false;
			}
			//std::cout << "flag3[subers]" << std::endl;
			if ( true == _NVMController->Check_ERS_suspend_threshold( chip->ChannelID, chip->ChipID, 0) )
			{
				Stats::Total_R_unserviced++;
				return false;
			}

			//std::cout << "flag4[subers]" << std::endl;

			if (gnStartPrint == true && chip->ChannelID == DEBUG_CH )
			{
				std::cout << "TSU::SRT: SUS: TRUE:"
					<< " CH_ID:" << chip->ChannelID
					<< " CHIP_ID: " << chip->ChipID
					<< " NVM_EFT: " << _NVMController->Expected_finish_time(chip) << " ST: " << Simulator->Time() << std::endl;
			}

			//std::cout << "suspensionRequired is true though erase suspension is not activated [doodu]" << std::endl;
			suspensionRequired = true;
			break;
			
		default:
			/*
			switch (cs) {// IDLE, CMD_IN, CMD_DATA_IN, DATA_OUT, READING, WRITING, ERASING, WAIT_FOR_DATA_OUT, WAIT_FOR_COPYBACK_CMD,
			case ChipStatus::CMD_IN:
				std::cout << "1, ";
				break;
			case ChipStatus::CMD_DATA_IN:
				std::cout << "2, ";
				break;
			case ChipStatus::DATA_OUT:
				std::cout << "3, ";
				break;
			case ChipStatus::READING:////
				std::cout << "4, ";
				break;
			case ChipStatus::WAIT_FOR_DATA_OUT:
				std::cout << "5, ";
				break;
			case ChipStatus::WAIT_FOR_COPYBACK_CMD:
				std::cout << "6, ";
				break;
			}
			*/

			if (cs==ChipStatus::READING) {// && srcQ1isUsrQ==true) {
				Stats::Total_USER_reads_while_reading++;
			}
			Stats::Total_R_unserviced++;
			return false;
		}
		
		flash_die_ID_type dieID = sourceQueue1->front()->Address.DieID;
		flash_page_ID_type pageID = sourceQueue1->front()->Address.PageID;
		unsigned int planeVector = 0;
		unsigned int cnt_command = 0;
		for (unsigned int i = 0; i < die_no_per_chip; i++)
		{
			transaction_dispatch_slots.clear();
			planeVector = 0;
			for (Flash_Transaction_Queue::iterator it = sourceQueue1->begin(); it != sourceQueue1->end();)
			{
				if ((*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID)) 
				{
#if NO_MULTIPLANE_COMMAND
					if (cnt_command >= 1) break;
#endif
					if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
					{
						(*it)->SuspendRequired = suspensionRequired;
						planeVector |= 1 << (*it)->Address.PlaneID;
						transaction_dispatch_slots.push_back(*it);
						sourceQueue1->remove(it++);
						cnt_command++;
						continue;
					}
				}
				it++;
			}

			if (sourceQueue2 != NULL && transaction_dispatch_slots.size() < plane_no_per_die)
				for (Flash_Transaction_Queue::iterator it = sourceQueue2->begin(); it != sourceQueue2->end();)
				{
					if ((*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
					{
#if NO_MULTIPLANE_COMMAND
						if (cnt_command >= 1) break;
#endif
						if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
						{
							(*it)->SuspendRequired = suspensionRequired;
							planeVector |= 1 << (*it)->Address.PlaneID;
							transaction_dispatch_slots.push_back(*it);
							sourceQueue2->remove(it++);
							cnt_command++;
							continue;
						}
					}
					it++;
				}

			////std::cout << "sourceQueue1->size: " << sourceQueue1->size() << ", selected requests to be serviced: " << transaction_dispatch_slots.size() << std::endl;
			if (transaction_dispatch_slots.size() > 0)
				_NVMController->Send_command_to_chip(transaction_dispatch_slots);
			transaction_dispatch_slots.clear();
			dieID = (dieID + 1) % die_no_per_chip;
		}

		return true;
	}
	
	bool TSU_OutOfOrder::service_write_transaction(NVM::FlashMemory::Flash_Chip* chip)
	{
		Flash_Transaction_Queue *sourceQueue1 = NULL, *sourceQueue2 = NULL;

		if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(chip))//If flash transactions related to GC are prioritzed (non-preemptive execution mode of GC), then GC queues are checked first
		{
#if (1)	
#if SCHEDULE_ATC
			if ((true == INCREMENTAL_GC) &&
				((chip->UrgentGCWriteSchCount % 20) == 0) && 
				(UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) )
			{				
				sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
				if ( (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() == 0 ) &&
					(GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0) )
				{
//					std::cout << "CH: " << chip->ChannelID 
//						<< " CHIP: "<< chip->ChipID 
//						<< " UserCount: " << UserWriteTRQueue[chip->ChannelID][chip->ChipID].size()
//						<< " ERSCount: " << GCEraseTRQueue[chip->ChannelID][chip->ChipID].size()
//						<< std::endl;
					return false;
				}

			}
			else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
				if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
					sourceQueue2 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];

			}
			else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				return false;
			}
			else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
			else
			{
				return false;
			}
#elif SCHEDULE_NORM
			if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
				if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				{
					sourceQueue2 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
				}
			}
			else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				return false;
			}
			else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
			else
			{
				return false;
			}
#endif

#else
			if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
				if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)	
					sourceQueue2 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
			else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
			else if (GCEraseTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				return false;
			else return false;
#endif
		}
		else //If GC is currently executed in the preemptive mode, then user IO transaction queues are checked first
		{
#if SCHEDULE_GC_USR
			
			if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
				if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
					//doodu_debug
					//std::cout << "GCWTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() <<", "<< "UsrWTRQ size: " << UserWriteTRQueue[chip->ChannelID][chip->ChipID].size()<< std::endl;
					sourceQueue2 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
				}
			}
			else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				//doodu_debug
				//std::cout << "GCWriteTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
				sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
			else return false;
			/*
			cnt_GC_opportunity++;
			if (cnt_GC_opportunity % 4 != 0) {
				if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				{
					sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
					if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
						//doodu_debug
						//std::cout << "GCWTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() <<", "<< "UsrWTRQ size: " << UserWriteTRQueue[chip->ChannelID][chip->ChipID].size()<< std::endl;
						sourceQueue2 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
					}
				}
				else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
					//doodu_debug
					//std::cout << "GCWriteTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
					sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
				}
				else return false;
			}
			else {
				if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
				{
					sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
					if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
						//doodu_debug
						//std::cout << "GCWTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() <<", "<< "UsrWTRQ size: " << UserWriteTRQueue[chip->ChannelID][chip->ChipID].size()<< std::endl;
						sourceQueue2 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
					}
				}
				else if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
					//doodu_debug
					//std::cout << "GCWriteTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
					sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
				}
				else return false;
			}
			*/

#else 
			//std::cout << "Channel: " << chip->ChannelID << ", Chip:" << chip->ChipID << std::endl;
			if (chip->ChannelID == 0 && chip->ChipID == 0) {
				//std::cout << "GCWQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() << ", " << "UsrWQ size: " << UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
			}
			if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0)
			{
				sourceQueue1 = &UserWriteTRQueue[chip->ChannelID][chip->ChipID];
				if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
					//doodu_debug
					//std::cout << "GCWTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() <<", "<< "UsrWTRQ size: " << UserWriteTRQueue[chip->ChannelID][chip->ChipID].size()<< std::endl;
					sourceQueue2 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
				}
			}
			else if (GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				//doodu_debug
				//std::cout << "GCWriteTRQ size: " << GCWriteTRQueue[chip->ChannelID][chip->ChipID].size() << std::endl;
				sourceQueue1 = &GCWriteTRQueue[chip->ChannelID][chip->ChipID];
			}
			else return false;
#endif
		}


		bool suspensionRequired = false;
		ChipStatus cs = _NVMController->GetChipStatus(chip);
#if NEW_SUBMIT
		ChipStatus subcs_w = _NVMController->GetChipSubStatus_W(chip);
#endif 
		Stats::total_write_try++;
		switch (cs)
		{
		case ChipStatus::IDLE:
			break;
		case ChipStatus::ERASING:
			if (!eraseSuspensionEnabled || _NVMController->HasSuspendedCommand(chip)) {
				return false;
			}
			if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < eraseReasonableSuspensionTimeForWrite) {
				return false;
			}
			suspensionRequired = true;
		case ChipStatus::WRITING: { 
			Stats::write_while_WRITING++;
			if (UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				Stats::USRwrite_while_WRITING++;
			}
			if (subcs_w == ChipStatus::ALL_PLANES_GCW && UserWriteTRQueue[chip->ChannelID][chip->ChipID].size() > 0) {
				//std::cout << "here!" << std::endl;
				Stats::USRwrite_while_GCWRITING++;
			}
			//std::cout << "[doodu]here3" << std::endl;
			return false;
		}
		default:
			//std::cout << "[doodu]here4" << std::endl;
			return false;
		}

		flash_die_ID_type dieID = sourceQueue1->front()->Address.DieID;
		flash_page_ID_type pageID = sourceQueue1->front()->Address.PageID;
		unsigned int planeVector = 0;
		unsigned int cnt_command = 0;
		for (unsigned int i = 0; i < die_no_per_chip; i++)
		{
			transaction_dispatch_slots.clear();
			planeVector = 0;

			for (Flash_Transaction_Queue::iterator it = sourceQueue1->begin(); it != sourceQueue1->end(); )
			{
				if (((NVM_Transaction_Flash_WR*)*it)->RelatedRead == NULL && (*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
				{
#if NO_MULTIPLANE_COMMAND
					if (cnt_command >= 1) break;
#endif
					if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
					{
						//std::cout << "pg Addr " << (*it)->Address.PageID << std::endl;
						(*it)->SuspendRequired = suspensionRequired;
						planeVector |= 1 << (*it)->Address.PlaneID;
						transaction_dispatch_slots.push_back(*it);
						sourceQueue1->remove(it++);
						cnt_command++;
						continue;
					}
				}
				it++;
			}

			if (sourceQueue2 != NULL)
				for (Flash_Transaction_Queue::iterator it = sourceQueue2->begin(); it != sourceQueue2->end(); )
				{
					if (((NVM_Transaction_Flash_WR*)*it)->RelatedRead == NULL && (*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
					{
#if NO_MULTIPLANE_COMMAND
						if (cnt_command >= 1) break;
#endif
						if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
						{
							(*it)->SuspendRequired = suspensionRequired;/////
							planeVector |= 1 << (*it)->Address.PlaneID;
							transaction_dispatch_slots.push_back(*it);
							sourceQueue2->remove(it++);
							cnt_command++;
							continue;
						}
					}
					it++;
				}

			if (transaction_dispatch_slots.size() > 0) {
				//doodu_debug
				/*
				for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_dispatch_slots.begin(); it != transaction_dispatch_slots.end(); it++) {
					if ((*it)->Source == Transaction_Source_Type::USERIO) {
						std::cout << "U ";
					}
					else if ((*it)->Source == Transaction_Source_Type::GC_WL) {
						std::cout << "G ";
					}
				}
				std::cout << std::endl;
				*/

				_NVMController->Send_command_to_chip(transaction_dispatch_slots);
			}
			transaction_dispatch_slots.clear();
			dieID = (dieID + 1) % die_no_per_chip;
		}

		
		chip->UrgentGCWriteSchCount++;
		return true;
	}

	bool TSU_OutOfOrder::service_erase_transaction(NVM::FlashMemory::Flash_Chip* chip)
	{
		if (_NVMController->GetChipStatus(chip) != ChipStatus::IDLE)
			return false;

		Flash_Transaction_Queue* source_queue = &GCEraseTRQueue[chip->ChannelID][chip->ChipID];
		if (source_queue->size() == 0)
			return false;

		flash_die_ID_type dieID = source_queue->front()->Address.DieID;
		unsigned int planeVector = 0;
		for (unsigned int i = 0; i < die_no_per_chip; i++)
		{
			transaction_dispatch_slots.clear();
			planeVector = 0;

			for (Flash_Transaction_Queue::iterator it = source_queue->begin(); it != source_queue->end(); )
			{
				if (((NVM_Transaction_Flash_ER*)*it)->Page_movement_activities.size() == 0 && (*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
				{

					planeVector |= 1 << (*it)->Address.PlaneID;
					transaction_dispatch_slots.push_back(*it);
					source_queue->remove(it++);
				}
				else
				{
					it++;
				}
			}


			if (transaction_dispatch_slots.size() > 0)
				_NVMController->Send_command_to_chip(transaction_dispatch_slots);
			transaction_dispatch_slots.clear();
			dieID = (dieID + 1) % die_no_per_chip;
		}


		for (Flash_Transaction_Queue::iterator it = transaction_dispatch_slots.begin(); it != transaction_dispatch_slots.end(); it++) //0113flag
		{
			//std::cout << "Cha_Chip_D_PL_isSubTr: " << (*it)->Address.ChannelID << ", " << (*it)->Address.ChipID << ", " << (*it)->Address.PlaneID << ", " << (*it)->Address.BlockID <<", "<< (*it)->isSubblock_Erase_Tr<< std::endl;
		}

		return true;
	}
}