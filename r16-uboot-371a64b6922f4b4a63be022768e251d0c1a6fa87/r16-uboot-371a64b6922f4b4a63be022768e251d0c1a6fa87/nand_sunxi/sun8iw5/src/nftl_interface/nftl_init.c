
#define _NFTL_INIT_C_

#include "../nftl_interface/nftl_blk.h"
#include "../phy/phy.h"
#include "../nftl/nftl_inc.h"


extern struct _nand_partition* build_nand_partition(struct _nand_phy_partition* phy_partition);
extern int free_nand_partition(struct _nand_partition*nand_partition);

int _nand_read(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
int _nand_write(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
int _nand_flush_write_cache(struct _nftl_blk *nftl_blk,uint32 num);
int _shutdown_op(struct _nftl_blk *nftl_blk);

int _nand_sector_read(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
int _nand_sector_write(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
int _nand_flush_sector_write_cache(struct _nftl_blk *nftl_blk,uint32 num);

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
//static uint32 total_size = 0;
void *nftl_malloc(uint32 size)
{
//    return kzalloc(size, GFP_KERNEL);
    if(size > 0x180000)
        NFTL_ERR("[NE]malloc size too large %d!\n",size);

//    total_size += size;

//    NFTL_ERR("nftl_malloc %d, %d!\n",size,total_size);

	return MALLOC(size);
}

void nftl_free(const void *ptr)
{
//    kfree(ptr);
	FREE((void *)ptr,0);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void add_nftl_blk_list(struct _nftl_blk*head,struct _nftl_blk *nftl_blk)
{
    struct _nftl_blk * p = head;

    nftl_blk->nftl_blk_next = NULL;
    while(p->nftl_blk_next != NULL)
    {
        p = p->nftl_blk_next;
    }
    p->nftl_blk_next = nftl_blk;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nftl_blk* del_last_nftl_blk(struct _nftl_blk*head)
{
    struct _nftl_blk *nftl_blk = NULL;
    struct _nftl_blk * p = head;
    while(p->nftl_blk_next != NULL)
    {
        nftl_blk = p->nftl_blk_next;
        if(nftl_blk->nftl_blk_next == NULL)
        {
            p->nftl_blk_next = NULL;
            return nftl_blk;
        }
        p = p->nftl_blk_next;
    }
    return NULL;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_initialize(struct _nftl_blk *nftl_blk,int no)
{
	struct _nand_partition *nand = nftl_blk->nand;
	uint32 ret;

	if (nand->spare_bytes < MIN_BYTES_OF_USER_PER_PAGE)
		return -EPERM;


    nftl_blk->nftl_cfg =  nftl_malloc(sizeof(struct _nftl_cfg));
    if (!nftl_blk->nftl_cfg)
    {
        NFTL_ERR("[NE]====no memory!!!!!=====\n");
        return NFTL_FAILURE;
    }

	nftl_blk->nftl_cfg->nftl_dont_use_cache = NFTL_DONT_CACHE_DATA;
	nftl_blk->nftl_cfg->nftl_use_cache_sort = NFTL_CACHE_SORT;
	nftl_blk->nftl_cfg->nftl_support_gc_read_reclaim = SUPPORT_GC_READ_RECLAIM;
	nftl_blk->nftl_cfg->nftl_support_wear_leveling = SUPPORT_WEAR_LEVELING;
	nftl_blk->nftl_cfg->nftl_support_fill_block = SUPPORT_FILL_BLOCK;
	nftl_blk->nftl_cfg->nftl_need_erase = NFTL_ERASE;
	nftl_blk->nftl_cfg->nftl_min_free_block_num = nand->phy_partition->nand_info->mini_free_block_first_reserved;
	nftl_blk->nftl_cfg->nftl_min_free_block = nand->phy_partition->nand_info->mini_free_block_reserved;
	nftl_blk->nftl_cfg->nftl_gc_threshold_free_block_num = GC_THRESHOLD_FREE_BLOCK_NUM ;
	nftl_blk->nftl_cfg->nftl_gc_threshold_ratio_numerator = GC_THRESHOLD_RATIO_NUMERATOR;
	nftl_blk->nftl_cfg->nftl_gc_threshold_ratio_denominator = GC_THRESHOLD_RATIO_DENOMINATOR;
	nftl_blk->nftl_cfg->nftl_read_claim_interval = READ_RECLAIM_INTERVAL_SEC;

	if(nand->phy_partition->nand_info->cache_level == 0)
	{
        nftl_blk->nftl_cfg->nftl_max_cache_num = MAX_CACHE_NUM;
        nftl_blk->nftl_cfg->nftl_max_cache_write_num = MAX_CACHE_WRITE_NUM;
	}
	else if(nand->phy_partition->nand_info->cache_level == 255)
	{
	    nftl_blk->nftl_cfg->nftl_dont_use_cache = 1;
	    nftl_blk->nftl_cfg->nftl_max_cache_num = 50;
	    nftl_blk->nftl_cfg->nftl_max_cache_write_num = 40;
	}
	else if(nand->phy_partition->nand_info->cache_level > 10)
	{
	    nand->phy_partition->nand_info->cache_level = 10;
        nftl_blk->nftl_cfg->nftl_max_cache_num = MAX_CACHE_NUM;
        nftl_blk->nftl_cfg->nftl_max_cache_write_num = MAX_CACHE_WRITE_NUM;
	}
	else
	{
	    nftl_blk->nftl_cfg->nftl_max_cache_num = nand->phy_partition->nand_info->cache_level * 50;
	    nftl_blk->nftl_cfg->nftl_max_cache_write_num = nand->phy_partition->nand_info->cache_level * 40;
	}

	nftl_blk->nftl_cfg->nftl_cross_talk = nand->phy_partition->CrossTalk;
    if(nftl_blk->nftl_cfg->nftl_cross_talk != 0)
	{
	    nftl_blk->nftl_cfg->nftl_max_cache_write_num = MIN_CACHE_WRITE_NUM;
	}
	else
    {
		nftl_blk->nftl_cfg->nftl_max_cache_write_num = 1;
	}

	nftl_blk->nftl_cfg->nftl_max_cache_write_num = 1;

	if((nand->phy_partition->nand_info->read_claim_interval <= MAX_READ_RECLAIM_INTERVAL_SEC) && (nand->phy_partition->nand_info->read_claim_interval >= MIN_READ_RECLAIM_INTERVAL_SEC ))
	{
	    nftl_blk->nftl_cfg->nftl_read_claim_interval = nand->phy_partition->nand_info->read_claim_interval;
	}

	ret = nftl_start((void*)nftl_blk,nftl_blk->nftl_cfg,&nftl_blk->nftl_zone,nand->cap_by_bytes,nand->total_by_bytes,nand->bytes_per_block,nand->bytes_per_page,nand->spare_bytes,nand->name,no,0);
	if(ret != NFTL_SUCCESS)
	    return ret;

	nftl_blk->nftl_zone->nand_chip->max_erase_times = nand->phy_partition->nand_info->MaxBlkEraseTimes;
	nftl_blk->nftl_zone->nand_chip->support_read_reclaim =  nand->phy_partition->nand_info->EnableReadReclaim;
	NFTL_DBG("[ND]max_erase_times = %i\n",nftl_blk->nftl_zone->nand_chip->max_erase_times);

	nftl_blk->nftl_logic_size = nftl_get_zone_cap(nftl_blk->nftl_zone);
	nftl_blk->read_data = _nand_read;
	nftl_blk->write_data = _nand_write;
	nftl_blk->flush_write_cache = _nand_flush_write_cache;
	nftl_blk->shutdown_op = _shutdown_op;

	nftl_blk->read_sector_data = _nand_sector_read;
	nftl_blk->write_sector_data = _nand_sector_write;
	nftl_blk->flush_sector_write_cache = _nand_flush_sector_write_cache;

	nftl_blk->nftl_blk_next = NULL;

	return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_exit(struct _nftl_blk *nftl_blk)
{
    free_nand_partition(nftl_blk->nand);
    nftl_zone_release(nftl_blk->nftl_zone);
    nftl_free(nftl_blk->nftl_cfg);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nand_read(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    uint32 ret = 0;
    ret =  __nand_read(nftl_blk->nftl_zone,start_sector,len,buf);
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nand_write(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    uint32 ret = 0;

    ret = __nand_write(nftl_blk->nftl_zone,start_sector,len,buf);
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nand_flush_write_cache(struct _nftl_blk *nftl_blk,uint32 num)
{
    uint32 ret = 0;

    ret =  __nand_flush_write_cache(nftl_blk->nftl_zone,num);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nand_sector_read(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    uint32 ret = 0;
    //ret =  __nand_sector_read(nftl_blk->nftl_zone,start_sector,len,buf);
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nand_sector_write(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    uint32 ret = 0;

    //ret = __nand_sector_write(nftl_blk->nftl_zone,start_sector,len,buf);
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nand_flush_sector_write_cache(struct _nftl_blk *nftl_blk,uint32 num)
{
    uint32 ret = 0;

    //ret =   __nand_flush_sector_write_cache(nftl_blk->nftl_zone,0xffffffff);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _shutdown_op(struct _nftl_blk *nftl_blk)
{
    return __shutdown_op(nftl_blk->nftl_zone);
}
