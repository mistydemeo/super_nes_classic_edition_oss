#include "dc_sunxi_defs.h"

/*For Debug*/
#ifdef DEBUG
#define DC_DEBUG
#endif

#ifdef DC_HWC_FALL_BACK_GPU
#define DC_ONLY_FRAMEBUFFERTARGET  //todo:
#endif

#ifdef DC_NEED_DEBUG_ENTRY
#define DC_ENTRYPOINT_DEBUG
#endif

#ifdef  DC_ENTRYPOINT_DEBUG
#define DC_ENTERED()        printk("%s: entered function. line:%d \n", __FUNCTION__,__LINE__)
#define DC_EXITED()         printk("%s: left    function. line:%d \n", __FUNCTION__,__LINE__)
#else
#define DC_ENTERED()
#define DC_EXITED()
#endif 

#ifdef DC_DEBUG
#define DC_DPF(x,...)       printk("###DC DEBUG INFO: [func:%s][line:%d] "x,__FUNCTION__,__LINE__,##__VA_ARGS__)
#else
#define DC_DPF(x,...)
#endif

#if 1 
#define DC_TAG(x,...)       printk("###DC_SUNXI TAG LINE: [func:%s][line:%d] "x,__FUNCTION__, __LINE__,##__VA_ARGS__)
#define DC_E_DPF(x,...)     printk("###DC ERROR: [func:%s][line:%d] "x,__FUNCTION__, __LINE__,##__VA_ARGS__)
#endif

static DC_SUNXI_DEVICE         *gps_Disp_Device = NULL;

#include <linux/spinlock.h>

typedef struct _S_CONFIG_DATA_
{
    struct _S_CONFIG_DATA_ *pNextConfig;
    IMG_HANDLE              hConfigData;
	IMG_UINT32              ui32FrameID;
}S_CONFIG_DATA;

typedef struct _CONFIG_DATA_QUEUE_
{
    S_CONFIG_DATA    *head;            // Configure 时add_queue 保存的hConfigData
    spinlock_t        head_lock;       // 控制  head 的读写
}CONFIG_DATA_QUEUE;

static CONFIG_DATA_QUEUE  *gps_config_queue = NULL;

static S_CONFIG_DATA  *add_queue(IMG_HANDLE hConfigData,IMG_UINT32 ui32FrameID)
{
    S_CONFIG_DATA *psConfig;

	if(!hConfigData)
	{
		return NULL;
	}
	
	psConfig = kmalloc(sizeof(S_CONFIG_DATA), GFP_KERNEL);
	if(!psConfig)
		return NULL;
	psConfig->hConfigData = hConfigData;
	psConfig->ui32FrameID = ui32FrameID;
	psConfig->pNextConfig = NULL;
	
    spin_lock(&gps_config_queue->head_lock);
	psConfig->pNextConfig = gps_config_queue->head;
	gps_config_queue->head = psConfig;
	spin_unlock(&gps_config_queue->head_lock);

	//DC_DPF("Frame %d add to queue\n",ui32FrameID);

	return psConfig;
}

static void remove_from_queue(IMG_HANDLE hOldConfigData)
{    
	S_CONFIG_DATA   *head = NULL;
	S_CONFIG_DATA   *tail = NULL;
	S_CONFIG_DATA   *pMove = NULL;

	S_CONFIG_DATA   *psOldConfigToFree = NULL;

	DC_ENTERED();

	spin_lock(&gps_config_queue->head_lock);
	head = gps_config_queue->head;
	spin_unlock(&gps_config_queue->head_lock);

    pMove = head;
	while(pMove)
	{
	    if(pMove->hConfigData == hOldConfigData)
	    {
			psOldConfigToFree = pMove;
			if(tail != NULL)
			{
				tail->pNextConfig = NULL;//disconnect queue.
			}
			else
			{
			    //which means (psOldConfigToFree == head) is true
			    //then we will try to free entire queue
				spin_lock(&gps_config_queue->head_lock);
				gps_config_queue->head = NULL;
				spin_unlock(&gps_config_queue->head_lock);
			}
		    break;
	    }

		//DC_DPF("Frame %d still alive\n",pMove->ui32FrameID);
		
		tail = pMove;
		pMove = pMove->pNextConfig;
	}

	if(!psOldConfigToFree)
	{
		DC_E_DPF(":can not find valide configdata to free \n");
		return;
	}

    //real free work:
    //TODO: 必须从队列的尾巴开始释放。
#if 0
    {
        S_CONFIG_DATA *pPrev = NULL;
		tail = NULL;
		while(psOldConfigToFree->pNextConfig)
		{
		    pPrev = psOldConfigToFree;
		    tail  = psOldConfigToFree;

			while(tail->pNextConfig)
			{
			    pPrev = tail;
			    tail  = tail->pNextConfig;
			}

			pPrev->pNextConfig = NULL;
			//DC_E_DPF("pPrev->\n");
			DCDisplayConfigurationRetired(tail->hConfigData);
			kfree(tail);
		}

		DCDisplayConfigurationRetired(psOldConfigToFree->hConfigData);
		kfree(psOldConfigToFree);
    }
#else
	while(psOldConfigToFree)
	{
		S_CONFIG_DATA *pNextToFree = psOldConfigToFree->pNextConfig;

		if(psOldConfigToFree->hConfigData)
		{
		    //DC_DPF("Frame %d going to die\n",psOldConfigToFree->ui32FrameID);
			DCDisplayConfigurationRetired(psOldConfigToFree->hConfigData);
			//DC_DPF("Frame %d destroyed\n",psOldConfigToFree->ui32FrameID);
		}
		else
		{
			DC_E_DPF(":Try to free NULL\n");
			break;
		}
		psOldConfigToFree->hConfigData = NULL;
		kfree(psOldConfigToFree);
		psOldConfigToFree = pNextToFree;
	}
#endif

	DC_EXITED();
}

static void aw_de_vysnc_callback(void *arg)// 中断响应
{ 
    S_CONFIG_DATA *psConfigData = arg;
    DC_ENTERED();
	//DC_E_DPF("Display Return %d\n",psConfigData->ui32FrameID);
    remove_from_queue(psConfigData->hConfigData);
    DC_EXITED();
}

static inline IMG_UINT32 IMGPixFmtGetBPP(IMG_PIXFMT ePixFormat)
{
    switch(ePixFormat)
    {
        case IMG_PIXFMT_R8G8B8X8_UNORM:
        case IMG_PIXFMT_R8G8B8A8_UNORM:
        case IMG_PIXFMT_B8G8R8A8_UNORM:
		case IMG_PIXFMT_B8G8R8X8_UNORM:
            return 32;
		case IMG_PIXFMT_B5G6R5_UNORM:
		    return 16;
		case IMG_PIXFMT_YVU420_2PLANE://NV21
		case IMG_PIXFMT_YUV420_2PLANE://NV12
		case IMG_PIXFMT_YVU420_3PLANE://YV12
		    return 12;
		default:
			return 0;
	}
}

static int aw_buffer_configuration_is_correct(IMG_UINT32 ui32NumPlanes,DC_BUFFER_IMPORT_INFO *psSurfInfo)
{
    int i = 0;
	
    if(ui32NumPlanes > 3
        ||
       ui32NumPlanes == 0)
    {
        DC_E_DPF("ui32NumPlanes : %u \n",ui32NumPlanes);
        goto err_out;
    }

	if(psSurfInfo == NULL)
	{
	    DC_E_DPF("psSurfInfo : NULL \n");
		goto err_out;
	}

    if(IMGPixFmtGetBPP(psSurfInfo->ePixFormat) == 0)
    {
        DC_E_DPF("ePixFormat: %u \n",psSurfInfo->ePixFormat);
        goto err_out;
    }

    if(!psSurfInfo->ui32BPP
        ||
        !psSurfInfo->ui32ByteStride[0])
    {
       DC_E_DPF("ui32BPP: %u, ui32ByteStride: %u \n",psSurfInfo->ui32BPP,psSurfInfo->ui32ByteStride[0]);
	   goto err_out;
    }

    for(i = 0; i < ui32NumPlanes;i++)
    {
        if(!psSurfInfo->ui32Width[i] ||
            !psSurfInfo->ui32Height[i] ||
            ((psSurfInfo->ui32Width[i] * psSurfInfo->ui32BPP) > psSurfInfo->ui32ByteStride[i]))
        {
            DC_E_DPF("[%d], w: %u, h: %u, Bpp: %u, bytestride: %u \n",
				        i,
				        psSurfInfo->ui32Width[i], psSurfInfo->ui32Height[i],
				        psSurfInfo->ui32BPP,psSurfInfo->ui32ByteStride[i]);
            goto err_out;
        }
    }

    return 0; // ok
err_out:
    return -1;// not correctly.
}

