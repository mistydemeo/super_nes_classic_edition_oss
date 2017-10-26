
/*
 * allwinner nftl logic
 *
 * (C) 2008
 */

#define _NFTL_LOGIC_C_

#include "nftl_inc.h"
#include "../nftl_interface/nftl_cfg.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nftl_nand_write_logic_page(struct _nftl_zone* zone,uint32 page_no,uchar *buf)
{
    if(nand_write_logic_page_no_gc(zone,page_no,buf) != 0)
    {
        NFTL_ERR("[NE]error nand_write_logic_page_no_gc\n");
        return NFTL_FAILURE;
    }

    garbage_collect(zone);

    do_prio_gc(zone);

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nftl_nand_read_logic_page(struct _nftl_zone* zone,uint32 page_no,uchar *buf)
{
    int ret;
    uchar spare_data[BYTES_OF_USER_PER_PAGE];
    _mapping_page* logic_page_map;
    _physic_op_par phy_op_par;
    _phy_block_info* p_phy_block;

    logic_page_map = get_logic_page_map(zone,page_no);
    if(logic_page_map->Block_NO == 0xffff)
    {
//      NFTL_ERR("[NE]read a page which not be write before!!\n");
        MEMSET(buf,0xff,zone->nand_chip->bytes_per_page);
        return NFTL_SUCCESS;
    }

    logic_page_map->Read_flag = zone->already_read_flag;

//  NFTL_ERR("[NE]read page: %d[%d][%d]\n",page_no,logic_page_map->Block_NO,logic_page_map->Page_NO);
    set_physic_op_par(&phy_op_par,logic_page_map->Block_NO,logic_page_map->Page_NO,zone->nand_chip->bitmap_per_page,buf,spare_data);
    ret = zone->nftl_nand_read_page(zone,&phy_op_par);
//    if(ret == ECC_LIMIT)
//    {
//        if((zone->cfg->nftl_support_gc_read_reclaim != 0) && (zone->nand_chip->support_read_reclaim != 0))
//        {
//            p_phy_block = get_phy_block_addr(zone,logic_page_map->Block_NO);
//            if(is_current_blcok(zone,p_phy_block)!=1)
//            {
//                add_prio_gc(zone,p_phy_block,GC_READ_RECLAIM);
//                NFTL_ERR("[NE]read a page: %d READ_RECLAIM!\n",page_no);
//            }
//        }
//        ret = NFTL_SUCCESS;
//    }
    if(ret == NAND_ERR_ECC)
    {
        p_phy_block = get_phy_block_addr(zone,logic_page_map->Block_NO);
        add_prio_gc(zone,p_phy_block,GC_READ_RECLAIM);
        NFTL_ERR("[NE]read a page: %d READ_RECLAIM!\n",page_no);
    }

    do_prio_gc(zone);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 do_write_error(struct _nftl_zone* zone,_current_block* current_block)
{
    uint32 ret,i,logic_page;
    _phy_block_info* p_phy_block_free;
    _physic_op_par phy_op_par;
    _mapping_page* logic_page_map_temp;

write_error:
    p_phy_block_free = out_phy_block_from_free_list(zone);
    if(p_phy_block_free == NULL)
    {
        NFTL_ERR("[NE]no free block\n");
        return NFTL_FAILURE;
    }

    set_physic_op_par(&phy_op_par,p_phy_block_free->phy_block.Block_NO,0,zone->nand_chip->bitmap_per_page,NULL,NULL);
    ret = zone->nftl_nand_erase_superblk(zone,&phy_op_par);
    p_phy_block_free->invalid_page_count = 0;
    p_phy_block_free->erase_count++;
    if(ret != 0)
    {
        zone->nftl_nand_mark_bad_blk(zone,&phy_op_par);
        NFTL_ERR("[NE]do_write_error erase error!\n");
        if(zone->free_block_num > (zone->gc_strategy.start_gc_free_blocks>>2))
        {
            goto write_error;
        }
        else
        {
            NFTL_ERR("[NE]no free block\n");
            return NFTL_FAILURE;
        }
    }
    p_phy_block_free->block_used_count = current_block->block_info->block_used_count + 1;

    for(i=0; i<current_block->page_used; i++)
    {
        logic_page = current_block->user_info.map_data[i];
        if(logic_page != 0xffffffff)
        {
            ret = zone->nftl_nand_copy_page(zone,current_block->block_info,p_phy_block_free,zone->temp_page_buf,i);
            if(ret != 0)
            {
                set_physic_op_par(&phy_op_par,p_phy_block_free->phy_block.Block_NO,0,zone->nand_chip->bitmap_per_page,NULL,NULL);
                zone->nftl_nand_mark_bad_blk(zone,&phy_op_par);
                goto write_error;
            }
        }
        else
        {
            break;
        }
    }
    p_phy_block_free->invalid_page_count = current_block->block_info->invalid_page_count;

    set_physic_op_par(&phy_op_par,current_block->block_info->phy_block.Block_NO,0,zone->nand_chip->bitmap_per_page,NULL,NULL);
    zone->nftl_nand_mark_bad_blk(zone,&phy_op_par);
    //erase_block(zone,current_block->block_info,6000);
    p_phy_block_free->info = current_block->block_info->info;
    current_block->block_info = p_phy_block_free;

    for(i=0; i<current_block->page_used; i++)
    {
        logic_page = current_block->user_info.map_data[i];
        if(logic_page != 0xffffffff)
        {
            logic_page_map_temp = get_logic_page_map(zone,logic_page);
            if(logic_page_map_temp != NULL)
            {
                logic_page_map_temp->Block_NO = current_block->block_info->phy_block.Block_NO;
                logic_page_map_temp->Page_NO = i;
            }
        }
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_write_data_page(struct _nftl_zone* zone,uint32 data_type,uint32 page_no,uchar *buf)
{
    if(current_block_at_last_page(zone,&zone->current_block) == NFTL_YES)
    {
        if(do_after_write(zone) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]do_after_write error1!\n");
            return NFTL_FAILURE;
        }
    }

    if(do_before_write(zone) != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]do_before_write error!\n");
        return NFTL_FAILURE;
    }

    if(write_data_to_page(zone,data_type,page_no,buf) != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]write data to page error!\n");
        return NFTL_FAILURE;
    }

    if(do_after_write(zone) != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]do_after_write error2!\n");
        return NFTL_FAILURE;
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_write_logic_page_no_gc(struct _nftl_zone* zone,uint32 page_no,uchar *buf)
{
    int ret;

    ret = nand_write_data_page(zone,FLAG_FOR_LOGIC_PAGE,page_no,buf);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int do_before_write(struct _nftl_zone* zone)
{
    //如果是新的block,需要先初始化
    while(current_block_at_first_page(zone,&zone->current_block) == NFTL_YES)
    {
        if(new_block_init_for_write(zone,zone->current_block.block_info,zone->current_block.block_info->block_used_count) != NFTL_SUCCESS)
        {
            zone->current_block.block_info->block_used_count--;
            if(zone->zone_attr&SUPPORT_COSS_TALK)
            {
                zone->current_block.block_info->block_used_count--;
            }
            if(get_new_current_block(zone,&zone->current_block) != NFTL_SUCCESS)
            {
                NFTL_ERR("[NE]get_new_current_block error3!\n");
                return NFTL_FAILURE;
            }
        }
        else
        {
            break;
        }
    }

    if(zone->zone_attr&SUPPORT_COSS_TALK)
    {
        while(current_block_at_first_page(zone,&zone->assist_block) != 0)
        {
            if(new_block_init_for_write(zone,zone->assist_block.block_info,zone->assist_block.block_info->block_used_count) != NFTL_SUCCESS)
            {
                zone->assist_block.block_info->block_used_count--;
                if(zone->zone_attr&SUPPORT_COSS_TALK)
                {
                    zone->assist_block.block_info->block_used_count--;
                }
                if(get_new_current_block(zone,&zone->assist_block) != NFTL_SUCCESS)
                {
                    NFTL_ERR("[NE]get_new_current_block error4!\n");
                    return NFTL_FAILURE;
                }
            }
            else
            {
                break;
            }
        }
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int do_after_write(struct _nftl_zone* zone)
{
    if(current_block_at_last_page(zone,&zone->current_block) == NFTL_YES)
    {
        if(write_phy_page_map_to_current_block(zone,&zone->current_block) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]write_phy_page_map_to_current_block error!\n");
            return NFTL_FAILURE;
        }

        if(zone->zone_attr&SUPPORT_COSS_TALK)
        {
            if(write_phy_page_map_to_current_block(zone,&zone->assist_block) != NFTL_SUCCESS)
            {
                NFTL_ERR("[NE]write_phy_page_map_to_current_block error!\n");
                //return NFTL_FAILURE;
            }
        }

        if(get_new_current_block(zone,&zone->current_block) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]get_new_current_block error1!\n");
            return NFTL_FAILURE;
        }

        if(zone->zone_attr&SUPPORT_COSS_TALK)
        {
            if(get_new_current_block(zone,&zone->assist_block) != NFTL_SUCCESS)
            {
                NFTL_ERR("[NE]get_new_current_block error2!\n");
                return NFTL_FAILURE;
            }
        }
    }
    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int write_data_to_page(struct _nftl_zone* zone,uint32 data_type,uint32 page_no,uchar *buf)
{
    if(write_logic_data_to_current_block(zone,&zone->current_block,data_type,page_no,buf) != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]write logic data to current block error\n");
        return NFTL_FAILURE;
    }

    if(zone->zone_attr&SUPPORT_COSS_TALK)
    {
        if(write_logic_data_to_current_block(zone,&zone->assist_block,data_type,page_no,buf) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]write logic data to current block error\n");
            return NFTL_FAILURE;
        }
    }
    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int write_phy_data_to_current_block(struct _nftl_zone* zone,_current_block* current_block,uchar* buf,uint32 page_no,uint32 type)
{
    int ret,retry_times;
    uchar spare_data[BYTES_OF_USER_PER_PAGE];
    _physic_op_par phy_op_par;

    MEMSET(spare_data,0xa5,BYTES_OF_USER_PER_PAGE);

    retry_times = 0;
write_data:
    set_physic_op_par(&phy_op_par,current_block->block_info->phy_block.Block_NO,current_block->page_used,zone->nand_chip->bitmap_per_page,buf,spare_data);

    if(type == PHY_MAPPING_PAGE)
    {
        if(current_block->page_used != (zone->nand_chip->pages_per_blk - 1))
        {
            //NFTL_ERR("[NE]mapping page must in last block %d!!\n",current_block->block_info->phy_block.Block_NO);
        }
        set_oob_special_page(zone,spare_data,PHY_MAPPING_PAGE,current_block->block_info->block_used_count,current_block->block_info->erase_count);
    }
    else if(type == FLAG_FOR_LOGIC_PAGE)
    {
        if(current_block->page_used == (zone->nand_chip->pages_per_blk - 1))
        {
            NFTL_ERR("[NE]last block can not used to logic page %d!!\n",current_block->block_info->phy_block.Block_NO);
        }
        set_oob_logic_page(zone,spare_data,page_no,current_block->block_info->block_used_count,current_block->block_info->erase_count);
    }
    else if(type == PHY_NORMAL_POWER_DOWN)
    {
        if(current_block->page_used == (zone->nand_chip->pages_per_blk - 1))
        {
            NFTL_ERR("[NE]last block can not used to power down page %d!!\n",current_block->block_info->phy_block.Block_NO);
        }
        set_oob_special_page(zone,spare_data,PHY_NORMAL_POWER_DOWN,current_block->block_info->block_used_count,current_block->block_info->erase_count);
    }
    else
    {
        ;
    }

    current_block->block_info->info = BLOCK_HAVE_USED;

    ret = zone->nftl_nand_write_page(zone,&phy_op_par);
    if((ret != 0) && (retry_times < 2))
    {
        NFTL_ERR("[NE]write_phy_data_to_current_block write error %d!\n",phy_op_par.phy_page.Block_NO);
        if(do_write_error(zone,current_block) == NFTL_SUCCESS)
        {
            retry_times++;
            goto write_data;
        }
        else
        {
            NFTL_ERR("[NE]do_write_error error\n");
            return NFTL_FAILURE;
        }
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int write_phy_page_map_to_current_block(struct _nftl_zone* zone,_current_block* current_block)
{
    int ret;

//    if(current_block->current_used_block != (zone->nand_chip->pages_per_blk -1))
//        return NFTL_SUCCESS;

    //print_smart(zone);

    if(current_block == &zone->assist_block)
    {
        MEMCPY(zone->assist_block.user_info.smart,zone->current_block.user_info.smart,sizeof(struct __smart));
    }

    ret = write_phy_data_to_current_block(zone,current_block,current_block->user_info.buf,0,PHY_MAPPING_PAGE);
    if(ret != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]write_phy_data_to_current_block error1\n");
        return ret;
    }
    current_block->page_used++;
    current_block->block_info->invalid_page_count++;

    check_invalid_page_right(zone,&current_block->user_info,current_block->block_info,current_block->page_used);

    if(current_block->page_used == zone->nand_chip->pages_per_blk)
    {
        put_phy_block_to_invalid_page_list(zone,current_block->block_info);
        adjust_invaild_list(zone);
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :这个函数只将数据写入current_block,调用之前保证有足够的空间,及正确的block
*****************************************************************************/
int write_logic_data_to_current_block(struct _nftl_zone* zone,_current_block* current_block,uint32 data_type,uint32 page_no,uchar *buf)
{
    int ret;
    uint32 flag = 0;
    _phy_block_info* p_phy_block;
    _mapping_page* logic_page_map;

    if(data_type == FLAG_FOR_LOGIC_PAGE)
    {
        logic_page_map = get_logic_page_map(zone,page_no);
        if(logic_page_map->Block_NO != 0xffff)
        {
            p_phy_block = get_phy_block_addr(zone,logic_page_map->Block_NO);
            if(p_phy_block == current_block->block_info)
            {
                p_phy_block->invalid_page_count++;
                if(p_phy_block->invalid_page_count > zone->nand_chip->pages_per_blk)
                {
                    NFTL_ERR("[NE]invalid_page_count more than pages_per_blk at write logic data to current block:%d!!\n",current_block->page_used);
                }
            }
            else
            {
                flag = 1;
            }
        }

        ret = write_phy_data_to_current_block(zone,current_block,buf,page_no,FLAG_FOR_LOGIC_PAGE);
        if(ret != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]write_phy_data_to_current_block error2\n");
            return ret;
        }
        logic_page_map->Block_NO = current_block->block_info->phy_block.Block_NO;
        logic_page_map->Page_NO = current_block->page_used;
        logic_page_map->Read_flag = !zone->already_read_flag;
        current_block->user_info.map_data[logic_page_map->Page_NO] = page_no;
        current_block->page_used++;  
    }
    else if(data_type == PHY_DISCARD_INFO_PAGE)
    {
        logic_page_map = get_logic_page_map(zone,page_no);
        if(logic_page_map->Block_NO != 0xffff)
        {
            p_phy_block = get_phy_block_addr(zone,logic_page_map->Block_NO);
            if(p_phy_block == current_block->block_info)
            {
                current_block->user_info.map_data[logic_page_map->Page_NO] = 0xffffffff;
                p_phy_block->invalid_page_count++;
                if(p_phy_block->invalid_page_count > zone->nand_chip->pages_per_blk)
                {
                    NFTL_ERR("[NE]invalid_page_count more than pages_per_blk at write logic data to current block:%d!!\n",current_block->page_used);
                }
            }
            else
            {
                flag = 1;
            }
        }
        logic_page_map->Block_NO = 0xffff;
        logic_page_map->Page_NO = 0xfff;
        logic_page_map->Read_flag = 0x01;
    }
    else if(data_type == PHY_NORMAL_POWER_DOWN)
    {
        current_block->block_info->invalid_page_count++;
        ret = write_phy_data_to_current_block(zone,current_block,buf,page_no,PHY_NORMAL_POWER_DOWN);
        if(ret != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]write phy data to current block error2\n");
            return ret;
        }
        //current_block->user_info->phy_page_map[logic_page_map->Page_NO] = PHY_NORMAL_POWER_DOWN;
        current_block->page_used++;  
    }
    
    else
    {
        ;
    }

    if(flag == 1)
    {
        phy_block_from_invalid_page_incr(zone,p_phy_block);
    }

    adjust_invaild_list(zone);

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int get_new_current_block(struct _nftl_zone* zone,_current_block* current_block)
{
    uint32 block_used_count;

    block_used_count = current_block->block_info->block_used_count;

    MEMSET(current_block->user_info.map_data,0xff,current_block->user_info.map_size);

    if(zone->s_wl.s_wl_start != 0)
    {
        current_block->block_info = out_phy_block_from_free_list_by_erase_time(zone,zone->nand_chip->max_erase_times);
        if (current_block->block_info != NULL)
        {
            #if 0
            if (current_block->block_info->erase_count >= zone->nand_chip->max_erase_times) {
                NFTL_ERR("[NE]swl: new_cur_blk->there are bad(%d) block in free list!!!\n", current_block->block_info->erase_count);
                return NFTL_FAILURE;
            }
            #endif
            NFTL_ERR("[ND]swl: new_cur_blk->alloc ok(max_ec): block_no = %d; ec = %d; block_used_count = %d\n",
                current_block->block_info->phy_block.Block_NO,current_block->block_info->erase_count,block_used_count + 1);
        }
    }
    else
    {
         current_block->block_info = out_phy_block_from_free_list(zone);
    }

    if(current_block->block_info == NULL)
    {
        NFTL_ERR("[NE]no free block!!!!\n");
        return NFTL_FAILURE;
    }
    //NFTL_ERR("[NE]zone->zone_current_used_block->phy_block.Block_NO %d\n\n",zone->zone_current_used_block->phy_block.Block_NO);

    current_block->page_used = 0;
    current_block->block_info->block_used_count = block_used_count + 1;
    if(zone->zone_attr&SUPPORT_COSS_TALK)
    {
        current_block->block_info->block_used_count++;
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int new_block_init_for_write(struct _nftl_zone* zone,_phy_block_info* block,uint32 block_used_count)
{
    int ret;
    _physic_op_par phy_op_par;

    set_physic_op_par(&phy_op_par,block->phy_block.Block_NO,0,zone->nand_chip->bitmap_per_page,NULL,NULL);
    ret = zone->nftl_nand_erase_superblk(zone,&phy_op_par);
    block->invalid_page_count = 0;
    block->erase_count++;
    if((block->erase_count > zone->max_erase_num) && (block->erase_count < zone->nand_chip->max_erase_times))
    {
        zone->max_erase_num = block->erase_count;
    }
    block->block_used_count = block_used_count;
    if(ret != 0)
    {
        zone->nftl_nand_mark_bad_blk(zone,&phy_op_par);
        NFTL_ERR("[NE]new_block_init_for_write erase error!\n");
        return NFTL_FAILURE;
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int check_invalid_page_right(struct _nftl_zone* zone,_user_info* user_info,_phy_block_info* block,uint16 page_num)
{
    uint32 i,logic_page,invalid_page_count;
    _mapping_page* logic_page_map_temp;

    invalid_page_count = 0;

//    for(i=0;i<(zone->nand_chip->pages_per_blk-1);i++)
    for(i=0 ; i<page_num; i++)
    {
        logic_page = user_info->map_data[i];
        if(logic_page != 0xffffffff)
        {
            logic_page_map_temp = get_logic_page_map(zone,logic_page);
            if(logic_page_map_temp == NULL)
            {
                NFTL_ERR("[NE]logic_page_map_temp NULL!!\n");
            }
            if((logic_page_map_temp->Block_NO != block->phy_block.Block_NO)||(logic_page_map_temp->Page_NO != i))
            {
                invalid_page_count++;
            }
        }
        else
        {
            //NFTL_ERR("[NE]logic_page_map_temp 0xffffffff %d!!\n",i);
            invalid_page_count++;
        }
    }
    if(invalid_page_count != block->invalid_page_count)
    {
        NFTL_ERR("[NE]invaild_page num error:[%d] [%d]  used_page:[%d]!!\n",invalid_page_count,block->invalid_page_count,page_num);
//        print_block_invalid_list(zone);
//        block->invalid_page_count = invalid_page_count;
        return NFTL_FAILURE;
    }

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void nftl_ops_init(struct _nftl_zone *zone)
{
    zone->nftl_nand_erase_superblk = _nftl_nand_erase_superblk;
    zone->nftl_nand_read_page = _nftl_nand_read_page;
    zone->nftl_nand_write_page = _nftl_nand_write_page;
    zone->nftl_nand_is_blk_good = _nftl_nand_is_blk_good;
    zone->nftl_nand_mark_bad_blk = _nftl_nand_mark_bad_blk;
    zone->nftl_nand_copy_page = _nftl_nand_copy_page;

    zone->nftl_nand_read_logic_page = _nftl_nand_read_logic_page;
    zone->nftl_nand_write_logic_page = _nftl_nand_write_logic_page;

    return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int current_block_at_last_page(struct _nftl_zone *zone,_current_block* current_block)
{
    if(current_block->page_used == (zone->nand_chip->pages_per_blk - 1))
        return 1;
    else
        return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int current_block_at_first_page(struct _nftl_zone *zone,_current_block* current_block)
{
    if(current_block->page_used == 0)
        return 1;
    else
        return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int fill_no_use_data_to_page(struct _nftl_zone* zone)
{
    //如果是写最后一个page
    if(current_block_at_last_page(zone,&zone->current_block) != 0)
    {
        if(write_phy_page_map_to_current_block(zone,&zone->current_block) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]write_phy_page_map_to_current_block error!\n");
            return NFTL_FAILURE;
        }

        if(zone->zone_attr&SUPPORT_COSS_TALK)
        {
            if(write_phy_page_map_to_current_block(zone,&zone->assist_block) != NFTL_SUCCESS)
            {
                NFTL_ERR("[NE]write_phy_page_map_to_current_block error!\n");
                //return NFTL_FAILURE;
            }
        }

        if(get_new_current_block(zone,&zone->current_block) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]get_new_current_block error1!\n");
            return NFTL_FAILURE;
        }

        if(zone->zone_attr&SUPPORT_COSS_TALK)
        {
            if(get_new_current_block(zone,&zone->assist_block) != NFTL_SUCCESS)
            {
                NFTL_ERR("[NE]get_new_current_block error2!\n");
                return NFTL_FAILURE;
            }
        }
        return NFTL_SUCCESS;
    }

    if(current_block_at_first_page(zone,&zone->current_block) != 0)
    {
        return NFTL_SUCCESS;
    }

    if(write_phy_page_map_to_current_block(zone,&zone->current_block) != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]write_phy_page_map_to_current_block error!\n");
    }

    if(zone->zone_attr&SUPPORT_COSS_TALK)
    {
        if(write_phy_page_map_to_current_block(zone,&zone->assist_block) != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]write_phy_page_map_to_current_block error!\n");
        }
    }

    return NFTL_COMPLETE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int fill_no_use_data_to_current_block(struct _nftl_zone* zone)
{
    int i,ret;

    for(i=0;i<zone->nand_chip->pages_per_blk;i++)
    {
        ret = fill_no_use_data_to_page(zone);
        if(ret == NFTL_SUCCESS)
        {
            return NFTL_SUCCESS;
        }
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
int fill_no_use_page_to_current_block(struct _nftl_zone* zone,int page_num)
{
    int i,ret;

    for(i=0;i<page_num;i++)
    {
        ret = fill_no_use_data_to_page(zone);
        if(ret == NFTL_SUCCESS)
        {
            return NFTL_SUCCESS;
        }
    }
    return NFTL_FAILURE;
}
