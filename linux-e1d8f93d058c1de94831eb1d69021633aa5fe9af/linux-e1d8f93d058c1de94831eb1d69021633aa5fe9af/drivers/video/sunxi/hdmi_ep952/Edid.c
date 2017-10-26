/******************************************************************************\

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED 

--------------------------------------------------------------------------------

  File        :  Edid.c 

  Description :  Implement the interfaces fo Edid

\******************************************************************************/

#include "Edid.h"
#include "EP952api.h"
#include <linux/mutex.h>

static DEFINE_MUTEX(VIC_lock);

unsigned char VIC_bitmap[((VIC_max - 1) / sizeof(char)) + 1];
int VIC_valid = 0;

enum {
	DATA_BLOCK_TAG_AUDIO    = 1,
	DATA_BLOCK_TAG_VIDEO    = 2,
	DATA_BLOCK_TAG_VENDOR   = 3,
};

//--------------------------------------------------------------------------------------------------

unsigned char EDID_GetHDMICap(unsigned char *pTarget)
{
	int i;

	if(pTarget[126] == 0) {
		return 0;
	}

	for(i=4; i<pTarget[EDID_BLOCK_SIZE+2]; ++i) {
		if((pTarget[EDID_BLOCK_SIZE+i] & 0xE0) == 0x60) { // find tag code - Vendor Specific Block
			if( (pTarget[EDID_BLOCK_SIZE+1+i] == 0x03) && (pTarget[EDID_BLOCK_SIZE+2+i] == 0x0C) && (pTarget[EDID_BLOCK_SIZE+3+i] == 0x00) ) {
				return 1;
			} else {
				return 0;
			}
		}
		else {
			i += (pTarget[EDID_BLOCK_SIZE+i] & 0x1F);
		}
	}
	if(i>=pTarget[EDID_BLOCK_SIZE+2]) { // Error, can not find the Vendor Specific Block
		return 0;
	}

	return 0;
}

unsigned char EDID_GetPCMFreqCap(unsigned char *pTarget)
{
	int i, j;

	if(pTarget[126] == 0) {
		return 0;
	}

	for(i=4; i<pTarget[EDID_BLOCK_SIZE+2]; ++i) {
		if((pTarget[EDID_BLOCK_SIZE+i] & 0xE0) == 0x20) { // find tag code - Audio Data Block
			for(j=1; j<(pTarget[EDID_BLOCK_SIZE+i] & 0x1F); j+=3) {
				if((pTarget[EDID_BLOCK_SIZE+i+j] >> 3) == 1) {
					return pTarget[EDID_BLOCK_SIZE+i+j+1];
				}
			}
		}
		else {
			i += (pTarget[EDID_BLOCK_SIZE+i] & 0x1F);
		}
	}
	if(i>=pTarget[EDID_BLOCK_SIZE+2]) { // Error, can not find the Audio Data Block
		return 0x07;
	}

	return 0;
}

unsigned char EDID_GetPCMChannelCap(unsigned char *pTarget)
{
	int i, j;
	unsigned char MaxPCMChannel = 1;

	if(pTarget[126] == 0) {
		return 0;
	}

	for(i=4; i<pTarget[EDID_BLOCK_SIZE+2]; ++i) {
		if((pTarget[EDID_BLOCK_SIZE+i] & 0xE0) == 0x20) { // find tag code - Audio Data Block
			for(j=1; j<(pTarget[EDID_BLOCK_SIZE+i] & 0x1F); j+=3) {
				if((pTarget[EDID_BLOCK_SIZE+i+j] >> 3) == 1) {
					//return pTarget[EDID_BLOCK_SIZE+i+j] & 0x07;
					MaxPCMChannel = max(MaxPCMChannel, pTarget[EDID_BLOCK_SIZE+i+j] & 0x07);
				}
			}
		}
		else {
			i += (pTarget[EDID_BLOCK_SIZE+i] & 0x1F);
		}
	}
	return MaxPCMChannel;

	return 0;
}

