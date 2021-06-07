#include "Lat_Model.h"

namespace NVM
{
	namespace FlashMemory
	{
		Lat_Model::Lat_Model(int row, int col, int sram)
		{
			row_arr = row;
			col_arr = col;
			sram = sram_size;
			arr_size = row_arr * col_arr;
		}

		Lat_Model::~Lat_Model()
		{

		}
		unsigned int Lat_Model::cal_latency(int operation_type) {
			if (operation_type == MAT2MAT) {
				if (arr_size == 16) {
					return 15000;
				}
				else {
				}
			}
			else if (operation_type == VEC2VEC) {
				return 150000;
			}
			else {
				PRINT_ERROR("this operation type is not supported");
			}
			
		}
	}
}