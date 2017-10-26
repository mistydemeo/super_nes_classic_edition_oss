

#define _NFTL_NFTL_HW_INTERFACE_C_

#include "../nftl_interface/nftl_blk.h"
#include "../phy/phy.h"
#include "../nftl/nftl_inc.h"


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int  _nftl_nand_erase_superblk(struct _nftl_zone *zone,_physic_op_par *p)
{
	struct _physic_par physic_par;
    struct _nftl_blk *nftl_blk = (struct _nftl_blk *)nftl_get_zone_priv(zone);
    struct _nand_partition *nand = nftl_blk->nand;

	physic_par.phy_page.Block_NO = p->phy_page.Block_NO;
	physic_par.phy_page.Page_NO = p->phy_page.Page_NO;
	physic_par.page_bitmap = p->page_bitmap;
	physic_par.main_data_addr = p->main_data_addr;
	physic_par.spare_data_addr = p->spare_data_addr;

    return nand->nand_erase_superblk(nftl_blk->nand,&physic_par);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nftl_nand_read_page(struct _nftl_zone *zone,_physic_op_par *p)
{
    int ret;
	struct _physic_par physic_par;
    struct _nftl_blk *nftl_blk = (struct _nftl_blk *)nftl_get_zone_priv(zone);
    struct _nand_partition *nand = nftl_blk->nand;

    zone->smart->total_real_read_pages++;

	physic_par.phy_page.Block_NO = p->phy_page.Block_NO;
	physic_par.phy_page.Page_NO = p->phy_page.Page_NO;
	physic_par.page_bitmap = p->page_bitmap;
	physic_par.main_data_addr = p->main_data_addr;
	physic_par.spare_data_addr = p->spare_data_addr;

    ret = nand->nand_read_page(nftl_blk->nand,&physic_par);

    if(ret == NAND_ERR_ECC)
    {
        zone->smart->total_real_read_error_pages++;
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
int _nftl_nand_write_page(struct _nftl_zone *zone,_physic_op_par *p)
{
    int ret;
	struct _physic_par physic_par;
    struct _nftl_blk *nftl_blk = (struct _nftl_blk *)nftl_get_zone_priv(zone);
    struct _nand_partition *nand = nftl_blk->nand;

    zone->smart->total_real_write_pages++;

	physic_par.phy_page.Block_NO = p->phy_page.Block_NO;
	physic_par.phy_page.Page_NO = p->phy_page.Page_NO;
	physic_par.page_bitmap = p->page_bitmap;
	physic_par.main_data_addr = p->main_data_addr;
	physic_par.spare_data_addr = p->spare_data_addr;

	ret = nand->nand_write_page(nftl_blk->nand,&physic_par);

    if(ret != 0)
    {
        zone->smart->total_real_write_error_pages++;
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
int _nftl_nand_is_blk_good(struct _nftl_zone *zone,_physic_op_par *p)
{
	struct _physic_par physic_par;
    struct _nftl_blk *nftl_blk = (struct _nftl_blk *)nftl_get_zone_priv(zone);
    struct _nand_partition *nand = nftl_blk->nand;

	physic_par.phy_page.Block_NO = p->phy_page.Block_NO;
	physic_par.phy_page.Page_NO = p->phy_page.Page_NO;
	physic_par.page_bitmap = p->page_bitmap;
	physic_par.main_data_addr = p->main_data_addr;
	physic_par.spare_data_addr = p->spare_data_addr;

    return nand->nand_is_blk_good(nftl_blk->nand,&physic_par);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _nftl_nand_mark_bad_blk(struct _nftl_zone *zone,_physic_op_par *p)
{
	struct _physic_par physic_par;
    struct _nftl_blk *nftl_blk = (struct _nftl_blk *)nftl_get_zone_priv(zone);
    struct _nand_partition *nand = nftl_blk->nand;

	physic_par.phy_page.Block_NO = p->phy_page.Block_NO;
	physic_par.phy_page.Page_NO = p->phy_page.Page_NO;
	physic_par.page_bitmap = p->page_bitmap;
	physic_par.main_data_addr = p->main_data_addr;
	physic_par.spare_data_addr = p->spare_data_addr;

    return nand->nand_mark_bad_blk(nftl_blk->nand,&physic_par);
}
