#ifndef _DISP_AL_H_
#define _DISP_AL_H_

#include "../bsp_display.h"
#include "../disp_private.h"
#include "de_hal.h"

extern int disp_al_manager_apply(unsigned int disp, struct disp_manager_data *data);
extern int disp_al_layer_apply(unsigned int disp, struct disp_layer_config_data *data, unsigned int layer_num);
extern s32 disp_init_al(disp_bsp_init_para * para);
extern int disp_al_manager_sync(unsigned int disp);
extern int disp_al_manager_update_regs(unsigned int disp);

#endif