static int aw_layer_is_intersect(PVRSRV_SURFACE_RECT* psLayer1, PVRSRV_SURFACE_RECT* psLayer2)
{
    int mid_x0, mid_y0, mid_x1, mid_y1;
    int mid_diff_x, mid_diff_y;
    int sum_width, sum_height;

	if(psLayer1 == NULL 
		|| psLayer2 == NULL)
	{
		DC_E_DPF("psLayer1: %p, psLayer2:%p \n",psLayer1,psLayer2);
		return 0;
	}

    mid_x0 = psLayer1->i32XOffset + (psLayer1->sDims.ui32Width >> 1);
    mid_y0 = psLayer1->i32YOffset + (psLayer1->sDims.ui32Height >> 1);
    mid_x1 = psLayer2->i32XOffset + (psLayer2->sDims.ui32Width >> 1);
    mid_y1 = psLayer2->i32YOffset + (psLayer2->sDims.ui32Height >> 1);

    mid_diff_x = (mid_x0 >= mid_x1)? (mid_x0 - mid_x1):(mid_x1 - mid_x0);
    mid_diff_y = (mid_y0 >= mid_y1)? (mid_y0 - mid_y1):(mid_y1 - mid_y0);

    sum_width = psLayer1->sDims.ui32Width + psLayer2->sDims.ui32Width;
    sum_height = psLayer1->sDims.ui32Height + psLayer2->sDims.ui32Height;

    if(mid_diff_x < (sum_width>>1) && mid_diff_y < (sum_height>>1))
    {
        return 1;// intersect is true
    }
	
    return 0;    // not intersect
}

static IMG_UINT32 aw_imgfmt_to_halfmt(IMG_UINT32 ui32Imgfmt)
{
    switch(ui32Imgfmt)
    {
        case IMG_PIXFMT_R8G8B8A8_UNORM:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        case IMG_PIXFMT_R8G8B8X8_UNORM:
            return HAL_PIXEL_FORMAT_RGBX_8888;
		case IMG_PIXFMT_B8G8R8X8_UNORM:
			return HAL_PIXEL_FORMAT_BGRX_8888;
        case IMG_PIXFMT_B5G6R5_UNORM:
            return HAL_PIXEL_FORMAT_RGB_565;
        case IMG_PIXFMT_B8G8R8A8_UNORM:
            return HAL_PIXEL_FORMAT_BGRA_8888;
        case IMG_PIXFMT_YVU420_2PLANE:
            return HAL_PIXEL_FORMAT_YCrCb_420_SP;//NV21
        case IMG_PIXFMT_YUV420_2PLANE:
            return HAL_PIXEL_FORMAT_NV12;        //NV12
		case IMG_PIXFMT_YVU420_3PLANE:
			return HAL_PIXEL_FORMAT_YV12;        //YV12
        default:
        {
			DC_E_DPF(":meet a unknown format: 0x%x(%d) \n",ui32Imgfmt,ui32Imgfmt);
            return -1;
        }
    }
}



static int aw_layer_is_de_supported_yuv_format(DC_SUNXI_BUFFER *psBuffer)
{ 
    IMG_UINT32 ui32HalFormat = aw_imgfmt_to_halfmt(psBuffer->ePixFormat);
    if(ui32HalFormat == HAL_PIXEL_FORMAT_NV12
		|| ui32HalFormat == HAL_PIXEL_FORMAT_YV12
		|| ui32HalFormat == HAL_PIXEL_FORMAT_YCrCb_420_SP)//NV21
        return 1; 
	return 0;
}


static int aw_layer_is_acceptable(PVRSRV_SURFACE_CONFIG_INFO *psLayerConfigInfo,DC_SUNXI_BUFFER *psBuffer)//IMG_HANDLE *phBuffer)
{
	if(!psBuffer)
	{
		DC_E_DPF(":meet a NULL Buffer \n");
		return 0;
	}
	
    if(IMGPixFmtGetBPP(psBuffer->ePixFormat) == 0)
    {
        DC_DPF(":Laye fmt is not acceptable. img_fmt: %d \n",
                psBuffer->ePixFormat);
        return 0;
    }

    if(psLayerConfigInfo->ui32Transform != 0)// display engine can not handle any type transform op...
    {
		DC_DPF(":Transform layer is not acceptable . tansforma: %d \n",
		         psLayerConfigInfo->ui32Transform);
        return 0;
    }

    // 强制UI Layer 走GLES 图层。
    // 这种情况下，FBT 可能需要放缩。
    // 只有FBT 和 YUV Layer 可以通过检测。 
	if(psLayerConfigInfo->ui32Custom & AW_BIT_FLAG_FORCE_UI_GLES)
	{
	    if(psLayerConfigInfo->ui32Custom & AW_BIT_FLAG_LAYER_TYPE_FRAMEBUFFERTARGET)
			return 1;

		if(aw_layer_is_de_supported_yuv_format(psBuffer))
			return 1;

		return 0;
	}

    return 1;
}

static int aw_layer_is_acceptable_scaled_layer(DC_SUNXI_DEVICE *pDev,
                                               PVRSRV_SURFACE_CONFIG_INFO *psLayerConfigInfo,
                                               //IMG_HANDLE *phBuffer,
                                               DC_SUNXI_BUFFER *psBuffer,
                                               int nDisp)
{
    int bCanUseFe = 0;
    bCanUseFe = pDev->sDePrivateData.disp_function_table.is_support_scaler_layer(
                nDisp,
                psLayerConfigInfo->sCrop.sDims.ui32Width,
                psLayerConfigInfo->sCrop.sDims.ui32Height,
                psLayerConfigInfo->sDisplay.sDims.ui32Width,
                psLayerConfigInfo->sDisplay.sDims.ui32Height);

    return bCanUseFe;// 0 : display engine can Not handle this scaled layer
                     // 1 : display engine can handle this scaled layer
}

static int aw_layer_is_scaled(PVRSRV_SURFACE_CONFIG_INFO *psLayerConfigInfo)
{
    IMG_UINT32 dispFrame_width,dispFrame_height;
    IMG_UINT32 src_width,src_height;

    dispFrame_width  = psLayerConfigInfo->sDisplay.sDims.ui32Width;
    dispFrame_height = psLayerConfigInfo->sDisplay.sDims.ui32Height;
    src_width = psLayerConfigInfo->sCrop.sDims.ui32Width;
    src_height = psLayerConfigInfo->sCrop.sDims.ui32Height;

    return (dispFrame_width != src_width)
            || ( dispFrame_height != src_height);// return 1 means : layer is scaled.
}

static int aw_layer_is_alpha_layer(PVRSRV_SURFACE_CONFIG_INFO *psLayerConfigInfo)
{
    return psLayerConfigInfo->eBlendType != PVRHWC_BLENDING_NONE;// 1: is alpha layer , 0 : not alpha layer
}

static int aw_setup_disp_private_data(DC_SUNXI_DEVICE *psDevInfo)
{
    DC_SUNXI_DISP_PRIVATE_DATA *psDePrivateData;

    psDePrivateData = &psDevInfo->sDePrivateData;

    memset(&psDePrivateData->disp_function_table,0,sizeof(struct disp_composer_ops));
    disp_get_composer_ops(&psDePrivateData->disp_function_table);

    /*
    if(psDePrivateData->disp_function_table.get_output_type != 0)
    {
    	DC_E_DPF("###aw_setup_disp_private_data\n");
        psDePrivateData->out_type[AW_PRIMARY_DISPLAY] 
			= psDePrivateData->disp_function_table.get_output_type(AW_PRIMARY_DISPLAY);
		psDePrivateData->out_type[AW_EXTERNEL_DISPLAY]
			= psDePrivateData->disp_function_table.get_output_type(AW_EXTERNEL_DISPLAY);

	    DC_E_DPF("### get_out_type success, set outtype: LCD type:[%x] HDMI type:[%x]\n",
			psDePrivateData->out_type[AW_PRIMARY_DISPLAY],
			psDePrivateData->out_type[AW_EXTERNEL_DISPLAY]);
    }
	else
	{
	    psDePrivateData->out_type[AW_PRIMARY_DISPLAY] = DISP_OUTPUT_TYPE_LCD;
		if(psDePrivateData->disp_function_table.hdmi_enable(AW_EXTERNEL_DISPLAY) == 1)
			psDePrivateData->out_type[AW_EXTERNEL_DISPLAY] = DISP_OUTPUT_TYPE_HDMI;
		else
			psDePrivateData->out_type[AW_EXTERNEL_DISPLAY] = DISP_OUTPUT_TYPE_NONE;
	    DC_E_DPF("# get_out_type is NULL, set default outtype: LCD type:[%x] HDMI type:[%x]\n",
			psDePrivateData->out_type[AW_PRIMARY_DISPLAY],
			psDePrivateData->out_type[AW_EXTERNEL_DISPLAY]);
	}
	*/

    psDePrivateData->disp0_width   = psDePrivateData->disp_function_table.get_screen_width(0);
    psDePrivateData->disp0_height  = psDePrivateData->disp_function_table.get_screen_height(0);
    psDePrivateData->disp0_percent = 100;

	psDePrivateData->disp2_width   = psDePrivateData->disp_function_table.get_screen_width(2);
	psDePrivateData->disp2_height  = psDePrivateData->disp_function_table.get_screen_height(2);
	psDePrivateData->disp2_percent = 100;
	
    psDevInfo->sDePrivateData.eFrameBufferPixlFormat = AW_SUN9IW1P1_FB_PIXELFORMAT;

    DC_DPF(":disp0{w,h}={%u %u}. disp2{w h}={%u %u}\n",
            psDePrivateData->disp0_width,psDePrivateData->disp0_height,
            psDePrivateData->disp2_width,psDePrivateData->disp2_height);
    return 0;
failed_out:
    return -1;
}

