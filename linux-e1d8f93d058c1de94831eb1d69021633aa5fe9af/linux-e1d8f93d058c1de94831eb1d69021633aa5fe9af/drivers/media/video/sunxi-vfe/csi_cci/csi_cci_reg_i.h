/*
 * sunxi csi cci register read/write interface register header file
 * Author:raymonxiu
 */
#ifndef __CSI__CCI__REG__I__H__
#define __CSI__CCI__REG__I__H__

//
// Register Offset
//
#define CCI_CTRL_OFF                     		0x0000
#define CCI_CFG_OFF                      		0x0004
#define CCI_FMT_OFF                      		0x0008
#define CCI_BUS_CTRL_OFF                 		0x000C
#define CCI_INT_CTRL_OFF                 		0x0014
#define CCI_LC_TRIG_OFF                  		0x0018
#define CCI_FIFO_ACC_OFF                 		0x0100
#define CCI_RSV_REG_OFF                  		0x0200

//
// Register Address
//
#define CCI_CTRL_ADDR                    		( CSI0_VBASE + CCI_CTRL_OFF                     )		// CCI control register
#define CCI_CFG_ADDR                     		( CSI0_VBASE + CCI_CFG_OFF                      )		// CCI transmission config register
#define CCI_FMT_ADDR                     		( CSI0_VBASE + CCI_FMT_OFF                      )		// CCI packet format register
#define CCI_BUS_CTRL_ADDR                		( CSI0_VBASE + CCI_BUS_CTRL_OFF                 )		// CCI bus control register
#define CCI_INT_CTRL_ADDR                		( CSI0_VBASE + CCI_INT_CTRL_OFF                 )		// CCI interrupt control register
#define CCI_LC_TRIG_ADDR                 		( CSI0_VBASE + CCI_LC_TRIG_OFF                  )		// CCI line counter trigger register
#define CCI_FIFO_ACC_ADDR                		( CSI0_VBASE + CCI_FIFO_ACC_OFF                 )		// CCI FIFO access register
#define CCI_RSV_REG_ADDR                 		( CSI0_VBASE + CCI_RSV_REG_OFF                  )		// CCI reserved register

// 
// Detail information of registers
//

typedef union
{
	unsigned int dwval;
	struct
	{
		unsigned int cci_en                      :  1 ;    // Default: 0; 
		unsigned int soft_reset                  :  1 ;    // Default: 0; 
		unsigned int res0                        : 14 ;    // Default: ; 
		unsigned int cci_sta                     :  8 ;    // Default: ; 
		unsigned int tran_result                 :  4 ;    // Default: 0; 
		unsigned int read_tran_mode              :  1 ;    // Default: 0; 
		unsigned int restart_mode                :  1 ;    // Default: 0; 
		unsigned int repeat_tran                 :  1 ;    // Default: 0; 
		unsigned int single_tran                 :  1 ;    // Default: 0; 
	} bits;
} CCI_CTRL_t;

typedef union
{
	unsigned int dwval;
	struct
	{
		unsigned int csi_trig                    :  4 ;    // Default: 0; 
		unsigned int trig_mode                   :  3 ;    // Default: 0; 
		unsigned int src_sel                     :  1 ;    // Default: 0; 
		unsigned int res0                        :  7 ;    // Default: ; 
		unsigned int packet_mode                 :  1 ;    // Default: 0; 
		unsigned int interval                    :  8 ;    // Default: 0x00; 
		unsigned int timeout_n                   :  8 ;    // Default: 0x10; 
	} bits;
} CCI_CFG_t;

typedef union
{
	unsigned int dwval;
	struct
	{
		unsigned int packet_cnt                  : 16 ;    // Default: 1; 
		unsigned int data_byte                   :  4 ;    // Default: 1; 
		unsigned int addr_byte                   :  4 ;    // Default: 1; 
		unsigned int cmd                         :  1 ;    // Default: 0; 
		unsigned int slv_id                      :  7 ;    // Default: 0; 
	} bits;
} CCI_FMT_t;

typedef union
{
	unsigned int dwval;
	struct
	{
		unsigned int sda_moe                     :  1 ;    // Default: 0; 
		unsigned int scl_moe                     :  1 ;    // Default: 0; 
		unsigned int sda_mov                     :  1 ;    // Default: 0; 
		unsigned int scl_mov                     :  1 ;    // Default: 0; 
		unsigned int sda_pen                     :  1 ;    // Default: 0; 
		unsigned int scl_pen                     :  1 ;    // Default: 0; 
		unsigned int sda_sta                     :  1 ;    // Default: ; 
		unsigned int scl_sta                     :  1 ;    // Default: ; 
		unsigned int clk_m                       :  4 ;    // Default: 0x5; 
		unsigned int clk_n                       :  3 ;    // Default: 0x2; 
		unsigned int dly_trig                    :  1 ;    // Default: 0; 
		unsigned int dly_cyc                     : 16 ;    // Default: 0; 
	} bits;
} CCI_BUS_CTRL_t;

typedef union
{
	unsigned int dwval;
	struct
	{
		unsigned int s_tran_com_pd               :  1 ;    // Default: 0; 
		unsigned int s_tran_err_pd               :  1 ;    // Default: 0; 
		unsigned int res0                        : 14 ;    // Default: ; 
		unsigned int s_tran_com_int_en           :  1 ;    // Default: 0; 
		unsigned int s_tran_err_int_en           :  1 ;    // Default: 0; 
		unsigned int res1                        : 14 ;    // Default: ; 
	} bits;
} CCI_INT_CTRL_t;

typedef union
{
	unsigned int dwval;
	struct
	{
		unsigned int ln_cnt                      : 13 ;    // Default: 0; 
		unsigned int res0                        : 19 ;    // Default: ; 
	} bits;
} CCI_LC_TRIG_t;

typedef union
{
	unsigned int  dwval;
	struct
	{
		unsigned int data_fifo               : 32         ;    // Default: 0; 
	} bits;
} CCI_FIFO_ACC_t;


//------------------------------------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------------------------------------

#endif // __CSI__CCI__REG__I__H__
