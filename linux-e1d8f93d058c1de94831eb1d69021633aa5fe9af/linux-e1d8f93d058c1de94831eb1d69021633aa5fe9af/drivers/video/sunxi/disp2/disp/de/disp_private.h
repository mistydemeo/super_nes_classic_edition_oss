#ifndef _DISP_PRIVATE_H_
#define _DISP_PRIVATE_H_
#include "bsp_display.h"

#if defined(__LINUX_PLAT__)
#define DE_INF __inf
#define DE_MSG __msg
#define DE_WRN __wrn
#define DE_DBG __debug
#define OSAL_IRQ_RETURN IRQ_HANDLED
#else
#define DE_INF(msg...)
#define DE_MSG __msg
#define DE_WRN __wrn
#define DE_DBG __debug
#ifndef OSAL_IRQ_RETURN
#define OSAL_IRQ_RETURN DIS_SUCCESS
#endif
#endif

typedef enum
{
	DIS_SUCCESS=0,
	DIS_FAIL=-1,
	DIS_PARA_FAILED=-2,
	DIS_PRIO_ERROR=-3,
	DIS_OBJ_NOT_INITED=-4,
	DIS_NOT_SUPPORT=-5,
	DIS_NO_RES=-6,
	DIS_OBJ_COLLISION=-7,
	DIS_DEV_NOT_INITED=-8,
	DIS_DEV_SRAM_COLLISION=-9,
	DIS_TASK_ERROR = -10,
	DIS_PRIO_COLLSION = -11
}disp_return_value;

/*basic data information definition*/
enum disp_layer_feat {
	DISP_LAYER_FEAT_GLOBAL_ALPHA        = 1 << 0,
	DISP_LAYER_FEAT_PIXEL_ALPHA         = 1 << 1,
	DISP_LAYER_FEAT_GLOBAL_PIXEL_ALPHA  = 1 << 2,
	DISP_LAYER_FEAT_PRE_MULT_ALPHA      = 1 << 3,
	DISP_LAYER_FEAT_COLOR_KEY           = 1 << 4,
	DISP_LAYER_FEAT_ZORDER              = 1 << 5,
	DISP_LAYER_FEAT_POS                 = 1 << 6,
	DISP_LAYER_FEAT_3D                  = 1 << 7,
	DISP_LAYER_FEAT_SCALE               = 1 << 8,
	DISP_LAYER_FEAT_DE_INTERLACE        = 1 << 9,
	DISP_LAYER_FEAT_COLOR_ENHANCE       = 1 << 10,
	DISP_LAYER_FEAT_DETAIL_ENHANCE      = 1 << 11,
};

typedef enum
{
	DISP_PIXEL_TYPE_RGB=0x0,
	DISP_PIXEL_TYPE_YUV=0x1,
}disp_pixel_type;

typedef enum
{
	LAYER_ATTR_DIRTY       = 0x00000001,
	LAYER_VI_FC_DIRTY      = 0x00000002,
	LAYER_HADDR_DIRTY      = 0x00000004,
	LAYER_SIZE_DIRTY       = 0x00000008,
	BLEND_ENABLE_DIRTY     = 0x00000010,
	BLEND_ATTR_DIRTY       = 0x00000020,
	BLEND_CTL_DIRTY        = 0x00000040,
	BLEND_OUT_DIRTY        = 0x00000080,
	LAYER_ALL_DIRTY        = 0x000000ff,
}disp_layer_dirty_flags;

typedef enum
{
	MANAGER_ENABLE_DIRTY     = 0x00000001,
	MANAGER_CK_DIRTY         = 0x00000002,
	MANAGER_BACK_COLOR_DIRTY = 0x00000004,
	MANAGER_SIZE_DIRTY       = 0x00000008,
	MANAGER_ALL_DIRTY        = 0x0000000f,
}disp_manager_dirty_flags;

struct disp_layer_config_data
{
	disp_layer_config config;
	disp_layer_dirty_flags flag;
};

struct disp_manager_info {
	disp_color back_color;
	disp_colorkey ck;
	disp_rectsz size;
	bool enable;
};

struct disp_manager_data
{
	struct disp_manager_info config;
	disp_manager_dirty_flags flag;
};

struct disp_clk_info
{
		u32                     clk;
		u32                     clk_div;
		u32                     h_clk;
		u32                     clk_src;
		u32                     clk_div2;

		u32                     clk_p;
		u32                     clk_div_p;
		u32                     h_clk_p;
		u32                     clk_src_p;

		u32                     ahb_clk;
		u32                     h_ahb_clk;
		u32                     dram_clk;
		u32                     h_dram_clk;

		bool                    enabled;
};

struct disp_lcd {
	struct disp_device dispdev;