static int aw_setup_de_layer(
    DC_SUNXI_DEVICE *pDev,
    PVRSRV_SURFACE_CONFIG_INFO *psSurfAttrib,
    DC_SUNXI_BUFFER *psBuffer,//IMG_HANDLE *phBuffer,
    int de_pipe_index,
    int layerZ,
    IMG_UINT32  disp_index,
    disp_layer_info *layer_info)
{
    int         iFormat;
    IMG_UINT32  display_width;
    IMG_UINT32  display_height;
    IMG_UINT32  display_percent;
	
	if(!psBuffer)
	{
	    DC_E_DPF(": meet a NULL Buffer: layerZ: %d \n",layerZ);
		return -1;
	}
    iFormat = aw_imgfmt_to_halfmt(psBuffer->ePixFormat);
	if(iFormat == -1)
	{
	    DC_E_DPF(":Unknown IMG Format:0x%x",psBuffer->ePixFormat);
	    return -1;
	}

    if(!psSurfAttrib)
    {
		DC_E_DPF(": meet a NULL SurfAttrib: layerZ:%d \n", layerZ);
		return -1;
    }

	switch(disp_index)
	{
	    case 0:
            display_width   = pDev->sDePrivateData.disp0_width;
            display_height  = pDev->sDePrivateData.disp0_height;
            display_percent = pDev->sDePrivateData.disp0_percent;
			break;
		case 1:
			display_width   = psSurfAttrib->hdmi_width;
            display_height  = psSurfAttrib->hdmi_height;
            display_percent = psSurfAttrib->hdmi_disp_percent;
			break;
		case 2:
			display_width = pDev->sDePrivateData.disp2_width;
			display_height  = pDev->sDePrivateData.disp2_height;
			display_percent = pDev->sDePrivateData.disp2_percent;
			break;
		default:
			DC_E_DPF("Output to where: disp[%d]\n",disp_index);
			return -1;
	}

	//DC_DPF("Disp[%d]. {w,h,percent}={%u,%u,%u}\n",disp_index,display_width,display_height,display_percent);	

    switch(iFormat)
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            layer_info->fb.format = DISP_FORMAT_ABGR_8888;
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            layer_info->fb.format = DISP_FORMAT_XBGR_8888;
            layer_info->alpha_mode = 1;
            layer_info->alpha_value = 0xff;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            layer_info->fb.format = DISP_FORMAT_BGR_888;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            layer_info->fb.format = DISP_FORMAT_RGB_565;
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            layer_info->fb.format = DISP_FORMAT_ARGB_8888;
            break;
		case HAL_PIXEL_FORMAT_BGRX_8888://add by chenyushuang 02-24
			layer_info->fb.format = DISP_FORMAT_XRGB_8888;
			layer_info->alpha_mode = 1;
			layer_info->alpha_value = 0xff;
			break;
        case HAL_PIXEL_FORMAT_YV12:
            layer_info->fb.format = DISP_FORMAT_YUV420_P;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            layer_info->fb.format = DISP_FORMAT_YUV420_SP_VUVU;
            break;
        case HAL_PIXEL_FORMAT_NV12:
            layer_info->fb.format = DISP_FORMAT_YUV420_SP_UVUV;
            break;
        default:
			DC_E_DPF(": meet unkown hal_format 0x%x(%d)to our display engine \n",
				       iFormat,iFormat);
            break;
    }
	
#ifdef AW_DEBUG_ENABLE_ALPHA_MODE_2
    if(layerZ != 0 && aw_layer_is_de_supported_yuv_format(psBuffer) != 1)
	{
	    if(psSurfAttrib->ui8PlaneAlpha < 0xff && aw_layer_is_alpha_layer(psSurfAttrib) == 1)
	    {   //both : point alpha + global alpha
	        layer_info->alpha_mode  = 2;
			layer_info->alpha_value = psSurfAttrib->ui8PlaneAlpha;

			DC_DPF("{z,pipe,alpha}={%u %u 0x%x} dst{x,y,w,h}={%u %u %u %u}",
				layerZ,de_pipe_index,layer_info->alpha_value,
				psSurfAttrib->sDisplay.i32XOffset,psSurfAttrib->sDisplay.i32YOffset,
				psSurfAttrib->sDisplay.sDims.ui32Width,psSurfAttrib->sDisplay.sDims.ui32Height);
	    }
		else if(psSurfAttrib->ui8PlaneAlpha < 0xff && aw_layer_is_alpha_layer(psSurfAttrib) != 1)
		{   //only global alpha
	        layer_info->alpha_mode  = 1;
			layer_info->alpha_value = psSurfAttrib->ui8PlaneAlpha;
		}
		else if(psSurfAttrib->ui8PlaneAlpha == 0xff && aw_layer_is_alpha_layer(psSurfAttrib) == 1)
		{   //only point alpha
	        layer_info->alpha_mode  = 0;
			layer_info->alpha_value = 0;//set global alpha = 0
		}
		else if(psSurfAttrib->ui8PlaneAlpha == 0xff && aw_layer_is_alpha_layer(psSurfAttrib) != 1)
		{   //no alpha
	        layer_info->alpha_mode  = 1;
			layer_info->alpha_value = 0xff;
		}
    }
	else if(layerZ == 0 && aw_layer_is_de_supported_yuv_format(psBuffer) != 1)//first layer do some special work: alpha mode should not be 0 except it is yuv layer.
    {
        layer_info->alpha_value = 0xff;
        if(aw_layer_is_alpha_layer(psSurfAttrib) == 1)
        {
            layer_info->alpha_mode = 2;
        }
		else
		{
		    layer_info->alpha_mode = 1;//global alpha
		}
    }
	else
	{
	    //For yuv layer , we dont need alpha, so set it alpha mode to 1, and set its global alpha value = 0xff
	    //TODO: Need to Test
	    layer_info->alpha_mode = 1;
		layer_info->alpha_value = 0xff;
	}
#else
    // for certain game debug,
    if(layerZ == 0)
    {
        if(aw_layer_is_de_supported_yuv_format(psBuffer) != 1)//for video scence
        {    //nv12 should never set alpha value.
            layer_info->alpha_mode = 1;//enable global alpha
            layer_info->alpha_value = 0xff;
        }
    }
	else
	{
		if(aw_layer_is_alpha_layer(psSurfAttrib) == 1)
		{   // alpha layer
		    layer_info->alpha_mode = 0;//pixel alpha. close gobal alpha
		    layer_info->alpha_value = 0;
		}
		else
		{  // not alpha layer.
		    layer_info->alpha_mode = 1;//enable gobal alpha
		    layer_info->alpha_value = 0xff;
		}
	}
