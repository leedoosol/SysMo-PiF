#ifndef LAT_MODEL_H
#define LAT_MODEL_H

#include "FlashTypes.h"
#include "Page.h"


#define MAT2MAT 0
#define VEC2VEC 1
#define RESIZE_FACTOR 20
#define DEFAULT_COMPUTE_LATENCY_4K 15000

namespace NVM
{
	namespace FlashMemory
	{
		class Lat_Model
		{
		//enum class OPERATION_TYPE { MAT2MAT, VEC2VEC };
		public:
			Lat_Model(int row, int col, int sram_size);
			~Lat_Model();
			unsigned int cal_latency(int operation_type);
		private:
			int row_arr;
			int col_arr;
			int sram_size;
			int arr_size;
			double calculated_latency;

		};
	}
}
#endif // ! LAT_MODEL_H
#pragma once
