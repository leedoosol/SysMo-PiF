#include <math.h>
#include <vector>
#include <set>
#include "GC_and_WL_Unit_Page_Level.h"
#include "Flash_Block_Manager.h"
#include "FTL.h"
#include "../host/IO_Flow_Base.h"

//extern std::ofstream doodu_log;
extern int primary_cnt;
FILE* free_log_file = fopen("free_block_log.txt", "a");
#define FREE_BLOCK_LOG 1
extern bool finished_preconditioning;
long degree_th = 0.7;


/*
struct subblk_entry {
	std::uint32_t ChannelID;
	std::uint32_t ChipID;
	std::uint32_t DieID;
	std::uint32_t PlaneID;
	std::uint32_t BlockID;
};

std::vector <struct subblk_entry> subblks;
*/
//extern struct subblk_entry;
extern struct subblk_entry {
	std::uint32_t ChannelID;
	std::uint32_t ChipID;
	std::uint32_t DieID;
	std::uint32_t PlaneID;
	std::uint32_t BlockID;
};
extern std::vector <subblk_entry> subblks;
int abc;


namespace SSD_Components
{

	GC_and_WL_Unit_Page_Level::GC_and_WL_Unit_Page_Level(const sim_object_id_type& id,
		Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller, 
		GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
		unsigned int ChannelCount, unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
		unsigned int block_no_per_plane, unsigned int Page_no_per_block, unsigned int sectors_per_page, 
		bool use_copyback, double rho, unsigned int max_ongoing_gc_reqs_per_plane, bool dynamic_wearleveling_enabled, bool static_wearleveling_enabled, unsigned int static_wearleveling_threshold, int seed)
		: GC_and_WL_Unit_Base(id, address_mapping_unit, block_manager, tsu, flash_controller, block_selection_policy, gc_threshold, preemptible_gc_enabled, gc_hard_threshold,
		ChannelCount, chip_no_per_channel, die_no_per_chip, plane_no_per_die, block_no_per_plane, Page_no_per_block, sectors_per_page, use_copyback, rho, max_ongoing_gc_reqs_per_plane, 
			dynamic_wearleveling_enabled, static_wearleveling_enabled, static_wearleveling_threshold, seed)
	{
		rga_set_size = (unsigned int)log2(block_no_per_plane);
	}
	
	bool GC_and_WL_Unit_Page_Level::GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip* chip)
	{
		if (!preemptible_gc_enabled)
			return true;

		NVM::FlashMemory::Physical_Page_Address addr;
		addr.ChannelID = chip->ChannelID; addr.ChipID = chip->ChipID;
		for (unsigned int die_id = 0; die_id < die_no_per_chip; die_id++)
			for (unsigned int plane_id = 0; plane_id < plane_no_per_die; plane_id++)
			{
				addr.DieID = die_id; addr.PlaneID = plane_id;
#if NEW_SUBMIT
				if (block_manager->Get_pool_size(addr) < block_pool_gc_hard_threshold * BLOCK_POOL_GC_HARD_THRESHOLD_ADJUST) {
#else
				if (block_manager->Get_pool_size(addr) < block_pool_gc_hard_threshold) {
#endif
					std::cout << "block_pool_gc_hard_threshold: " << block_pool_gc_hard_threshold <<" > "<< " pool_size: " << block_manager->Get_pool_size(addr) << std::endl;

					return true;
				}
			}
		return false;
	}
	//don't use this function
	int GC_and_WL_Unit_Page_Level::Check_gc_required4align(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address, int nextVictim)
	{
		//std::cout << "[DOODU_DEBUG] channel,chip,die,plane: " << plane_address.ChannelID <<","<< plane_address.ChipID << "," << plane_address.DieID << "," << plane_address.PlaneID << std::endl;
		//std::cout << "[DOODU_DEBUG 1] free_block_pool_size: " << free_block_pool_size << ", GC threshold: " << block_pool_gc_threshold << std::endl;
		//if ((free_block_pool_size < block_pool_gc_threshold)) {
		//std::cout << "Subblock erase is ongoing [doodu]" << std::endl;
		if (nextVictim < 0) {
			std::cout << "[Error] check 'check_gc_requiredForce()" << std::endl;
			exit(1);
		}
		flash_block_ID_type gc_candidate_block_id = nextVictim;
		PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);

		if (pbke->Ongoing_erase_operations.size() >= max_ongoing_gc_reqs_per_plane) {
			return -1;
		}

		//This should never happen, but we check it here for safty
		if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id) != pbke->Ongoing_erase_operations.end()) {
			return -1;
		}

