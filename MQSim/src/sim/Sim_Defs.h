#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include<cstdint>
#include<string>
#include<iostream>

#define ACCEL_MODEL 1
#define THREE_VAR 3

//constant vars
#define NOT_GC_WL 0
#define YES_GC_WL 1
#define NOT_ALL_GC_W 0

#define NEW_SUBMIT true
#define SOFT_ISOLATION true

#define MLC_TLC_GC_THRESHOLD (false)

#define SET_PRECONDION_TO_UNIFORM_RND (true) // Trace-base WL  
#define MULTI_PROCESS_IO_GEN  (false)
#define ACT_SIMULATION        (false)
	#define ACT_STRESS_CHECK_ENABLE   (false)

#define ERS_CANCEL_ENABLE             (false) 
	#define ERS_CANCEL_TO_ENABLE      (false)  
	#define ERS_DEFER_SUS_ENABLE      (false) 
	#define ERS_IDEAL_SUS_ENABLE      (false)
	#define ERS_SUS_FAST12            (false)
	#define ADAPTIVE_RPS_SCH_ENABLE   (false)

#define PGM_SUS_ENABLE        (false) //

#define INCREMENTAL_GC (true) //
#define SCHEDULE_NORM	(true) //
	#define ADAPTIVE_GCUSR (false) //
	#define SCHEDULE_ATC (false)

#define SCHEDULE_GC_USR (false) //

#define MAX_SUBBLOCK_ERS_CNT 0//
#define INVALID_GRANU 10 
	#define THRESHOLD_INVALID 0 //

#define REMOVE_LAT_GC_W 0 
#define DEVISION_FACTOR 1.5 


#define NO_MULTIPLANE_COMMAND 0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define FREE_ALIGN 1
	#define STRICT_ALIGN 0
#define BLOCK_POOL_GC_HARD_THRESHOLD_ADJUST 0.01

typedef uint64_t sim_time_type;
typedef uint16_t stream_id_type;
typedef sim_time_type data_timestamp_type;

#define INVALID_TIME 18446744073709551615ULL 
#define T0 0
#define INVALID_TIME_STAMP 18446744073709551615ULL
#define MAXIMUM_TIME 18446744073709551615ULL
#define ONE_SECOND 1000000000
typedef std::string sim_object_id_type;

#define CurrentTimeStamp Simulator->Time()
#define PRINT_ERROR(MSG) {\
							std::cerr << "ERROR:" ;\
							std::cerr << MSG << std::endl; \
							std::cin.get();\
							exit(1);\
						 }
#define PRINT_ERROR_NO_CIN(MSG) {\
							std::cerr << "ERROR:" ;\
							std::cerr << MSG << std::endl; \
							exit(1);\
						 }

#define PRINT_MESSAGE(M) std::cout << M << std::endl;
#define DEBUG(M) //std::cout<<M<<std::endl;
#define DEBUG2(M) //std::cout<<M<<std::endl;

#define DCACHE_DEBUG(M) (std::cout << "D-CACHE_DEBUG : " << M << std::endl)

#define SIM_TIME_TO_MICROSECONDS_COEFF 1000
#define SIM_TIME_TO_SECONDS_COEFF 1000000000
#define SIM_TIME_TO_MS_COEFF 1000000

//#define INCREMENTAL_GC (false) //true
#define BACK_PRESSUER_BUF_MAG (128)
#define ERS_SUS_DIST_COUNT   (2000)

#define ERS_TIME_PER_LOOP (1000000)  //default: 1000000, 줄이면 Issued_Flash_Suspend_Erase_CMD가 늘어나지만 read 성능은 좋아지고 딱히 free block수도 변동x

#define ACT_SMALL_DIV (2000)
#define ACT_LARGE_DIV  (24)
#define ACT_STRESS_TO (10000000000)


extern sim_time_type gnACTLargeReadCount;
extern sim_time_type gnACTLargeWriteCount;
extern sim_time_type gnACTSmallReadCount;

extern bool READ_PRIO_SCH;

extern size_t gnERSSuspendOffCount;
extern unsigned int ERS_SUS_TO_Time_MS; 	   
extern unsigned int ACT_MAG_COUNT;

extern size_t gnErsSusToCount[];


extern size_t gnERSSusCount;
extern size_t gnDefERSCount;
extern size_t gnTotalERSCount;


#endif // !DEFINITIONS_H