#endif

    if(psSurfAttrib->eBlendType == PVRHWC_BLENDING_PREMULT)
    {
        layer_info->fb.pre_multiply = 1;
    }

    if(iFormat == HAL_PIXEL_FORMAT_YV12
        ||iFormat == HAL_PIXEL_FORMAT_YCrCb_420_SP)
    {
        layer_info->fb.size.width
            = (psBuffer->ui32Width + 15)/16*16;
    }
    else
    {
        layer_info->fb.size.width =
            (psBuffer->ui32Width + 31)/32*32;// pixels 
    }

    layer_info->fb.size.height = psBuffer->ui32Height;
    layer_info->fb.addr[0]     = psBuffer->ui32PhyAddrs;//first physical page addr.
    if(layer_info->fb.format == DISP_FORMAT_YUV420_P)
    {
        layer_info->fb.addr[2] = layer_info->fb.addr[0] +
                                layer_info->fb.size.width * layer_info->fb.size.height;
        layer_info->fb.addr[1] = layer_info->fb.addr[2] +
                                (layer_info->fb.size.width * layer_info->fb.size.height)/4;
    }
    else if(layer_info->fb.format == DISP_FORMAT_YUV420_SP_VUVU
        ||
        layer_info->fb.format == DISP_FORMAT_YUV420_SP_UVUV)
    {
        layer_info->fb.addr[1] = layer_info->fb.addr[0] +
            layer_info->fb.size.width * layer_info->fb.size.height;
    }

    //cal src window. (source framebuffer display window)
    layer_info->fb.src_win.x = psSurfAttrib->sCrop.i32XOffset;
    layer_info->fb.src_win.y = psSurfAttrib->sCrop.i32YOffset;
    layer_info->fb.src_win.width = psSurfAttrib->sCrop.sDims.ui32Width;
    layer_info->fb.src_win.height = psSurfAttrib->sCrop.sDims.ui32Height;
	layer_info->mode = DISP_LAYER_WORK_MODE_NORMAL;//default is normal layer (non-scale)

    //cal screen display window(depending physical panel dims restriction. such as display percent)
	if((psSurfAttrib->ui32Custom & AW_BIT_FLAG_FORCE_UI_GLES)
		&&psSurfAttrib->ui32Custom & AW_BIT_FLAG_LAYER_TYPE_FRAMEBUFFERTARGET)
	{
	    //for a80 box solution.
	    layer_info->screen_win.x = 0;
		layer_info->screen_win.y = 0;
		layer_info->screen_win.width = display_width;
		layer_info->screen_win.height = display_height;
		layer_info->mode = DISP_LAYER_WORK_MODE_SCALER;
		printk("Box Solutions:%s %d %u %u\n",__FUNCTION__,__LINE__,display_width,display_height);		
	}
	else
	{
	    layer_info->screen_win.x = psSurfAttrib->sDisplay.i32XOffset + (display_width * (100 - display_percent) / display_percent / 2);
	    layer_info->screen_win.y = psSurfAttrib->sDisplay.i32YOffset + (display_height * (100 - display_percent) / display_percent / 2);
	    layer_info->screen_win.width  = psSurfAttrib->sDisplay.sDims.ui32Width;// * display_percent / 100;
	    layer_info->screen_win.height = psSurfAttrib->sDisplay.sDims.ui32Height;// * display_percent / 100;
	}

    if(aw_layer_is_scaled(psSurfAttrib)
       ||
       aw_layer_is_de_supported_yuv_format(psBuffer))
    {
        int cut_size_scn, cut_size_src;

        hwc_rect_t scn_bound;

        layer_info->mode = DISP_LAYER_WORK_MODE_SCALER;

        scn_bound.left = display_width * (100 - display_percent) / 100 / 2;
        scn_bound.top  = display_height * (100 - display_percent) / 100 / 2;
		scn_bound.right = scn_bound.left + (display_width * display_percent) / 100;
		scn_bound.bottom = scn_bound.top + (display_height * display_percent) / 100;


        /*
        if(disp_index == 0)
        {
            scn_bound.right = scn_bound.left + (display_width * display_percent) / 100;
            scn_bound.bottom = scn_bound.top + (display_height * display_percent) / 100;
        }
        else
        {
            scn_bound.left += psSurfAttrib->sDisplay.i32XOffset;
            scn_bound.top  += psSurfAttrib->sDisplay.i32YOffset;
            scn_bound.right = scn_bound.left + psSurfAttrib->sDisplay.sDims.ui32Width;
            scn_bound.bottom = scn_bound.top + psSurfAttrib->sDisplay.sDims.ui32Height;
        }
		*/

        if(layer_info->fb.src_win.x < 0)
        {
            cut_size_src = (0 - layer_info->fb.src_win.x);

            layer_info->fb.src_win.x += cut_size_src;
            layer_info->fb.src_win.width -= cut_size_src;
        }
        if((layer_info->fb.src_win.x + layer_info->fb.src_win.width) > psBuffer->ui32Width)
        {
            cut_size_src = (layer_info->fb.src_win.x + layer_info->fb.src_win.width) - psBuffer->ui32Width;
            layer_info->fb.src_win.width -= cut_size_src;
        }
		
        if(layer_info->fb.src_win.y < 0)
        {
            cut_size_src = (0 - layer_info->fb.src_win.y);

            layer_info->fb.src_win.y += cut_size_src;
            layer_info->fb.src_win.height -= cut_size_src;
        }
        if((layer_info->fb.src_win.y + layer_info->fb.src_win.height) > psBuffer->ui32Height)
        {
            cut_size_src = (layer_info->fb.src_win.x + layer_info->fb.src_win.height) - psBuffer->ui32Height;
            layer_info->fb.src_win.height -= cut_size_src;
        }

        if(layer_info->screen_win.x < scn_bound.left)
        {
            cut_size_scn = (scn_bound.left - layer_info->screen_win.x);
            cut_size_src = cut_size_scn * layer_info->fb.src_win.width / layer_info->screen_win.width;

            layer_info->fb.src_win.x += cut_size_src;
            layer_info->fb.src_win.width -= cut_size_src;

            layer_info->screen_win.x += cut_size_scn;
            layer_info->screen_win.width -= cut_size_scn;
        }
        if((layer_info->screen_win.x + layer_info->screen_win.width) > scn_bound.right)
        {
            cut_size_scn = (layer_info->screen_win.x + layer_info->screen_win.width) - scn_bound.right;
            cut_size_src = cut_size_scn * layer_info->fb.src_win.width / layer_info->screen_win.width;

            layer_info->fb.src_win.width -= cut_size_src;

            layer_info->screen_win.width -= cut_size_scn;
        }
        if(layer_info->screen_win.y < scn_bound.top)
        {
            cut_size_scn = (scn_bound.top - layer_info->screen_win.y);
            cut_size_src = cut_size_scn * layer_info->fb.src_win.height / layer_info->screen_win.height;

            layer_info->fb.src_win.y += cut_size_src;
            layer_info->fb.src_win.height -= cut_size_src;

            layer_info->screen_win.y += cut_size_scn;
            layer_info->screen_win.height -= cut_size_scn;
        }
        if((layer_info->screen_win.y + layer_info->screen_win.height) > scn_bound.bottom)
        {
            cut_size_scn = (layer_info->screen_win.y + layer_info->screen_win.height) - scn_bound.bottom;
            cut_size_src = cut_size_scn * layer_info->fb.src_win.height / layer_info->screen_win.height;

            layer_info->fb.src_win.height -= cut_size_src;

            layer_info->screen_win.height -= cut_size_scn;
        }
    }

    layer_info->pipe = de_pipe_index;


#if 0 
    if(1)
    {
		DC_DPF(":SurfAttrib:%p ## sCrop: x:%u y:%u w:%u h:%u ## sDisplay: x:%u y:%u w:%u h:%u ## transform: 0x%x ## blendType: 0x%x ## custom: 0x%x ## hdmi: w:%u h:%u percent:%u 3d_mode:%u ## planeAlpha: 0x%x \n",
			     psSurfAttrib,
			     psSurfAttrib->sCrop.i32XOffset,psSurfAttrib->sCrop.i32YOffset,psSurfAttrib->sCrop.sDims.ui32Width,psSurfAttrib->sCrop.sDims.ui32Height,
			     psSurfAttrib->sDisplay.i32XOffset,psSurfAttrib->sDisplay.i32YOffset,psSurfAttrib->sDisplay.sDims.ui32Width,psSurfAttrib->sDisplay.sDims.ui32Height,
			     psSurfAttrib->ui32Transform,
			     psSurfAttrib->eBlendType,
			     psSurfAttrib->ui32Custom,
			     psSurfAttrib->hdmi_width,psSurfAttrib->hdmi_height,psSurfAttrib->hdmi_disp_percent,psSurfAttrib->hdmi_3d_config_mode,
			     psSurfAttrib->ui8PlaneAlpha);

		DC_DPF(":Buffer:%p  ## w:%u h:%u ## img_format:0x%x(%d), hal_fmt: 0x%x(%d) ## custom:0x%x, ## scrop_w:%u scrop_h:%u \n",
		        psBuffer, 
		        psBuffer->ui32Width,psBuffer->ui32Height,
		        psBuffer->ePixFormat,psBuffer->ePixFormat,
		        iFormat,iFormat,
		        psSurfAttrib->ui32Custom, 
		        psSurfAttrib->sCrop.sDims.ui32Width, psSurfAttrib->sCrop.sDims.ui32Height);

		
		DC_DPF(":layer_info: layerZ: %d,  ## fb format: %d , fb pre_multiply: %d ,fb size width: %d,  fb addr[0]:0x%x, fb addr[1]:0x%x, fb addr[2]:0x%x, fb src_win: x:%d y:%d w:%d h:%d, ## screen_win: x:%d y:%d w:%d h:%d, ## alpha_mode: %d alpha_value:0x%x, ## mode:%d, pipe:%d \n", 
			    layerZ,
			    layer_info->fb.format,layer_info->fb.pre_multiply,layer_info->fb.size.width,
			    layer_info->fb.addr[0],layer_info->fb.addr[1],layer_info->fb.addr[2],
			    layer_info->fb.src_win.x,layer_info->fb.src_win.y,layer_info->fb.src_win.width,layer_info->fb.src_win.height,
			    layer_info->screen_win.x,layer_info->screen_win.y,layer_info->screen_win.width,layer_info->screen_win.height,
			    layer_info->alpha_mode,layer_info->alpha_value,
			    layer_info->mode,layer_info->pipe);
    }
#endif 

    return 0;// 0 is ok
}


