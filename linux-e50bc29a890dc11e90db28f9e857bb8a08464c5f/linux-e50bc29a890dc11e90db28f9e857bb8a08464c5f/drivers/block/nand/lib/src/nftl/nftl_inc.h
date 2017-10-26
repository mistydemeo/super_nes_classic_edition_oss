
#define __NFTL_INCLUDE_H__


#ifndef _NFTL_INC_H
#define _NFTL_INC_H

#ifndef _NFTL_BLK_H
#include "../nftl_interface/nftl_blk.h"
#endif


#include "nftl.h"
#include "../phy/phy.h"

#ifdef _NFTL_BUILD_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL _phy_block_info* get_phy_block_addr(struct _nftl_zone *zone,uint16 block);
GLOBAL uint32 is_phy_block_valid(_phy_block_info* p_phy_block_info,struct _nftl_zone * zone);
GLOBAL uint32 is_last_phy_block(_phy_block_info* p_phy_block_info,struct _nftl_zone * zone);
GLOBAL int mark_free_block(struct _nftl_zone* zone,_phy_block_info* block);
GLOBAL _phy_block_info* current_block_ecc_error(struct _nftl_zone * zone,_phy_block_info* phy_block_ptr);
GLOBAL uint32 recover_phy_page_mapping(struct _nftl_zone * zone,_phy_block_info* phy_block_ptr,uint32* data);
GLOBAL uint32 recover_block_phy_page_mapping(struct _nftl_zone * zone,_phy_block_info* phy_block_ptr,uint32* data);
GLOBAL uint32 do_write_error_in_build_list(struct _nftl_zone* zone,_phy_block_info* block1, _phy_block_info* block2,uint16 page_num);
//GLOBAL uint32 get_used_page_num(struct _nftl_zone * zone,_phy_block_info* phy_block_ptr,uint32 *ecc);
GLOBAL int erase_block(struct _nftl_zone* zone,_phy_block_info* block,uint16 erase_num);
GLOBAL uint32 get_used_block_count(struct _nftl_zone * zone,_phy_block_info* phy_block_ptr);
GLOBAL void print_nftl_zone(struct _nftl_zone * zone);
GLOBAL int zone_param_exit(struct _nftl_zone *zone);
GLOBAL uint32 get_vaild_blocks(struct _nftl_zone * zone,uint32 start_block,uint32 blocks);
GLOBAL _phy_block_info* cross_talk_rebuild_current_block(struct _nftl_zone* zone,_phy_block_info* block1, _phy_block_info* block2);
GLOBAL _phy_block_info* no_cross_talk_rebuild_current_block(struct _nftl_zone* zone,_phy_block_info* block1, _phy_block_info* block2);
GLOBAL uint32 first_scan_all_blocks(struct _nftl_zone * zone);
GLOBAL uint32 init_zone_after_first_scan(struct _nftl_zone * zone,uint32 block_nums);
GLOBAL uint32 second_scan_all_blocks(struct _nftl_zone * zone);
GLOBAL uint32 get_current_used_block(struct _nftl_zone * zone,_phy_block_info* p_phy_block_info,uint32 block_used_count);
GLOBAL uint32 recover_logic_page_mapping(struct _nftl_zone* zone,_phy_block_info* p_phy_block_info,uint32* data,uint32 phy_page_nums);
GLOBAL int zone_param_init(struct _nftl_zone *zone,uint16 start_block,uint32 logic_sects,uint32 backup_cap_in_sects);
GLOBAL uint32 is_no_use_device(struct _nftl_zone * zone,uint32 size);
GLOBAL uint32 build_zone_list_first(struct _nftl_zone * zone,uint32 size);
GLOBAL uint32 build_zone_list(struct _nftl_zone * zone);
GLOBAL uint32 init_smart_info(struct _nftl_zone * zone,_phy_block_info* phy_block_ptr);


