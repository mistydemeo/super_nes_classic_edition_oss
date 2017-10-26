
/*
 * allwinner nftl cache
 *
 * (C) 2008
 */

#define _NFTL_CACHE_C_

#include "nftl_inc.h"
#include "../nftl_interface/nftl_cfg.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_cache_init(struct _nftl_zone* zone)
{
    uint32 i;
    _cache* cache;

    cache = &zone->cache;

    cache->cache_totals = zone->cfg->nftl_max_cache_num;
    cache->cache_read_nums = 0;
    cache->cache_write_nums = 0;

    cache->cache_read_head.cache_read_next = NULL;
    cache->cache_read_head.cache_read_prev = NULL;
    cache->cache_read_head.cache_write_next = NULL;
    cache->cache_read_head.cache_write_prev = NULL;

    cache->cache_write_head.cache_read_next = NULL;
    cache->cache_write_head.cache_read_prev = NULL;
    cache->cache_write_head.cache_write_next = NULL;
    cache->cache_write_head.cache_write_prev = NULL;

    for(i=0; i<cache->cache_totals; i++)
    {
        cache->cache_node[i].cache_no = i;
        cache->cache_node[i].cache_info = CACHE_EMPTY;
        cache->cache_node[i].page_no = 0xffffffff;
        cache->cache_node[i].cache_read_next = NULL;
        cache->cache_node[i].cache_read_prev = NULL;
        cache->cache_node[i].cache_write_next = NULL;
        cache->cache_node[i].cache_write_prev = NULL;
        cache->cache_node[i].start_sector = 0;
        cache->cache_node[i].sector_len = 0;

        cache->cache_node[i].buf = nftl_malloc(zone->nand_chip->bytes_per_page);
        if(cache->cache_node[i].buf == NULL)
        {
            NFTL_ERR("[NE]====no memory!!!!!=====\n");
            return NFTL_FAILURE;
        }
    }
    cache->cache_page_buf = nftl_malloc(zone->nand_chip->bytes_per_page);
    if(cache->cache_page_buf == NULL)
    {
        NFTL_ERR("[NE]====no memory!!!!!=====\n");
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
int nftl_cache_exit(struct _nftl_zone* zone)
{
    int i;
    _cache* cache;

    cache = &zone->cache;
    for(i=0; i<cache->cache_totals; i++)
    {
        nftl_free(cache->cache_node[i].buf);
    }
    nftl_free(cache->cache_page_buf);
    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_cache_clean(struct _nftl_zone* zone)
{
    uint32 i;
    _cache* cache;

    cache = &zone->cache;

    cache->cache_totals = zone->cfg->nftl_max_cache_num;
    cache->cache_read_nums = 0;
    cache->cache_write_nums = 0;

    cache->cache_read_head.cache_read_next = NULL;
    cache->cache_read_head.cache_read_prev = NULL;
    cache->cache_read_head.cache_write_next = NULL;
    cache->cache_read_head.cache_write_prev = NULL;

    cache->cache_write_head.cache_read_next = NULL;
    cache->cache_write_head.cache_read_prev = NULL;
    cache->cache_write_head.cache_write_next = NULL;
    cache->cache_write_head.cache_write_prev = NULL;

    for(i=0; i<cache->cache_totals; i++)
    {
        cache->cache_node[i].cache_no = i;
        cache->cache_node[i].cache_info = CACHE_EMPTY;
        cache->cache_node[i].page_no = 0xffffffff;
        cache->cache_node[i].cache_read_next = NULL;
        cache->cache_node[i].cache_read_prev = NULL;
        cache->cache_node[i].cache_write_next = NULL;
        cache->cache_node[i].cache_write_prev = NULL;
        cache->cache_node[i].start_sector = 0;
        cache->cache_node[i].sector_len = 0;
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
_cache_node *search_from_cache_read_list(_cache* cache,uint32 page)
{
    _cache_node* p = &cache->cache_read_head;

    for(p=p->cache_read_next; p; p=p->cache_read_next)
    {
        if(p->page_no == page)
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
_cache_node *search_from_cache_write_list(_cache* cache,uint32 page)
{
    _cache_node * p = &cache->cache_write_head;

    for(p=p->cache_write_next; p; p=p->cache_write_next)
    {
        if(p->page_no == page)
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
_cache_node * get_empty_cahce_node(_cache *cache)
{
    uint16 i;
    for(i=0; i<cache->cache_totals; i++)
    {
        if(cache->cache_node[i].cache_info == CACHE_EMPTY)
            return &cache->cache_node[i];
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
int add_to_cache_read_list_tail(_cache *cache,_cache_node* cache_node)
{
    _cache_node * p = &cache->cache_read_head;

    while(p->cache_read_next != NULL)
    {
        p = p->cache_read_next;
    }

    p->cache_read_next = cache_node;
    cache_node->cache_read_next = NULL;
    cache_node->cache_read_prev = p;
    cache_node->cache_info = CACHE_READ;
    cache->cache_read_nums += 1;
    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_to_cache_write_list_tail(_cache *cache,_cache_node* cache_node)
{
    _cache_node * p = &cache->cache_write_head;

    while(p->cache_write_next != NULL)
    {
        p = p->cache_write_next;
    }

    p->cache_write_next = cache_node;
    cache_node->cache_write_next = NULL;
    cache_node->cache_write_prev = p;
    cache_node->cache_info = CACHE_WRITE;
    cache->cache_write_nums += 1;

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_cache_node* del_from_cache_read_list(_cache *cache,_cache_node* cache_node)
{
    _cache_node * p = cache_node->cache_read_prev;

    p->cache_read_next = cache_node->cache_read_next;
    if(cache_node->cache_read_next != NULL)
        cache_node->cache_read_next->cache_read_prev = p;

    cache_node->cache_read_next = NULL;
    cache_node->cache_read_prev = NULL;
    cache_node->cache_info = CACHE_EMPTY;
    cache->cache_read_nums -= 1;
    return cache_node;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_cache_node* del_from_cache_write_list(_cache *cache,_cache_node* cache_node)
{
    _cache_node * p = cache_node->cache_write_prev;

    p->cache_write_next = cache_node->cache_write_next;
    if(cache_node->cache_write_next != NULL)
        cache_node->cache_write_next->cache_write_prev = p;

    cache_node->cache_write_next = NULL;
    cache_node->cache_write_prev = NULL;
    cache_node->cache_info = CACHE_EMPTY;
    cache->cache_write_nums -= 1;
    return cache_node;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 __nand_read(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf)
{
    uint32 ret;
    if(zone->test == 3)
    {
        NFTL_ERR("[NE]read sector:0x%x,len:0x%x\n",start_sector,len);
    }

    zone->smart->total_recv_read_sectors += len;

    ret = nand_op(0,zone,start_sector,len,buf);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 __nand_write(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf)
{
    uint32 ret;

    if(zone->test == 3)
    {
        NFTL_ERR("[NE]write sector:0x%x,len:0x%x\n",start_sector,len);
    }

    zone->smart->total_recv_write_sectors += len;

    ret = nand_op(1,zone,start_sector,len,buf);
    return  ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 nand_op(uchar flag,struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf)
{
    uint16  temp1,temp2,len_save,bitmap;
    uchar  *buf_save;
    uint32 page_save,page_no,ret;
    uint32 (*nand_op2)(struct _nftl_zone* zone,uint32 page_no,uint16 bitmap,uchar *buf);

    ret = 0;

    if(len == 0)
        return NFTL_SUCCESS;

    if((start_sector+len) > zone->logic_cap_in_sects)
    {
        NFTL_ERR("[NE]paramter error!\n");
        return NFTL_FAILURE;
    }

    if(flag == 0)
    {
//      NFTL_ERR("[NE]read start_sector :%d; len: %d; buf %x\n",start_sector,len,buf);
        nand_op2 = nand_cache_read;
    }
    else
    {
//      NFTL_ERR("[NE]write start_sector :%d; len: %d; buf %x\n",start_sector,len,buf);
        nand_op2 = nand_cache_write;
    }
//////////////////////////////////////////////////////////////
    page_no = start_sector / zone->nand_chip->sector_per_page;
    page_save = page_no;

    temp1 = (uint16)(start_sector % zone->nand_chip->sector_per_page);    //start sector
    temp2 = zone->nand_chip->sector_per_page - temp1;                    //sector len
    if(len <= temp2)
    {
        temp2 = len;
        len = 0;
    }
    else
    {
        len -= temp2;
    }

    buf_save = buf;
    len_save = temp2;
    bitmap = (temp1 << 8) | temp2;
//    NFTL_ERR("[NE]page :%d; bitmap: %x; buf %x\n",page_save,bitmap,buf_save);
    ret = nand_op2(zone,page_save,bitmap,buf_save);
    if(len == 0)
    {
        return ret;
    }

//////////////////////////////////////////////////////////////
    while(len > zone->nand_chip->sector_per_page)
    {
        temp2 = zone->nand_chip->sector_per_page;
        page_save += 1;
        buf_save += len_save << SHIFT_PER_SECTOR;
        bitmap = temp2;
        len -= zone->nand_chip->sector_per_page;
        len_save = zone->nand_chip->sector_per_page;
//        NFTL_ERR("[NE]page :%d; bitmap: %x; buf %x\n",page_save,bitmap,buf_save);
        ret |= nand_op2(zone,page_save,bitmap,buf_save);
    }

//////////////////////////////////////////////////////////////
    temp2 = (uint16)len;
    page_save += 1;
    buf_save += len_save << SHIFT_PER_SECTOR;
    bitmap = temp2;
//    NFTL_ERR("[NE]page :%d; bitmap: %x; buf %x\n",page_save,bitmap,buf_save);
    ret |= nand_op2(zone,page_save,bitmap,buf_save);
//////////////////////////////////////////////////////////////
    //NFTL_ERR("[NE]op end!\n");
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 nand_cache_read(struct _nftl_zone* zone,uint32 page_no,uint16 bitmap,uchar *buf)
{
    uint32 ret;
    uint16 byte_start;
    uint16 byte_len;
    uint32 start,len;
    uchar* t_buf;
    _cache_node * node;
    _cache *cache;

    if(buf == NULL)
    {
        NFTL_ERR("[NE]nand_cache_read error  NULL!\n");
        return NFTL_SUCCESS;
    }

    ret = 0;
    start = bitmap >> 8;
    len = bitmap & 0x00ff;
    start <<= SHIFT_PER_SECTOR;
    len <<= SHIFT_PER_SECTOR;

    cache = &zone->cache;

    if(zone->cfg->nftl_dont_use_cache)
    {
        t_buf = cache->cache_node[0].buf;
        ret = zone->nftl_nand_read_logic_page(zone,page_no,t_buf);
        MEMCPY(buf,t_buf+start,len);
        if(ret == ECC_LIMIT)
        {
            //NFTL_ERR("[NE]ECC_LIMIT happened! page:%d!\n",page_no);
            zone->smart->total_recv_read_claim_pages++;
            zone->nftl_nand_write_logic_page(zone,page_no,t_buf);
            ret = NFTL_SUCCESS;
        }
        return ret;
    }

    node = search_from_cache_write_list(cache,page_no);
    if(node != NULL)
    {
        t_buf = node->buf;
        byte_start = node->start_sector << SHIFT_PER_SECTOR;
        byte_len = node->sector_len << SHIFT_PER_SECTOR;

        if(start <= (byte_start + byte_len))
        {
            if((start+len) <= (byte_start + byte_len))
            {
                MEMCPY(buf,t_buf+start,len);
                return NFTL_SUCCESS;
            }
        }

        ret = zone->nftl_nand_read_logic_page(zone,page_no,cache->cache_page_buf);
        MEMCPY(t_buf+byte_len,cache->cache_page_buf+byte_len,zone->nand_chip->bytes_per_page-byte_len);
        node->sector_len = zone->nand_chip->sector_per_page;
        MEMCPY(buf,t_buf+start,len);
        return NFTL_SUCCESS;
    }

    node = search_from_cache_read_list(cache,page_no);
    if(node != NULL)
    {
        del_from_cache_read_list(cache,node);
        t_buf = node->buf;
        MEMCPY(buf,t_buf+start,len);
        add_to_cache_read_list_tail(cache,node);
        return NFTL_SUCCESS;
    }

    node = get_empty_cahce_node(cache);
    if(node == NULL)
    {
        node = del_from_cache_read_list(cache,cache->cache_read_head.cache_read_next);
        if(node == NULL)
        {
            NFTL_ERR("[NE]error1 node %d,%d!\n",cache->cache_write_nums,cache->cache_read_nums);
            return NFTL_FAILURE;
        }
    }

    node->page_no = page_no;
    t_buf = node->buf;
    ret = zone->nftl_nand_read_logic_page(zone,page_no,t_buf);
    add_to_cache_read_list_tail(cache,node);
    MEMCPY(buf,t_buf+start,len);

    if(ret == ECC_LIMIT)
    {
        //NFTL_ERR("[NE]ECC_LIMIT happened! page:%d!\n",page_no);
        zone->smart->total_recv_read_claim_pages++;
        zone->nftl_nand_write_logic_page(zone,page_no,t_buf);
        ret = NFTL_SUCCESS;
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
uint32 nand_cache_write(struct _nftl_zone* zone,uint32 page_no,uint16 bitmap,uchar *buf)
{
    uint32 ret;
    uint16 byte_len;
    uint16 start,len;
    uchar* t_buf;
    _cache_node * node;
    _cache *cache;

    if(buf == NULL)
    {
        NFTL_ERR("[NE]nand_cache_write error  NULL!\n");
        return NFTL_SUCCESS;
    }

    start = bitmap >> 8;
    len = bitmap & 0x00ff;
    start <<= SHIFT_PER_SECTOR;
    len <<= SHIFT_PER_SECTOR;

    ret = 0;
    cache = &zone->cache;

    if(zone->cfg->nftl_dont_use_cache)
    {
        t_buf = cache->cache_node[0].buf;
        zone->nftl_nand_read_logic_page(zone,page_no,t_buf);
        MEMCPY(t_buf+start,buf,len);
        ret = zone->nftl_nand_write_logic_page(zone,page_no,t_buf);
        return ret;
    }

// case 1
    if((!zone->cfg->nftl_use_cache_sort)&&(bitmap == zone->nand_chip->sector_per_page)&&(!zone->cfg->nftl_cross_talk))
    {
        node = search_from_cache_write_list(cache,page_no);
        if(node != NULL)
        {
            del_from_cache_write_list(cache,node);
        }

        node = search_from_cache_read_list(cache,page_no);
        if(node != NULL)
        {
            del_from_cache_read_list(cache,node);
        }

        ret = zone->nftl_nand_write_logic_page(zone,page_no,buf);
        if(ret != 0)
        {
            NFTL_ERR("[NE]error1 bitmap %x!\n",bitmap);
            return NFTL_FAILURE;
        }
        else
        {
            return NFTL_SUCCESS;
        }
    }

// case 2
    node = search_from_cache_write_list(cache,page_no);
    if(node != NULL)
    {
        del_from_cache_write_list(cache,node);
        t_buf = node->buf;

        byte_len = node->sector_len << SHIFT_PER_SECTOR;
        if(start == byte_len)
        {
            node->sector_len += len >> SHIFT_PER_SECTOR;
        }
        else
        {
            ret = zone->nftl_nand_read_logic_page(zone,page_no,cache->cache_page_buf);
            MEMCPY(t_buf+byte_len,cache->cache_page_buf+byte_len,zone->nand_chip->bytes_per_page-byte_len);
            node->sector_len = zone->nand_chip->sector_per_page;
        }
        MEMCPY(t_buf+start,buf,len);
        add_to_cache_write_list_tail(cache,node);

        node = search_from_cache_read_list(cache,page_no);
        if(node != NULL)
        {
            del_from_cache_read_list(cache,node);
        }
        //nand_flush_write_cache(zone);
        return NFTL_SUCCESS;
    }

// case 3
    node = search_from_cache_read_list(cache,page_no);
    if(node != NULL)
    {
        t_buf = node->buf;
        MEMCPY(t_buf+start,buf,len);
        if(start == 0)
        {
            node->start_sector = 0;
            node->sector_len = len>>SHIFT_PER_SECTOR;
        }
        else
        {
            node->sector_len = (start+len)>>SHIFT_PER_SECTOR;
        }
        del_from_cache_read_list(cache,node);
        add_to_cache_write_list_tail(cache,node);

        if(cache->cache_write_nums >= zone->cfg->nftl_max_cache_write_num)
        {
            if(flush_write_cache_to_nand(zone) != 0)
            {
                NFTL_ERR("[NE]error1 flush_write_cache_to_nand\n");
                return NFTL_FAILURE;
            }
        }
        return NFTL_SUCCESS;
    }

// case 4
    while(cache->cache_write_nums >= zone->cfg->nftl_max_cache_write_num)
    {
        if(flush_write_cache_to_nand(zone) != 0)
        {
            NFTL_ERR("[NE]error2 flush_write_cache_to_nand\n");
            return NFTL_FAILURE;
        }
    }

    node = get_empty_cahce_node(cache);
    if(node == NULL)
    {
        node = del_from_cache_read_list(cache,cache->cache_read_head.cache_read_next);
    }
    node->page_no = page_no;
    t_buf = node->buf;
    if(start == 0)
    {
        node->start_sector = 0;
        node->sector_len = len>>SHIFT_PER_SECTOR;
    }
    else
    {
        ret = zone->nftl_nand_read_logic_page(zone,page_no,t_buf);
//        if(ret == NAND_ERR_ECC)
//        {
//            NFTL_ERR("[NE]error3 bitmap %x!\n",bitmap);
//            return NFTL_FAILURE;
//        }
        node->start_sector = 0;
        node->sector_len = zone->nand_chip->sector_per_page;
    }

    MEMCPY(t_buf+start,buf,len);
    add_to_cache_write_list_tail(cache,node);
    //nand_flush_write_cache(zone);

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 flush_write_cache_to_nand(struct _nftl_zone* zone)
{
    uint32 ret;
    uint16 byte_len;
    uchar* t_buf;
    _cache_node * node;
    _cache *cache;

    ret = 0;
    cache = &zone->cache;
    node = del_from_cache_write_list(cache,cache->cache_write_head.cache_write_next);
    if(node == NULL)
    {
        return NFTL_SUCCESS;
    }

    t_buf = node->buf;
    if(node->sector_len != zone->nand_chip->sector_per_page)
    {
        ret = zone->nftl_nand_read_logic_page(zone,node->page_no,cache->cache_page_buf);
        byte_len = node->sector_len << SHIFT_PER_SECTOR;
        MEMCPY(t_buf+byte_len,cache->cache_page_buf+byte_len,zone->nand_chip->bytes_per_page-byte_len);
    }
    node->start_sector = 0;
    node->sector_len = 0;
    ret = zone->nftl_nand_write_logic_page(zone,node->page_no,t_buf);
    if(ret != 0)
    {
        NFTL_ERR("[NE]flush_write_cache_to_nand error!\n");
        //return NFTL_FAILURE;
    }

    node->sector_len = zone->nand_chip->sector_per_page;
    add_to_cache_read_list_tail(cache,node);

    return NFTL_SUCCESS;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 __nand_flush_write_cache(struct _nftl_zone* zone,uint32 num)
{
    uint32 ret,write_nums = 0;
    _cache *cache;

    cache = &zone->cache;

    ret = 0;

    if(num > cache->cache_totals)
    {
        //NFTL_ERR("[ND]flush cache %d %d!\n",cache->cache_write_nums,zone->zone_no);
        write_nums = cache->cache_write_nums;
    }

    while(cache->cache_write_head.cache_write_next != NULL)
    {
        ret |= flush_write_cache_to_nand(zone);
        num--;
        if(num == 0)
        {
            break;
        }
    }

    if(ret != 0)
    {
        NFTL_ERR("[NE]nand_flush_write_cache write error\n");
    }

    if(write_nums != 0)
    {
        NFTL_DBG("[ND]flush cache %d %d!\n",write_nums,zone->zone_no);
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
uint32 __shutdown_op(struct _nftl_zone* zone)
{
    uint32 ret = 0;

//    prio_gc_all(zone);
//
//    if((zone->cfg->nftl_support_fill_block == 0) || (zone->current_block_used_page == 0))
//    {
//        goto  retl;
//    }
//
//    if(zone->current_block_used_page == (zone->nand_chip->pages_per_blk -1))
//    {
//        NFTL_ERR("[ND]shutdown0 zone no:%d block:%d,page: %d!\n",zone->zone_no,zone->zone_current_used_block->phy_block.Block_NO,zone->current_block_used_page);
//        if(write_phy_page_map_to_current_block(zone) != NFTL_SUCCESS)
//        {
//            ret = 1;
//            goto  retl_fail;
//        }
//        if(get_new_current_block(zone) != NFTL_SUCCESS)
//        {
//            ret = 2;
//            goto  retl_fail;
//        }
//        NFTL_ERR("[ND]shutdown end!\n");
//        goto  retl;
//    }
//
//    NFTL_ERR("[ND]shutdown1 zone no:%d block:%d,page: %d!\n",zone->zone_no,zone->zone_current_used_block->phy_block.Block_NO,zone->current_block_used_page);
//    while(zone->current_block_used_page != (zone->nand_chip->pages_per_blk -1))
//    {
////        nand_write_logic_page_no_gc(zone,page_no,node->buf);
//        write_phy_page_map_to_current_block(zone);
//    }
//
//    if(zone->current_block_used_page == (zone->nand_chip->pages_per_blk -1))
//    {
//        NFTL_ERR("[ND]shutdown1 zone no:%d block:%d,page: %d!\n",zone->zone_no,zone->zone_current_used_block->phy_block.Block_NO,zone->current_block_used_page);
//        if(write_phy_page_map_to_current_block(zone) != NFTL_SUCCESS)
//        {
//            ret = 3;
//            goto  retl_fail;
//        }
//        if(get_new_current_block(zone) != NFTL_SUCCESS)
//        {
//            ret = 4;
//            goto  retl_fail;
//        }
//        NFTL_ERR("[ND]shutdown end!\n");
//        goto  retl;
//    }
//
//retl_fail:
//    NFTL_ERR("[NE]shutdown error!\n");
//retl:
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 print_logic_page_map(struct _nftl_zone *zone,uint32 start_page)
{
    uint32 total_pages,i;
    _mapping_page* page_map;

    total_pages = zone->logic_cap_in_sects / zone->nand_chip->sector_per_page;
    for(i=0; i<total_pages; i++)
    {
        page_map = get_logic_page_map(zone,i);
        if(page_map->Block_NO != 0xffff)
        {
            NFTL_ERR("%4d:<%4d %3d><%d>\n",i,page_map->Block_NO,page_map->Page_NO,page_map->Read_flag);
        }
    }
    return 0;
}
