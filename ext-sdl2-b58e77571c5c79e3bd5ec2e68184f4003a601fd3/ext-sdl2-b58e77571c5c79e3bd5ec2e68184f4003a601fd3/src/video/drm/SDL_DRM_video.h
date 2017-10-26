/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _SDL_drmvideo_h
#define _SDL_drmvideo_h

#include "../../SDL_internal.h"
#include "../SDL_sysvideo.h"

#include <xf86drm.h>
#include <xf86drmMode.h>

typedef struct SDL_DRM_VideoData {
  int           fd;
  drmModeResPtr res;
} SDL_DRM_VideoData;

typedef struct SDL_DRM_DisplayData {
  drmModeConnectorPtr conn;
  drmModeEncoderPtr   enc;
  drmModeCrtcPtr      crtc;
} SDL_DRM_DisplayData;

typedef struct SDL_DRM_Framebuffer {
  uint32_t handle;
  uint32_t pitch;
  uint64_t size;
  uint32_t buffer_id;
  uint8_t* pixels;
} SDL_DRM_Framebuffer;

typedef struct SDL_DRM_WindowData {
  uint32_t            conn_id;
  drmModeCrtcPtr      prev_crtc;
  drmModeCrtcPtr      crtc;
  SDL_DRM_Framebuffer framebuffer;
} SDL_DRM_WindowData;

int  DRM_VideoInit(_THIS);
void DRM_VideoQuit(_THIS);
void DRM_GetDisplayModes(_THIS, SDL_VideoDisplay* display);
int DRM_SetDisplayMode(_THIS, SDL_VideoDisplay* display, SDL_DisplayMode* mode);
void DRM_PumpEvents(_THIS);
int  DRM_CreateWindow(_THIS, SDL_Window* window);
void DRM_DestroyWindow(_THIS, SDL_Window* window);
int  DRM_CreateWindowFramebuffer(_THIS, SDL_Window* window, Uint32* format,
                                 void** pixels, int* pitch);
int  DRM_UpdateWindowFramebuffer(_THIS, SDL_Window* window,
                                 const SDL_Rect* rects, int numrects);
void DRM_DestroyWindowFramebuffer(_THIS, SDL_Window* window);

#endif /* _SDL_drmvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
