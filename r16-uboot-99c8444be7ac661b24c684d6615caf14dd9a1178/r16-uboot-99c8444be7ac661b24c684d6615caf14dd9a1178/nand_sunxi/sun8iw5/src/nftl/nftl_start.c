
/*
 * allwinner nftl start
 *
 * (C) 2008
 */

#define _NFTL_START_C_

#include "nftl_inc.h"
#include "../nftl_interface/nftl_cfg.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_start(void* priv,void* cfg,struct _nftl_zone ** pzone,uint64 logic_size,uint64 total_size,unsigned erasesize,unsigned writesize,unsigned oobavail,char* name,int no,char type)
{
    uint32 size_in_blk,total_block,total_vaild_block,temp,i;
    //uint32 phys_erase_shift;
    uint32 ret;
    uint32 try_count = 0;
    struct _nftl_zone* zone;

    uint32 logic_size_sector,total_size_sector,erasesize_sector,writesize_sector;

    logic_size >>= 9;
    total_size >>= 9;
    erasesize >>= 9;
    writesize >>= 9;
    logic_size_sector = logic_size;
    total_size_sector = total_size;
    erasesize_sector = erasesize;
    writesize_sector = writesize;

    *pzone = nftl_malloc(sizeof(struct _nftl_zone));
    zone = *pzone;
    if (!zone)
    {
        NFTL_ERR("[NE]====no memory!!!!!=====\n");
        return -ENOMEM;
    }
Try_Again:
    try_count++;
    MEMSET(zone,0,sizeof(struct _nftl_zone));

    zone->priv = priv;
    zone->cfg = (struct _nftl_cfg *)cfg;

    zone->nand_chip = nftl_malloc(sizeof(struct nand_chip_t));
    if (!zone->nand_chip)
    {
        NFTL_ERR("[NE]====no memory!!!!!=====\n");
        return -ENOMEM;
    }

//    zone->version[0] = '1';
//    zone->version[1] = '3';
//    zone->version[2] = '0';
//    zone->version[3] = '5';
//    zone->version[4] = '1';
//    zone->version[5] = '3';
//    zone->version[6] = 'a' + type;
//    zone->version[7] = '\0';
//
//    NFTL_DBG("[ND]nand version %s\n",zone->version);

    zone->nand_chip->bytes_per_page = writesize_sector << 9;
    zone->nand_chip->bytes_user_date = oobavail;

    //phys_erase_shift = ffs(erasesize) - 1;
    //size_in_blk =  (size >> phys_erase_shift);

    size_in_blk = total_size_sector / erasesize_sector;
    if (size_in_blk <= zone->cfg->nftl_min_free_block_num)
        return -EPERM;

    zone->nand_chip->blk_per_chip = size_in_blk;
    zone->nand_chip->pages_per_blk = erasesize_sector / writesize_sector;
    zone->nand_chip->sector_per_page =  writesize_sector;
    zone->nand_chip->bitmap_per_page = zone->nand_chip->sector_per_page;
    zone->nand_chip->max_erase_times = MAX_ERASE_NUM;

    total_block = size_in_blk;

    zone->nand_chip->nand_block_info = (_phy_block_info*)nftl_malloc((total_block+1) * sizeof(_phy_block_info));
    if(zone->nand_chip->nand_block_info == NULL)
    {
        NFTL_ERR("[NE]====no memory!!!!!=====\n");
        return -ENOMEM;
    }
    MEMSET((void*)zone->nand_chip->nand_block_info,0xff,total_block*sizeof(_phy_block_info));

    for(i=0; i<total_block+1; i++)
    {
        zone->nand_chip->nand_block_info[i].phy_block.Block_NO = i;
        zone->nand_chip->nand_block_info[i].invalid_page_count = 0;
        zone->nand_chip->nand_block_info[i].info = 0;
        zone->nand_chip->nand_block_info[i].block_used_count = 0xffffffff;
        zone->nand_chip->nand_block_info[i].erase_count = 0;
        zone->nand_chip->nand_block_info[i].invalid_page_next = NULL;
        zone->nand_chip->nand_block_info[i].invalid_page_prev = NULL;
        zone->nand_chip->nand_block_info[i].free_next = NULL;
        zone->nand_chip->nand_block_info[i].free_prev = NULL;
        zone->nand_chip->nand_block_info[i].block_used_next = NULL;
        zone->nand_chip->nand_block_info[i].block_used_prev = NULL;
    }

    nftl_ops_init(*pzone);

    total_vaild_block = logic_size_sector / erasesize_sector;
    zone->free_block_num = size_in_blk - total_vaild_block;
    if (zone->free_block_num < zone->cfg->nftl_min_free_block_num)
    {
        NFTL_ERR("[NE]not enough blocks %d!\n",zone->free_block_num);
        return -ENOMEM;
    }

    NFTL_DBG("[ND]nftl start:%d,%d\n",size_in_blk,zone->free_block_num);

    temp = erasesize_sector;
    ret = zone_param_init(*pzone,0,(size_in_blk - zone->free_block_num)*temp,zone->free_block_num*temp);
    if(ret != 0)
    {
        NFTL_ERR("[NE] zone_param_init fail!\n");
        return ret;
    }

    zone->zone_no = no;
    if(zone->cfg->nftl_cross_talk != 0)
    {
        zone->zone_attr |= SUPPORT_COSS_TALK;
    }

    if(zone->cfg->nftl_need_erase != 0)
    {
        if(is_no_use_device(*pzone,size_in_blk) == 1)
        {
            ret = build_zone_list_first(*pzone,size_in_blk);
            if(ret != 0)
            {
                NFTL_ERR("[NE] build_zone_list_first fail!\n");
                return ret;
            }
        }
    }

    ret = build_zone_list(*pzone);
    if(ret != 0)
    {
        NFTL_ERR("[NE] build_zone_list fail!\n");
        if(try_count<2)
        {
            NFTL_ERR("try again\n");
            goto Try_Again;
        }
        return ret;
    }

    nftl_cache_init(*pzone);
    //nftl_sector_cache_init(*pzone);

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void nftl_zone_release(struct _nftl_zone *zone)
{
    nftl_cache_exit(zone);
    //nftl_sector_cache_exit(zone);
    zone_param_exit(zone);
    nftl_free(zone->nand_chip->nand_block_info);
    nftl_free(zone->nand_chip);
    nftl_free(zone);
}
