#ifndef __NFTL_H
#define __NFTL_H

//#include "./../nftl_interface/nftl_type.h"
#include "nftl_type.h"


#define MAX_PAGE_PER_BLOCK                       1024

#define MAX_CACHE_NUM                            20
#define MAX_CACHE_WRITE_NUM                      10
#define MIN_CACHE_WRITE_NUM                      8

#define MAX_SECTOR_CACHE_NUM                     (2048)
#define MAX_PRIO_GC_NUM                          10

#define MAX_ERASE_NUM                          3000

#define PHY_MAPPING_PAGE                  0xaaaaffff
#define PHY_FILL_PAGE                     0x55555555
//-----------------------------------------------------------
#define PHY_FUNCTION_INFO_PAGE              0xaa110000
#define PHY_DISCARD_INFO_PAGE               0xaa111111 
#define PHY_ERROR_INFO_PAGE                 0xaa112222
#define PHY_NORMAL_POWER_DOWN               0xaa113333
#define PHY_XXX_1                           0xaa114444
#define PHY_XXX_2                           0xaa115555
#define PHY_XXX_3                           0xaa116666
#define PHY_XXX_4                           0xaa117777
#define PHY_XXX_5                           0xaa118888

//-----------------------------------------------------------

#define SPARE_SPECIAL_DATA                0xaa
#define SPARE_NOUSE_DATA                  0xbb
#define BLOCK_FOR_FTL_END                 0xcc
#define BLOCK_FOR_FTL_START               0xee

#define FLAG_FOR_LOGIC_PAGE               0xC0

#define SPARE_OFFSET_BAD_FLAG               0
#define SPARE_OFFSET_LOGIC_PAGE             1
#define SPARE_OFFSET_SPECIAL_FLAG           1
#define SPARE_OFFSET_SPECIAL_TYPE           (SPARE_OFFSET_SPECIAL_FLAG+1)
#define SPARE_OFFSET_ERASE_TIMES            (SPARE_OFFSET_LOGIC_PAGE+4)
#define SPARE_OFFSET_COUNTER                (SPARE_OFFSET_ERASE_TIMES+2)
#define SPARE_OFFSET_NFTL_SIZE              (SPARE_OFFSET_SPECIAL_TYPE+1)



#define BLOCK_NO_USED                     0
#define BLOCK_HAVE_USED                   1
#define BLOCK_ALL_INFO_PAGE               2
#define BLOCK_IS_BAD                      0xff


//nand block
typedef struct{
    uint16  Block_NO;
}_nand_block;


//nand page
typedef struct{
    uint16  Page_NO  : 12;
    uint16  Read_flag : 1;
    uint16  Nouse :     3;  
    uint16  Block_NO;
}_mapping_page;

//phy block数组
typedef struct __phy_block{
    _nand_block         phy_block;
    uint16              info;                     //0xaa:表示有page ECC error
    uint16              invalid_page_count;       //[0,pages_per_blk-1]
    uint16              erase_count;              //[0,10000]
    sint32              block_used_count;

    //block内无效page数链表，以vaild_page_count大小为顺序的单向链表，用于garbage collect 常驻内存，启动时恢复。
    struct __phy_block*    invalid_page_next;
    struct __phy_block*    invalid_page_prev;

    //free block链表,以erase_count大小为顺序的单向链表
    struct __phy_block*    free_next;
    struct __phy_block*    free_prev;

    //block使用先后链表
    struct __phy_block*    block_used_next;
    struct __phy_block*    block_used_prev;
}_phy_block_info;


struct  nand_chip_t{
    uchar           nand_id[8];
    uchar           sector_per_page;
    uchar           frequence_par;
    uchar           bytes_user_date;
    uchar           bytes_ecc_date;
    uchar           ecc_bits;            //8,24,32
    uchar           ecc_for_bytes;       //512 or 1024
    uchar           two_plane_support;
    uint16          max_erase_times;
    uint16          two_plane_offset;
    uint16          pages_per_blk;
    uint16          page_offset_in_nextblk;
    uint16          blk_per_chip;
    uint16          bytes_per_page;
    uint16          bitmap_per_page;
    uint16          support_read_reclaim;
    _phy_block_info* nand_block_info;
};