#undef GLOBAL
#ifdef  _NFTL_CACHE_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL uint32 nand_cache_read(struct _nftl_zone* zone,uint32 page_no,uint16 bitmap,uchar *buf);
GLOBAL uint32 nand_cache_write(struct _nftl_zone* zone,uint32 page_no,uint16 bitmap,uchar *buf);
GLOBAL uint32 nand_op(uchar flag,struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf);
GLOBAL uint32 __nand_read(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf);
GLOBAL uint32 __nand_write(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf);
GLOBAL uint32 __nand_flush_write_cache(struct _nftl_zone* zone,uint32 num);
GLOBAL uint32 flush_write_cache_to_nand(struct _nftl_zone* zone);
GLOBAL uint32 __shutdown_op(struct _nftl_zone* zone);
GLOBAL _cache_node *search_from_cache_read_list(_cache* cache,uint32 page);
GLOBAL _cache_node *search_from_cache_write_list(_cache* cache,uint32 page);
GLOBAL _cache_node * get_empty_cahce_node(_cache *cache);
GLOBAL int add_to_cache_read_list_tail(_cache *cache,_cache_node* cache_node);
GLOBAL int add_to_cache_write_list_tail(_cache *cache,_cache_node* cache_node);
GLOBAL _cache_node* del_from_cache_read_list(_cache *cache,_cache_node* cache_node);
GLOBAL _cache_node* del_from_cache_write_list(_cache *cache,_cache_node* cache_node);
GLOBAL int nftl_cache_init(struct _nftl_zone* zone);
GLOBAL int nftl_cache_exit(struct _nftl_zone* zone);
GLOBAL uint32 print_logic_page_map(struct _nftl_zone *zone,uint32 start_page);


#undef GLOBAL
#ifdef  _NFTL_SECTOR_CACHE_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int nftl_sector_cache_init(struct _nftl_zone* zone);
GLOBAL int nftl_sector_cache_exit(struct _nftl_zone* zone);
GLOBAL _sector_cache_node *search_from_sector_cache_write_list(_sector_cache* cache,uint32 sector);
GLOBAL _sector_cache_node * get_empty_sector_cahce_node(_sector_cache *cache);
GLOBAL int add_to_sector_cache_write_list_tail(_sector_cache *cache,_sector_cache_node* cache_node);
GLOBAL _sector_cache_node* del_from_sector_cache_write_list(_sector_cache *cache,_sector_cache_node* cache_node);
GLOBAL uint32 __nand_sector_read(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf);
GLOBAL uint32 __nand_sector_write(struct _nftl_zone* zone,uint32 start_sector,uint32 len,uchar *buf);
GLOBAL uint32 nand_sector_cache_read(struct _nftl_zone* zone,uint32 sector_no,uchar *buf);
GLOBAL uint32 nand_sector_cache_write(struct _nftl_zone* zone,uint32 sector_no,uchar *buf);
GLOBAL uint32 __nand_flush_sector_write_cache(struct _nftl_zone* zone,uint32 num);

#undef GLOBAL
#ifdef _NFTL_COMMON_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int _nftl_nand_copy_page(struct _nftl_zone *zone,_phy_block_info* a, _phy_block_info* b,uchar *buf,uint16 page);
GLOBAL void set_oob_special_page(struct _nftl_zone *zone,uchar* buf,uint32 special_data,uint32 block_used_count,uint16 erase_times);
GLOBAL uint32 get_logic_page_from_oob(uchar* buf);
GLOBAL uint16 get_erase_count_from_oob(uchar* buf);
GLOBAL void set_oob_logic_page(struct _nftl_zone *zone,uchar* buf,uint32 logic_page,uint32 block_used_count,uint16 erase_times);
GLOBAL uint32 get_special_data_from_oob(uchar* buf);
GLOBAL _mapping_page* get_logic_page_map(struct _nftl_zone *zone,uint32 logic_page);
GLOBAL int malloc_logic_page_map(struct _nftl_zone *zone,uint32 total_pages);
GLOBAL int is_ftl_start_flag_page(uchar* buf);
GLOBAL void set_start_block_flag(struct _nftl_zone *zone,uchar* buf,uint32 size);
GLOBAL int is_ftl_end_flag_page(uchar* buf);
GLOBAL void set_end_block_flag(struct _nftl_zone *zone,uchar* buf);
GLOBAL int is_ftl_special_data(uchar* buf);
GLOBAL int is_ftl_logic_page_data(uchar* buf);
GLOBAL void set_no_use_page(struct _nftl_zone *zone,uchar* buf);
GLOBAL int is_nouse_page(uchar* buf);
GLOBAL int is_fill_page(uchar* buf);
GLOBAL int is_phy_mapping_page(uchar* buf);
GLOBAL int is_power_down_info_page(uchar* buf);
GLOBAL void set_physic_op_par(_physic_op_par* phy_op_par,uint16 blkNO,uint16 Page_NO,uint16 page_bitmap,uchar*main_buf,uchar*spare_buf);
GLOBAL int free_logic_page_map(struct _nftl_zone *zone);
GLOBAL uint32 get_spare_data(uchar* buf,uchar offset,uchar num);
GLOBAL uint32 get_block_used_count_from_oob(uchar* buf);

