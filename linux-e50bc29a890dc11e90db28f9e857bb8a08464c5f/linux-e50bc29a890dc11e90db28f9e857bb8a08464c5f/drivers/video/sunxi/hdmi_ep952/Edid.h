/******************************************************************************\

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED 

--------------------------------------------------------------------------------

  File        :  Edid.h 

  Description :  Head file of Edid IO Interface 

\******************************************************************************/

#ifndef EDID_H
#define EDID_H

#define EDID_BLOCK_SIZE  128

enum EDID_Extension_Flag {
	EDID_EXT_CEA       = 0x02,
	EDID_EXT_BLOCK_MAP = 0xf0,
};

#define EDID_MAX_BLOCK_COUNT 8

#define EDID_ADDR       		0x50     // EDID Address
#define EDID_SEGMENT_PTR		0x30	 // EDID segment pointer

//
extern unsigned char EDID_GetHDMICap(unsigned char *pTarget);
extern unsigned char EDID_GetPCMFreqCap(unsigned char *pTarget);
extern unsigned char EDID_GetPCMChannelCap(unsigned char *pTarget);
extern unsigned char EDID_GetDataBlockAddr(unsigned char *pTarget, unsigned char Tag);
extern unsigned char EDID_GetCNCFlags(unsigned char *pTarget, unsigned char *CNC_flags);
extern unsigned char EDID_ParseVIC(unsigned char *pTarget, unsigned char *VIC_bitmap);
extern void EDID_ValidateVIC(void);
extern void EDID_InvalidateVIC(void);
extern int EDID_get_vic_support(unsigned char vic);
extern unsigned char EDID_ValidateBlockChecksum(unsigned char *pTarget);

#define VIC_max 127
extern unsigned char VIC_bitmap[((VIC_max - 1) / sizeof(char)) + 1];

#endif // EDID_H