//错误记录
typedef struct{
    uint16   phy_read_ecc_error_times; 
    uint16   phy_write_error_times;
    uint16   logic_error_times;
    uint16   logic_error_type[16];
    uint16   phy_read_ecc_error_block[16];
    uint16   phy_write_ecc_error_block[16];
}_error_notes;


//此结构只能在后面增加，不能修改

#define  SMART_VERSION     0xaaaaaa01   //在后面增加成员不用修改版本号

typedef struct __smart{

    uint32 gap[16];

    uint32 version;

    uint64 total_recv_read_sectors;
    uint64 total_recv_write_sectors;

    uint64 total_real_read_pages;
    uint64 total_real_write_pages;

    uint32 total_recv_read_claim_pages;

    uint32 total_gc_times;
    uint32 total_gc_pages;
    uint32 total_wl_times;

    uint32 total_real_read_error_pages;
    uint32 total_real_write_error_pages;

    uint32 total_normal_power_cycles;
    uint32 total_unusual_power_cycles;

    uint32 max_block_erase_times;
    uint32 min_block_erase_times;

    uint32 max_block_used_counter;
    uint32 min_block_used_counter;

    uint32 read_reclaim_utc;           
    uint32 cross_talk_times;       //43

    uint32  reserve[80-43];       //补齐320字节

}_smart;


//常驻内存，当这个block写到最后一页时将此数据写入。
typedef struct __user_info{
    uchar*             buf;
    uint32             buf_size;

    uint32*            map_data;
    uint32             map_size;

    _smart*           smart;
    uint32            smart_size;

}_user_info;


#define  GC_ON             0
#define  GC_STOP           1
//Schmidt oscillator
typedef struct{
    uint16   start_gc_free_blocks;  //free 块小于此值开始gc
    uint16   stop_gc_free_blocks;   //free 块大于此值停止gc
    uint16   process;
    uint16   flag_gc_block;
    uint16   gc_page;
}_gc_strategy;


#define   PRIO_NONE              0
#define   GC_READ_RECLAIM        1
#define   GC_WEAR_LEVELING       2
#define   GC_CHANGE              3
#define   GC_READ_ERROR          4

typedef struct __prio_gc_node
{
    uint16                   gc_no;
    uint16                   prio_type;
    _phy_block_info*         phy_block_info;
    struct __prio_gc_node*   prio_gc_next;
    struct __prio_gc_node*   prio_gc_prev;
//  _phy_page_mapping *      zone_phy_page_map_for_prio;
}_prio_gc_node;

typedef struct
{
    uint16                   gc_num;
    uint16                   prio_type_now;
    _prio_gc_node            prio_gc_node[MAX_PRIO_GC_NUM];
    _prio_gc_node            prio_gc_head;
}_prio_gc;

typedef struct __sector_cache_node
{
    uint16 cache_no;
    uint16 cache_info;                  //[CACHE_EMPTY,CACHE_READ,CACHE_WRITE]
    uint32 sector_no;
    uchar* buf;
//    _sector_cache_node*   cache_read_next;
//    _sector_cache_node*   cache_read_prev;
    struct __sector_cache_node*   cache_write_next;
    struct __sector_cache_node*   cache_write_prev;
}_sector_cache_node;

typedef struct
{
    uint16 cache_totals;
//    uint16 cache_read_nums;
    uint16 cache_write_nums;
    uchar* cache_buf;
    _sector_cache_node   cache_node[MAX_SECTOR_CACHE_NUM];
//    _sector_cache_node   cache_read_head;
    _sector_cache_node   cache_write_head;
}_sector_cache;

#define CACHE_WRITE          1
#define CACHE_READ           0
#define CACHE_EMPTY          0xff

typedef struct __cache_node
{
    uint16 cache_no;
    uint16 cache_info;                  //[CACHE_EMPTY,CACHE_READ,CACHE_WRITE]
    uint16 start_sector;
    uint16 sector_len;
    uint32 page_no;
    uchar* buf;
    struct __cache_node*   cache_read_next;
    struct __cache_node*   cache_read_prev;
    struct __cache_node*   cache_write_next;
    struct __cache_node*   cache_write_prev;
}_cache_node;

