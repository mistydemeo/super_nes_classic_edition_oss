
/*
 * allwinner nftl interface
 *
 * (C) 2008
 */

#define _NFTL_INTERFACE_C_

#include "nftl_inc.h"


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 nftl_get_zone_cap(void * _zone)
{
    struct _nftl_zone* zone = _zone;
    return zone->logic_cap_in_sects;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void nftl_set_zone_test(void * _zone,uint32 test)
{
    struct _nftl_zone* zone = _zone;
    zone->test = test;
    print_nftl_zone(zone);
    return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void* nftl_get_zone_priv(void * _zone)
{
    struct _nftl_zone* zone = _zone;
    return zone->priv;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint16 nftl_get_zone_write_cache_nums(void * _zone)
{
    struct _nftl_zone* zone = _zone;
    return zone->cache.cache_write_nums;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void print_smart(void * _zone)
{
    struct _nftl_zone* zone = (struct _nftl_zone*)_zone;
    _smart* smart = zone->smart;

    NFTL_DBG("[NE]smart->version:%x;\n",smart->version);

    NFTL_DBG("[NE]smart->total_recv_read_sectors:%lld;\n",smart->total_recv_read_sectors);
    NFTL_DBG("[NE]smart->total_recv_write_sectors:%lld;\n",smart->total_recv_write_sectors);
    NFTL_DBG("[NE]smart->total_real_read_pages:%lld;\n",smart->total_real_read_pages);
    NFTL_DBG("[NE]smart->total_real_write_pages:%lld;\n",smart->total_real_write_pages);

    NFTL_DBG("[NE]smart->total_recv_read_claim_pages:%d;\n",smart->total_recv_read_claim_pages);

    NFTL_DBG("[NE]smart->total_gc_times:%d;\n",smart->total_gc_times);
    NFTL_DBG("[NE]smart->total_gc_pages:%d;\n",smart->total_gc_pages);
    NFTL_DBG("[NE]smart->total_wl_times:%d;\n",smart->total_wl_times);

    NFTL_DBG("[NE]smart->total_real_read_error_pages:%d;\n",smart->total_real_read_error_pages);
    NFTL_DBG("[NE]smart->total_real_write_error_pages:%d;\n",smart->total_real_write_error_pages);

    NFTL_DBG("[NE]smart->total_normal_power_cycles:%x;\n",smart->total_normal_power_cycles);
    NFTL_DBG("[NE]smart->total_unusual_power_cycles:%x;\n",smart->total_unusual_power_cycles);

    NFTL_DBG("[NE]smart->max_block_erase_times:%d;\n",smart->max_block_erase_times);
    NFTL_DBG("[NE]smart->min_block_erase_times:%d;\n",smart->min_block_erase_times);

    NFTL_DBG("[NE]smart->max_block_used_counter:%d;\n",smart->max_block_used_counter);
    NFTL_DBG("[NE]smart->min_block_used_counter:%d;\n",smart->min_block_used_counter);

    NFTL_DBG("[NE]smart->read_reclaim_utc:%d;\n",smart->read_reclaim_utc);
    NFTL_DBG("[NE]smart->cross_talk_times:%d;\n",smart->cross_talk_times);

}

