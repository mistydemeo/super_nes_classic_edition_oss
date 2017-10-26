/*
 * sunxi csi cci register read/write interface
 * Author:raymonxiu
 */

#include "csi_cci_reg_i.h"
#include "csi_cci_reg.h"

CCI_CTRL_t				*cci_ctrl[MAX_CSI];
CCI_CFG_t					*cci_cfg[MAX_CSI];
CCI_FMT_t					*cci_fmt[MAX_CSI];
CCI_BUS_CTRL_t		*cci_bus_ctrl[MAX_CSI];
CCI_INT_CTRL_t		*cci_int_ctrl[MAX_CSI];
CCI_LC_TRIG_t			*cci_lc_trig[MAX_CSI];
CCI_FIFO_ACC_t		*cci_fifo_acc[MAX_CSI];
unsigned int		fifo_last_pt[MAX_CSI];

int csi_cci_set_base_addr(unsigned int sel, unsigned int addr)
{
  if(sel > MAX_CSI)
    return -1;
  
  cci_ctrl[sel]                =  (CCI_CTRL_t		             *)(addr + CCI_CTRL_OFF             );
  cci_cfg[sel]              	 =  (CCI_CFG_t			           *)(addr + CCI_CFG_OFF              );
  cci_fmt[sel]                 =  (CCI_FMT_t			           *)(addr + CCI_FMT_OFF              );
  cci_bus_ctrl[sel]            =  (CCI_BUS_CTRL_t            *)(addr + CCI_BUS_CTRL_OFF         );
  cci_int_ctrl[sel]            =  (CCI_INT_CTRL_t            *)(addr + CCI_INT_CTRL_OFF         );
  cci_lc_trig[sel]             =  (CCI_LC_TRIG_t	           *)(addr + CCI_LC_TRIG_OFF          );
  cci_fifo_acc[sel]            =  (CCI_FIFO_ACC_t            *)(addr + CCI_FIFO_ACC_OFF         );
                                                                       
  return 0;
}

void csi_cci_enable(unsigned int sel)
{
  cci_ctrl[sel]->bits.cci_en = 1;
}

void csi_cci_disable(unsigned int sel)
{
  cci_ctrl[sel]->bits.cci_en = 0;
}

void csi_cci_reset(unsigned int sel)
{
	cci_ctrl[sel]->bits.soft_reset = 1;
	
	cci_ctrl[sel]->bits.soft_reset = 0;
}

void csi_cci_set_clk_div(unsigned int sel, unsigned char *div_coef)
{
	cci_bus_ctrl[sel]->bits.clk_m = div_coef[0];
	cci_bus_ctrl[sel]->bits.clk_n = div_coef[1];
}

//interval unit in 40 scl cycles
void csi_cci_set_pkt_interval(unsigned int sel, unsigned char interval)
{
	cci_cfg[sel]->bits.interval = interval;
}

//timeout unit in scl cycle
void csi_cci_set_ack_timeout(unsigned int sel, unsigned char to_val)
{
	cci_cfg[sel]->bits.timeout_n = to_val;
}

//trig delay unit in scl cycle
void csi_cci_set_trig_dly(unsigned int sel, unsigned int dly)
{
	if(dly == 0) {	
		cci_bus_ctrl[sel]->bits.dly_trig = 0;
	} else {
		cci_bus_ctrl[sel]->bits.dly_trig = 1;
		cci_bus_ctrl[sel]->bits.dly_cyc = dly;
	}
}

void csi_cci_trans_start(unsigned int sel, enum cci_trans_mode trans_mode)
{
  fifo_last_pt[sel] = 0;
  switch(trans_mode)
  {
  	case SINGLE:
  		cci_ctrl[sel]->bits.repeat_tran = 0;
  		cci_ctrl[sel]->bits.single_tran = 1;
  		break;
  	case REPEAT:
  		cci_ctrl[sel]->bits.single_tran = 0;
  		cci_ctrl[sel]->bits.repeat_tran = 1;
  		break;
  	default:
  		cci_ctrl[sel]->bits.single_tran = 0;
  		cci_ctrl[sel]->bits.repeat_tran = 0;
  		break;
  }
}

