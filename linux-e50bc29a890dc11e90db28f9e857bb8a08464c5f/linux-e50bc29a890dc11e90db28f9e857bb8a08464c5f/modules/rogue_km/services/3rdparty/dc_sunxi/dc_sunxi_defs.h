#ifndef _DC_SUNXI_DEFS_
#define _DC_SUNXI_DEFS_

#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/fb.h>
#include "kerneldisplay.h"
#include "imgpixfmts_km.h"
#include "pvrmodule.h"   /* for MODULE_LICENSE() */
#include "drv_display.h"

#define  DRVNAME					              "dc_sunxi"
#define  SUNXI_MAX_COMMANDS_IN_FLIGHT                      2

#define  MAX_DISPLAY_NAME_SIZE       32
#define  FALLBACK_REFRESH_RATE		 60
#define  FALLBACK_DPI		        160
#ifndef  PAGE_SIZE
#define  PAGE_SIZE                 4096
#endif
#ifndef  PAGE_SHIFT
#define  PAGE_SHIFT                  12
#endif
#define  AW_BIT_FLAG_LAYER_TYPE_FRAMEBUFFERTARGET   (0x1 << 0) 
#define  AW_BIT_FLAG_LAYER_ALONG_TO_PRIMARY_DISPLAY (0x1 << 1) 
#define  AW_BIT_FLAG_LAYER_ALONG_TO_EXT_DISPLAY_1   (0x1 << 2)


#define  AW_BIT_FLAG_FORCE_UI_GLES                  (0x1 << 3) // Ç¿ÖÆUI Layer ×ßGLES Í¼²ã¡£


#define AW_SUN9IW1P1_FB_PIXELFORMAT IMG_PIXFMT_R8G8B8A8_UNORM //used TO set allwinner LCD framebuffer device framebuffer format .

#define AW_DISPLAY_MAX_NUM  2
#define AW_PRIMARY_DISPLAY  0
#define AW_EXTERNEL_DISPLAY 1

#define AW_DEBUG_ENABLE_ALPHA_MODE_2

typedef enum
{
    AW_DISP_NONE          = -1,
	AW_OUTPUT_TO_DISPLAY0 = 0,
	AW_OUTPUT_TO_DISPLAY1 = 1,
	AW_OUTPUT_TO_DISPLAY2 = 2,
}AW_DISPLAY_STATUS;

 
typedef enum{
    HAL_PIXEL_FORMAT_RGBA_8888 = 1,
    HAL_PIXEL_FORMAT_RGBX_8888 = 2,
    HAL_PIXEL_FORMAT_RGB_888   = 3,
    HAL_PIXEL_FORMAT_RGB_565   = 4,
    HAL_PIXEL_FORMAT_BGRA_8888 = 5,
    HAL_PIXEL_FORMAT_YCrCb_420_SP = 0x11,
    HAL_PIXEL_FORMAT_NV12         = 0x100, //NV12
    HAL_PIXEL_FORMAT_YV12         = 0x32315659,
    HAL_PIXEL_FORMAT_BGRX_8888  = 0x1ff,//img internal used fmt. aw de support this format
}HAL_FMT;//android hal fmts which aw de support.

typedef enum
{
    PVRHWC_BLENDING_NONE        = 0,
	PVRHWC_BLENDING_PREMULT     = 1,
	PVRHWC_BLENDING_COVERAGE    = 2,
}HWC_BLENDING_TYPE;

typedef struct
{
    IMG_UINT32     left;
    IMG_UINT32     top;
    IMG_UINT32     right;
    IMG_UINT32     bottom;
}hwc_rect_t;

typedef struct
{
    int                 layer_num[3];
	disp_layer_info     layer_info[3][4];
	IMG_HANDLE          hConfigData;
}setup_dispc_data_t;

struct disp_composer_ops
{
	int (*get_screen_width)(u32 screen_id);
	int (*get_screen_height)(u32 screen_id);
	int (*get_output_type)(u32 screen_id);//add new
	int (*hdmi_enable)(u32 screen_id);
	int (*hdmi_disable)(u32 screen_id);
	int (*hdmi_set_mode)(u32 screen_id,  disp_tv_mode mode);
	int (*hdmi_get_mode)(u32 screen_id);
	int (*hdmi_check_support_mode)(u32 screen_id,  u8 mode);
	int (*is_support_scaler_layer)(unsigned int screen_id,unsigned int src_w, unsigned int src_h,unsigned int out_w, unsigned int out_h);//0 is not support, 1 is support
	int (*dispc_gralloc_queue)(setup_dispc_data_t *psDispcData, int ui32DispcDataLength, void (*cb_fn)(void *));
};
extern int disp_get_composer_ops(struct disp_composer_ops *ops);

typedef struct
{
    IMG_PIXFMT    eFrameBufferPixlFormat;
	IMG_UINT32    disp0_width;
	IMG_UINT32    disp0_height;
	IMG_UINT32    disp0_percent;
	IMG_UINT32    disp2_width;
	IMG_UINT32    disp2_height;
	IMG_UINT32    disp2_percent;
    struct disp_composer_ops    disp_function_table;// interface get from de driver
}DC_SUNXI_DISP_PRIVATE_DATA;

typedef struct
{
    IMG_HANDLE                     hSrvHandle;

    struct fb_info                *psLINFBInfo;

    DC_SUNXI_DISP_PRIVATE_DATA     sDePrivateData;

    setup_dispc_data_t             sDataToDeDriver;
}DC_SUNXI_DEVICE;

typedef struct
{
    DC_SUNXI_DEVICE                *psDevice;
}DC_SUNXI_CONTEXT;

typedef struct
{
    DC_SUNXI_CONTEXT     *psDeviceContext;
    IMG_UINT32            ePixFormat;
    IMG_UINT32            ui32NumPlanes;
    IMG_UINT32            ui32Width;
    IMG_UINT32            ui32Height;
    IMG_UINT32            ui32ByteStride;
    IMG_UINT32            ui32NumPages;

    IMG_HANDLE            hImport;
    IMG_UINT32      	  ui32PhyAddrs;
}DC_SUNXI_BUFFER;

#endif