#ifdef NEW_SUBMIT
		/*
	PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
	return plane_record->Blocks[block_address.BlockID].Has_ongoing_gc_wl;
		*/
		////std::cout << "[DOODU_DEBUG](Check_gc_required2) gc_candidate_block_id: " << gc_candidate_block_id << std::endl;

#endif
		NVM::FlashMemory::Physical_Page_Address gc_candidate_address(plane_address);
		gc_candidate_address.BlockID = gc_candidate_block_id;
		Block_Pool_Slot_Type* block = &pbke->Blocks[gc_candidate_block_id];

		//No invalid page to erase
		if (block->Current_page_write_index == 0 || block->Invalid_page_count == 0) {
			return -1;
		}

		//Run the state machine to protect against race condition
		block_manager->GC_WL_started(gc_candidate_address);
		pbke->Ongoing_erase_operations.insert(gc_candidate_block_id);
		address_mapping_unit->Set_barrier_for_accessing_physical_block(gc_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing

		//If there are ongoing requests targeting the candidate block, the gc execution should be postponed
		if (block_manager->Can_execute_gc_wl(gc_candidate_address)) {
			Stats::Total_gc_executions++;
			tsu->Prepare_for_transaction_submit();

			NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, pbke->Blocks[gc_candidate_block_id].Stream_id, gc_candidate_address);

			//If there are some valid pages in block, then prepare flash transactions for page movement
			//카피할 게 있으면 카피하고(read_tr, write_tr등), 없으면 gc_erase_tr만 submit()함. doodu
			if (block->Current_page_write_index - block->Invalid_page_count > 0) {
				NVM_Transaction_Flash_RD* gc_read = NULL;
				NVM_Transaction_Flash_WR* gc_write = NULL;
				for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++) {
					if (block_manager->Is_page_valid(block, pageID)) {
						Stats::Total_page_movements_for_gc++;
						gc_candidate_address.PageID = pageID;
						if (use_copyback) {
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
							tsu->Submit_transaction(gc_write);
						}
						else {
							gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), gc_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, NO_PPA, gc_candidate_address, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
							gc_write->RelatedErase = gc_erase_tr;
							gc_read->RelatedWrite = gc_write;
							tsu->Submit_transaction(gc_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined

						}
						gc_erase_tr->Page_movement_activities.push_back(gc_write);
					}
				}
			}

			block->Erase_transaction = gc_erase_tr;
			tsu->Submit_transaction(gc_erase_tr);
			tsu->Schedule();
		}
		return -1;

		//}
	}

	int GC_and_WL_Unit_Page_Level::Check_gc_requiredForce(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address, int nextVictim, int isSubblock)
	{
		//std::cout << "[DOODU_DEBUG] channel,chip,die,plane: " << plane_address.ChannelID <<","<< plane_address.ChipID << "," << plane_address.DieID << "," << plane_address.PlaneID << std::endl;
		//std::cout << "[DOODU_DEBUG 1] free_block_pool_size: " << free_block_pool_size << ", GC threshold: " << block_pool_gc_threshold << std::endl;
		//if ((free_block_pool_size < block_pool_gc_threshold)) {
		//std::cout << "Subblock erase is ongoing [doodu]" << std::endl;
		if (nextVictim < 0) {
			std::cout << "[Error] check 'check_gc_requiredForce()" << std::endl;
			exit(1);
		}
		flash_block_ID_type gc_candidate_block_id = nextVictim;
		PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);

		if (pbke->Ongoing_erase_operations.size() >= max_ongoing_gc_reqs_per_plane) {
			return -1;
		}

		//This should never happen, but we check it here for safty
		if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id) != pbke->Ongoing_erase_operations.end()) {
			return -1;
		}

#ifdef NEW_SUBMIT
		/*
	PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
	return plane_record->Blocks[block_address.BlockID].Has_ongoing_gc_wl;
		*/
		////std::cout << "[DOODU_DEBUG](Check_gc_required2) gc_candidate_block_id: " << gc_candidate_block_id << std::endl;