unsigned int csi_cci_get_trans_done(unsigned int sel)
{
	if(cci_ctrl[sel]->bits.single_tran == 0)
		return 0;
	else
		return 1;
}

void csi_cci_set_bus_fmt(unsigned int sel, struct cci_bus_fmt *bus_fmt)
{
	cci_ctrl[sel]->bits.restart_mode = bus_fmt->rs_mode;
	cci_ctrl[sel]->bits.read_tran_mode = bus_fmt->rs_start;
	cci_fmt[sel]->bits.slv_id = bus_fmt->saddr_7bit;
	cci_fmt[sel]->bits.cmd = bus_fmt->wr_flag;
	cci_fmt[sel]->bits.addr_byte = bus_fmt->addr_len;
	cci_fmt[sel]->bits.data_byte = bus_fmt->data_len;
}

void csi_cci_set_tx_buf_mode(unsigned int sel, struct cci_tx_buf *tx_buf_mode)
{
	cci_cfg[sel]->bits.src_sel = tx_buf_mode->buf_src;
	cci_cfg[sel]->bits.packet_mode = tx_buf_mode->pkt_mode;
	cci_fmt[sel]->bits.packet_cnt = tx_buf_mode->pkt_cnt;
}

void csi_cci_fifo_pt_reset(unsigned int sel)
{
	fifo_last_pt[sel] = 0;
}

void csi_cci_fifo_pt_add(unsigned int sel, unsigned int byte_cnt)
{
	fifo_last_pt[sel] += byte_cnt;
}

void csi_cci_fifo_pt_sub(unsigned int sel, unsigned int byte_cnt)
{
	fifo_last_pt[sel] -= byte_cnt;
}

static void cci_wr_tx_buf(unsigned int sel, unsigned int byte_index, unsigned char value)
{
	unsigned int index_remain,index_dw_align,tmp;
	index_remain = (byte_index)%4;
	index_dw_align = (byte_index)/4;

	tmp = (cci_fifo_acc[sel] + index_dw_align)->dwval;
	tmp &= ~(0xff << (index_remain<<3));
	tmp |= value << (index_remain<<3);
	(cci_fifo_acc[sel] + index_dw_align)->dwval = tmp;
}

static void cci_rd_tx_buf(unsigned int sel, unsigned int byte_index, unsigned char *value)
{
	unsigned int index_remain,index_dw_align,tmp;
	index_remain = (byte_index)%4;
	index_dw_align = (byte_index)/4;

	tmp = (cci_fifo_acc[sel] + index_dw_align)->dwval;
	*value = (tmp & ( 0xff << (index_remain<<3) )) >> (index_remain<<3);
}

void csi_cci_wr_tx_buf(unsigned int sel, unsigned char *buf, unsigned int byte_cnt)
{
	unsigned int i;
	//cci_print_info(sel);
	for(i = 0; i < byte_cnt; i++,fifo_last_pt[sel]++)
	{
		cci_wr_tx_buf(sel, fifo_last_pt[sel], buf[i]);
	}
}

void csi_cci_rd_tx_buf(unsigned int sel, unsigned char *buf, unsigned int byte_cnt)
{
	unsigned int i;
	//cci_print_info(sel);
	for(i = 0; i < byte_cnt; i++,fifo_last_pt[sel]++)
	{
		cci_rd_tx_buf(sel, fifo_last_pt[sel], &buf[i]);
	}
}

void csi_cci_set_trig_mode(unsigned int sel, struct cci_tx_trig *tx_trig_mode)
{
	cci_cfg[sel]->bits.trig_mode = tx_trig_mode->trig_src;
	cci_cfg[sel]->bits.csi_trig = tx_trig_mode->trig_con;
}