typedef struct{
	PVRSRV_SURFACE_CONFIG_INFO        *apsSurfArrtib[4];//input
	DC_SUNXI_BUFFER                   *apsBuffer[4];//input
	IMG_UINT32                         nValideLayerNum;//input

    IMG_UINT32                         bFEUsed;    //control: one display no more than 2 scaled layer.

    IMG_UINT32                         nAssignedPipeIndex[4];//out
	IMG_UINT32                         nDispAssignedLayerNum;// total layer num = lcd num + hdmi num
	IMG_UINT32                         nPipeAssignedLayerNum[2];//0: lcd num, 1:hdmi num
}DC_PIPE_ALLOC_INFO;
static int aw_try_assigned_to_depipe_new(DC_SUNXI_CONTEXT *pDevContext,
	                                  int nDisp,
		                              int nDePipeIndex,
		                              int nLayerIndex,
		                              DC_PIPE_ALLOC_INFO *psPipeAllocInfo)
{
	PVRSRV_SURFACE_CONFIG_INFO    *psLayer  = psPipeAllocInfo->apsSurfArrtib[nLayerIndex];
	DC_SUNXI_BUFFER               *psBuffer = psPipeAllocInfo->apsBuffer[nLayerIndex];
    int                            nCurrentPipeLayerNum = psPipeAllocInfo->nPipeAssignedLayerNum[nDePipeIndex];
	int                            nCurrentDisplayLayerNum = psPipeAllocInfo->nDispAssignedLayerNum;
    int                            bLayerCanUseFe = 0;

    if(psPipeAllocInfo->nDispAssignedLayerNum == 4)
    {
        goto err_fail_out;
    }

    if(!aw_layer_is_acceptable(psLayer,psBuffer))
    {
        DC_DPF(":layer %d can not be assigned to de pipe %d. Buffer : w:%d h:%d format:%d \n",
			    nLayerIndex,nDePipeIndex,psBuffer->ui32Width,psBuffer->ui32Height,psBuffer->ePixFormat);
        goto err_fail_out;
    }

    if(aw_layer_is_scaled(psLayer))
    {
        bLayerCanUseFe =
            aw_layer_is_acceptable_scaled_layer(pDevContext->psDevice,
                                                psLayer,
                                                psBuffer,
                                                nDisp);

        if(bLayerCanUseFe != 1)
        {
			goto err_fail_out;
        }

		if(psLayer->ui32Custom & AW_BIT_FLAG_FORCE_UI_GLES)
		{
		    if((psLayer->ui32Custom & AW_BIT_FLAG_LAYER_TYPE_FRAMEBUFFERTARGET) == 0
				&& aw_layer_is_de_supported_yuv_format(psBuffer) != 1) // not fbt, not yuv, can not use fe
				goto err_fail_out;

			if(psPipeAllocInfo->bFEUsed == 2)//fe already been used up
			{
			    DC_E_DPF("FE had been used up \n");
				goto err_fail_out;
			}
		}
		else
		{
		    if(aw_layer_is_de_supported_yuv_format(psBuffer) != 1 || psPipeAllocInfo->bFEUsed == 1)// 没有要求UI 强制GPU合成， 那么默认认为FBT不能够用FE
				goto err_fail_out;
		}
    }

    if(aw_layer_is_alpha_layer(psLayer))
    {
        int i;
		PVRSRV_SURFACE_RECT* psCurrentLayerDisplayRegion;
		PVRSRV_SURFACE_RECT* psCurrentPipeAssignedLayerDisplayRegion;
        if(nCurrentPipeLayerNum > 0)
        {
			psCurrentLayerDisplayRegion = &psLayer->sDisplay;
            for(i = 0; i < nCurrentDisplayLayerNum; i++)//在所有assigned 到disp上的layer中找属于当前pipe的layer 。
            {
                if(psPipeAllocInfo->nAssignedPipeIndex[i] == nDePipeIndex)
                {
                    psCurrentPipeAssignedLayerDisplayRegion = &psPipeAllocInfo->apsSurfArrtib[i]->sDisplay;
				    if(aw_layer_is_intersect(psCurrentLayerDisplayRegion, psCurrentPipeAssignedLayerDisplayRegion))
				    {
					    goto err_fail_out;
				    }
                }
            }
        }
    }

    psPipeAllocInfo->nAssignedPipeIndex[nLayerIndex] = nDePipeIndex;//layer [nLayerIndex] use de pipe [nDePipeIndex],and its priority is nLayerIndex
    psPipeAllocInfo->nPipeAssignedLayerNum[nDePipeIndex]++;
	psPipeAllocInfo->nDispAssignedLayerNum++;
    if(bLayerCanUseFe)
    {
        psPipeAllocInfo->bFEUsed++;//
    }
    return 0;
err_fail_out:
    return -1;
}

static int aw_get_disp_pipe_alloc_info(DC_SUNXI_CONTEXT    *pDevContext,
				                       DC_PIPE_ALLOC_INFO  *psPipeAlloc,
				                       int                  nDispIndex)//get specified disp pipe alloc info.
{
    int i;
	int nDeFirstPipeHasBeenLocked = 0;//once second pipe has been assigned layer, the first layer should not be used(locked).

	int nDispLayerNum = psPipeAlloc->nValideLayerNum;

    if(nDispLayerNum == 0)
		return 0;//ok, no problem

    if(nDispLayerNum > 4)
        goto LAYER_ASSIGNED_FAILED;	

	for(i = 0; i < nDispLayerNum; i++)
	{
	    if(0 == nDeFirstPipeHasBeenLocked)
	    {
			if(aw_try_assigned_to_depipe_new(pDevContext,
				                         nDispIndex,
				                         0,//de pipe 0
				                         i,
				                         psPipeAlloc) == 0)
			{
				continue;
			}

			if(aw_try_assigned_to_depipe_new(pDevContext,
				                         nDispIndex,
				                         1,//de pipe 1
				                         i,
				                         psPipeAlloc) == 0)
			{
				nDeFirstPipeHasBeenLocked = 1;
			    continue;
			}
	    }
		else// if any prevous layer assigned to de pipe1, then all other later layers must try use de pipe1
		{
		    if(aw_try_assigned_to_depipe_new(pDevContext,
				                         nDispIndex,
				                         1, //de pipe 1
				                         i,
				                         psPipeAlloc) == 0)
		    {
				continue;	
		    }
		}
		goto LAYER_ASSIGNED_FAILED;//de pipe0 ,de pipe1 both can not handle this layer, goto err out.
				
	}
    return 0;//success
LAYER_ASSIGNED_FAILED:
    return -1;//failed
}
static void aw_setup_de_layers_per_disp(
                                  DC_SUNXI_DEVICE *pDev,
                                  int nDispIndex,
                                  DC_PIPE_ALLOC_INFO  *psPipeAllocInfo)
{
    int i;
    disp_layer_info *pas_dispLayers_to_setup = &pDev->sDataToDeDriver.layer_info[nDispIndex][0];
    
    if(psPipeAllocInfo->nValideLayerNum == 0
		|| psPipeAllocInfo->nValideLayerNum != psPipeAllocInfo->nDispAssignedLayerNum)
    {
		DC_DPF("Setup layer To Zero: disp[%d] , layernum:[%d]\n",nDispIndex,psPipeAllocInfo->nValideLayerNum);
        goto set_disp_layer_num_zero;
    }
	
    pDev->sDataToDeDriver.layer_num[nDispIndex] = psPipeAllocInfo->nValideLayerNum;

	memset(pas_dispLayers_to_setup,0, 4 * sizeof(disp_layer_info));
    for(i = 0; i < psPipeAllocInfo->nValideLayerNum; i++)
    {
        if(aw_setup_de_layer(pDev,
                             psPipeAllocInfo->apsSurfArrtib[i],
                             psPipeAllocInfo->apsBuffer[i],
                             psPipeAllocInfo->nAssignedPipeIndex[i],
                             i,
                             nDispIndex,
                             pas_dispLayers_to_setup + i) != 0)
        {

			DC_TAG(": set up de layer %d failed, now set disp %d layer_num to Zero \n",i,nDispIndex);
            goto set_disp_layer_num_zero;
        }
    }

    return;//success!
set_disp_layer_num_zero://success but no layer
    if(nDispIndex == 1)
		DC_DPF("Disp 1 No Layer\n");
    pDev->sDataToDeDriver.layer_num[nDispIndex] = 0;
}




static int aw_init_fbdev(DC_SUNXI_DEVICE *psDevInfo)
{
    struct fb_info *psLINFBInfo;
    struct module  *psLINFBOwner;

    int  eError  = -1;
    //console_lock();

    psLINFBInfo = registered_fb[0];
    if (!psLINFBInfo)
    {
        goto ErrorRelSem;
    }

    if(!lock_fb_info(psLINFBInfo))
        goto ErrorRelSem;

    psLINFBOwner = psLINFBInfo->fbops->owner;
    if (!try_module_get(psLINFBOwner))
    {
		DC_E_DPF(":try_module_get faile \n");
        goto ErrorUnlockFB;
    }

    if (psLINFBInfo->fbops->fb_open != NULL)
    {
        if (psLINFBInfo->fbops->fb_open(psLINFBInfo, 0) != 0)
        {
			DC_E_DPF(": try open fb0 failed \n");
            goto ErrorModPut;
        }
    }
    psDevInfo->psLINFBInfo = psLINFBInfo;
    eError = 0;
    goto ErrorUnlockFB;
	
    if (psLINFBInfo->fbops->fb_release != NULL)
    {
        psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
    }
ErrorModPut:
    module_put(psLINFBOwner);
ErrorUnlockFB:
    unlock_fb_info(psLINFBInfo);
ErrorRelSem:
    return eError;
}

static int aw_create_sunxi_device(DC_SUNXI_DEVICE** ppDevCreated)
{
    DC_SUNXI_DEVICE *pDevice = 0;

    pDevice = kmalloc(sizeof(DC_SUNXI_DEVICE),GFP_KERNEL);
    if(!pDevice)
    {
		DC_E_DPF(": malloc mem for device failed , out of mem \n");
        return 0;
    }
    memset(pDevice,0,sizeof(DC_SUNXI_DEVICE));

    if(aw_init_fbdev(pDevice) != 0)
    {
		DC_E_DPF(": init fb dev failed \n");
        goto err_ion_client_destroy;
    }

    if(aw_setup_disp_private_data(pDevice) != 0)
    {
		DC_E_DPF(":set up disp private data failed \n");
        goto err_ion_client_destroy;
    }

    *ppDevCreated = pDevice;
    return 0;
err_ion_client_destroy:
    kfree(pDevice);
    return -1;
}
/*************************************Interface To RGX***********************************************************/
static IMG_VOID DC_SUNXI_BUFFER_Free(IMG_HANDLE hBuffer)
{
    DC_SUNXI_BUFFER *psBuffer = (DC_SUNXI_BUFFER *)hBuffer;

    //DC_ENTERED();

    if(!psBuffer)
    {
        kfree(psBuffer);
		psBuffer = NULL;
    }

	//DC_EXITED();
}