typedef struct
{
    uint16          cache_totals;
    uint16          cache_read_nums;
    uint16          cache_write_nums;
    _cache_node     cache_node[MAX_CACHE_NUM];
    _cache_node     cache_read_head;
    _cache_node     cache_write_head;
    uchar*          cache_page_buf;
}_cache;

#define  WL_ON             1
#define  WL_STOP           0

typedef struct
{
    uint16              erase_threshold;
    uint16              erase_span;
    uint16              s_wl_status;
    uint16              s_wl_start;
    _phy_block_info*    block_for_s_wl; 
}_s_wl;

#define  MAX_MALLOC_BYTES    0x100000
#define  MAX_LOGIC_MAP_NUM   25
#define  MALLOC_FLAG         1


#define   SUPPORT_COSS_TALK         1

typedef struct
{
    _phy_block_info  *              block_info;                    //指向当前使用block
    _user_info                      user_info;                          //指向当前使用block物理到逻辑的映射表
    uint16                          page_used;
    //uint32                          block_used_count;
}_current_block;


struct _nftl_zone{
//  struct _nand_partition*         nand;                                       //这个物理分区所属的物理nand chip
    void*                           priv;
    struct  nand_chip_t*            nand_chip;                                //这个物理分区所属的物理nand chip
    char                            version[8];
    uchar                           zone_no;                                    //这个分区的编号
    uchar                           zone_attr;                                  //这个分区的属性
    uint16                          blocks;                                     //这个物理分区一共有多少个可用的物理block
    uint16                          bad_block;
    uint16                          free_block_num;                             //这个分区free block数目
    uint32                          logic_cap_in_sects;                         //这个物理分区的逻辑容量
    uint32                          logic_cap_in_page;
    uint32                          backup_cap_in_sects;                        //这个分区留给算法的备份空间
    _nand_block                     zone_start_blk_NO;                          //首个物理block的编号
    _phy_block_info*                zone_start_phy_block;                       //指向物理block数组的首个元素
    _phy_block_info*                zone_end_phy_block;                         //指向物理block数组的首个元素

#if  MALLOC_FLAG
    _mapping_page*                  zone_logic_page_map[MAX_LOGIC_MAP_NUM];     //指向逻辑page映射表首个元素
#else
    _mapping_page*                  zone_logic_page_map;                        //指向逻辑page映射表首个元素
#endif

    _phy_block_info                 free_head;                                  //指向free block排列链表
    _phy_block_info                 invalid_page_head;                          //指向block内无效page数排列链表
    _phy_block_info                 block_used_head;                            //指向used block顺序排列链表
    _user_info                      zone_phy_page_map_for_gc;                   //garbage_collect时使用
    uchar*                          temp_page_buf;
    uchar*                          logic_page_buf;

    _current_block                  current_block;
    _current_block                  assist_block;

    uint32                          test;
    uint32                          read_reclaim_complete;
    uint32                          already_read_flag;

    uint32                          max_erase_num;
    _gc_strategy                    gc_strategy;
    _prio_gc                        prio_gc;
    _s_wl                           s_wl;
    _cache                          cache;
    _sector_cache                   sector_cache;
    _smart*                         smart;
    struct _nftl_cfg*               cfg;

    int (*nftl_nand_erase_superblk)(struct _nftl_zone* zone,_physic_op_par *p);
    int (*nftl_nand_read_page)(struct _nftl_zone* zone,_physic_op_par *p);
    int (*nftl_nand_write_page)(struct _nftl_zone* zone,_physic_op_par *p);
    int (*nftl_nand_copy_page)(struct _nftl_zone *zone,_phy_block_info* a, _phy_block_info* b,uchar *buf,uint16 page);
    int (*nftl_nand_is_blk_good)(struct _nftl_zone* zone,_physic_op_par *p);
    int (*nftl_nand_mark_bad_blk)(struct _nftl_zone* zone,_physic_op_par *p);
    int (*nftl_nand_write_logic_page)(struct _nftl_zone* zone,uint32 page_no,uchar *buf);
    int (*nftl_nand_read_logic_page)(struct _nftl_zone* zone,uint32 page_no,uchar *buf);

};

extern void *nftl_malloc(uint32 size);
extern void nftl_free(const void *ptr);

#endif /*__NFTL_H*/