#undef GLOBAL
#ifdef _NFTL_GC_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL uint32 garbage_collect_first(struct _nftl_zone* zone,_phy_block_info* block,uint16 page_num);
GLOBAL void add_prio_gc(struct _nftl_zone* zone,_phy_block_info* block,uint16 type);
GLOBAL uint32 prio_gc_all(struct _nftl_zone* zone);
GLOBAL uint32 do_prio_gc(struct _nftl_zone* zone);
GLOBAL uint32 gc_all(struct _nftl_zone* zone,uint16 invalid_page_count);
GLOBAL uint32 gc_one(struct _nftl_zone* zone, uint16 invalid_page_count);
GLOBAL uint32 garbage_collect(struct _nftl_zone* zone);
GLOBAL uint32 do_static_wear_leveling(struct _nftl_zone* zone);
GLOBAL uint32 done_static_wear_leveling(struct _nftl_zone* zone);
GLOBAL uint32 prio_gc_one(struct _nftl_zone* zone,uint16 block,uint32 flag);
GLOBAL uint32 gc_win_block(struct _nftl_zone* zone,uint16 block);
GLOBAL uint32 is_current_blcok(struct _nftl_zone* zone,_phy_block_info* block);

#undef GLOBAL
#ifdef _NFTL_INTERFACE_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL uint32 nftl_get_zone_cap(void * _zone);
GLOBAL void nftl_set_zone_test(void * _zone,uint32 test);
GLOBAL void* nftl_get_zone_priv(void * _zone);
GLOBAL uint16 nftl_get_zone_write_cache_nums(void * _zone);
GLOBAL void print_smart(void * _zone);

#undef GLOBAL
#ifdef _NFTL_LIST_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int adjust_invaild_list(struct _nftl_zone* zone);
GLOBAL int put_phy_block_to_invalid_page_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr);
GLOBAL _phy_block_info* out_phy_block_from_invalid_page_list_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr);
GLOBAL _phy_block_info* out_phy_block_from_free_list_by_block(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr);
GLOBAL _phy_block_info* out_phy_block_from_free_list_by_erase_time(struct _nftl_zone* zone,uint16 erase_time);
GLOBAL uint16 get_max_erase_time_from_free_list(struct _nftl_zone* zone,uint16 erase_time);
GLOBAL _phy_block_info* get_block_max_erase_time_from_free_list(struct _nftl_zone* zone,uint16 erase_time);
GLOBAL _phy_block_info* get_block_max_erase_time_from_invalid_list(struct _nftl_zone* zone,uint16 erase_time);
GLOBAL _phy_block_info* get_block_min_erase_time_from_invalid_list(struct _nftl_zone* zone,uint16 erase_time,uint32 max_count);
GLOBAL _phy_block_info* get_block_max_count_from_invalid_list(struct _nftl_zone* zone);
GLOBAL _phy_block_info* get_block_min_count_from_invalid_list(struct _nftl_zone* zone);
GLOBAL _phy_block_info* out_phy_block_from_free_list(struct _nftl_zone* zone);
GLOBAL int phy_block_from_invalid_page_incr(struct _nftl_zone* zone,_phy_block_info* block);
GLOBAL _phy_block_info* out_phy_block_from_invalid_page_list(struct _nftl_zone* zone);
GLOBAL void print_block_invalid_list(struct _nftl_zone* zone);
GLOBAL uint32 is_block_in_invalid_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr);
GLOBAL uint32 is_block_in_free_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr);
GLOBAL int put_phy_block_to_free_list(struct _nftl_zone* zone,_phy_block_info* phy_block_ptr);
GLOBAL int add_block_count_list(struct _nftl_zone* zone, _phy_block_info* phy_block_ptr);
GLOBAL void print_free_list(struct _nftl_zone* zone);