#endif
		NVM::FlashMemory::Physical_Page_Address gc_candidate_address(plane_address);
		gc_candidate_address.BlockID = gc_candidate_block_id;
		Block_Pool_Slot_Type* block = &pbke->Blocks[gc_candidate_block_id];

		//No invalid page to erase
		if (block->Current_page_write_index == 0 || block->Invalid_page_count == 0) {
			return -1;
		}

		//Run the state machine to protect against race condition
		block_manager->GC_WL_started(gc_candidate_address);
		pbke->Ongoing_erase_operations.insert(gc_candidate_block_id);
		address_mapping_unit->Set_barrier_for_accessing_physical_block(gc_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing

		//If there are ongoing requests targeting the candidate block, the gc execution should be postponed
		if (block_manager->Can_execute_gc_wl(gc_candidate_address)) {
			//Stats::Total_gc_executions++; //We doesn't count this GC because this is for subblock (multi-block erase in plane) erase [doodu]
			tsu->Prepare_for_transaction_submit();

			NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, pbke->Blocks[gc_candidate_block_id].Stream_id, gc_candidate_address);
			gc_erase_tr->isSubblock_Erase_Tr = true;
			
			//If there are some valid pages in block, then prepare flash transactions for page movement
			//카피할 게 있으면 카피하고(read_tr, write_tr등), 없으면 gc_erase_tr만 submit()함. doodu
			if (block->Current_page_write_index - block->Invalid_page_count > 0) {
				NVM_Transaction_Flash_RD* gc_read = NULL;
				NVM_Transaction_Flash_WR* gc_write = NULL;
				for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++) {
					if (block_manager->Is_page_valid(block, pageID)) {
						Stats::Total_page_movements_for_gc++;
						gc_candidate_address.PageID = pageID;
						if (use_copyback) {
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
							tsu->Submit_transaction(gc_write);
						}
						else {
							gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), gc_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
							gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
								NO_LPA, NO_PPA, gc_candidate_address, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
							gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
							gc_write->RelatedErase = gc_erase_tr;
							gc_read->RelatedWrite = gc_write;
							tsu->Submit_transaction(gc_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined

						}
						gc_erase_tr->Page_movement_activities.push_back(gc_write);
					}
				}
			}

			block->Erase_transaction = gc_erase_tr;
			tsu->Submit_transaction(gc_erase_tr);
			tsu->Schedule();
		}
		return -1;

		//}
	}

	struct cand_blks GC_and_WL_Unit_Page_Level::Check_gc_required(const unsigned int free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address,int victim)
	{
		
		//std::cout << "[DOODU] free_block_pool_size: " << free_block_pool_size <<", block_pool_gc_threshold: "<< block_pool_gc_threshold << std::endl;
		bool foundGC_candidate_id2 = false;
		bool foundGC_candidate_id3 = false;
		bool foundGC_candidate_id4 = false;
		bool foundGC_candidate_id5 = false;
		bool foundGC_candidate_id6 = false;
		bool foundGC_candidate_id7 = false;
		bool foundGC_candidate_id8 = false;
		struct cand_blks victims; victims.cnt = 0; 

		if (finished_preconditioning) degree_th = 0.7;
		//std::cout << "free_block_pool_size: " << free_block_pool_size << " < " << block_pool_gc_threshold * 0.7 << std::endl;
		//block_pool_gc_threshold = (unsigned int)(gc_threshold * (double)block_no_per_plane);
		if (free_block_pool_size < block_pool_gc_threshold* 0.7) //1.2 before 2020-12-13
		{
			Stats::cnt_check_gc++;
#if FREE_BLOCK_LOG
			fprintf(free_log_file, "%d %d %d %d %d %d\n", (Simulator->Time() / SIM_TIME_TO_MICROSECONDS_COEFF),free_block_pool_size, plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID);
#endif
			//std::cout<<"free_block_pool_size: "<< free_block_pool_size<<" < "<< block_pool_gc_threshold * 0.7<<std::endl;
			//free_block_log<<
			
#if SOFT_ISOLATION
			flash_block_ID_type gc_candidate_block_id_codes = 0;
#endif
			flash_block_ID_type gc_candidate_block_id = block_manager->Get_coldest_block_id(plane_address);
			PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
//#if NEW_SUBMIT
			flash_block_ID_type gc_candidate_block_id2 = block_manager->Get_2ndColdest_block_id(plane_address);
			flash_block_ID_type gc_candidate_block_id3 = 0;
			flash_block_ID_type gc_candidate_block_id4 = 0;
			flash_block_ID_type gc_candidate_block_id5 = 0;
			flash_block_ID_type gc_candidate_block_id6 = 0;
			flash_block_ID_type gc_candidate_block_id7 = 0;
			flash_block_ID_type gc_candidate_block_id8 = 0;

			if (gc_candidate_block_id == gc_candidate_block_id2) {
				std::cout << "Stop!! candidate blocks are same" << std::endl;
				exit(1);
			}
//#endif
			/*//doodu 
			if (pbke->Ongoing_erase_operations.size() >= max_ongoing_gc_reqs_per_plane*10) {
				std::cout << "Ongoing erase_operations size() >= max_ongoing_gc_reqs_per_plane" << std::endl;
				std::cout << "Ongoing_erase_operations.size():" << pbke->Ongoing_erase_operations.size() << ", max_ongoing_gc_reqs_per_plane*10: " << max_ongoing_gc_reqs_per_plane*10 << std::endl;
				victims.cnt = 0;
				return victims;
			}
			*/

			switch (block_selection_policy)
			{
			case SSD_Components::GC_Block_Selection_Policy_Type::GREEDY://Find the set of blocks with maximum number of invalid pages and no free pages
			{
				gc_candidate_block_id = 0;
//#if NEW_SUBMIT
				gc_candidate_block_id2 = 0;
				gc_candidate_block_id3 = 0;
				gc_candidate_block_id4 = 0;
				gc_candidate_block_id5 = 0;
				gc_candidate_block_id6 = 0;
				gc_candidate_block_id7 = 0;
				gc_candidate_block_id8 = 0;
//#endif
				if (pbke->Ongoing_erase_operations.find(0) != pbke->Ongoing_erase_operations.end()) {
//#ifdef NEW_SUBMIT3
					gc_candidate_block_id2++;
					gc_candidate_block_id3++;
					gc_candidate_block_id4++;
					gc_candidate_block_id5++;
					gc_candidate_block_id6++;
					gc_candidate_block_id7++;
					gc_candidate_block_id8++;
//#endif
					gc_candidate_block_id++;
				}

				//find block with 1nd fewest valid pages
				int invalid_pgs = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++)
				{
					if (pbke->Blocks[block_id].Invalid_page_count > pbke->Blocks[gc_candidate_block_id].Invalid_page_count
						&& pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block
						&& is_safe_gc_wl_candidate(pbke, block_id)) {
							gc_candidate_block_id = block_id;
							invalid_pgs = pbke->Blocks[block_id].Invalid_page_count;
					}
				}
				////std:: cout << "invalid_pgs: " << invalid_pgs << std::endl;

#ifdef NEW_SUBMIT
				
				
#if FREE_ALIGN
				//int cnt = 0;
				int min_erased_block2_v = 0;
				int threshold_Invalid = pages_no_per_block / INVALID_GRANU; 
				threshold_Invalid *= THRESHOLD_INVALID; 
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					//std::cout << "?????????????????????????" << std::endl;
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block2_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block2_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id2 = block_id;
						foundGC_candidate_id2 = true;
					}
				}

				if (MAX_SUBBLOCK_ERS_CNT >= 1) {
					if (foundGC_candidate_id2) {
						victims.blocks.push_back(gc_candidate_block_id2); victims.cnt++;
						subblk_entry sub2 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id2 };
						subblks.push_back(sub2);
					}
				}


				//Sel 1: find block with 3nd fewest valid pages among all blocks
				int min_erased_block3_v = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block3_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block3_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id3 = block_id;
						foundGC_candidate_id3 = true;
						//cnt++;
					}
				}

				if (MAX_SUBBLOCK_ERS_CNT >= 2) {
					if (foundGC_candidate_id3) {
						victims.blocks.push_back(gc_candidate_block_id3); victims.cnt++;
						subblk_entry sub3 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id3 };
						subblks.push_back(sub3);
					}
				}
				

				//Sel 1: find block with 4nd fewest valid pages among all blocks
				int min_erased_block4_v = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block4_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block4_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id4 = block_id;
						foundGC_candidate_id4 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 3) {
					if (foundGC_candidate_id4) {
						victims.blocks.push_back(gc_candidate_block_id4); victims.cnt++;
						subblk_entry sub4 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id4 };
						subblks.push_back(sub4);
					}
				}

				//Sel 1: find block with 5nd fewest valid pages among all blocks
				int min_erased_block5_v = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block5_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block5_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id5 = block_id;
						foundGC_candidate_id5 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 4) {
					if (foundGC_candidate_id5) {
						victims.blocks.push_back(gc_candidate_block_id5); victims.cnt++;
						subblk_entry sub5 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id5 };
						subblks.push_back(sub5);
					}
				}

				//Sel 1: find block with 6nd fewest valid pages among all blocks
				int min_erased_block6_v = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block6_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4) && (block_id != gc_candidate_block_id5)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block6_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id6 = block_id;
						foundGC_candidate_id6 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 5) {
					if (foundGC_candidate_id6) {
						victims.blocks.push_back(gc_candidate_block_id6); victims.cnt++;
						subblk_entry sub6 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id6 };
						subblks.push_back(sub6);
					}
				}

				//Sel 1: find block with 7nd fewest valid pages among all blocks
				int min_erased_block7_v = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block7_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4) && (block_id != gc_candidate_block_id5) && (block_id != gc_candidate_block_id6)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block7_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id7 = block_id;
						foundGC_candidate_id7 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 6) {
					if (foundGC_candidate_id7) {
						victims.blocks.push_back(gc_candidate_block_id7); victims.cnt++;
						subblk_entry sub7 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id7 };
						subblks.push_back(sub7);
					}
				}


				//Sel 1: find block with 7nd fewest valid pages among all blocks
				int min_erased_block8_v = 0;
				for (flash_block_ID_type block_id = 1; block_id < block_no_per_plane; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block8_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4) && (block_id != gc_candidate_block_id5) && (block_id != gc_candidate_block_id6) && (block_id != gc_candidate_block_id7)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block8_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id8 = block_id;
						foundGC_candidate_id8 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 7) {
					if (foundGC_candidate_id8) {
						victims.blocks.push_back(gc_candidate_block_id8); victims.cnt++;
						subblk_entry sub8 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id8 };
						subblks.push_back(sub8);
					}
				}
