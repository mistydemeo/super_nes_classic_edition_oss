
#ifndef _NFTL_TYPE_H
#define _NFTL_TYPE_H

//#include "type.h"
#include "../../osal/nand_osal.h"

#define FACTORY_BAD_BLOCK_ERROR                   159
#define BYTES_PER_SECTOR                          512
#define SHIFT_PER_SECTOR                          9
#define BYTES_OF_USER_PER_PAGE                    16
#define MIN_BYTES_OF_USER_PER_PAGE                16


#define EPERM                   1
#define ENOMEM                  2

#ifndef ECC_LIMIT
#define ECC_LIMIT               10
#endif

#ifndef NAND_ERR_ECC
#define NAND_ERR_ECC            (-12)
#endif


//#define  PRINT printf

typedef enum {
    NFTL_SUCCESS                =0,
    NFTL_FAILURE                =1,
    NFTL_INVALID_PARTITION      =2,
    NFTL_INVALID_ADDRESS        =3,
    NFTL_FLUSH_ERROR            =4,
    NFTL_PAGENOTFOUND           =5,
    NFTL_NO_FREE_BLOCKS         =6,
    NFTL_NO_INVALID_BLOCKS      =7,
    NFTL_COMPLETE               =8,
    NFTL_YES                    =1,
    NFTL_NO                     =0,
}_nftl_error;

//nand page
typedef struct{
    uint16  Page_NO;
    uint16  Block_NO;
//    uchar   page_status;
}_nand_page;

//从物理的角度看到的接口
typedef struct{
    _nand_page    phy_page;
    uint16        page_bitmap;
    uchar*        main_data_addr;
    uchar*        spare_data_addr;
}_physic_op_par;


#define    NFTL_DBG_MESSAGE_ON    1
#define    NFTL_ERR_MESSAGE_ON    1


#if NFTL_DBG_MESSAGE_ON
#define     NFTL_DBG(...)                PRINT(__VA_ARGS__)
#else
#define     NFTL_DBG(...)
#endif

#if NFTL_ERR_MESSAGE_ON
#define     NFTL_ERR(...)                PRINT(__VA_ARGS__)
#else
#define     NFTL_ERR(...)
#endif




#endif /*_NFTL_TYPE_H*/
