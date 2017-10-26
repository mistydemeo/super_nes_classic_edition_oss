#include "disp_smart_backlight.h"

struct disp_smbl_info
{
	u32                      mode;
	disp_rect                window;
	u32                      enable;
};
struct disp_smbl_private_data
{
	u32                       irq_no;
	u32                       reg_base;
	bool                      user_info_dirty;
	struct disp_smbl_info     user_info;

	bool                      info_dirty;
	struct disp_smbl_info     info;

	bool                      shadow_info_dirty;
	s32 (*shadow_protect)(u32 sel, bool protect);

	u32                       enabled;
};
#if defined(__LINUX_PLAT__)
static spinlock_t smbl_data_lock;
#endif

#define SMBL_NO_AL
static struct disp_smbl *smbls = NULL;
static struct disp_smbl_private_data *smbl_private;

struct disp_smbl* disp_get_smbl(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(disp >= num_screens) {
		DE_WRN("screen_id %d out of range\n", disp);
		return NULL;
	}

#if !defined(SMBL_NO_AL)
	if(!disp_al_query_drc_mod(disp)) {
		//DE_WRN("drc %d is not registered\n", screen_id);
		return NULL;
	}
#endif
	return &smbls[disp];
}
struct disp_smbl_private_data *disp_smbl_get_priv(struct disp_smbl *smbl)
{
	if(NULL == smbl) {
		DE_INF("NULL hdl!\n");
		return NULL;
	}
#if !defined(SMBL_NO_AL)
	if(!disp_al_query_drc_mod(smbl->disp)) {
		DE_WRN("drc %d is not registered\n", smbl->disp);
		return NULL;
	}
#endif

	return &smbl_private[smbl->disp];
}

s32 disp_smbl_update_regs(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);
	struct disp_smbl_info smbl_info;

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		if((!smbl->manager) || (!smbl->manager->is_enabled) ||
			(!smbl->manager->is_enabled(smbl->manager)) || (!smblp->info_dirty)) {
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&smbl_data_lock, flags);
#endif
			return DIS_SUCCESS;
		}
		memcpy(&smbl_info, &smblp->info, sizeof(struct disp_smbl_info));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	/* if this func called by isr, can't call shadow protect
	 *    if called by non_isr, shadow protect is must
	 */
	//disp_smbl_shadow_protect(smbl, 1);
	DE_INF("smbl %d update_regs ok, enable=%d\n", smbl->disp, smbl_info.enable);
#if !defined(SMBL_NO_AL)
	disp_al_smbl_enable(smbl->disp, smbl_info.enable);
	disp_al_smbl_set_window(smbl->disp, &smbl_info.window);
#endif
	//disp_smbl_shadow_protect(smbl, 0);

#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->info_dirty = false;
		smblp->shadow_info_dirty = true;
		smblp->enabled = smbl_info.enable;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	return 0;
}

s32 disp_smbl_apply(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("smbl %d apply\n", smbl->disp);

#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		if(smblp->user_info_dirty) {
			memcpy(&smblp->info, &smblp->user_info, sizeof(struct disp_smbl_info));
			smblp->user_info_dirty = false;
			smblp->info_dirty = true;
		}
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	/* can't call update_regs at apply at single register buffer plat(e.g., a23,a31)
	 *   but it's recommended at double register buffer plat
	 */
	//disp_smbl_update_regs(smbl);
	return 0;
}

s32 disp_smbl_force_update_regs(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_smbl_force_update_regs, smbl %d\n", smbl->disp);
#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	disp_smbl_apply(smbl);

	return 0;
}

s32 disp_smbl_sync(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	disp_smbl_update_regs(smbl);
	if(smblp->shadow_info_dirty || smblp->enabled) {
#if !defined(SMBL_NO_AL)
		u32 backlight_dimming;
		disp_al_smbl_sync(smbl->disp);
#endif
#if !defined(SMBL_NO_AL)
		backlight_dimming = disp_al_smbl_get_backlight_dimming(smbl->disp);
#endif
	}
#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->shadow_info_dirty = false;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	return 0;
}

bool disp_smbl_is_enabled(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return false;
	}

	return (smblp->user_info.enable == 1);
}

s32 disp_smbl_enable(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("smbl %d enable\n", smbl->disp);
#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->user_info.enable = 1;
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	disp_smbl_apply(smbl);

	return 0;
}

s32 disp_smbl_disable(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("smbl %d disable\n", smbl->disp);

#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->user_info.enable = 0;
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	disp_smbl_apply(smbl);
	return 0;
}

s32 disp_smbl_shadow_protect(struct disp_smbl *smbl, bool protect)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if(smblp->shadow_protect)
		return smblp->shadow_protect(smbl->disp, protect);

	return -1;
}

s32 disp_smbl_set_window(struct disp_smbl* smbl, disp_rect *window)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
		unsigned long flags;
		spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		memcpy(&smblp->user_info.window, window, sizeof(disp_rect));
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
  disp_smbl_apply(smbl);

	return 0;
}

s32 disp_smbl_set_manager(struct disp_smbl* smbl, struct disp_manager *mgr)
{
	if((NULL == smbl) || (NULL == mgr)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smbl->manager = mgr;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif

	return 0;
}

s32 disp_smbl_init(struct disp_smbl *smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	return 0;
}

s32 disp_smbl_exit(struct disp_smbl *smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	return 0;
}

s32 disp_init_smbl(disp_bsp_init_para * para)
{
	u32 num_smbls;
	u32 disp;
	struct disp_smbl *smbl;
	struct disp_smbl_private_data *smblp;

	DE_INF("disp_init_smbl\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&smbl_data_lock);
#endif
	num_smbls = bsp_disp_feat_get_num_screens();
	smbls = (struct disp_smbl *)disp_sys_malloc(sizeof(struct disp_smbl) * num_smbls);
	if(NULL == smbls) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	smbl_private = (struct disp_smbl_private_data *)disp_sys_malloc(sizeof(struct disp_smbl_private_data) * num_smbls);
	if(NULL == smbl_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(disp=0; disp<num_smbls; disp++) {
		smbl = &smbls[disp];
		smblp = &smbl_private[disp];

		switch(disp) {
		case 0:
			smbl->name = "smbl0";
			smbl->disp = 0;

			break;
		case 1:
			smbl->name = "smbl1";
			smbl->disp = 1;

			break;
		case 2:
			smbl->name = "smbl2";
			smbl->disp = 2;

			break;
		}
		smblp->shadow_protect = para->shadow_protect;

		smbl->enable = disp_smbl_enable;
		smbl->disable = disp_smbl_disable;
		smbl->is_enabled = disp_smbl_is_enabled;
		smbl->init = disp_smbl_init;
		smbl->exit = disp_smbl_exit;
		smbl->apply = disp_smbl_apply;
		smbl->update_regs = disp_smbl_update_regs;
		smbl->sync = disp_smbl_sync;
		smbl->set_manager = disp_smbl_set_manager;
		smbl->set_window = disp_smbl_set_window;

		smbl->init(smbl);
	}

	return 0;
}