static
PVRSRV_ERROR DC_SUNXI_BUFFER_Import(IMG_HANDLE hDisplayContext,
                                   IMG_UINT32 ui32NumPlanes,
                                   IMG_HANDLE **paphImport,
                                   DC_BUFFER_IMPORT_INFO *psSurfInfo,
                                   IMG_HANDLE *phBuffer)
{
    DC_SUNXI_CONTEXT *psDeviceContext = hDisplayContext;
    PVRSRV_ERROR eError = PVRSRV_OK;
    DC_SUNXI_BUFFER *psBuffer = NULL;
    IMG_DEV_PHYADDR      *apDevAddrs;

	//DC_ENTERED();

    if(aw_buffer_configuration_is_correct(ui32NumPlanes,psSurfInfo) != 0)
    {
        goto err_out;
    }

    psBuffer = kmalloc(sizeof(DC_SUNXI_BUFFER),GFP_KERNEL);

    if(!psBuffer)
    {
		DC_E_DPF(":Out of mem \n");
        eError = PVRSRV_ERROR_OUT_OF_MEMORY;
        goto err_out;
    }

    psBuffer->psDeviceContext = psDeviceContext;
    psBuffer->ui32NumPlanes = ui32NumPlanes;
    psBuffer->ePixFormat = psSurfInfo->ePixFormat;

    psBuffer->ui32Width = psSurfInfo->ui32Width[0];
    psBuffer->ui32Height = psSurfInfo->ui32Height[0];
    psBuffer->ui32ByteStride = psSurfInfo->ui32ByteStride[0];

    psBuffer->hImport = paphImport[0];
    eError = DCImportBufferAcquire(psBuffer->hImport,
                                       PAGE_SHIFT,
                                       &psBuffer->ui32NumPages,
                                       (IMG_DEV_PHYADDR **)&apDevAddrs);

    if(eError != PVRSRV_OK)
    {		
		DC_E_DPF("eError: %d \n",eError);
        goto err_out;
    }

	psBuffer->ui32PhyAddrs = apDevAddrs[0].uiAddr;
    *phBuffer = psBuffer;
	DCImportBufferRelease(psBuffer->hImport, apDevAddrs);

#if 0
    DC_DPF(":DC_SUNXI Import buffer success: w:%d h:%d format:%d devaddr:0x%x \n",
             psBuffer->ui32Width,psBuffer->ui32Height,psBuffer->ePixFormat,psBuffer->ui32PhyAddrs);
#endif

    DC_EXITED();
	return eError;
err_out:
    if(psBuffer)
        kfree(psBuffer);
    return eError;
}

static
IMG_VOID DC_SUNXI_GetInfo(IMG_HANDLE hDeviceData,
                          DC_DISPLAY_INFO *psDisplayInfo)
{
    PVR_UNREFERENCED_PARAMETER(hDeviceData);

    strncpy(psDisplayInfo->szDisplayName, DRVNAME " 1", DC_NAME_SIZE);

    psDisplayInfo->ui32MinDisplayPeriod    = 0;
    psDisplayInfo->ui32MaxDisplayPeriod    = 1;
    psDisplayInfo->ui32MaxPipes            = 4;
    psDisplayInfo->bUnlatchedSupported     = IMG_FALSE;
}

static
PVRSRV_ERROR DC_SUNXI_PanelQueryCount(IMG_HANDLE hDeviceData,
                                        IMG_UINT32 *pui32NumPanels)
{
    PVR_UNREFERENCED_PARAMETER(hDeviceData);
    *pui32NumPanels = 1;
    return PVRSRV_OK;
}

static
PVRSRV_ERROR DC_SUNXI_PanelQuery(IMG_HANDLE hDeviceData,
                                 IMG_UINT32 ui32PanelsArraySize,
                                 IMG_UINT32 *pui32NumPanels,
                                 PVRSRV_PANEL_INFO *psPanelInfo)
{
    DC_SUNXI_DEVICE *psDeviceData = hDeviceData;

    struct fb_var_screeninfo *psVar = &psDeviceData->psLINFBInfo->var;
    struct fb_var_screeninfo sVar = { .pixclock = 0 };
	IMG_UINT32 ui32xdpi,ui32ydpi,ui32RefreshRate;

    if(!lock_fb_info(psDeviceData->psLINFBInfo))
        return PVRSRV_ERROR_RETRY;
    
    ui32xdpi = FALLBACK_DPI;
    ui32ydpi = FALLBACK_DPI;
    ui32RefreshRate = FALLBACK_REFRESH_RATE;
    if(psVar->xres > 0 && psVar->yres > 0 && psVar->pixclock > 0)
        sVar = *psVar;
    else if(psDeviceData->psLINFBInfo->mode)
        fb_videomode_to_var(&sVar, psDeviceData->psLINFBInfo->mode);
    if(sVar.xres > 0 && sVar.yres > 0 && sVar.pixclock > 0)
    {
        /*
        can not use this method ,because kernel not support ulong divide op.
        uint64_t divider = (sVar.upper_margin + sVar.lower_margin + sVar.yres + sVar.vsync_len) 
                   * (sVar.left_margin + sVar.right_margin + sVar.xres + sVar.hsync_len) 
                   * sVar.pixclock ;
        ui32RefreshRate = 1000000000000LLU / divider ;
        */
        ui32RefreshRate = 1000000000LU /
			((sVar.upper_margin + sVar.lower_margin +
			  sVar.yres + sVar.vsync_len) *
			 (sVar.left_margin  + sVar.right_margin +
			  sVar.xres + sVar.hsync_len) *
			 (sVar.pixclock / 1000));
    }

    if((int)sVar.width > 0 && (int)sVar.height > 0)
    {
        ui32xdpi = 254000 / sVar.width * psVar->xres / 10000 ;
        ui32ydpi = 254000 / sVar.height * psVar->yres / 10000 ;
    }
    unlock_fb_info(psDeviceData->psLINFBInfo);

    *pui32NumPanels = 1;
    psPanelInfo[0].sSurfaceInfo.sFormat.ePixFormat = psDeviceData->sDePrivateData.eFrameBufferPixlFormat;
    psPanelInfo[0].sSurfaceInfo.sDims.ui32Width    = psVar->xres;
    psPanelInfo[0].sSurfaceInfo.sDims.ui32Height   = psVar->yres;
    psPanelInfo[0].ui32XDpi = ui32xdpi;
    psPanelInfo[0].ui32YDpi = ui32ydpi;
    psPanelInfo[0].ui32RefreshRate = ui32RefreshRate;
    return PVRSRV_OK;
}

static
PVRSRV_ERROR DC_SUNXI_FormatQuery(IMG_HANDLE hDeviceData,
                                  IMG_UINT32 ui32NumFormats,
                                  PVRSRV_SURFACE_FORMAT *pasFormat,
                                  IMG_UINT32 *pui32Supported)
{
    int i;
    for(i = 0; i < ui32NumFormats; i++)
    {
        pui32Supported[i] = 0;

        if(IMGPixFmtGetBPP(pasFormat[i].ePixFormat))
            pui32Supported[i]++;
    }
    return PVRSRV_OK;
}

static
PVRSRV_ERROR DC_SUNXI_DimQuery(IMG_HANDLE hDeviceData,
                               IMG_UINT32 ui32NumDims,
                               PVRSRV_SURFACE_DIMS *psDim,
                               IMG_UINT32 *pui32Supported)//HWC 不会用到 这个函数。
{
    DC_SUNXI_DEVICE *psDeviceData = hDeviceData;
    struct fb_var_screeninfo *psVar = &psDeviceData->psLINFBInfo->var;
    int i;

    for(i = 0; i < ui32NumDims; i++)
    {
        pui32Supported[i] = 0;

        if(psDim[i].ui32Width  == psVar->xres &&
           psDim[i].ui32Height == psVar->yres)
            pui32Supported[i]++;
    }

    return PVRSRV_OK;
}

static
PVRSRV_ERROR DC_SUNXI_ContextCreate(IMG_HANDLE hDeviceData,
                                    IMG_HANDLE *hDisplayContext)
{
    DC_SUNXI_CONTEXT *psDevContext = 0;
    PVRSRV_ERROR eError = PVRSRV_OK;

    psDevContext = kmalloc(sizeof(DC_SUNXI_CONTEXT),GFP_KERNEL);
    if(!psDevContext)
    {
        eError = PVRSRV_ERROR_OUT_OF_MEMORY;
        goto err_out;
    }
    memset(psDevContext,0,sizeof(DC_SUNXI_CONTEXT));

    psDevContext->psDevice = hDeviceData;
	
    *hDisplayContext = psDevContext;

err_out:
   return eError;
}

static
IMG_VOID DC_SUNXI_ContextDestroy(IMG_HANDLE hDisplayContext)
{
    DC_SUNXI_CONTEXT *pDevContext = (DC_SUNXI_CONTEXT *)hDisplayContext;
    kfree(pDevContext);
}

static
PVRSRV_ERROR DC_SUNXI_SetBlank(IMG_HANDLE hDeviceData,
                                 IMG_BOOL bEnabled)
{
    PVR_UNREFERENCED_PARAMETER(hDeviceData);
    PVR_UNREFERENCED_PARAMETER(bEnabled);
    return PVRSRV_OK;
}