unsigned char EDID_GetDataBlockAddr(unsigned char *pTarget, unsigned char Tag)
{
	int i;

	if(pTarget[126] == 0) {
		return 0;
	}

	for(i=4; i<pTarget[EDID_BLOCK_SIZE+2]; ++i) {
		if((pTarget[EDID_BLOCK_SIZE+i] & 0xE0) == Tag) { // find tag code 
			return i+128;
		}
		else {
			i += (pTarget[EDID_BLOCK_SIZE+i] & 0x1F);
		}
	}
	if(i>=pTarget[EDID_BLOCK_SIZE+2]) { // Error, can not find
		return 0;
	}

	return 0;
}

unsigned char EDID_GetCNCFlags(unsigned char *pTarget, unsigned char *CNC_flags)
{
	int found, len, tag, dtd_off;
	unsigned char *block, *dtd_start, *cea_ext;

	*CNC_flags = 0;

	if(pTarget[126] == 0) {
		return 0;
	}

	// The vendor-specific data block is found in the first CEA extension
	cea_ext = pTarget + EDID_BLOCK_SIZE;
	dtd_off = cea_ext[2];
	dtd_start = cea_ext + dtd_off;

	found = 0;
	for(block = cea_ext + 4; block < dtd_start;) {

		len = ((*block) & 0x1F) + 1;
		tag = ((*block) & 0xE0) >> 5;

		if(tag == DATA_BLOCK_TAG_VENDOR) { // find tag code - Video Data Block
			found = 1;
			break;
		}
		else {
			block += len;
		}
	}

	if (!found || len < 9) {
		if (found) {
			DBG("vendor block does not contain CNC\n");
		} else {
			DBG("vendor block not found\n");
		}
		return 0;
	}

	*CNC_flags = block[8] & 0x0F;
	return 0;
}

unsigned char EDID_ValidateBlockChecksum(unsigned char *pTarget) {
	unsigned char *end = pTarget + 128;

	unsigned char checksum = 0;
	for (; pTarget < end; pTarget++) {
		checksum += *pTarget;
	}

	return checksum != 0;
}

unsigned char EDID_ParseVIC(unsigned char *pTarget, unsigned char *VIC_bitmap)
{
	int i, found, video_found = 0, len, tag, dtd_off, sz;
	unsigned char *block, *dtd_start, *cea_ext, *svd;

	if(pTarget[126] == 0) {
		return 0;
	}

	cea_ext = pTarget;

	for (i = 0; i < pTarget[126]; i++) {
		cea_ext += EDID_BLOCK_SIZE;
		dtd_off = cea_ext[2];
		dtd_start = cea_ext + dtd_off;

		found = 0;
		for(block = cea_ext + 4; block < dtd_start;) {

			len = ((*block) & 0x1F) + 1;
			tag = ((*block) & 0xE0) >> 5;

			if(tag == DATA_BLOCK_TAG_VIDEO) { // find tag code - Video Data Block
				found = 1;
				break;
			}
			else {
				block += len;
			}
		}

		if (!found) {
			continue;
		}

		video_found = 1;

		DBG("Parsing Video Data Block\n");
		mutex_lock(&VIC_lock);
		for (svd = block + 1; svd < block + len; ++svd) {
			i = (*svd) & 0x7f;
			DBG("Found VIC %i\n", i);
			sz = sizeof(VIC_bitmap[0]);
			VIC_bitmap[i / sz] |= (1 << (i % sz));
		}
		mutex_unlock(&VIC_lock);
		DBG("\n");
	}

	if (!video_found) {
		ERR("could not find Video Data Block in CEA extension\n");
		return 1;
	}

	return 0;
}

void EDID_ValidateVIC(void) {
	mutex_lock(&VIC_lock);
	// VIC_valid is 1 when the VIC array represents the state of a plugged screen,
	// regardless of potential VIC parsing errors
	VIC_valid = 1;
	mutex_unlock(&VIC_lock);
}

void EDID_InvalidateVIC(void) {
	mutex_lock(&VIC_lock);
	VIC_valid = 0;
	mutex_unlock(&VIC_lock);
}

int EDID_get_vic_support(unsigned char vic) {
	int sz = sizeof(VIC_bitmap[0]);
	int ret;

	if (vic > VIC_max) {
		return 0;
	}

	mutex_lock(&VIC_lock);
	if (!VIC_valid) {
		ret = 2;
	} else {
		ret = (VIC_bitmap[vic / sz] & (1 << (vic % sz))) >> (vic % sz);
	}
	mutex_unlock(&VIC_lock);
	return ret;
}
