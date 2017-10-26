#include "disp_al.h"
#include "de_hal.h"

int disp_al_layer_apply(unsigned int disp, struct disp_layer_config_data *data, unsigned int layer_num)
{
	return de_al_lyr_apply(disp, data, layer_num);
}

int disp_al_manager_apply(unsigned int disp, struct disp_manager_data *data)
{
	return de_al_mgr_apply(disp, data);
}

int disp_al_manager_sync(unsigned int disp)
{
	return de_al_mgr_sync(disp);
}

int disp_al_manager_update_regs(unsigned int disp)
{
	return de_al_mgr_update_regs(disp);
}

int disp_init_al(disp_bsp_init_para * para)
{
	de_al_init(para);

	return 0;
}