#else if STRICT_ALIGN
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				
				//Sel 2: find block with 2nd fewest valid pages restrictly (find among 4 blocks near to gc_candidate_block_id. Inconsitency problem can arise depend on trace characteristic
				int align_unit = 16; //flexibility of victim block selection 16
				int threshold_Invalid = pages_no_per_block / INVALID_GRANU;
				threshold_Invalid *= THRESHOLD_INVALID;
				flash_block_ID_type start_block_id = (gc_candidate_block_id / align_unit);
				start_block_id *= align_unit;

				int min_erased_block2_v = 0;
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id+ align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count >= min_erased_block2_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) 
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						min_erased_block2_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id2 = block_id;
						foundGC_candidate_id2 = true;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 1) {
					if (foundGC_candidate_id2) {
						victims.blocks.push_back(gc_candidate_block_id2); victims.cnt++;
						subblk_entry sub2 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id2 };
						subblks.push_back(sub2);
					}
				}

				//Sel 1: find block with 3nd fewest valid pages among all blocks
				int min_erased_block3_v = 0;
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id + align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block3_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block3_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id3 = block_id;
						foundGC_candidate_id3 = true;
						//cnt++;
					}
				}

				if (MAX_SUBBLOCK_ERS_CNT >= 2) {
					if (foundGC_candidate_id3) {
						victims.blocks.push_back(gc_candidate_block_id3); victims.cnt++;
						subblk_entry sub3 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id3 };
						subblks.push_back(sub3);
					}
				}


				//Sel 1: find block with 4nd fewest valid pages among all blocks
				int min_erased_block4_v = 0;
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id + align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block4_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block4_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id4 = block_id;
						foundGC_candidate_id4 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 3) {
					if (foundGC_candidate_id4) {
						victims.blocks.push_back(gc_candidate_block_id4); victims.cnt++;
						subblk_entry sub4 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id4 };
						subblks.push_back(sub4);
					}
				}

				//Sel 1: find block with 5nd fewest valid pages among all blocks
				int min_erased_block5_v = 0;
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id + align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block5_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block5_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id5 = block_id;
						foundGC_candidate_id5 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 4) {
					if (foundGC_candidate_id5) {
						victims.blocks.push_back(gc_candidate_block_id5); victims.cnt++;
						subblk_entry sub5 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id5 };
						subblks.push_back(sub5);
					}
				}

				//Sel 1: find block with 6nd fewest valid pages among all blocks
				int min_erased_block6_v = 0;
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id + align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block6_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4) && (block_id != gc_candidate_block_id5)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block6_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id6 = block_id;
						foundGC_candidate_id6 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 5) {
					if (foundGC_candidate_id6) {
						victims.blocks.push_back(gc_candidate_block_id6); victims.cnt++;
						subblk_entry sub6 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id6 };
						subblks.push_back(sub6);
					}
				}

				//Sel 1: find block with 7nd fewest valid pages among all blocks
				int min_erased_block7_v = 0;
				//threshold_Invalid /= INVALID_GRANU;//0112 flag
				//threshold_Invalid *= 6;//0112 flag
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id + align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block7_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4) && (block_id != gc_candidate_block_id5) && (block_id != gc_candidate_block_id6)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block7_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id7 = block_id;
						foundGC_candidate_id7 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 6) {
					if (foundGC_candidate_id7) {
						victims.blocks.push_back(gc_candidate_block_id7); victims.cnt++;
						subblk_entry sub7 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id7 };
						subblks.push_back(sub7);
					}
				}

				//Sel 1: find block with 7nd fewest valid pages among all blocks
				int min_erased_block8_v = 0;
				//threshold_Invalid /= THRESHOLD_INVALID;//0112 flag
				//threshold_Invalid *= 8;//0112 flag
				for (flash_block_ID_type block_id = start_block_id; block_id < start_block_id + align_unit; block_id++) {
					if ((pbke->Blocks[block_id].Invalid_page_count > min_erased_block8_v)
						&& (pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block) && (block_id != gc_candidate_block_id) && (block_id != gc_candidate_block_id2) && (block_id != gc_candidate_block_id3) && (block_id != gc_candidate_block_id4) && (block_id != gc_candidate_block_id5) && (block_id != gc_candidate_block_id6) && (block_id != gc_candidate_block_id7)
						&& (pbke->Blocks[block_id].Invalid_page_count > threshold_Invalid)
						&& is_safe_gc_wl_candidate(pbke, block_id)) { //to avoid "Error inconsitency"
						//std::cout << "[debug] block_id: " << block_id << "," << "candidate_id:" << gc_candidate_block_id << std::endl;
						min_erased_block8_v = pbke->Blocks[block_id].Invalid_page_count;
						gc_candidate_block_id8 = block_id;
						foundGC_candidate_id8 = true;
						//cnt++;
					}
				}
				if (MAX_SUBBLOCK_ERS_CNT >= 7) {
					if (foundGC_candidate_id8) {
						victims.blocks.push_back(gc_candidate_block_id8); victims.cnt++;
						subblk_entry sub8 = { plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, gc_candidate_block_id8 };
						subblks.push_back(sub8);
					}
				}

