
/*
 * allwinner nftl list
 *
 * (C) 2008
 */

#define _NFTL_LIST_C_

#include "nftl_inc.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :首节点不存放有效数据，这样便于快速插入
*****************************************************************************/
//int add_free_list(struct _nftl_zone* zone, _phy_block_info* phy_block_ptr)
//{
//    _phy_block_info* p = &zone->free_head;
//
//    for(; p->free_next; p=p->free_next)
//    {
//        if(p->free_next->erase_count >= phy_block_ptr->erase_count)
//        {
//            phy_block_ptr->free_next = p->free_next;
//            p->free_next = phy_block_ptr;
//            return NFTL_SUCCESS;
//        }
//    }
//    p->free_next = phy_block_ptr;
//    phy_block_ptr->free_next = NULL;
//
//    return NFTL_SUCCESS;
//}

int add_free_list(struct _nftl_zone* zone, _phy_block_info* phy_block_ptr)
{
    _phy_block_info* p;

    for(p = &zone->free_head; p->free_next; p=p->free_next)
    {
        if(p->free_next->erase_count >= phy_block_ptr->erase_count)
        {
            p->free_next->free_prev = phy_block_ptr;
            phy_block_ptr->free_next = p->free_next;
            p->free_next = phy_block_ptr;
            phy_block_ptr->free_prev = p;
            return NFTL_SUCCESS;
        }
    }
    p->free_next = phy_block_ptr;
    phy_block_ptr->free_next = NULL;
    phy_block_ptr->free_prev = p;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
//_phy_block_info* del_free_list(struct _nftl_zone* zone)
//{
//    _phy_block_info* p = &zone->free_head;
//
//    p = p->free_next;
//    if(p != NULL)
//        zone->free_head.free_next = p->free_next;
//
//    return p;
//}

_phy_block_info* del_free_list(struct _nftl_zone* zone)
{
    _phy_block_info* p = &zone->free_head;

    p = p->free_next;
    if(p != NULL)
    {
        zone->free_head.free_next = p->free_next;

        zone->free_head.free_next = p->free_next;
        if(p->free_next != NULL)
            p->free_next->free_prev = &zone->free_head;
    }

    return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* del_free_list_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* p = &zone->free_head;

    for(p=p->free_next; p; p=p->free_next)
    {
        if(p == phy_block_ptr)
        {
            p->free_prev->free_next = p->free_next;
            if(p->free_next != NULL)
                p->free_next->free_prev = p->free_prev;
            p->free_prev = NULL;
            p->free_next = NULL;
            break;
        }
    }

    return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 is_block_in_free_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* p = &zone->free_head;

    for(p=p->free_next; p; p=p->free_next)
    {
        if(p == phy_block_ptr)
        {
            return NFTL_YES;
        }
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
void print_free_list(struct _nftl_zone* zone)
{
    _phy_block_info* p = &zone->free_head;

    NFTL_ERR("[NE]free list: \n");
    for(p=p->free_next; p; p=p->free_next)
    {
        NFTL_ERR("[NE]block NO:%4d; erase:%3d; ",p->phy_block.Block_NO,p->erase_count);
        NFTL_ERR("count:%4d; ",p->block_used_count);
        NFTL_ERR("info:0x%x\n",p->info);
    }
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_block_invalid_list(struct _nftl_zone* zone, _phy_block_info* phy_block_ptr)
{
    _phy_block_info* p;

    for(p=&zone->invalid_page_head; p->invalid_page_next; p=p->invalid_page_next)
    {
        if(p->invalid_page_next->invalid_page_count <= phy_block_ptr->invalid_page_count)
        {
            p->invalid_page_next->invalid_page_prev = phy_block_ptr;
            phy_block_ptr->invalid_page_next = p->invalid_page_next;
            p->invalid_page_next = phy_block_ptr;
            phy_block_ptr->invalid_page_prev = p;
            //print_block_invalid_list(list_head);
            return NFTL_SUCCESS;
        }
    }

    //print_block_invalid_list(list_head);
    p->invalid_page_next = phy_block_ptr;
    phy_block_ptr->invalid_page_next = NULL;
    phy_block_ptr->invalid_page_prev = p;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* del_block_invalid_list(struct _nftl_zone* zone)
{
    _phy_block_info* p = &zone->invalid_page_head;

    p = p->invalid_page_next;
    if(p != NULL)
    {
        zone->invalid_page_head.invalid_page_next = p->invalid_page_next;
        if(p->invalid_page_next != NULL)
            p->invalid_page_next->invalid_page_prev = &zone->invalid_page_head;
    }

    return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* del_block_invalid_list_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* p = &zone->invalid_page_head;

    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        if(p == phy_block_ptr)
        {
            p->invalid_page_prev->invalid_page_next = p->invalid_page_next;
            if(p->invalid_page_next != NULL)
                p->invalid_page_next->invalid_page_prev = p->invalid_page_prev;
            p->invalid_page_prev = NULL;
            p->invalid_page_next = NULL;
            break;
        }
    }

    return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 is_block_in_invalid_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* p = &zone->invalid_page_head;

    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        if(p == phy_block_ptr)
        {
            return NFTL_YES;
        }
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
void print_block_invalid_list(struct _nftl_zone* zone)
{
    _phy_block_info* p = &zone->invalid_page_head;

    NFTL_ERR("[NE]invalid_page_count list: \n");
    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        NFTL_ERR("[NE]block NO:%4d; erase:%3d; ",p->phy_block.Block_NO,p->erase_count);
        NFTL_ERR("count:%4d; invalid:%3d;",p->block_used_count,p->invalid_page_count);
        NFTL_ERR("info:0x%x\n",p->info);
    }
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_block_count_list(struct _nftl_zone* zone, _phy_block_info* phy_block_ptr)
{
    _phy_block_info* p;

    for(p = &zone->block_used_head; p->block_used_next; p=p->block_used_next)
    {
        if(p->block_used_next->block_used_count >= phy_block_ptr->block_used_count)
        {
            p->block_used_next->block_used_prev = phy_block_ptr;
            phy_block_ptr->block_used_next = p->block_used_next;
            p->block_used_next = phy_block_ptr;
            phy_block_ptr->block_used_prev = p;
            return NFTL_SUCCESS;
        }
    }
    p->block_used_next = phy_block_ptr;
    phy_block_ptr->block_used_next = NULL;
    phy_block_ptr->block_used_prev = p;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* del_block_count_list(struct _nftl_zone* zone)
{
    _phy_block_info* p = &zone->block_used_head;

    if(p->block_used_next != NULL)
        p->block_used_next = p->block_used_next->block_used_next;

    return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* del_block_count_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* p = &zone->block_used_head;

    for(p=p->block_used_next; p; p=p->block_used_next)
    {
        if(p == phy_block_ptr)
        {
            p->block_used_prev->block_used_next = p->block_used_next;
            if(p->block_used_next != NULL)
                p->block_used_next->block_used_prev = p->block_used_prev;
            p->block_used_prev = NULL;
            p->block_used_next = NULL;
            break;
        }
    }

    return p;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void print_block_count_list(struct _nftl_zone* zone)
{
    _phy_block_info* p = &zone->block_used_head;

    NFTL_ERR("[NE]block_used_count list: \n");
    for(p=p->block_used_next; p; p=p->block_used_next)
    {
        NFTL_ERR("[NE]block NO: %d ",p->phy_block.Block_NO);
        NFTL_ERR("[NE]block_used_count: %d \n",p->block_used_count);
    }
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int put_phy_block_to_free_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    int ret;

    phy_block_ptr->free_next = NULL;
    phy_block_ptr->free_prev = NULL;
    ret = add_free_list(zone,phy_block_ptr);

    if(ret == 0)
        zone->free_block_num ++;
    else
        NFTL_ERR("[NE]add free_list error !\n");

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* out_phy_block_from_free_list(struct _nftl_zone* zone)
{
    _phy_block_info* block;

    block = del_free_list(zone);
    if(block != NULL)
    {
        block->free_next = NULL;
        block->free_prev = NULL;
        if(zone->free_block_num > 0)
            zone->free_block_num --;
    }

    return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* out_phy_block_from_free_list_by_erase_time(struct _nftl_zone* zone,uint16 erase_time)
{
    _phy_block_info* block = NULL;
    uint16 erase_max = 0;

    _phy_block_info* p = &zone->free_head;

    for(p=p->free_next; p; p=p->free_next)
    {
        if(p->erase_count < erase_time)
        {
            if(p->erase_count >= erase_max)
            {
                erase_max = p->erase_count;
                block = p;
            }
        }
    }

    if(block != NULL)
    {
        block = out_phy_block_from_free_list_by_block(zone,block);
    }

    return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :在参数erase_time之下的最大擦除次数
*****************************************************************************/
uint16 get_max_erase_time_from_free_list(struct _nftl_zone* zone,uint16 erase_time)
{
    _phy_block_info* block = NULL;
    uint16 erase_max = 0;

    _phy_block_info* p = &zone->free_head;

    for(p=p->free_next; p; p=p->free_next)
    {
        if(p->erase_count < erase_time)
        {
            if(p->erase_count >= erase_max)
            {
                erase_max = p->erase_count;
                block = p;
            }
        }
    }

    if(block != NULL)
    {
        return block->erase_count;
    }

    return 0xffff;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :在参数erase_time之下的最大擦除次数
*****************************************************************************/
_phy_block_info* get_block_max_erase_time_from_free_list(struct _nftl_zone* zone,uint16 erase_time)
{
    _phy_block_info* block = NULL;
    uint16 erase_max = 0;

    _phy_block_info* p = &zone->free_head;

    for(p=p->free_next; p; p=p->free_next)
    {
        if(p->erase_count < erase_time)
        {
            if(p->erase_count >= erase_max)
            {
                erase_max = p->erase_count;
                block = p;
            }
        }
    }

    if(block != NULL)
    {
        return block;
    }

    return NULL;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :在参数erase_time之下的最大擦除次数
*****************************************************************************/
_phy_block_info* get_block_max_erase_time_from_invalid_list(struct _nftl_zone* zone,uint16 erase_time)
{
    _phy_block_info* block = NULL;
    uint16 erase_max = 0;

    _phy_block_info* p = &zone->invalid_page_head;

    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        if(p->erase_count < erase_time)
        {
            if(p->erase_count >= erase_max)
            {
                erase_max = p->erase_count;
                block = p;
            }
        }
    }

    if(block != NULL)
    {
        return block;
    }

    return NULL;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :在参数erase_time之下,max_count之下,的最小擦除次数
*****************************************************************************/
_phy_block_info* get_block_min_erase_time_from_invalid_list(struct _nftl_zone* zone,uint16 erase_time,uint32 max_count)
{
    _phy_block_info* block = NULL;
    uint16 erase_min = zone->nand_chip->max_erase_times;

    _phy_block_info* p = &zone->invalid_page_head;

    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        if((p->erase_count < erase_time)&&(p->block_used_count < max_count))
        {
            if(p->erase_count <= erase_min)
            {
                erase_min = p->erase_count;
                block = p;
            }
        }
    }

    if(block != NULL)
    {
        return block;
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
_phy_block_info* get_block_max_count_from_invalid_list(struct _nftl_zone* zone)
{
    _phy_block_info* block = NULL;
    uint32 count_max = 0;

    _phy_block_info* p = &zone->invalid_page_head;

    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        if(p->block_used_count >= count_max)
        {
            count_max = p->block_used_count;
            block = p;
        }
    }

    if(block != NULL)
    {
        return block;
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
_phy_block_info* get_block_min_count_from_invalid_list(struct _nftl_zone* zone)
{
    _phy_block_info* block = NULL;
    uint32 count_min = 0xffffffff;

    _phy_block_info* p = &zone->invalid_page_head;

    for(p=p->invalid_page_next; p; p=p->invalid_page_next)
    {
        if(p->block_used_count <= count_min)
        {
            count_min = p->block_used_count;
            block = p;
        }
    }

    if(block != NULL)
    {
        return block;
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
_phy_block_info* out_phy_block_from_free_list_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* block;

    block = del_free_list_by_block(zone,phy_block_ptr);
    if(block != NULL)
    {
        block->free_next = NULL;
        block->free_prev = NULL;
        if(zone->free_block_num > 0)
            zone->free_block_num --;
        return block;
    }

    return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int put_phy_block_to_invalid_page_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    int ret;

    phy_block_ptr->invalid_page_next = NULL;
    phy_block_ptr->invalid_page_prev = NULL;

    ret = add_block_invalid_list(zone,phy_block_ptr);
    if(ret != 0)
        NFTL_ERR("[NE]add invalid_list error !\n");

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* out_phy_block_from_invalid_page_list(struct _nftl_zone* zone)
{
    _phy_block_info* block;

    block = del_block_invalid_list(zone );
    if(block != NULL)
    {
        block->invalid_page_next = NULL;
        block->invalid_page_prev = NULL;
        return block;
    }

    return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_phy_block_info* out_phy_block_from_invalid_page_list_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr)
{
    _phy_block_info* block;

    block = del_block_invalid_list_by_block(zone,phy_block_ptr);
    if(block != NULL)
    {
        block->invalid_page_next = NULL;
        block->invalid_page_prev = NULL;
        return block;
    }

    return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int phy_block_from_invalid_page_incr(struct _nftl_zone* zone,_phy_block_info* block)
{
    _phy_block_info* ptr1;
    _phy_block_info* ptr2;

    if((block->invalid_page_next == NULL)&&(block->invalid_page_prev == NULL))
    {
        block->invalid_page_count++;
        if(block->invalid_page_count > zone->nand_chip->pages_per_blk)
        {
            NFTL_ERR("[NE]invalid_page_count more than pages per blk 0 %d %d!!\n",block->phy_block.Block_NO,block->invalid_page_count);
        }
        return NFTL_SUCCESS;
    }

#if 0

    ptr = del_block_invalid_list_by_block(zone,block);
    ptr->invalid_page_count++;
    add_block_invalid_list(zone,ptr);

#else

    block->invalid_page_count++;

//  if(block == zone->zone_current_used_block)
//  {
//      NFTL_ERR("[NE]here!!\n");
//  }

    if(block->invalid_page_count > zone->nand_chip->pages_per_blk)
    {
        NFTL_ERR("[NE]invalid_page_count more than pages per blk 1!!\n");
    }

    ptr1 = block->invalid_page_prev;
    ptr2 = block;

    if(ptr1 == &zone->invalid_page_head)
    {
        return NFTL_SUCCESS;
    }

    while(ptr1->invalid_page_count < ptr2->invalid_page_count)
    {
        ptr2->invalid_page_prev = ptr1->invalid_page_prev;
        ptr1->invalid_page_prev->invalid_page_next = ptr2;
        ptr1->invalid_page_next = ptr2->invalid_page_next;
        if(ptr2->invalid_page_next != NULL)
            ptr2->invalid_page_next->invalid_page_prev = ptr1;
        ptr2->invalid_page_next = ptr1;
        ptr1->invalid_page_prev = ptr2;
        ptr1 = ptr2->invalid_page_prev;
        if(ptr1->invalid_page_prev == NULL)
        {
            break;
        }
    }

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
int adjust_invaild_list(struct _nftl_zone* zone)
{
    _phy_block_info* phy_block;

    while(zone->invalid_page_head.invalid_page_next!=NULL)
    {
        phy_block = zone->invalid_page_head.invalid_page_next;
        if(phy_block->invalid_page_count == zone->nand_chip->pages_per_blk)
        {
            phy_block = out_phy_block_from_invalid_page_list(zone);
            put_phy_block_to_free_list(zone,phy_block);
        }
        else
        {
            break;
        }
    }

    return NFTL_SUCCESS;
}