static PVRSRV_ERROR DC_SUNXI_ContextConfigureCheck(IMG_HANDLE hDisplayContext,
								                   IMG_UINT32 ui32PipeCount,
								                   PVRSRV_SURFACE_CONFIG_INFO *pasSurfAttrib,
								                   IMG_HANDLE *ahBuffers)
{
    DC_SUNXI_CONTEXT *pDevContext      = (DC_SUNXI_CONTEXT *)hDisplayContext;
	int               nDispLayerNum[2] = {0};
	int               bCheckResult[2]  = {-1};
	int               i                = 0;
	IMG_UINT32        ui32FrameID = 0;
	DC_PIPE_ALLOC_INFO asDispPipeAllocInfo[2];// one for lcd , two for hdmi
    memset(&asDispPipeAllocInfo,0,sizeof(DC_PIPE_ALLOC_INFO) * 2);

	if(ui32PipeCount == 0 || ui32PipeCount > 8
		|| !pasSurfAttrib
		|| !ahBuffers)
    {
		DC_E_DPF(":configure check meet pipecount:%d pasSurfAttrib:%p ahBuffers:%p \n",ui32PipeCount,pasSurfAttrib,ahBuffers);
        goto LAYER_ASSIGNED_FAILED;
    }

	ui32FrameID = pasSurfAttrib[0].hdmi_3d_config_mode;
    for(i = 0 ; i < ui32PipeCount; i++)
    {
        if(pasSurfAttrib[i].ui32Custom & AW_BIT_FLAG_LAYER_ALONG_TO_PRIMARY_DISPLAY)
        {
			if(nDispLayerNum[0] >= 4)
			{
				goto LAYER_ASSIGNED_FAILED;
			}

            asDispPipeAllocInfo[0].apsBuffer[nDispLayerNum[0]]     = (DC_SUNXI_BUFFER *)ahBuffers[i];
			asDispPipeAllocInfo[0].apsSurfArrtib[nDispLayerNum[0]] = &pasSurfAttrib[i];
			asDispPipeAllocInfo[0].nValideLayerNum++;//disp0(lcd or edp)
			nDispLayerNum[0]++;
        }
        else
        {
        	if(nDispLayerNum[1] >= 4)
			{
				goto LAYER_ASSIGNED_FAILED;
			}
			
            asDispPipeAllocInfo[1].apsBuffer[nDispLayerNum[1]]     = (DC_SUNXI_BUFFER *)ahBuffers[i];
			asDispPipeAllocInfo[1].apsSurfArrtib[nDispLayerNum[1]] = &pasSurfAttrib[i];
			asDispPipeAllocInfo[1].nValideLayerNum++;//disp1(hdmi)
			nDispLayerNum[1]++;
        }
    }

	if(nDispLayerNum[0] != asDispPipeAllocInfo[0].nValideLayerNum
		|| nDispLayerNum[1] != asDispPipeAllocInfo[1].nValideLayerNum)
	{
	    DC_E_DPF("Error!!!!! Should Never meet this situation\n");
		goto LAYER_ASSIGNED_FAILED;
	}

	bCheckResult[0] = aw_get_disp_pipe_alloc_info(pDevContext,&asDispPipeAllocInfo[0],0);
	bCheckResult[1] = aw_get_disp_pipe_alloc_info(pDevContext,&asDispPipeAllocInfo[1],1);
	
    if(bCheckResult[0] == 0
		&& bCheckResult[1] == 0)
    {
#if 0
		if(1)
		{
			DC_DPF("Disp[0] ID[%d].[%d]:[%d %d %d %d]\n",
						pasSurfAttrib[0].hdmi_3d_config_mode,
						asDispPipeAllocInfo[0].nValideLayerNum,
						asDispPipeAllocInfo[0].nAssignedPipeIndex[0],
						asDispPipeAllocInfo[0].nAssignedPipeIndex[1],
						asDispPipeAllocInfo[0].nAssignedPipeIndex[2],
						asDispPipeAllocInfo[0].nAssignedPipeIndex[3]);
			for(i = 0; i < asDispPipeAllocInfo[0].nValideLayerNum;i++)
			{
			    //DC_DPF(":layer[%d] dst{x,y,w,h}={%u,%u,%u,%u}\n",
				DC_DPF(":layer[%d]:{%u,%u,%u,%u}\n",
				    i,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32XOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32YOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Width,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Height);
			}
		}
#endif

#if 0
		if(0)
		{
		    int i;
			DC_DPF("ID:%d . Valide num[%d]: {0,%d} {1,%d} {2,%d} {3,%d}\n",
						pasSurfAttrib[0].hdmi_3d_config_mode,
						asDispPipeAllocInfo[0].nValideLayerNum,
						asDispPipeAllocInfo[0].nAssignedPipeIndex[0],
						asDispPipeAllocInfo[0].nAssignedPipeIndex[1],
						asDispPipeAllocInfo[0].nAssignedPipeIndex[2],
						asDispPipeAllocInfo[0].nAssignedPipeIndex[3]);
			
			for(i = 0; i < asDispPipeAllocInfo[0].nValideLayerNum; i++)
			{
			    DC_DPF(":layer[%d] src{x,y,w,h}={%u,%u,%u,%u},dst{x,y,w,h}={%u,%u,%u,%u},attr{custom,format,addr}={0x%x,0x%x,0x%x}\n",
                    i,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.i32XOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.i32YOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.sDims.ui32Width,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.sDims.ui32Height,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32XOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32YOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Width,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Height,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->ui32Custom,
					asDispPipeAllocInfo[0].apsBuffer[i]->ePixFormat,
					asDispPipeAllocInfo[0].apsBuffer[i]->ui32PhyAddrs);
			}
			
			for(i = 0; i < asDispPipeAllocInfo[0].nValideLayerNum; i++)
			{
			    DC_DPF(":layer[%d] src{x,y,w,h}={%u,%u,%u,%u},dst{x,y,w,h}={%u,%u,%u,%u}\n",
                    i,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.i32XOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.i32YOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.sDims.ui32Width,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sCrop.sDims.ui32Height,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32XOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32YOffset,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Width,
					asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Height);
			}

		}
#endif
        return PVRSRV_OK;
    }
	
LAYER_ASSIGNED_FAILED:
	return PVRSRV_ERROR_DC_INVALID_CONFIG;
}