#endif
				
				////std::cout << "gc_candidate_block_id: "<<gc_candidate_block_id <<", gc_candidate_block_id2: "<< gc_candidate_block_id2 << std::endl;




				if (gc_candidate_block_id == gc_candidate_block_id2 && foundGC_candidate_id2) {
					std::cout << "ERROR!, gc_candidate block ids are same, " << gc_candidate_block_id << ", " << gc_candidate_block_id2 << std::endl;
					exit(1);
				}
#endif		//endif of NEW_SUBMIT
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RGA:
			{
				std::set<flash_block_ID_type> random_set;
				while (random_set.size() < rga_set_size)
				{
					flash_block_ID_type block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
					if (pbke->Ongoing_erase_operations.find(block_id) == pbke->Ongoing_erase_operations.end()
						&& is_safe_gc_wl_candidate(pbke, block_id))
						random_set.insert(block_id);
				}
				gc_candidate_block_id = *random_set.begin();
				for(auto &block_id : random_set)
					if (pbke->Blocks[block_id].Invalid_page_count > pbke->Blocks[gc_candidate_block_id].Invalid_page_count
						&& pbke->Blocks[block_id].Current_page_write_index == pages_no_per_block)
						gc_candidate_block_id = block_id;
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;
				while (!is_safe_gc_wl_candidate(pbke, gc_candidate_block_id) && repeat++ < block_no_per_plane)//A write frontier block should not be selected for garbage collection
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_P:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;

				//A write frontier block or a block with free pages should not be selected for garbage collection
				while ((pbke->Blocks[gc_candidate_block_id].Current_page_write_index < pages_no_per_block || !is_safe_gc_wl_candidate(pbke, gc_candidate_block_id))
					&& repeat++ < block_no_per_plane)
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::RANDOM_PP:
			{
				gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				unsigned int repeat = 0;

				//The selected gc block should have a minimum number of invalid pages
				while ((pbke->Blocks[gc_candidate_block_id].Current_page_write_index < pages_no_per_block 
					|| pbke->Blocks[gc_candidate_block_id].Invalid_page_count < random_pp_threshold
					|| !is_safe_gc_wl_candidate(pbke, gc_candidate_block_id))
					&& repeat++ < block_no_per_plane)
					gc_candidate_block_id = random_generator.Uniform_uint(0, block_no_per_plane - 1);
				break;
			}
			case SSD_Components::GC_Block_Selection_Policy_Type::FIFO:
				gc_candidate_block_id = pbke->Block_usage_history.front();
				pbke->Block_usage_history.pop();
				break;
			default:
				break;
			}
			
			if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id) != pbke->Ongoing_erase_operations.end()) {//This should never happen, but we check it here for safty
				victims.cnt = -1;
				std::cout << "flag2" << std::endl;
				return victims;
			}

