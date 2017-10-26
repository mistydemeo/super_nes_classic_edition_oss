#ifndef _NFTL_BLK_H
#define _NFTL_BLK_H

#include "../nftl/nftl_type.h"
#include "nftl_cfg.h"

//extern struct _nftl_zone;
//extern struct _nand_partition;

//struct _nftl_blk;
struct _nftl_blk{
	unsigned int                nftl_logic_size;
	struct _nand_partition*     nand;
	struct _nftl_blk*           nftl_blk_next;
	struct _nftl_zone*          nftl_zone;
	struct _nftl_cfg*           nftl_cfg;
	unsigned int		        time;
	unsigned int		        ops_time;	/* add ecape time for ops nftl */
	unsigned int		        time_flush;
	void*                       nftl_thread;
    void*                       blk_lock;
	int (*read_data)         (struct _nftl_blk *nftl_blk, unsigned int block, unsigned int nblk, unsigned char *buf);
	int (*write_data)        (struct _nftl_blk *nftl_blk, unsigned int block, unsigned int nblk, unsigned char *buf);
	int (*flush_write_cache) (struct _nftl_blk *nftl_blk,unsigned int num);
	int (*shutdown_op)       (struct _nftl_blk *nftl_blk);
	int (*read_sector_data)         (struct _nftl_blk *nftl_blk, unsigned int block, unsigned int nblk, unsigned char *buf);
	int (*write_sector_data)        (struct _nftl_blk *nftl_blk, unsigned int block, unsigned int nblk, unsigned char *buf);
	int (*flush_sector_write_cache) (struct _nftl_blk *nftl_blk,unsigned int num);
};

#ifndef __NFTL_INCLUDE_H__
extern int nftl_initialize(struct _nftl_blk *nftl_blk,int no);
#endif

#endif /*_NFTL_BLK_H*/
