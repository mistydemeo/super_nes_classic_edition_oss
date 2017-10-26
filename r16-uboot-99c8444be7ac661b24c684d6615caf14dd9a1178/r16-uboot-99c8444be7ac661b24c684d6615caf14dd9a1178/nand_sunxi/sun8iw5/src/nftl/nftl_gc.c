
/*
 * allwinner nftl gc
 *
 * (C) 2008
 */

#define _NFTL_GC_C_

#include "nftl_inc.h"
#include "../nftl_interface/nftl_cfg.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 garbage_collect(struct _nftl_zone* zone)
{
    uint16 page_num;
    _phy_block_info* phy_block;

    if(zone->free_block_num <= zone->gc_strategy.start_gc_free_blocks)
    {
        zone->gc_strategy.process = GC_ON;
    }

    if(zone->gc_strategy.process == GC_ON)
    {
        if(zone->free_block_num < zone->gc_strategy.stop_gc_free_blocks)
        {
            if(zone->gc_strategy.flag_gc_block == 0)
            {
                phy_block = zone->invalid_page_head.invalid_page_next;
                if(phy_block == NULL)
                {
                    NFTL_ERR("[NE]garbage_collect phy_block null\n");
                    return NFTL_FAILURE;
                }
                if(phy_block->invalid_page_count >= (zone->nand_chip->pages_per_blk>>1))
                {
                    zone->gc_strategy.gc_page = 4;
                }
                else if(phy_block->invalid_page_count >= (zone->nand_chip->pages_per_blk>>2))
                {
                    zone->gc_strategy.gc_page = 8;
                }
                else if(phy_block->invalid_page_count >= (zone->nand_chip->pages_per_blk>>3))
                {
                    zone->gc_strategy.gc_page = 16;
                }
                else
                {
                    NFTL_ERR("[ND]garbage too frequency! %d:%d\n",zone->free_block_num,phy_block->invalid_page_count);
                    zone->gc_strategy.gc_page = 0xffff;
//                  print_block_invalid_list(zone->zone_phy_block_invalid_page_head);
                }

//              if(zone->free_block_num <= (zone->gc_strategy.start_gc_free_blocks>>1))
//              {
//                  //NFTL_ERR("[NE]something is wrong 2! %d:%d\n",zone->free_block_num,phy_block->invalid_page_count);
//                  zone->gc_strategy.gc_page = 0xffff;
//              }

                if(zone->free_block_num <= zone->cfg->nftl_min_free_block)
                {
                    //NFTL_ERR("[NE]something is wrong 2! %d:%d\n",zone->free_block_num,phy_block->invalid_page_count);
                    zone->gc_strategy.gc_page = 0xffff;
                }
            }

            page_num = zone->gc_strategy.gc_page;

            if(page_num != 0xffff)
            {

                if(garbage_collect_first(zone,NULL,page_num) != NFTL_SUCCESS)
                {
                    NFTL_ERR("[NE]garbage_collect_first wrong! %d\n",zone->free_block_num);
                    return NFTL_FAILURE;
                }
            }
            else
            {
                gc_win_block(zone,1);
            }
        }
        else
        {
            zone->gc_strategy.process = GC_STOP;
            zone->gc_strategy.flag_gc_block = 0;
            zone->gc_strategy.gc_page = 0;
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
uint32 garbage_collect_first(struct _nftl_zone* zone,_phy_block_info* block,uint16 page_num)
{
    uint32 i,logic_page,ret,read;
    uint32 have_page_error_flag = 0;
    uchar spare_data[BYTES_OF_USER_PER_PAGE];
    _phy_block_info* p_phy_block;
    _mapping_page* logic_page_map;
    _physic_op_par phy_op_par;

    uint32 try_num = 0;
    MEMSET(spare_data,0x5a,BYTES_OF_USER_PER_PAGE);

    //if(zone->test == 4)
    //    NFTL_ERR("[NE]garbage_collect_first %d!!\n",page_num);

    if(block == NULL)
    {
        p_phy_block = out_phy_block_from_invalid_page_list(zone);
    }
    else
    {
        //if(block == zone->current_block.block_info)
        if(is_current_blcok(zone,block)==1)
        {
            return NFTL_SUCCESS;
        }
        p_phy_block = out_phy_block_from_invalid_page_list_by_block(zone,block);
    }

    if(p_phy_block == NULL)
    {
        NFTL_ERR("[NE]garbage_collect_first none!!\n");
        return NFTL_COMPLETE;
    }

    zone->smart->total_gc_times++;

    set_physic_op_par(&phy_op_par,p_phy_block->phy_block.Block_NO,zone->nand_chip->pages_per_blk - 1,zone->nand_chip->bitmap_per_page,zone->temp_page_buf,spare_data);
    zone->nftl_nand_read_page(zone,&phy_op_par);
    if(is_phy_mapping_page(spare_data) != NFTL_YES)
    {
        NFTL_ERR("[NE]garbage_collect_first wrong!!\n");
        NFTL_ERR("[NE]garbage_collect_first cannot find PHY_MAPPING_PAGE %d %d %d %d!!\n",p_phy_block->phy_block.Block_NO,zone->nand_chip->pages_per_blk - 1,try_num,zone->nand_chip->bitmap_per_page);
        NFTL_ERR("[NE]garbage_collect_first cannot find PHY_MAPPING_PAGE %x %x %x %x !!\n",spare_data[SPARE_OFFSET_SPECIAL_FLAG],spare_data[SPARE_OFFSET_SPECIAL_FLAG+1],spare_data[SPARE_OFFSET_SPECIAL_FLAG+2],spare_data[SPARE_OFFSET_SPECIAL_FLAG+3]);
        NFTL_ERR("[NE]garbage_collect_first cannot find PHY_MAPPING_PAGE %x %x %x %x %x!!\n",spare_data[5],spare_data[6],spare_data[7],spare_data[8],spare_data[9]);
        //重建mapping表
        NFTL_ERR("[NE]garbage_collect_first rebuild %d,%d!!\n",zone->zone_no,p_phy_block->phy_block.Block_NO);
        //recover_phy_page_mapping(zone,p_phy_block,zone->zone_phy_page_map_for_gc.map_data);
        recover_block_phy_page_mapping(zone,p_phy_block,zone->zone_phy_page_map_for_gc.map_data);
        have_page_error_flag = 1;
    }
    else
    {
        MEMCPY(zone->zone_phy_page_map_for_gc.map_data,zone->temp_page_buf,zone->zone_phy_page_map_for_gc.map_size);
    }

    //先计算有效page数目是不是与内存中保持的数值一致
    ret = check_invalid_page_right(zone,&zone->zone_phy_page_map_for_gc,p_phy_block,zone->nand_chip->pages_per_blk);
    if(ret != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]garbage_collect_first invaild_page num error!!\n");
        have_page_error_flag = 1;
    }

    if(have_page_error_flag == 1)
    {
        page_num = 0xffff;
    }

    for(i=0; i<(zone->nand_chip->pages_per_blk-1); i++)
    {
        logic_page = zone->zone_phy_page_map_for_gc.map_data[i];
        if(logic_page != 0xffffffff)
        {
            logic_page_map = get_logic_page_map(zone,logic_page);
            if((logic_page_map->Block_NO == p_phy_block->phy_block.Block_NO) && (logic_page_map->Page_NO == i))
            {
                //有效page，需要移动
                zone->smart->total_gc_pages++;
                set_physic_op_par(&phy_op_par,p_phy_block->phy_block.Block_NO,i,zone->nand_chip->bitmap_per_page,zone->logic_page_buf,spare_data);
                zone->nftl_nand_read_page(zone,&phy_op_par);
                read = get_logic_page_from_oob(spare_data);

                if(read != logic_page)
                {
                    NFTL_ERR("[NE]garbage_collect_first read a logic page error %d %d %d %d!!\n",p_phy_block->phy_block.Block_NO,i,read,logic_page);
                    NFTL_ERR("[NE]%x %x %x %x %x !!\n",spare_data[0],spare_data[1],spare_data[2],spare_data[3],spare_data[4]);
                    NFTL_ERR("[NE]%x %x %x %x %x!!\n",spare_data[5],spare_data[6],spare_data[7],spare_data[8],spare_data[9]);
                    logic_page_map->Block_NO = 0xffff;
                    logic_page_map->Page_NO = 0xfff;
                    have_page_error_flag = 1;
                    page_num = 0xffff;
                }
                else
                {
                    ret = nand_write_logic_page_no_gc(zone,logic_page,zone->logic_page_buf);
                    if(ret != 0)
                    {
                        NFTL_ERR("[NE]nand_write_logic_page_no_gc gc fail!!\n");
                        return NFTL_FAILURE;
                    }
                }
                page_num--;
                if(page_num == 0)
                {
                    break;
                }
            }
        }
    }

    if(i == (zone->nand_chip->pages_per_blk-1))
    {
        zone->gc_strategy.flag_gc_block = 0;
    }

    if(have_page_error_flag==1)
    {
        //erase ecc error block
        NFTL_ERR("[NE]erase ecc error block\n");
        erase_block(zone,p_phy_block,p_phy_block->erase_count+1);
        return NFTL_SUCCESS;
    }

    put_phy_block_to_invalid_page_list(zone,p_phy_block);
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
_prio_gc_node* get_empty_gc_node(_prio_gc *prio_gc)
{
    uint16 i;
    for(i=0; i<MAX_PRIO_GC_NUM; i++)
    {
        if(prio_gc->prio_gc_node[i].prio_type == PRIO_NONE)
        {
            return &prio_gc->prio_gc_node[i];
        }
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
_prio_gc_node *search_gc_node(_prio_gc *prio_gc,_phy_block_info* block)
{
    _prio_gc_node * p = &prio_gc->prio_gc_head;

    for(p=p->prio_gc_next; p; p=p->prio_gc_next)
    {
        if(p->phy_block_info == block)
        {
            return p;
        }
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
int add_to_gc_list_tail(_prio_gc *prio_gc,_prio_gc_node* gc_node)
{
    _prio_gc_node * p = &prio_gc->prio_gc_head;

    while(p->prio_gc_next != NULL)
    {
        p = p->prio_gc_next;
    }

    p->prio_gc_next = gc_node;
    gc_node->prio_gc_next = NULL;
    gc_node->prio_gc_prev = p;
    prio_gc->gc_num += 1;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_prio_gc_node* del_from_gc_list(_prio_gc *prio_gc,_prio_gc_node* gc_node)
{
    _prio_gc_node * p = gc_node->prio_gc_prev;

    p->prio_gc_next = gc_node->prio_gc_next;
    if(gc_node->prio_gc_next != NULL)
        gc_node->prio_gc_next->prio_gc_prev = p;

    gc_node->prio_gc_next = NULL;
    gc_node->prio_gc_prev = NULL;
    prio_gc->gc_num -= 1;

    return gc_node;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void add_prio_gc(struct _nftl_zone* zone,_phy_block_info* block,uint16 type)
{
    _prio_gc_node * node;

    node = search_gc_node(&zone->prio_gc,block);
    if(node != NULL)
    {
        return;
    }
    node = get_empty_gc_node(&zone->prio_gc);
    if(node == NULL)
    {
        return;
    }
    node->phy_block_info = block;
    node->prio_type = type;
    node->prio_gc_next = NULL;
    node->prio_gc_prev = NULL;

    add_to_gc_list_tail(&zone->prio_gc,node);
    return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 do_prio_gc(struct _nftl_zone* zone)
{
    uint32 ret;
    _prio_gc_node * p;
    _phy_block_info* block;

    if(zone->prio_gc.prio_gc_head.prio_gc_next == NULL)
    {
        return NFTL_SUCCESS;
    }

    p = del_from_gc_list(&zone->prio_gc,zone->prio_gc.prio_gc_head.prio_gc_next);

    if(is_current_blcok(zone,p->phy_block_info)==1)
    {
        //NFTL_ERR("[ND]dealy to prio gc\n");
        add_to_gc_list_tail(&zone->prio_gc,p);
        return NFTL_SUCCESS;
    }

    if(p->phy_block_info == NULL)
    {
        NFTL_ERR("[ND]no block to prio gc\n");
        p->prio_type = PRIO_NONE;
        zone->prio_gc.prio_type_now = PRIO_NONE;
        p->phy_block_info = NULL;
        return NFTL_SUCCESS;
    }

    if(is_block_in_invalid_list(zone,p->phy_block_info) == NFTL_NO)
    {
        NFTL_ERR("[ND]no need gc:%d, type:%d\n",p->phy_block_info->phy_block.Block_NO,p->prio_type);
        p->prio_type = PRIO_NONE;
        zone->prio_gc.prio_type_now = PRIO_NONE;
        p->phy_block_info = NULL;
        return NFTL_SUCCESS;
    }

    NFTL_ERR("[ND]do_prio_gc block:%d, type:%d\n",p->phy_block_info->phy_block.Block_NO,p->prio_type);
    zone->prio_gc.prio_type_now = p->prio_type;
    ret = garbage_collect_first(zone,p->phy_block_info,0xffff);
    if(ret != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]do_prio_gc garbage_collect_first fail:%d, type:%d\n",p->phy_block_info->phy_block.Block_NO,p->prio_type);
        p->prio_type = PRIO_NONE;
        zone->prio_gc.prio_type_now = PRIO_NONE;
        p->phy_block_info = NULL;
        return NFTL_FAILURE;
    }

    if(is_block_in_free_list(zone,p->phy_block_info) == NFTL_NO)
    {
        NFTL_ERR("[ND]prio gc fail1:%d, type:%d\n",p->phy_block_info->phy_block.Block_NO,p->prio_type);
        p->prio_type = PRIO_NONE;
        zone->prio_gc.prio_type_now = PRIO_NONE;
        p->phy_block_info = NULL;
        return NFTL_SUCCESS;
    }

    block = out_phy_block_from_free_list_by_block(zone,p->phy_block_info);
    if(block == NULL)
    {
        NFTL_ERR("[ND]prio gc fail2:%d, type:%d\n",p->phy_block_info->phy_block.Block_NO,p->prio_type);
        p->prio_type = PRIO_NONE;
        zone->prio_gc.prio_type_now = PRIO_NONE;
        p->phy_block_info = NULL;
        return NFTL_SUCCESS;
    }

    if(zone->prio_gc.prio_type_now == GC_READ_RECLAIM)
    {
        erase_block(zone,block,block->erase_count+1);
    }
    else if(zone->prio_gc.prio_type_now == GC_WEAR_LEVELING)
    {
        erase_block(zone,block,block->erase_count+1);
    }
    else if(zone->prio_gc.prio_type_now == GC_READ_ERROR)
    {
        //CHECK BLOCK
        erase_block(zone,block,block->erase_count+1);
    }
    else
    {
        erase_block(zone,block,block->erase_count+1);
    }

    p->prio_type = PRIO_NONE;
    zone->prio_gc.prio_type_now = PRIO_NONE;
    p->phy_block_info = NULL;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 prio_gc_all(struct _nftl_zone* zone)
{
    uint32 i,ret=NFTL_SUCCESS;

    for(i=0; i<MAX_PRIO_GC_NUM; i++)
    {
        ret |= do_prio_gc(zone);
    }
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 check_static_wear_leveling(struct _nftl_zone* zone)
{
    _phy_block_info *mim_ec_blk = NULL;
    _phy_block_info *max_ec_blk = NULL;
    uint16 min_ec = 0xffff;
    uint16 max_ec = 0;

    if(zone->s_wl.block_for_s_wl != NULL)
    {
        return NFTL_FAILURE;    /* static WL is doing... */
    }

    if(!zone->invalid_page_head.invalid_page_next || !zone->free_head.free_next)
    {
        return NFTL_FAILURE;    /* nothing to do in */
    }

    if(zone->current_block.block_info->block_used_count < zone->blocks)
    {
        return NFTL_FAILURE;    /* 总写入数据量较少 */
    }

    max_ec_blk = get_block_max_erase_time_from_free_list(zone, zone->nand_chip->max_erase_times);
    if(max_ec_blk == NULL)
    {
        return NFTL_FAILURE;    /* 一般不会 */
    }

    max_ec = max_ec_blk->erase_count;
    if(max_ec < (zone->nand_chip->max_erase_times/5))
    {
        return NFTL_FAILURE;   /* free block比较新，不应该换入 */
    }

    mim_ec_blk = get_block_min_erase_time_from_invalid_list(zone,zone->nand_chip->max_erase_times,zone->current_block.block_info->block_used_count - zone->blocks);
    if(mim_ec_blk == NULL)
    {
        return NFTL_FAILURE;    /* nothing to do in */
    }

    min_ec = mim_ec_blk->erase_count;

    if(max_ec <= min_ec)
    {
        return NFTL_FAILURE;    /* wrong */
    }

#if 0
    if((max_ec >= zone->nand_chip->max_erase_times) || (min_ec >= zone->nand_chip->max_erase_times))
    {
        NFTL_ERR("[NE]swl: there are bad(%d) block in free list!!!\n", max_ec);
        return NFTL_FAILURE;
    }
#endif

    /** 当使用到ec越高，做静态WL的门槛越低(max_ec与min_ec的差值越小)
     *  当使用到ec还不太高时，做静态WL的门槛相对较高，防止静态WL太频繁
     */
    if (zone->nand_chip->max_erase_times * 2 < max_ec * 3) //大于2000 erase_span必须大于250
    {
        zone->s_wl.erase_span = zone->nand_chip->max_erase_times/12; //250
    }
    else if (zone->nand_chip->max_erase_times < max_ec * 2) //大于1500 erase_span必须大于500
    {
        zone->s_wl.erase_span = zone->nand_chip->max_erase_times/10; //300
    }
    else if(zone->nand_chip->max_erase_times < max_ec * 3)  //大于1000 erase_span必须大于1000
    {
        zone->s_wl.erase_span = zone->nand_chip->max_erase_times/7; //428
    }
    else
    {
        zone->s_wl.erase_span = zone->nand_chip->max_erase_times/5; //600
    }

    if((max_ec - min_ec) < zone->s_wl.erase_span)
    {
        return NFTL_FAILURE;   /* 差值不满足条件 */
    }

    NFTL_ERR("[ND]swl: static WL need to do(%d : %d)\n", max_ec, min_ec);
    NFTL_ERR("[ND]swl: min_ec_blk_no = %d,\n",mim_ec_blk->phy_block.Block_NO);
    NFTL_ERR("[ND]swl: zone->blocks = %d)\n", zone->blocks);

    //print_block_invalid_list(zone);
    //print_free_list(zone);

    zone->s_wl.block_for_s_wl = mim_ec_blk;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 done_static_wear_leveling(struct _nftl_zone* zone)
{
    uint32 ret;

    if(!zone->s_wl.block_for_s_wl)
    {
        return 2;
    }

    if(is_current_blcok(zone,zone->s_wl.block_for_s_wl)==1)
    {
        zone->s_wl.block_for_s_wl = NULL;
        return 2;
    }

    if(is_block_in_invalid_list(zone,zone->s_wl.block_for_s_wl) == NFTL_NO)
    {
        NFTL_ERR("[ND]swl: the block(%d) for static WL is not in invalid_list\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO);
        zone->s_wl.block_for_s_wl = NULL;
        return 2;
    }

    NFTL_ERR("[ND]swl: done_static_wear_leveling(min_ec) block:%d, erase_count:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO,zone->s_wl.block_for_s_wl->erase_count);

    zone->s_wl.s_wl_start = 1;

    zone->smart->total_wl_times++;

    fill_no_use_data_to_current_block(zone);

    ret = garbage_collect_first(zone, zone->s_wl.block_for_s_wl, 0xffff);
    if(ret != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]swl: garbage_collect_first fail:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO);
        zone->s_wl.block_for_s_wl = NULL;
        return 1;
    }

    zone->s_wl.s_wl_start = 0;

    if(is_block_in_free_list(zone,zone->s_wl.block_for_s_wl) == NFTL_NO)
    {
        NFTL_ERR("[NE]swl: last->check fail:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO);
        zone->s_wl.block_for_s_wl = NULL;
        return 0;
    }

    zone->s_wl.block_for_s_wl = NULL;

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 do_static_wear_leveling(struct _nftl_zone* zone)
{
    if(zone->cfg->nftl_support_wear_leveling == 0)
    {
        return 0;
    }

    if(GC_ON == zone->gc_strategy.process)
    {
        return NFTL_FAILURE;    /* gc is doing... */
    }

    if (NFTL_SUCCESS == check_static_wear_leveling(zone))
    {
        return done_static_wear_leveling(zone);
    }

    return 0;
}

#if 0
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 check_static_wear_leveling(struct _nftl_zone* zone)
{
    _phy_block_info* block_ptr;
    _phy_block_info* p;
    uint16 earse_max,earse_min;

    if(zone->s_wl.block_for_s_wl != NULL)
    {
        return NFTL_SUCCESS;
    }

    if(zone->s_wl.s_wl_status != WL_ON)
    {
        if((zone->current_block.block_info->erase_count != 0xffff)&&(zone->current_block.block_info->erase_count > zone->s_wl.erase_threshold))
        {
            zone->s_wl.s_wl_status = WL_ON;
        }
        else
        {
            return NFTL_FAILURE;
        }
    }

    if(zone->invalid_page_head.invalid_page_next == NULL)
    {
        return NFTL_FAILURE;
    }

    block_ptr = zone->invalid_page_head.invalid_page_next;
    //earse_max = zone->max_erase_num;
    earse_max = get_max_erase_time_from_free_list(zone,zone->nand_chip->max_erase_times);
    earse_min = block_ptr->erase_count;

    for(p=&zone->invalid_page_head; p->invalid_page_next; p=p->invalid_page_next)
    {
        if(p->invalid_page_next->erase_count < earse_max)
        {
            if(((earse_max - p->invalid_page_next->erase_count) > zone->s_wl.erase_span) && (zone->current_block.block_info->block_used_count-p->invalid_page_next->block_used_count) > (zone->blocks>>2))
            {
                //NFTL_ERR("[ND]a block to be static wear leveling :%d\n",p->invalid_page_next->phy_block.Block_NO);
                zone->s_wl.block_for_s_wl = p->invalid_page_next;
                break;
            }
        }
        else
        {
            continue;
        }
    }

    if(zone->s_wl.block_for_s_wl == NULL)
    {
        NFTL_ERR("[ND]do_static_wear_leveling end %d!!!\n",zone->s_wl.erase_threshold);
        zone->s_wl.s_wl_status = WL_STOP;
        zone->s_wl.erase_threshold += zone->s_wl.erase_span;
        return NFTL_FAILURE;
    }
    else
    {
        return NFTL_SUCCESS;
    }
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 done_static_wear_leveling(struct _nftl_zone* zone)
{
    uint32 ret,i;

    if(zone->s_wl.block_for_s_wl == NULL)
    {
        return NFTL_SUCCESS;
    }

    if(is_current_blcok(zone,zone->s_wl.block_for_s_wl)==1)
    {
        zone->s_wl.block_for_s_wl = NULL;
        return NFTL_SUCCESS;
    }

    if(is_block_in_invalid_list(zone,zone->s_wl.block_for_s_wl) == NFTL_NO)
    {
        NFTL_ERR("[ND]no need done_static_wear_leveling:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO);
        zone->s_wl.block_for_s_wl = NULL;
        return NFTL_SUCCESS;
    }

    NFTL_ERR("[ND]done_static_wear_leveling block:%d, erase_count:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO,zone->s_wl.block_for_s_wl->erase_count);

    zone->s_wl.s_wl_start = 1;

    for(i=0;i<zone->nand_chip->pages_per_blk;i++)
    {
        fill_no_use_data_to_page(zone);
    }
    zone->s_wl.s_wl_start = 0;

    ret = garbage_collect_first(zone,zone->s_wl.block_for_s_wl,0xffff);
    if(ret != NFTL_SUCCESS)
    {
        NFTL_ERR("[NE]done_static_wear_leveling garbage_collect_first fail:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO);
        zone->s_wl.block_for_s_wl = NULL;
        return NFTL_FAILURE;
    }

    if(is_block_in_free_list(zone,zone->s_wl.block_for_s_wl) == NFTL_NO)
    {
        NFTL_ERR("[NE]done_static_wear_leveling garbage_collect_first fail:%d\n",zone->s_wl.block_for_s_wl->phy_block.Block_NO);
        zone->s_wl.block_for_s_wl = NULL;
        return NFTL_SUCCESS;
    }

    zone->s_wl.block_for_s_wl = NULL;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 do_static_wear_leveling(struct _nftl_zone* zone)
{
    uint32 ret = NFTL_SUCCESS;

#if SUPPORT_WEAR_LEVELING

    if(zone->gc_strategy.process == GC_ON)
    {
        return NFTL_SUCCESS;
    }

    ret = check_static_wear_leveling(zone);
    if(ret == NFTL_SUCCESS)
    {
        ret = done_static_wear_leveling(zone);
    }
    else
    {
        ret = NFTL_SUCCESS;
    }

#endif

    return ret;
}
#endif

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 prio_gc_one(struct _nftl_zone* zone,uint16 block,uint32 flag)
{
    _phy_block_info* block_info;

    block_info = get_phy_block_addr(zone,block);

    if(flag == 1)
    {
        flag = GC_READ_RECLAIM;
    }
    else if(flag == 2)
    {
        flag = GC_WEAR_LEVELING;
    }
    else if(flag == 3)
    {
        flag = GC_CHANGE;
    }
    else
    {
        flag = GC_CHANGE;
    }

    add_prio_gc(zone,block_info,flag);

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :参数invalid_page_count 应该大于0
*Return       :
*Note         :
*****************************************************************************/
uint32 gc_one(struct _nftl_zone* zone, uint16 invalid_page_count)
{
    uint32 ret;
    _phy_block_info* phy_block;

    phy_block = zone->invalid_page_head.invalid_page_next;

    if(phy_block == NULL)
    {
        return NFTL_COMPLETE;
    }

    if(phy_block->invalid_page_count > invalid_page_count)
    {
        ret = garbage_collect_first(zone,NULL,0xffff);
        if(ret == NFTL_COMPLETE)
        {
            return NFTL_COMPLETE;
        }
        else if(ret == NFTL_SUCCESS)
        {
            NFTL_ERR("[NE]gc_one ok!\n");
            return NFTL_SUCCESS;
        }
        else
        {
            NFTL_ERR("[NE]gc_one error!\n");
            return NFTL_FAILURE;
        }
    }
    else
    {
        return NFTL_COMPLETE;
    }
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 gc_all(struct _nftl_zone* zone,uint16 invalid_page_count)
{
    int ret;

    while(1)
    {
        ret = gc_one(zone,invalid_page_count);
        if(ret == NFTL_COMPLETE)
        {
            break;
        }
    }

    NFTL_ERR("[NE]gc all end\n");

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 gc_win_block(struct _nftl_zone* zone,uint16 block)
{
    uint32 ret;
    int  win_page_nums = 0;
    int  total_win_page_nums;

    total_win_page_nums = block * zone->nand_chip->pages_per_blk;

    while( win_page_nums < total_win_page_nums)
    {
        if(zone->invalid_page_head.invalid_page_next == NULL)
        {
            return NFTL_SUCCESS;
        }

        win_page_nums += zone->invalid_page_head.invalid_page_next->invalid_page_count;
        win_page_nums -= 1;

        ret = gc_one(zone,1);
        if(ret != NFTL_SUCCESS)
        {
            NFTL_ERR("[NE] gc_win_block gc_one error \n");
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
uint32 get_total_invalid_page(struct _nftl_zone* zone)
{
    _phy_block_info* p;
    int  total_invalid_page = 0;

    for(p=&zone->invalid_page_head; p->invalid_page_next; p=p->invalid_page_next)
    {
        total_invalid_page += p->invalid_page_next->invalid_page_count;
    }

    return total_invalid_page;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
//uint32 check_block_phy_mapping(struct _nftl_zone* zone,_phy_block_info* block)
//{
//    uint32 i,logic_page,ret,read;
//    uchar spare_data[BYTES_OF_USER_PER_PAGE];
//    _mapping_page* logic_page_map;
//    _physic_op_par phy_op_par;
//    _phy_page_mapping* phy_page_map;
//    uint32 try_num = 0;
//
//    ret = NFTL_SUCCESS;
//    MEMSET(spare_data,0x5a,BYTES_OF_USER_PER_PAGE);
//
//    set_physic_op_par(&phy_op_par,block->phy_block.Block_NO,zone->nand_chip->pages_per_blk - 1,zone->nand_chip->bitmap_per_page,zone->logic_page_buf,spare_data);
//    zone->nftl_nand_read_page(zone,&phy_op_par);
//    if(is_phy_mapping_page(spare_data) != NFTL_YES)
//    {
//        NFTL_ERR("[NE]check_block_phy_mapping wrong!!\n");
//        NFTL_ERR("[NE]check_block_phy_mapping cannot find PHY_MAPPING_PAGE %d %d %d %d!!\n",block->phy_block.Block_NO,zone->nand_chip->pages_per_blk - 1,try_num,zone->nand_chip->bitmap_per_page);
//        NFTL_ERR("[NE]check_block_phy_mapping cannot find PHY_MAPPING_PAGE %x %x %x %x  %x!!\n",spare_data[1],spare_data[1],spare_data[2],spare_data[3],spare_data[4]);
//        NFTL_ERR("[NE]check_block_phy_mapping cannot find PHY_MAPPING_PAGE %x %x %x %x %x!!\n",spare_data[5],spare_data[6],spare_data[7],spare_data[8],spare_data[9]);
//        return NFTL_FAILURE;
//    }
//
//    phy_page_map = (_phy_page_mapping*)zone->logic_page_buf;
//
//    for(i=0; i<(zone->nand_chip->pages_per_blk-1); i++)
//    {
//        logic_page = phy_page_map->phy_page_map[i];
//        if(logic_page != 0xffffffff)
//        {
//            logic_page_map = get_logic_page_map(zone,logic_page);
//            if((logic_page_map->Block_NO == block->phy_block.Block_NO) && (logic_page_map->Page_NO == i))
//            {
//                //有效page 只需要读spare区即可
//                set_physic_op_par(&phy_op_par,block->phy_block.Block_NO,i,zone->nand_chip->bitmap_per_page,zone->temp_page_buf,spare_data);
//                zone->nftl_nand_read_page(zone,&phy_op_par);
//                read = get_logic_page_from_oob(spare_data);
//                if(read != logic_page)
//                {
//                    ret |= NFTL_FAILURE;
//                    NFTL_ERR("[NE]garbage_collect_first read a logic page error %d %d %d %d!!\n",block->phy_block.Block_NO,i,read,logic_page);
//                    NFTL_ERR("[NE]%x %x %x %x %x !!\n",spare_data[0],spare_data[1],spare_data[2],spare_data[3],spare_data[4]);
//                    NFTL_ERR("[NE]%x %x %x %x %x!!\n",spare_data[5],spare_data[6],spare_data[7],spare_data[8],spare_data[9]);
//                }
//            }
//        }
//    }
//    return ret;
//}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 is_current_blcok(struct _nftl_zone* zone,_phy_block_info* block)
{
    uint32 ret=0;
    if(zone->zone_attr&SUPPORT_COSS_TALK)
    {
        if((block == zone->assist_block.block_info)||(block == zone->current_block.block_info))
            ret = 1;
    }
    else
    {
        if(block == zone->current_block.block_info)
            ret = 1;
    }
    return ret;
}