#ifdef NEW_SUBMIT
			/*
			if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id2) != pbke->Ongoing_erase_operations.end()) {
				std::cout << "flag3" << std::endl;
				victims.cnt=-1;
				return victims;
			}

			if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id3) != pbke->Ongoing_erase_operations.end()) {
				std::cout << "flag4" << std::endl;
				victims.cnt=-1;
				return victims;
			}
			if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id4) != pbke->Ongoing_erase_operations.end()) {
				std::cout << "flag5" << std::endl;
				victims.cnt = -1;
				return victims;
			}
			if (pbke->Ongoing_erase_operations.find(gc_candidate_block_id5) != pbke->Ongoing_erase_operations.end()) {
				std::cout << "flag6" << std::endl;
				victims.cnt = -1;
				return victims;
			}
			*/
#endif

			NVM::FlashMemory::Physical_Page_Address gc_candidate_address(plane_address);
			gc_candidate_address.BlockID = gc_candidate_block_id;
			Block_Pool_Slot_Type* block = &pbke->Blocks[gc_candidate_block_id];
			if (block->Current_page_write_index == 0 || block->Invalid_page_count == 0) {//No invalid page to erase
				//std::cout << "flag5" << std::endl;
				victims.cnt = -1;
				return victims;
			}
			//std::cout <<"block->Invlaid_page_count :"<< block->Invalid_page_count << std::endl;