void csi_cci_set_trig_line_cnt(unsigned int sel, unsigned int line_cnt)
{
	cci_lc_trig[sel]->bits.ln_cnt = line_cnt;
}

void cci_int_enable(unsigned int sel, enum cci_int_sel interrupt)
{
  cci_int_ctrl[sel]->dwval |= (interrupt<<16)&0xffff0000;
}

void cci_int_disable(unsigned int sel, enum cci_int_sel interrupt)
{
  cci_int_ctrl[sel]->dwval &= (~(interrupt<<16))&0xffff0000;
}

void CCI_INLINE_FUNC cci_int_get_status(unsigned int sel, struct cci_int_status *status)
{
	status->complete = cci_int_ctrl[sel]->bits.s_tran_com_pd;
	status->error		 = cci_int_ctrl[sel]->bits.s_tran_err_pd;
}

void CCI_INLINE_FUNC cci_int_clear_status(unsigned int sel, enum cci_int_sel interrupt)
{
	cci_int_ctrl[sel]->dwval &= 0xffff0000;
	cci_int_ctrl[sel]->dwval |= interrupt;
}

enum cci_bus_status CCI_INLINE_FUNC cci_get_bus_status(unsigned int sel)
{
	return cci_ctrl[sel]->bits.cci_sta;
}

void cci_get_line_status(unsigned int sel, struct cci_line_status *status)
{
	status->cci_sck = cci_bus_ctrl[sel]->bits.scl_sta;
	status->cci_sda = cci_bus_ctrl[sel]->bits.sda_sta;
}

void cci_pad_en(unsigned int sel)
{
	cci_bus_ctrl[sel]->bits.sda_pen = 1;
	cci_bus_ctrl[sel]->bits.scl_pen = 1;
}

void cci_stop(unsigned int sel)
{
	cci_bus_ctrl[sel]->bits.scl_moe = 1;
	cci_bus_ctrl[sel]->bits.sda_moe = 1;
	cci_bus_ctrl[sel]->bits.scl_mov = 1;
	csi_cci_udelay(5);
	cci_bus_ctrl[sel]->bits.sda_mov = 0;
	csi_cci_udelay(5);
	cci_bus_ctrl[sel]->bits.scl_moe = 0;
	cci_bus_ctrl[sel]->bits.sda_moe = 0;
}

void cci_sck_cycles(unsigned int sel, unsigned int cycle_times)
{
	cci_bus_ctrl[sel]->bits.scl_moe = 1;
	cci_bus_ctrl[sel]->bits.sda_moe = 1;
	while(cycle_times)
	{
		cci_bus_ctrl[sel]->bits.scl_mov = 1;
		csi_cci_udelay(5);
		cci_bus_ctrl[sel]->bits.scl_mov = 0;
		csi_cci_udelay(5);
		cycle_times--;
	}
	cci_bus_ctrl[sel]->bits.scl_moe = 0;
	cci_bus_ctrl[sel]->bits.sda_moe = 0;
}

void cci_print_info(unsigned int sel)
{
	unsigned int data;
	unsigned int i=0;
	data=cci_ctrl[sel]->dwval;
	printk("Print ctrl 0x%p=0x%x\n",cci_ctrl[sel], data);
	data=cci_fmt[sel]->dwval;
	printk("Print fmt 0x%p=0x%x\n",cci_fmt[sel], data);
	data=cci_cfg[sel]->dwval;
	printk("Print cfg 0x%p=0x%x\n",cci_cfg[sel], data);
	data=cci_bus_ctrl[sel]->dwval;
	printk("Print cci_bus_ctrl 0x%p=0x%x\n",cci_bus_ctrl[sel], data);
	printk("Print CCI_FIFO\n");
	for(i=0;i<16;i+=4)
	{
		data=(cci_fifo_acc[sel]+i)->dwval;
		printk("0x%p=0x%x\n",cci_fifo_acc[sel]+i, data);
	}
}


