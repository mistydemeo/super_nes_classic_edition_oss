
/*
 * Aml nftl common
 *
 * (C) 2008
 */

#define _NFTL_COMMON_C_

#include "nftl_inc.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nftl_nand_copy_page(struct _nftl_zone *zone,_phy_block_info* a, _phy_block_info* b,uchar *buf,uint16 page)
{
    int ret,ret1,ret2;
    uchar spare[BYTES_OF_USER_PER_PAGE];
    uint32 logic_page;
    _physic_op_par phy_op_par1;
    _physic_op_par phy_op_par2;
    
    
    set_physic_op_par(&phy_op_par1,a->phy_block.Block_NO,page,0,NULL,NULL);
    set_physic_op_par(&phy_op_par2,b->phy_block.Block_NO,page,0,NULL,NULL);

    ret = NFTL_SUCCESS;
    phy_op_par1.page_bitmap = zone->nand_chip->bitmap_per_page;
    phy_op_par2.page_bitmap = zone->nand_chip->bitmap_per_page;
    phy_op_par1.main_data_addr = buf;
    phy_op_par2.main_data_addr = buf;
    phy_op_par1.spare_data_addr = spare;
    phy_op_par2.spare_data_addr = spare;

    ret1 = zone->nftl_nand_read_page(zone,&phy_op_par1);
    if(ret1 == ECC_LIMIT)
    {
        ret1 = NFTL_SUCCESS;
    }
    logic_page = get_special_data_from_oob(spare);
    set_oob_special_page(zone,spare,logic_page,a->block_used_count,b->erase_count+1);

    ret2 = zone->nftl_nand_write_page(zone,&phy_op_par2);

    ret = ret1 | (ret2<<8);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_spare_data(uchar* buf,uint32 data,uchar offset,uchar num)
{
    uint32 shift,i;
    for(i=0; i<num; i++)
    {
        shift = num-i;
        shift -= 1;
        shift <<= 3;
        buf[offset+i] = data>>shift;
    }
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 get_spare_data(uchar* buf,uchar offset,uchar num)
{
    uint32 data,i;
    for(i=0,data=0; i<num; i++)
    {
        data <<= 8;
        data |= buf[offset+i];
    }
    return data;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_oob_logic_page(struct _nftl_zone *zone,uchar* buf,uint32 logic_page,uint32 block_used_count,uint16 erase_times)
{
    buf[SPARE_OFFSET_BAD_FLAG] = 0xff;
    set_spare_data(buf,logic_page,SPARE_OFFSET_LOGIC_PAGE,sizeof(logic_page));
    set_spare_data(buf,erase_times,SPARE_OFFSET_ERASE_TIMES,sizeof(erase_times));
    set_spare_data(buf,block_used_count,SPARE_OFFSET_COUNTER,sizeof(block_used_count));
    buf[SPARE_OFFSET_LOGIC_PAGE] |= FLAG_FOR_LOGIC_PAGE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_oob_special_page(struct _nftl_zone *zone,uchar* buf,uint32 special_data,uint32 block_used_count,uint16 erase_times)
{
    buf[SPARE_OFFSET_BAD_FLAG] = 0xff;
    set_spare_data(buf,special_data,SPARE_OFFSET_SPECIAL_FLAG,sizeof(special_data));
    set_spare_data(buf,erase_times,SPARE_OFFSET_ERASE_TIMES,sizeof(erase_times));
    set_spare_data(buf,block_used_count,SPARE_OFFSET_COUNTER,sizeof(block_used_count));
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_no_use_page(struct _nftl_zone *zone,uchar* buf)
{
    buf[SPARE_OFFSET_BAD_FLAG] = 0xff;
    set_spare_data(buf,SPARE_SPECIAL_DATA,SPARE_OFFSET_SPECIAL_FLAG,1);
    set_spare_data(buf,SPARE_NOUSE_DATA,SPARE_OFFSET_SPECIAL_TYPE,1);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_start_block_flag(struct _nftl_zone *zone,uchar* buf,uint32 size)
{
    buf[SPARE_OFFSET_BAD_FLAG] = 0xff;
    set_spare_data(buf,SPARE_SPECIAL_DATA,SPARE_OFFSET_SPECIAL_FLAG,1);
    set_spare_data(buf,BLOCK_FOR_FTL_START,SPARE_OFFSET_SPECIAL_TYPE,1);
    set_spare_data(buf,size,SPARE_OFFSET_NFTL_SIZE,sizeof(size));
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_end_block_flag(struct _nftl_zone *zone,uchar* buf)
{
    buf[SPARE_OFFSET_BAD_FLAG] = 0xff;
    set_spare_data(buf,SPARE_SPECIAL_DATA,SPARE_OFFSET_SPECIAL_FLAG,1);
    set_spare_data(buf,BLOCK_FOR_FTL_END,SPARE_OFFSET_SPECIAL_TYPE,1);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uchar get_bad_flag_from_oob(uchar* buf)
{
    uint32 special_data;

    special_data = get_spare_data(buf,SPARE_OFFSET_SPECIAL_FLAG,sizeof(special_data));
    if(special_data == 0)
    {
        return NFTL_SUCCESS;
    }
    return NFTL_FAILURE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 get_block_used_count_from_oob(uchar* buf)
{
    uint32 block_used_count;

    block_used_count = get_spare_data(buf,SPARE_OFFSET_COUNTER,sizeof(block_used_count));
    return block_used_count;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 get_logic_page_from_oob(uchar* buf)
{
    uint32 logic_page;

    logic_page = get_spare_data(buf,SPARE_OFFSET_LOGIC_PAGE,sizeof(logic_page));
    logic_page &= 0x0fffffff;
    return logic_page;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 get_special_data_from_oob(uchar* buf)
{
    uint32 special_data;
    special_data = get_spare_data(buf,SPARE_OFFSET_SPECIAL_FLAG,sizeof(special_data));
    return special_data;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint16 get_erase_count_from_oob(uchar* buf)
{
    uint16 erase_count;

    erase_count = get_spare_data(buf,SPARE_OFFSET_ERASE_TIMES,sizeof(erase_count));
    return erase_count;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_ftl_start_flag_page(uchar* buf)
{
    if((buf[SPARE_OFFSET_SPECIAL_FLAG] == SPARE_SPECIAL_DATA)&&(buf[SPARE_OFFSET_SPECIAL_TYPE] == BLOCK_FOR_FTL_START))
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_ftl_end_flag_page(uchar* buf)
{
    if((buf[SPARE_OFFSET_SPECIAL_FLAG] == SPARE_SPECIAL_DATA)&&(buf[SPARE_OFFSET_SPECIAL_TYPE] == BLOCK_FOR_FTL_END))
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_ftl_special_data(uchar* buf)
{
    if(buf[SPARE_OFFSET_SPECIAL_FLAG] == SPARE_SPECIAL_DATA)
        return NFTL_YES;
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_ftl_logic_page_data(uchar* buf)
{
    if((buf[SPARE_OFFSET_LOGIC_PAGE] & 0xf0) == FLAG_FOR_LOGIC_PAGE)
        return NFTL_YES;
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_nouse_page(uchar* buf)
{
    uint32 i,num=0;

    for(i=0;i<10;i++)
    {
        if(buf[i] == 0xff)
        {
            num++;
        }
    }

    if((num >= 9)&&(buf[0]==0xff))
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_fill_page(uchar* buf)
{
    uint32 special_data;
    special_data = get_special_data_from_oob(buf);
    if(special_data == PHY_FILL_PAGE)
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_phy_mapping_page(uchar* buf)
{
    uint32 special_data;
    special_data = get_special_data_from_oob(buf);
    if(special_data == PHY_MAPPING_PAGE)
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_function_info_page(uchar* buf)
{
    uint32 special_data;
    special_data = get_special_data_from_oob(buf);
    if((special_data&0xffff0000) == PHY_FUNCTION_INFO_PAGE)
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_power_down_info_page(uchar* buf)
{
    uint32 special_data;
    special_data = get_special_data_from_oob(buf);
    if(special_data == PHY_NORMAL_POWER_DOWN)
    {
        return NFTL_YES;
    }
    return NFTL_NO;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_mapping_page* get_logic_page_map(struct _nftl_zone *zone,uint32 logic_page)
{
    _mapping_page* page;
    _mapping_page* page_ptr;
    uint32 i,len;

#if   MALLOC_FLAG

    len = MAX_MALLOC_BYTES / sizeof(_mapping_page);
    i = 0;

    while(logic_page >= len)
    {
        i++;
        logic_page -= len;
    }

    page_ptr = zone->zone_logic_page_map[i];

    page = page_ptr + logic_page;

#else

    page = &zone->zone_logic_page_map[logic_page];

#endif

    return page;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int malloc_logic_page_map(struct _nftl_zone *zone,uint32 total_pages)
{
#if   MALLOC_FLAG

    uint32 bytes,i,len;

    len = MAX_MALLOC_BYTES / sizeof(_mapping_page);

    for(i=0; i<MAX_LOGIC_MAP_NUM; i++)
        zone->zone_logic_page_map[i] = NULL;

    for(i=0; i<MAX_LOGIC_MAP_NUM; i++)
    {
//        NFTL_DBG("[ND] malloc_logic_page_map malloc_logic_page_map  %d\n",i);
        if(total_pages >= len)
        {
            bytes = len * sizeof(_mapping_page);
            zone->zone_logic_page_map[i] = (_mapping_page*)nftl_malloc(bytes);
            if (zone->zone_logic_page_map[i] == NULL)
            {
                NFTL_ERR("[NE] zone_param_init malloc_logic_page_map error %d\n",i);
                return -ENOMEM;
            }
            total_pages -= len;
            MEMSET((void*) zone->zone_logic_page_map[i],0xff,bytes);
        }
        else
        {
            bytes = total_pages * sizeof(_mapping_page);
            zone->zone_logic_page_map[i] = (_mapping_page*)nftl_malloc(bytes);
            if (zone->zone_logic_page_map[i] == NULL)
            {
                NFTL_ERR("[NE] zone_param_init malloc_logic_page_map error %d\n",i);
                return -ENOMEM;
            }
            MEMSET((void*) zone->zone_logic_page_map[i],0xff,bytes);
            total_pages = 0;
            break;
        }
    }

    if(total_pages != 0)
    {
        NFTL_ERR("[NE] zone_param_init malloc_logic_page_map error %d\n",total_pages);
        return -ENOMEM;
    }

#else

    zone->zone_logic_page_map = (_mapping_page*)nftl_malloc(total_pages * sizeof(_mapping_page));
    if (zone->zone_logic_page_map == NULL)
    {
        NFTL_ERR("[NE] zone_param_init malloc_logic_page_map error2 %d\n",total_pages);
        return -ENOMEM;
    }
    MEMSET((void*) zone->zone_logic_page_map,0xff,total_pages*sizeof(_mapping_page));

#endif

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int free_logic_page_map(struct _nftl_zone *zone)
{
#if   MALLOC_FLAG

    uint32 i;
    for(i=0; i<MAX_LOGIC_MAP_NUM; i++)
    {
        if(zone->zone_logic_page_map[i] != NULL)
            nftl_free(zone->zone_logic_page_map[i]);
    }

#else

    nftl_free(zone->zone_logic_page_map);

#endif

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_physic_op_par(_physic_op_par* phy_op_par,uint16 blkNO,uint16 Page_NO,uint16 page_bitmap,uchar*main_buf,uchar*spare_buf)
{
    phy_op_par->phy_page.Page_NO = Page_NO;
    phy_op_par->phy_page.Block_NO = blkNO;
    phy_op_par->page_bitmap = page_bitmap;
    phy_op_par->main_data_addr = main_buf;
    phy_op_par->spare_data_addr = spare_buf;
}