	s32 (*backlight_enable)(struct disp_lcd *lcd);
	s32 (*backlight_disable)(struct disp_lcd *lcd);
#if 0
	s32 (*pwm_enable)(struct disp_lcd *lcd);
	s32 (*pwm_disable)(struct disp_lcd *lcd);
#endif
	s32 (*power_enable)(struct disp_lcd *lcd, u32 power_id);
	s32 (*power_disable)(struct disp_lcd *lcd, u32 power_id);
	s32 (*tcon_enable)(struct disp_lcd *lcd);
	s32 (*tcon_disable)(struct disp_lcd *lcd);
	s32 (*set_bright)(struct disp_lcd *lcd, u32 birhgt);
	s32 (*get_bright)(struct disp_lcd *lcd);
	s32 (*set_bright_dimming)(struct disp_lcd *lcd, u32 *birhgt);
	disp_lcd_flow *(*get_open_flow)(struct disp_lcd *lcd);
	disp_lcd_flow *(*get_close_flow)(struct disp_lcd *lcd);
	s32 (*enable)(struct disp_lcd *lcd);
	s32 (*disable)(struct disp_lcd *lcd);
	s32 (*pre_enable)(struct disp_lcd *lcd);
	s32 (*post_enable)(struct disp_lcd *lcd);
	s32 (*pre_disable)(struct disp_lcd *lcd);
	s32 (*post_disable)(struct disp_lcd *lcd);
	s32 (*set_panel_func)(struct disp_lcd *lcd, disp_lcd_panel_fun * lcd_cfg);
	s32 (*get_panel_driver_name)(struct disp_lcd *lcd, char *name);
	s32 (*pin_cfg)(struct disp_lcd *lcd, u32 bon);
	//s32 (*set_open_func)(struct disp_lcd* lcd, LCD_FUNC func, u32 delay);
	//s32 (*set_close_func)(struct disp_lcd* lcd, LCD_FUNC func, u32 delay);
	s32 (*set_gamma_tbl)(struct disp_lcd* lcd, u32 *tbl, u32 size);
	s32 (*enable_gamma)(struct disp_lcd* lcd);
	s32 (*disable_gamma)(struct disp_lcd* lcd);
	s32 (*register_panel)(struct disp_lcd* lcd, sunxi_lcd_panel *panel);
};

extern struct disp_device* disp_get_lcd(u32 disp);

struct disp_hdmi {
	struct disp_device dispdev;

	s32 (*check_support_mode)(struct disp_hdmi* hdmi, u8 mode);
	s32 (*set_func)(struct disp_hdmi* hdmi, disp_hdmi_func* func);
	s32 (*set_video_info)(struct disp_hdmi* hdmi, disp_video_timings* video_info);
	s32 (*read_edid)(struct disp_hdmi* hdmi, u8 *buf, u32 len);
};

extern struct disp_hdmi* disp_get_hdmi(u32 disp);

extern struct disp_manager* disp_get_layer_manager(u32 disp);

struct disp_layer {
	/* data fields */
	char name[16];
	u32 disp;
	u32 chn;
	u32 id;

	enum disp_layer_feat caps;
	struct disp_manager *manager;
	struct list_head list;
	void* data;

	/* function fileds */

	s32 (*is_support_caps)(struct disp_layer* layer, enum disp_layer_feat caps);
	s32 (*is_support_format)(struct disp_layer* layer, disp_pixel_format fmt);
	s32 (*set_manager)(struct disp_layer* layer, struct disp_manager *mgr);
	s32 (*unset_manager)(struct disp_layer* layer);

	s32 (*check)(struct disp_layer* layer, disp_layer_config *config);
	s32 (*save_and_dirty_check)(struct disp_layer* layer, disp_layer_config *config);
	s32 (*get_config)(struct disp_layer* layer, disp_layer_config *config);
	s32 (*apply)(struct disp_layer* layer);
	s32 (*force_apply)(struct disp_layer* layer);
	s32 (*is_dirty)(struct disp_layer* layer);

	/* init: NULL
	 * exit: NULL
	 */
	s32 (*init)(struct disp_layer *layer);
	s32 (*exit)(struct disp_layer *layer);

	s32 (*get_frame_id)(struct disp_layer *layer);

	s32 (*dump)(struct disp_layer* layer, char *buf);
};

struct disp_smbl {
	/* static fields */
	char *name;
	u32 disp;
	struct disp_manager *manager;

	/*
	 * The following functions do not block:
	 *
	 * is_enabled
	 * set_layer_info
	 * get_layer_info
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_smbl *smbl);
	s32 (*disable)(struct disp_smbl *smbl);
	bool (*is_enabled)(struct disp_smbl *smbl);
	s32 (*set_manager)(struct disp_smbl* smbl, struct disp_manager *mgr);
	s32 (*unset_manager)(struct disp_smbl* smbl, struct disp_manager *mgr);

	/* init: NULL
	 * exit: NULL
	 */
	s32 (*init)(struct disp_smbl *smbl);
	s32 (*exit)(struct disp_smbl *smbl);

	s32 (*apply)(struct disp_smbl *smbl);
	s32 (*update_regs)(struct disp_smbl *smbl);
	s32 (*force_apply)(struct disp_smbl *smbl);
	s32 (*sync)(struct disp_smbl *smbl);

	s32 (*set_window)(struct disp_smbl* smbl, disp_rect *window);
	s32 (*get_window)(struct disp_smbl* smbl, disp_rect *window);
};

extern struct disp_layer* disp_get_layer(u32 disp, u32 chn, u32 layer_id);
extern struct disp_layer* disp_get_layer_1(u32 disp, u32 layer_id);
extern struct disp_smbl* disp_get_smbl(u32 disp);
extern struct disp_smcl* disp_get_smcl(u32 disp);

extern s32 disp_delay_ms(u32 ms);
extern s32 disp_delay_us(u32 us);
extern s32 disp_init_lcd(disp_bsp_init_para * para);
extern s32 disp_init_feat(void);
extern s32 disp_init_mgr(disp_bsp_init_para * para);

//FIXME, add micro
#include "./lowlevel_sun8iw6/disp_al.h"
#include "disp_device.h"

u32 dump_layer_config(struct disp_layer_config_data *data);

#endif

