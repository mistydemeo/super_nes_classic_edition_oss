
/*
 * allwinner nftl cache
 *
 * (C) 2008
 */

#define _NFTL_SECTOR_CACHE_C_

#include "nftl_inc.h"
#include "../nftl_interface/nftl_cfg.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_sector_cache_init(struct _nftl_zone* zone)
{
    uint32 i;
    uchar *ptr;
    _sector_cache* sector_cache;

    sector_cache = &zone->sector_cache;

    sector_cache->cache_totals = MAX_SECTOR_CACHE_NUM;
    sector_cache->cache_write_nums = 0;
    sector_cache->cache_buf = NULL;
    sector_cache->cache_buf = nftl_malloc(512 * MAX_SECTOR_CACHE_NUM);
    if(sector_cache->cache_buf == NULL)
    {
        NFTL_ERR("[NE]====no memory!!!!!=====\n");
        return NFTL_FAILURE;
    }

    sector_cache->cache_write_head.cache_write_next = NULL;
    sector_cache->cache_write_head.cache_write_prev = NULL;

    ptr = sector_cache->cache_buf;
    for(i=0; i<MAX_SECTOR_CACHE_NUM; i++)
    {
        sector_cache->cache_node[i].cache_no = i;
        sector_cache->cache_node[i].cache_info = CACHE_EMPTY;
        sector_cache->cache_node[i].sector_no = 0xffffffff;
        sector_cache->cache_node[i].cache_write_next = NULL;
        sector_cache->cache_node[i].cache_write_prev = NULL;
        sector_cache->cache_node[i].buf = ptr;
        ptr += 512;
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
int nftl_sector_cache_exit(struct _nftl_zone* zone)
{
    _sector_cache* cache;

    cache = &zone->sector_cache;
    nftl_free(cache->cache_buf);
    return NFTL_SUCCESS;

}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
_sector_cache_node *search_from_sector_cache_write_list(_sector_cache* cache,uint32 sector)
{
    _sector_cache_node * p = &cache->cache_write_head;

    for(p=p->cache_write_next; p; p=p->cache_write_next)
    {
        if(p->sector_no == sector)
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
_sector_cache_node * get_empty_sector_cahce_node(_sector_cache *cache)
{
    uint16 i;
    for(i=0; i<MAX_SECTOR_CACHE_NUM; i++)
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
int add_to_sector_cache_write_list_tail(_sector_cache *cache,_sector_cache_node* cache_node)
{
    _sector_cache_node * p = &cache->cache_write_head;

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
_sector_cache_node* del_from_sector_cache_write_list(_sector_cache *cache,_sector_cache_node* cache_node)
{
    _sector_cache_node * p = cache_node->cache_write_prev;

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
uint32 __nand_sector_read(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf)
{
    uint32 ret,i,sector_no;
    uchar *temp_buf;

    ret = 0;
    sector_no = start_sector;
    temp_buf = buf;

    for(i=0;i<len;i++)
    {
        ret |= nand_sector_cache_read(zone,sector_no,temp_buf);
        sector_no++;
        temp_buf+= 512;
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
uint32 __nand_sector_write(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf)
{
    uint32 ret,i,sector_no;
    uchar *temp_buf;

    ret = 0;
    sector_no = start_sector;
    temp_buf = buf;

    for(i=0;i<len;i++)
    {
        ret |= nand_sector_cache_write(zone,sector_no,temp_buf);
        sector_no++;
        temp_buf += 512;
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
uint32 nand_sector_cache_read(struct _nftl_zone* zone,uint32 sector_no,uchar *buf)
{
    uint32 ret;
    uint32 len;
    _sector_cache_node * node;
    _sector_cache *cache;

    len = 512;
    cache = &zone->sector_cache;

    node = search_from_sector_cache_write_list(cache,sector_no);
    if(node != NULL)
    {
        MEMCPY(buf,node->buf,len);

        return NFTL_SUCCESS;
    }

    ret = __nand_read(zone,sector_no,1,buf);
    if(ret != 0)
    {
        NFTL_ERR("[NE]nand_sector_cache_read 1!\n");
        return NFTL_FAILURE;
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
uint32 nand_sector_cache_write(struct _nftl_zone* zone,uint32 sector_no,uchar *buf)
{
    uint32 ret;
    uint16 len;
    _sector_cache_node * node;
    _sector_cache *cache;

    cache = &zone->sector_cache;

    len = 512;

// case 2
    node = search_from_sector_cache_write_list(cache,sector_no);
    if(node != NULL)
    {
        del_from_sector_cache_write_list(cache,node);
        MEMCPY(node->buf,buf,len);
        add_to_sector_cache_write_list_tail(cache,node);

        return NFTL_SUCCESS;
    }

// case 4
    //if(cache->cache_write_nums >= zone->cfg->nftl_max_cache_write_num)
    if(cache->cache_write_nums >= (MAX_SECTOR_CACHE_NUM/2))
    {
        node = del_from_sector_cache_write_list(cache,cache->cache_write_head.cache_write_next);
        ret = __nand_write(zone,node->sector_no,1,node->buf);
        if(ret != 0)
        {
            NFTL_ERR("[NE]nand_sector_cache_write 1!\n");
            return NFTL_FAILURE;
        }
    }
    else
    {
        node = get_empty_sector_cahce_node(cache);
        if(node == NULL)
        {
            NFTL_ERR("[NE]nand_sector_cache_write 2!\n");
            return NFTL_FAILURE;
        }
    }

    node->sector_no = sector_no;
    MEMCPY(node->buf,buf,len);
    add_to_sector_cache_write_list_tail(cache,node);
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
uint32 __nand_flush_sector_write_cache(struct _nftl_zone* zone,uint32 num)
{
    uint32 ret,t;
    _sector_cache_node * node;
    _sector_cache *cache;

    cache = &zone->sector_cache;

    ret = 0;

    if(cache->cache_write_nums != 0)
    {
        //NFTL_ERR("[NE]flush_sector_write_cache %d\n",cache->cache_write_nums);
        t = 1;
    }
    else
    {
        t = 0;
    }

    while(cache->cache_write_head.cache_write_next != NULL)
    {
        node = del_from_sector_cache_write_list(cache,cache->cache_write_head.cache_write_next);
        ret = __nand_write(zone,node->sector_no,1,node->buf);
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

    if(t != 0)
    {
        //NFTL_ERR("[NE]flush_write_cache end!\n");
    }

    return ret;
}