static
IMG_VOID DC_SUNXI_ContextConfigure(IMG_HANDLE hDisplayContext,
                                   IMG_UINT32 ui32PipeCount,
                                   PVRSRV_SURFACE_CONFIG_INFO *pasSurfAttrib,
                                   IMG_HANDLE *ahBuffers,
                                   IMG_UINT32 ui32DisplayPeriod,
                                   IMG_HANDLE hConfigData)
{
    DC_SUNXI_CONTEXT *pDevContext = (DC_SUNXI_CONTEXT *)hDisplayContext;
    DC_SUNXI_DEVICE  *pDev         = pDevContext->psDevice;

    int              nDispLayerNum[2];
    int i;

	S_CONFIG_DATA    *psConfigDataPostToDisplay;

	IMG_UINT32       ui32FrameID = 0;


    DC_PIPE_ALLOC_INFO asDispPipeAllocInfo[2];// one for lcd , two for hdmi

	int android_disp_status[3];
	int de_disp_status[3];      // 0: can accept new data. 1:already locked(used).

    if(ui32PipeCount == 0 
		|| ui32PipeCount > 8
		|| !pasSurfAttrib
		|| !ahBuffers)
    {
        DC_E_DPF(":configure meet pipecount:%d  pasSurfAttrib:%p ahBuffers:%p \n\n",
			       ui32PipeCount,pasSurfAttrib,ahBuffers);
		pDev->sDataToDeDriver.layer_num[0] = 0;
		pDev->sDataToDeDriver.layer_num[1] = 0;
		goto err_out;
    }
	ui32FrameID = pasSurfAttrib[0].hdmi_3d_config_mode;

	android_disp_status[0] = pasSurfAttrib[0].android_disp_status[0];
	android_disp_status[1] = pasSurfAttrib[0].android_disp_status[1];
	android_disp_status[2] = pasSurfAttrib[0].android_disp_status[2];
#if 1
    memset(&pDev->sDataToDeDriver,0,sizeof(setup_dispc_data_t));
#endif

    nDispLayerNum[0] = 0;
	nDispLayerNum[1] = 0;
	memset(&asDispPipeAllocInfo,0,sizeof(DC_PIPE_ALLOC_INFO) << 1);
    for(i = 0 ; i < ui32PipeCount; i++)
    {
        if(pasSurfAttrib[i].ui32Custom & AW_BIT_FLAG_LAYER_ALONG_TO_PRIMARY_DISPLAY)
        {
            asDispPipeAllocInfo[0].apsBuffer[nDispLayerNum[0]]     = (DC_SUNXI_BUFFER *)ahBuffers[i];
			asDispPipeAllocInfo[0].apsSurfArrtib[nDispLayerNum[0]] = &pasSurfAttrib[i];
			asDispPipeAllocInfo[0].nValideLayerNum++;
			nDispLayerNum[0]++;//android disp0
        }
        else
        {
            asDispPipeAllocInfo[1].apsBuffer[nDispLayerNum[1]]     = (DC_SUNXI_BUFFER *)ahBuffers[i];
			asDispPipeAllocInfo[1].apsSurfArrtib[nDispLayerNum[1]] = &pasSurfAttrib[i];
			asDispPipeAllocInfo[1].nValideLayerNum++;
			nDispLayerNum[1]++;//android disp1
        }
    }

	//DC_DPF("disp[0]:%d .  disp[1]:%d \n",nDispLayerNum[0],nDispLayerNum[1]);

	if(aw_get_disp_pipe_alloc_info(pDevContext,&asDispPipeAllocInfo[0],0) != 0)
	{
	    DC_E_DPF(":LCD Should never meet this situation \n");
		pDev->sDataToDeDriver.layer_num[0] = 0;
		pDev->sDataToDeDriver.layer_num[1] = 0;
		goto err_out;
	}
	
	if(aw_get_disp_pipe_alloc_info(pDevContext,&asDispPipeAllocInfo[1],1) != 0)
	{
	    pDev->sDataToDeDriver.layer_num[1] = 0;
	}


#if 1
    de_disp_status[0] = de_disp_status[1] = de_disp_status[2] = 0;
    for(i = 0 ; i < 2; i++)//Currently we do not support android's 3rd display
    {
        switch(android_disp_status[i])
        {
			case AW_OUTPUT_TO_DISPLAY0:
				if(de_disp_status[0] == 1)
				{
				    DC_E_DPF("de disp0 is already used. Android display[%d] can not out to de disp0 . \n",i);
				}
				else
				{
				    aw_setup_de_layers_per_disp(pDev,0,&asDispPipeAllocInfo[i]);
					de_disp_status[0] = 1;
				}
				break;
			case AW_OUTPUT_TO_DISPLAY1:
				if(de_disp_status[1] == 1)
				{
				    DC_E_DPF("de disp1 is already used. Android display[%d] can not out to de disp1 . \n",i);
				}
				else
				{
				    aw_setup_de_layers_per_disp(pDev,1,&asDispPipeAllocInfo[i]);
					de_disp_status[1] = 1;
				}
				break;
			case AW_OUTPUT_TO_DISPLAY2:
				if(de_disp_status[2] == 1)
				{
				    DC_E_DPF("de disp2 is already used. Android display[%d] can not out to de disp2 . \n",i);
				}
				else
				{
				    aw_setup_de_layers_per_disp(pDev,2,&asDispPipeAllocInfo[i]);
					de_disp_status[2] = 1;
				}
				break;        
			case AW_DISP_NONE:
				//DC_DPF("Android display[%d] is closed\n",i);
				break;
			default:
				DC_E_DPF("Android display[%d] expect output to de disp[%d]\n",i,android_disp_status[i]);
				break;
        	}
    }
#else
	if(android_disp_status[0] != AW_DISP_NONE)//决定当前android的primary display 输出到我们的哪一个display上。
	{
	    if(android_disp_status[0] == AW_OUTPUT_TO_DISPLAY0)
	    {
			aw_setup_de_layers_per_disp(pDev,0,&asDispPipeAllocInfo[0]);
	    }
		else if(android_disp_status[0] == AW_OUTPUT_TO_DISPLAY1)
		{
			aw_setup_de_layers_per_disp(pDev,1,&asDispPipeAllocInfo[0]);
		}
		else if(android_disp_status[0] == AW_OUTPUT_TO_DISPLAY2)
		{
			aw_setup_de_layers_per_disp(pDev,2,&asDispPipeAllocInfo[0]);
		}
		else
		{
		    DC_DPF("Debug:no output for primary display, check.\n");
		}
	}

	if(android_disp_status[1] != AW_DISP_NONE)//决定当前android的external display 输出到哪一个display上。 默认应该为disp1
	{
	    //当前android传入了 primary display和external display。
	    if(android_disp_status[1] == AW_OUTPUT_TO_DISPLAY1)
	    {
	        //DC_DPF("External display try to output to disp1\n");
			if(android_disp_status[0] == AW_OUTPUT_TO_DISPLAY1)
			{
				//DC_DPF("Error! disp1 has already been use to output android's primary display, so, what you want? \n");
				//goto err_out;
			}
			else
			{
			    aw_setup_de_layers_per_disp(pDev,1,&asDispPipeAllocInfo[1]);
			}
	    }
	}
#endif
	
#if 0
	if(1)
	{
		DC_E_DPF("ID:%d . Valide num[%d]: {0,%d} {1,%d} {2,%d} {3,%d}\n",
					pasSurfAttrib[0].hdmi_3d_config_mode,
					asDispPipeAllocInfo[0].nValideLayerNum,
					asDispPipeAllocInfo[0].nAssignedPipeIndex[0],
					asDispPipeAllocInfo[0].nAssignedPipeIndex[1],
					asDispPipeAllocInfo[0].nAssignedPipeIndex[2],
					asDispPipeAllocInfo[0].nAssignedPipeIndex[3]);
		
		for(i = 0; i < asDispPipeAllocInfo[0].nValideLayerNum;i++)
		{
		    //disp_layer_info *pas_dispLayers_to_setup = &pDev->sDataToDeDriver.layer_info[1][i];
		
			DC_E_DPF(":layer[%d] dst{x,y,w,h}={%u,%u,%u,%u}\n",
				i,
				asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32XOffset,
				asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.i32YOffset,
				asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Width,
				asDispPipeAllocInfo[0].apsSurfArrtib[i]->sDisplay.sDims.ui32Height);//,
				//pas_dispLayers_to_setup->fb.addr[0]);
		}
		
	}
#endif
err_out:
	psConfigDataPostToDisplay = add_queue(hConfigData,ui32FrameID);
    if(psConfigDataPostToDisplay == NULL)
    {
		DC_E_DPF("################### Fatal Error,################### Add Config Data to queue Failed . \n");
		DCDisplayConfigurationRetired(hConfigData);//make system dead.
    }
	pDev->sDataToDeDriver.hConfigData = psConfigDataPostToDisplay;
    pDev->sDePrivateData.disp_function_table.dispc_gralloc_queue(&pDev->sDataToDeDriver,
                                                                  sizeof(pDev->sDataToDeDriver),
                                                                  &aw_de_vysnc_callback);
}

/***********************km part 初始化函数********************************/
static int __init DC_SUNXI_DEV_init(void)
{
    static DC_DEVICE_FUNCTIONS sDCFunctions =
    {
        .pfnGetInfo                    = DC_SUNXI_GetInfo,
		.pfnPanelQueryCount            = DC_SUNXI_PanelQueryCount,
        .pfnPanelQuery                 = DC_SUNXI_PanelQuery,
        .pfnFormatQuery                = DC_SUNXI_FormatQuery,
        .pfnDimQuery                   = DC_SUNXI_DimQuery,

        .pfnSetBlank                   = DC_SUNXI_SetBlank,

        .pfnSetVSyncReporting          = IMG_NULL,
        .pfnLastVSyncQuery             = IMG_NULL,

        .pfnContextCreate              = DC_SUNXI_ContextCreate,
        .pfnContextDestroy             = DC_SUNXI_ContextDestroy,

        .pfnContextConfigure           = DC_SUNXI_ContextConfigure,
        .pfnContextConfigureCheck      = DC_SUNXI_ContextConfigureCheck,

        .pfnBufferAlloc              = IMG_NULL,
        .pfnBufferAcquire            = IMG_NULL,
        .pfnBufferRelease            = IMG_NULL,

		.pfnBufferFree				 = DC_SUNXI_BUFFER_Free,
        .pfnBufferImport            = DC_SUNXI_BUFFER_Import,

        .pfnBufferMap               = IMG_NULL,
        .pfnBufferUnmap             = IMG_NULL,

        .pfnBufferSystemAcquire     = IMG_NULL,
        .pfnBufferSystemRelease     = IMG_NULL,
    };
	
    if(aw_create_sunxi_device(&gps_Disp_Device) != 0)//create our disp device.
    {
		DC_E_DPF(":create sunxi device failed \n");
		goto err_out;
    }

    //init work queue
	gps_config_queue = kmalloc(sizeof(CONFIG_DATA_QUEUE),GFP_KERNEL);
	if(!gps_config_queue)
	{
		DC_E_DPF(":Out of mem\n");
		goto err_kfree_dev;
	}
	memset(gps_config_queue,0,sizeof(CONFIG_DATA_QUEUE));
	spin_lock_init(&gps_config_queue->head_lock);

    //register it
    if(DCRegisterDevice(&sDCFunctions,
                        SUNXI_MAX_COMMANDS_IN_FLIGHT,
                        gps_Disp_Device,
                        &gps_Disp_Device->hSrvHandle) != PVRSRV_OK)
    {
        DC_E_DPF(": Should never failed here \n");
        goto err_kfree_dev;
    }
	
    return 0;
err_kfree_dev:
    kfree(gps_Disp_Device);
	if(gps_config_queue)
		kfree(gps_config_queue);
err_out:
	return -1;
}

static void __exit DC_SUNXI_DEV_exit(void)
{
    DC_SUNXI_DEVICE      *pDevice        = gps_Disp_Device;
	CONFIG_DATA_QUEUE    *pConfigQueue   = gps_config_queue;
	S_CONFIG_DATA        *pNextConfig;

	DCUnregisterDevice(pDevice->hSrvHandle);
    kfree(pDevice);

	if(pConfigQueue)
	{
	    while(pConfigQueue->head)
	    {
	        pNextConfig = pConfigQueue->head->pNextConfig;
	        kfree(pConfigQueue->head);
			pConfigQueue->head = pNextConfig;
	    }

		kfree(pConfigQueue);
	}	
}

module_init(DC_SUNXI_DEV_init);
module_exit(DC_SUNXI_DEV_exit);
