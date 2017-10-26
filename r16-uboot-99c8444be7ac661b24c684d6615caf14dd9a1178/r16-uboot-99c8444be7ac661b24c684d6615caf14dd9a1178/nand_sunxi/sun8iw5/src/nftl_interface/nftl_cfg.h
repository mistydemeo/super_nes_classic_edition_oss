#ifndef _NFTL_CFG_H
#define _NFTL_CFG_H

#define NFTL_DONT_CACHE_DATA                      0
#define NFTL_CACHE_SORT                           1
#define SUPPORT_GC_READ_RECLAIM                   1
#define SUPPORT_WEAR_LEVELING                     1
#define SUPPORT_FILL_BLOCK                        0
#define NFTL_ERASE                                0

/////////////////////////////////////////////////////////////
#define ZONE_RESERVED_BLOCK_RATIO                 16
#define SYS_ZONE_RESERVED_BLOCK_RATIO             4
#define MIN_FREE_BLOCK_NUM                        20
#define MIN_FREE_BLOCK                            2
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
#define SYS_RESERVED_BLOCK_RATIO                   5
#define NORM_RESERVED_BLOCK_RATIO                 10
#define MIN_FREE_BLOCK_NUM_RUNNING                15
#define MIN_FREE_BLOCK_NUM_V2                     25
#define MAX_FREE_BLOCK_NUM                        120
#define MIN_FREE_BLOCK_REMAIN                     4
/////////////////////////////////////////////////////////////

#define GC_THRESHOLD_FREE_BLOCK_NUM               8
#define GC_THRESHOLD_RATIO_NUMERATOR              2
#define GC_THRESHOLD_RATIO_DENOMINATOR            3
#define GC_START_RATIO                            5

/////////////////////////////////////////////////////////////
#define READ_RECLAIM_INTERVAL_SEC                     (1296000)  // 15 day
#define MAX_READ_RECLAIM_INTERVAL_SEC                 (7779000)  // 90 day
#define MIN_READ_RECLAIM_INTERVAL_SEC                 (86400)    // 1  day

struct _nftl_cfg{
    unsigned short nftl_dont_use_cache;
    unsigned short nftl_use_cache_sort;
    unsigned short nftl_support_gc_read_reclaim;
    unsigned short nftl_support_wear_leveling;
    unsigned short nftl_support_fill_block;
    unsigned short nftl_need_erase;
    unsigned short nftl_min_free_block_num;
    unsigned short nftl_gc_threshold_free_block_num;
    unsigned short nftl_min_free_block;
    unsigned short nftl_gc_threshold_ratio_numerator;
    unsigned short nftl_gc_threshold_ratio_denominator;
    unsigned short nftl_max_cache_num;
    unsigned short nftl_max_cache_write_num;
    unsigned short nftl_cross_talk;
    unsigned int   nftl_read_claim_interval;

};

#endif /*_NFTL_CFG_H*/