#ifdef NEW_SUBMIT
			/*
			NVM::FlashMemory::Physical_Page_Address gc_candidate_address2(plane_address);
			gc_candidate_address2.BlockID = gc_candidate_block_id2;
			Block_Pool_Slot_Type* block2 = &pbke->Blocks[gc_candidate_block_id2];
			//No invalid page to erase
			if (block2->Current_page_write_index == 0 || block2->Invalid_page_count == 0) {
				std::cout << "flag6" << std::endl;
				victims.cnt = -1;
				return victims;
			}

			NVM::FlashMemory::Physical_Page_Address gc_candidate_address3(plane_address);
			gc_candidate_address3.BlockID = gc_candidate_block_id3;
			Block_Pool_Slot_Type* block3 = &pbke->Blocks[gc_candidate_block_id3];
			//No invalid page to erase
			if (block3->Current_page_write_index == 0 || block3->Invalid_page_count == 0) {
				std::cout << "flag7" << std::endl;
				victims.cnt = -1;
				return victims;
			}

			NVM::FlashMemory::Physical_Page_Address gc_candidate_address4(plane_address);
			gc_candidate_address4.BlockID = gc_candidate_block_id4;
			Block_Pool_Slot_Type* block4 = &pbke->Blocks[gc_candidate_block_id4];
			//No invalid page to erase
			if (block4->Current_page_write_index == 0 || block4->Invalid_page_count == 0) {
				std::cout << "flag7" << std::endl;
				victims.cnt = -1;
				return victims;
			}


			NVM::FlashMemory::Physical_Page_Address gc_candidate_address5(plane_address);
			gc_candidate_address5.BlockID = gc_candidate_block_id5;
			Block_Pool_Slot_Type* block5 = &pbke->Blocks[gc_candidate_block_id5];
			//No invalid page to erase
			if (block5->Current_page_write_index == 0 || block5->Invalid_page_count == 0) {
				std::cout << "flag8" << std::endl;
				victims.cnt = -1;
				return victims;
			}

			NVM::FlashMemory::Physical_Page_Address gc_candidate_address6(plane_address);
			gc_candidate_address6.BlockID = gc_candidate_block_id6;
			Block_Pool_Slot_Type* block6 = &pbke->Blocks[gc_candidate_block_id6];
			//No invalid page to erase
			if (block6->Current_page_write_index == 0 || block6->Invalid_page_count == 0) {
				std::cout << "flag9" << std::endl;
				victims.cnt = -1;
				return victims;
			}
			*/