#undef GLOBAL
#ifdef _NFTL_LOGIC_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int nand_write_logic_page_no_gc(struct _nftl_zone *zone,uint32 page_no,uchar *buf);
GLOBAL int get_new_current_block(struct _nftl_zone* zone,_current_block* current_block);
GLOBAL int write_phy_page_map_to_current_block(struct _nftl_zone* zone,_current_block* current_block);
GLOBAL int write_logic_data_to_current_block(struct _nftl_zone* zone,_current_block* current_block,uint32 data_type,uint32 page_no,uchar *buf);
GLOBAL int new_block_init_for_write(struct _nftl_zone* zone,_phy_block_info* block,uint32 block_used_count);
GLOBAL int write_phy_data_to_current_block(struct _nftl_zone* zone,_current_block* current_block,uchar* buf,uint32 page_no,uint32 type);
GLOBAL int check_invalid_page_right(struct _nftl_zone* zone,_user_info* user_info,_phy_block_info* block,uint16 page_num);
GLOBAL void nftl_ops_init(struct _nftl_zone *zone);
GLOBAL int current_block_at_last_page(struct _nftl_zone *zone,_current_block* current_block);
GLOBAL int current_block_at_first_page(struct _nftl_zone *zone,_current_block* current_block);
GLOBAL int do_before_write(struct _nftl_zone* zone);
GLOBAL int do_after_write(struct _nftl_zone* zone);
GLOBAL int write_data_to_page(struct _nftl_zone* zone,uint32 data_type,uint32 page_no,uchar *buf);
GLOBAL int fill_no_use_data_to_page(struct _nftl_zone* zone);
GLOBAL int fill_no_use_data_to_current_block(struct _nftl_zone* zone);
GLOBAL int fill_no_use_page_to_current_block(struct _nftl_zone* zone,int page_num);

#undef GLOBAL
#ifdef _NFTL_START_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int nftl_start(void* priv,void* cfg,struct _nftl_zone ** pzone,uint64 logic_size,uint64 total_size,unsigned erasesize,unsigned writesize,unsigned oobavail,char* name,int no,char type);
GLOBAL void nftl_zone_release(struct _nftl_zone *zone);

#undef GLOBAL
#ifdef _NFTL_NFTL_HW_INTERFACE_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int _nftl_nand_read_page(struct _nftl_zone *zone,_physic_op_par *p);
GLOBAL int _nftl_nand_write_page(struct _nftl_zone *zone,_physic_op_par *p);
GLOBAL int  _nftl_nand_erase_superblk(struct _nftl_zone *zone,_physic_op_par *p);
GLOBAL int _nftl_nand_mark_bad_blk(struct _nftl_zone *zone,_physic_op_par *p);
GLOBAL int _nftl_nand_is_blk_good(struct _nftl_zone *zone,_physic_op_par *p);

#undef GLOBAL
#ifdef  _NFTL_INIT_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL void *nftl_malloc(uint32 size);
GLOBAL void nftl_free(const void *ptr);
GLOBAL int nftl_initialize(struct _nftl_blk *nftl_blk,int no);
GLOBAL int nftl_exit(struct _nftl_blk *nftl_blk);
GLOBAL int _nand_read(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
GLOBAL int _nand_write(struct _nftl_blk *nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
GLOBAL int _nand_flush_write_cache(struct _nftl_blk *nftl_blk,uint32 num);
GLOBAL int    _shutdown_op(struct _nftl_blk *nftl_blk);
GLOBAL struct _nftl_blk* del_last_nftl_blk(struct _nftl_blk*head);
GLOBAL void add_nftl_blk_list(struct _nftl_blk*head,struct _nftl_blk *nftl_blk);

#undef GLOBAL
#ifdef  _NFTL_UBOOT_INTERFACE_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif


GLOBAL struct _nftl_blk * nftl_add(struct _nand_phy_partition* phy_partition);
GLOBAL void print_all_nftl_blk(void);
GLOBAL uint32 nftl_read(uint32 start_sector,uint32 len,unsigned char *buf);
GLOBAL uint32 nftl_write(uint32 start_sector,uint32 len,unsigned char *buf);
GLOBAL uint32 nftl_flush_write_cache(void);


#undef GLOBAL
#ifdef _NFTL_CLASS_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

#endif /*_NFTL_INC_H*/
