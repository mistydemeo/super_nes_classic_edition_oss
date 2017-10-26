
#ifndef _NFTL_INTERFACE_H
#define _NFTL_INTERFACE_H

#include "../phy/phy.h"
#include "../nftl/nftl_inc.h"
#include "../nftl_interface/nftl_blk.h"

extern int nand_info_init(struct _nand_info*nand_info,unsigned short chip,unsigned short start_block,unsigned char * mbr_data);
extern int nftl_build_all(struct _nand_info*nand_info);
extern uint32 get_nftl_num(void);
extern int nftl_build_one(struct _nand_info*nand_info,uint32 num);
extern unsigned int get_nftl_cap(void);
extern uint32 get_first_nftl_cap(void);

extern unsigned int nftl_read(unsigned int start_sector,unsigned int len,unsigned char *buf);
extern unsigned int nftl_write(unsigned int start_sector,unsigned int len,unsigned char *buf);
extern unsigned int nftl_flush_write_cache(void);

#endif /*_NFTL_INTERFACE_H*/