#endif

			//Run the state machine to protect against race condition
			block_manager->GC_WL_started(gc_candidate_address); 
#if SOFT_ISOLATION
			/*
			NVM::FlashMemory::Physical_Page_Address gc_candidate_address_codes(plane_address);
			gc_candidate_address_codes.BlockID = gc_candidate_block_id/2;
			Block_Pool_Slot_Type* block_codes = &pbke->Blocks[gc_candidate_block_id_codes];
			//No invalid page to erase
			if (block_codes->Current_page_write_index == 0 || block_codes->Invalid_page_count == 0) {
				return -1;
			}
			*/
#endif
			pbke->Ongoing_erase_operations.insert(gc_candidate_block_id);
			address_mapping_unit->Set_barrier_for_accessing_physical_block(gc_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing
			if (block_manager->Can_execute_gc_wl(gc_candidate_address))//If there are ongoing requests targeting the candidate block, the gc execution should be postponed
			{
				Stats::Total_gc_executions++;
				tsu->Prepare_for_transaction_submit();

				NVM_Transaction_Flash_ER* gc_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, pbke->Blocks[gc_candidate_block_id].Stream_id, gc_candidate_address);
				if (block->Current_page_write_index - block->Invalid_page_count > 0)//If there are some valid pages in block, then prepare flash transactions for page movement
				{
					NVM_Transaction_Flash_RD* gc_read = NULL;
					NVM_Transaction_Flash_WR* gc_write = NULL;
					for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++)
					{
						if (block_manager->Is_page_valid(block, pageID))
						{
							Stats::Total_page_movements_for_gc++;
							gc_candidate_address.PageID = pageID;
							if (use_copyback)
							{
								gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
									NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
								gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
								tsu->Submit_transaction(gc_write);
							}
							else
							{
								gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
									NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address), gc_candidate_address, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
								gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
									NO_LPA, NO_PPA, gc_candidate_address, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
								gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
								gc_write->RelatedErase = gc_erase_tr;
								gc_read->RelatedWrite = gc_write;
								tsu->Submit_transaction(gc_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined
							}
							gc_erase_tr->Page_movement_activities.push_back(gc_write);
						}
					}
#if SOFT_ISOLATION
					/*
					std::cout << "gc_candidate_address_codes.BlockID: " << gc_candidate_address_codes.BlockID << std::endl;
					if (gc_candidate_address_codes.BlockID != gc_candidate_address.BlockID && gc_candidate_address_codes.BlockID != gc_candidate_address2.BlockID) {
						for (flash_page_ID_type pageID = 0; pageID < block_codes->Current_page_write_index / 16; pageID++)
						{
							if (block_manager->Is_page_valid(block_codes, pageID))
							{
								gc_candidate_address_codes.PageID = pageID;
								if (use_copyback)
								{

									gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block_codes->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
										NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address_codes), NULL, 0, NULL, 0, INVALID_TIME_STAMP);
									gc_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
									tsu->Submit_transaction(gc_write);

								}
								else
								{
									std::cout << "gc_read will be submitted" << std::endl;
									gc_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::USERIO, block_codes->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
										NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_candidate_address_codes), gc_candidate_address_codes, NULL, 0, NULL, 0, INVALID_TIME_STAMP);
									//gc_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block_codes->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
									//	NO_LPA, NO_PPA, gc_candidate_address_codes, NULL, 0, gc_read, 0, INVALID_TIME_STAMP);
									//gc_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
									//gc_write->RelatedErase = gc_erase_tr;
									//gc_read->RelatedWrite = gc_write;
									tsu->Submit_transaction(gc_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined
								}
								//gc_erase_tr->Page_movement_activities.push_back(gc_write);
							}
						}
					}
					*/
					
#endif
				}
				block->Erase_transaction = gc_erase_tr;
				tsu->Submit_transaction(gc_erase_tr);
				primary_cnt++;
				tsu->Schedule();
			}
			else
			{
//				std::cout << gc_candidate_address.ChannelID << " : " << gc_candidate_address.ChipID << " : " << gc_candidate_address.BlockID << std::endl;
			}
#if NEW_SUBMIT
			switch (block_selection_policy) {
				case SSD_Components::GC_Block_Selection_Policy_Type::GREEDY:
					return victims;
				default:
					PRINT_ERROR("select different GC Policy")
					victims.cnt = -1;
					return victims;
			}
#else
			return -1;
#endif
		}
		return victims; //if (free_block_pool_size >= block_pool_gc_threshold), in short, if no gc required [doodu]
	}
}