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
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_DRM

/* SDL internals */
#include "../../events/SDL_events_c.h"
#include "../SDL_sysvideo.h"
#include "SDL_events.h"
#include "SDL_loadso.h"
#include "SDL_syswm.h"
#include "SDL_version.h"

#ifdef SDL_INPUT_LINUXEV
#include "../../core/linux/SDL_evdev.h"
#endif

#include "SDL_DRM_video.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define DEFAULT_DRM_DEVICE "/dev/dri/card0"

static int DRM_OpenDev(int* fd) {
  char const* path;
  uint64_t    has_dumb;

  path = SDL_getenv("SDL_VIDEO_DRM_DEVICE");
  if (path == NULL) {
    path = DEFAULT_DRM_DEVICE;
  }

  *fd = open(path, O_RDWR | O_CLOEXEC);
  if (*fd < 0)
    return SDL_SetError("Unable to open DRI device %s", path);

  if (drmGetCap(*fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
    close(*fd);
    return SDL_SetError("DRI device cannot be used with dumb buffers");
  }

  return 0;
}

static void DRM_CloseDev(int fd) {
  if (fd >= 0)
    close(fd);
}

static int DRM_Available(void) {
  int fd;

  if (DRM_OpenDev(&fd) < 0)
    return 0;
  DRM_CloseDev(fd);

  return fd >= 0;
}

static void DRM_Destroy(SDL_VideoDevice* device) {
  SDL_DRM_VideoData* data = device->driverdata;
  if (data == NULL)
    return;

  drmModeFreeResources(data->res);
  DRM_CloseDev(data->fd);

  SDL_free(data);
  device->driverdata = NULL;
}

static SDL_VideoDevice* DRM_Create() {
  SDL_VideoDevice*   device;
  SDL_DRM_VideoData* data;

  device = (SDL_VideoDevice*)SDL_calloc(1, sizeof(SDL_VideoDevice));
  if (device == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  data = (SDL_DRM_VideoData*)SDL_calloc(1, sizeof(SDL_DRM_VideoData));
  if (data == NULL) {
    SDL_OutOfMemory();
    SDL_free(device);
    return NULL;
  }

  if (DRM_OpenDev(&data->fd)) {
    SDL_free(data);
    SDL_free(device);
    return NULL;
  }

  data->res = drmModeGetResources(data->fd);
  if (!data->res) {
    SDL_SetError("Unable to list DRM resources");
    SDL_free(data);
    SDL_free(device);
    return NULL;
  }

  device->driverdata = data;

  device->VideoInit                = DRM_VideoInit;
  device->VideoQuit                = DRM_VideoQuit;
  device->GetDisplayModes          = DRM_GetDisplayModes;
  device->SetDisplayMode           = DRM_SetDisplayMode;
  device->PumpEvents               = DRM_PumpEvents;
  device->CreateWindow             = DRM_CreateWindow;
  device->DestroyWindow            = DRM_DestroyWindow;
  device->CreateWindowFramebuffer  = DRM_CreateWindowFramebuffer;
  device->UpdateWindowFramebuffer  = DRM_UpdateWindowFramebuffer;
  device->DestroyWindowFramebuffer = DRM_DestroyWindowFramebuffer;

  device->free = DRM_Destroy;

  return device;
}

VideoBootStrap DRM_bootstrap = {"drm", "DRM Video Driver", DRM_Available,
                                DRM_Create};

static int DRM_IsVideoDisplayCRTCUsed(_THIS, uint32_t crtc_id) {
  int                  i;
  SDL_DRM_DisplayData* data;

  for (i = 0; i < _this->num_displays; ++i) {
    data = (SDL_DRM_DisplayData*)_this->displays[i].driverdata;
    if (data->crtc->crtc_id == crtc_id)
      return 1;
  }

  return 0;
}

static int DRM_SetVideoDisplayCRTC(_THIS, SDL_DRM_DisplayData* data) {
  SDL_DRM_VideoData* driver = _this->driverdata;
  drmModeRes*        res    = driver->res;
  drmModeConnector*  conn   = data->conn;
  drmModeEncoder*    enc;
  unsigned int       i, j;
  int                crtc_id = -1;

  if (conn->encoder_id)
    enc = drmModeGetEncoder(driver->fd, conn->encoder_id);
  else
    enc = NULL;

  if (enc) {
    if (!DRM_IsVideoDisplayCRTCUsed(_this, enc->crtc_id))
      crtc_id = enc->crtc_id;

    if (crtc_id < 0)
      drmModeFreeEncoder(enc);
  }

  for (i = 0; i < conn->count_encoders && crtc_id < 0; ++i) {
    enc = drmModeGetEncoder(driver->fd, conn->encoders[i]);
    if (!enc)
      continue;

    for (j = 0; j < res->count_crtcs && crtc_id < 0; ++j) {
      if (!(enc->possible_crtcs & (1 << j)))
        continue;

      if (!DRM_IsVideoDisplayCRTCUsed(_this, enc->crtc_id))
        crtc_id = enc->crtc_id;
    }

    if (crtc_id < 0)
      drmModeFreeEncoder(enc);
  }

  if (crtc_id < 0)
    return SDL_SetError("Unable to find CRTC for display %d",
                        conn->connector_id);

  data->enc  = enc;
  data->crtc = drmModeGetCrtc(driver->fd, crtc_id);

  return 0;
}

static int DRM_AddVideoDisplay(_THIS, drmModeConnector* conn) {
  SDL_DRM_DisplayData* data;
  SDL_VideoDisplay     display;
  int                  ret;

  data = (SDL_DRM_DisplayData*)SDL_calloc(1, sizeof(SDL_DRM_DisplayData));
  if (data == NULL)
    return SDL_OutOfMemory();

  data->conn = conn;

  if ((ret = DRM_SetVideoDisplayCRTC(_this, data)) < 0) {
    SDL_free(data);
    return ret;
  }

  SDL_zero(display);

  SDL_zero(display.desktop_mode);
  display.desktop_mode.w            = data->crtc->mode.hdisplay;
  display.desktop_mode.h            = data->crtc->mode.vdisplay;
  display.desktop_mode.refresh_rate = data->crtc->mode.vrefresh;

  display.current_mode = display.desktop_mode;
  display.driverdata   = data;

  SDL_AddVideoDisplay(&display);

  return 0;
}

static int DRM_AddVideoDisplays(_THIS) {
  SDL_DRM_VideoData* driver = _this->driverdata;
  drmModeRes*        res    = driver->res;
  drmModeConnector*  conn;
  unsigned int       i;
  int                ret;

  for (i = 0; i < res->count_connectors; ++i) {
    conn = drmModeGetConnector(driver->fd, res->connectors[i]);
    if (!conn)
      return SDL_SetError("Unable to get DRM connector %u", i);

    if (conn->connection != DRM_MODE_CONNECTED || conn->count_modes == 0) {
      drmModeFreeConnector(conn);
      continue;
    }

    if ((ret = DRM_AddVideoDisplay(_this, conn)) < 0) {
      drmModeFreeConnector(conn);
      return ret;
    }
  }

  return 0;
}

static void DRM_RemoveVideoDisplays(_THIS) {
  SDL_VideoDisplay*    display;
  SDL_DRM_DisplayData* data;
  int                  i;

  for (i = 0; i < _this->num_displays; ++i) {
    display = _this->displays + i;
    data    = (SDL_DRM_DisplayData*)display->driverdata;

    drmModeFreeConnector(data->conn);
    drmModeFreeEncoder(data->enc);
    drmModeFreeCrtc(data->crtc);
  }
}

int DRM_VideoInit(_THIS) {
  int ret;
  if ((ret = DRM_AddVideoDisplays(_this)) < 0)
    return ret;

#ifdef SDL_INPUT_LINUXEV
  if ((ret = SDL_EVDEV_Init()) < 0)
    return ret;
#endif

  return 0;
}

void DRM_VideoQuit(_THIS) {
#ifdef SDL_INPUT_LINUXEV
  SDL_EVDEV_Quit();
#endif

  DRM_RemoveVideoDisplays(_this);
}

void DRM_GetDisplayModes(_THIS, SDL_VideoDisplay* display) {
  SDL_AddDisplayMode(display, &display->current_mode);
}

int DRM_SetDisplayMode(_THIS, SDL_VideoDisplay* display,
                       SDL_DisplayMode* mode) {
  return 0;
}

void DRM_PumpEvents(_THIS) {
#ifdef SDL_INPUT_LINUXEV
  SDL_EVDEV_Poll();
#endif
}

int DRM_CreateWindow(_THIS, SDL_Window* window) {
  SDL_DRM_WindowData* data;
  SDL_VideoDisplay*   display;

  data = (SDL_DRM_WindowData*)SDL_calloc(1, sizeof(SDL_DRM_WindowData));
  if (data == NULL)
    return SDL_OutOfMemory();

  display = SDL_GetDisplayForWindow(window);
  data->conn_id =
      ((SDL_DRM_DisplayData*)display->driverdata)->conn->connector_id;
  data->crtc = ((SDL_DRM_DisplayData*)display->driverdata)->crtc;

  window->driverdata = data;

  window->flags |= SDL_WINDOW_FULLSCREEN;
  window->flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

  return 0;
}

void DRM_DestroyWindow(_THIS, SDL_Window* window) {
  SDL_DRM_WindowData* data = (SDL_DRM_WindowData*)window->driverdata;
  if (data == NULL)
    return;

  SDL_free(data);
  window->driverdata = NULL;
}

static int DRM_ToDRMPixelFormat(uint32_t sdl_format, uint32_t* drm_format,
                                int* bpp) {
  uint32_t rmask;
  uint32_t gmask;
  uint32_t bmask;
  uint32_t amask;

  SDL_PixelFormatEnumToMasks(sdl_format, bpp, &rmask, &gmask, &bmask, &amask);

  if (*bpp == 16) {
    if (rmask == 0xf800 && gmask == 0x07e0 && bmask == 0x001f)
      *drm_format = DRM_FORMAT_RGB565;
    else if (bmask == 0xf800 && gmask == 0x07e0 && rmask == 0x001f)
      *drm_format = DRM_FORMAT_BGR565;
    else
      return SDL_SetError(
          "Could not use pixel format %04x mask:%04x %04x %04x %04x",
          sdl_format, rmask, gmask, bmask, amask);
  } else if (*bpp == 32) {
    if (rmask == 0xff000000 && gmask == 0x00ff0000 && bmask == 0x0000ff00)
      *drm_format = DRM_FORMAT_RGB888;
    else if (bmask == 0xff000000 && gmask == 0x00ff0000 && rmask == 0x0000ff00)
      *drm_format = DRM_FORMAT_BGR888;
    else
      return SDL_SetError(
          "Could not use pixel format %04x mask:%04x %04x %04x %04x",
          sdl_format, rmask, gmask, bmask, amask);
  } else {
    return SDL_SetError("Could not use pixel format %04x", sdl_format);
  }

  return 0;
}

static int DRM_CreateFramebuffer(_THIS, SDL_DRM_Framebuffer* fb,
                                 uint32_t format, uint32_t width,
                                 uint32_t height) {
  SDL_DRM_VideoData*           driver = (SDL_DRM_VideoData*)_this->driverdata;
  struct drm_mode_create_dumb  creq;
  struct drm_mode_destroy_dumb dreq;
  struct drm_mode_map_dumb     mreq;
  int                          ret;
  int                          bpp;
  uint32_t                     drm_format = DRM_FORMAT_RGB565;

  if ((ret = DRM_ToDRMPixelFormat(format, &drm_format, &bpp)) < 0)
    return ret;

  SDL_zero(creq);
  creq.width  = width;
  creq.height = height;
  creq.bpp    = bpp;
  if ((ret = drmIoctl(driver->fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq)) < 0)
    return SDL_SetError("Could not create DRM dumb");

  SDL_zero(*fb);
  fb->handle = creq.handle;
  fb->pitch  = creq.pitch;
  fb->size   = creq.size;

  uint32_t bo_handles[4] = {fb->handle};
  uint32_t pitches[4]    = {fb->pitch};
  uint32_t offsets[4]    = {0};
  if ((ret = drmModeAddFB2(driver->fd, creq.width, creq.height, drm_format,
                           bo_handles, pitches, offsets, &fb->buffer_id, 0)) <
      0) {
    ret = SDL_SetError("Could not create framebuffer");
    goto destroy;
  }

  SDL_zero(mreq);
  mreq.handle = creq.handle;
  if ((ret = drmIoctl(driver->fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq)) < 0) {
    ret = SDL_SetError("Could not map framebuffer");
    goto remove;
  }

  if ((fb->pixels = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                         driver->fd, mreq.offset)) == MAP_FAILED) {
    ret = SDL_SetError("Could not mmap framebuffer");
    goto remove;
  }

  return 0;

remove:
  drmModeRmFB(driver->fd, fb->buffer_id);

destroy:
  SDL_zero(dreq);
  dreq.handle = fb->handle;
  drmIoctl(driver->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
  return ret;
}

int DRM_DestroyFramebuffer(_THIS, SDL_DRM_Framebuffer* fb) {
  SDL_DRM_VideoData*           driver = (SDL_DRM_VideoData*)_this->driverdata;
  struct drm_mode_destroy_dumb dreq;

  munmap(fb->pixels, fb->size);
  drmModeRmFB(driver->fd, fb->buffer_id);

  SDL_zero(dreq);
  dreq.handle = fb->handle;
  drmIoctl(driver->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);

  return 0;
}

int DRM_CreateWindowFramebuffer(_THIS, SDL_Window* window, Uint32* format,
                                void** pixels, int* pitch) {
  SDL_DRM_VideoData*  driver = (SDL_DRM_VideoData*)_this->driverdata;
  SDL_DRM_WindowData* data   = (SDL_DRM_WindowData*)window->driverdata;
  int                 ret;

  *format = SDL_PIXELFORMAT_RGB565;

  if ((ret = DRM_CreateFramebuffer(_this, &data->framebuffer, *format,
                                   window->w, window->h)) < 0)
    return ret;

  *pitch  = data->framebuffer.pitch;
  *pixels = data->framebuffer.pixels;

  data->prev_crtc = drmModeGetCrtc(driver->fd, data->crtc->crtc_id);
  drmModeSetCrtc(driver->fd, data->crtc->crtc_id, data->framebuffer.buffer_id,
                 0, 0, &data->conn_id, 1, &data->crtc->mode);

  return 0;
}

static void DRM_PageFlipHandler(int fd, unsigned int frame, unsigned int sec,
                                unsigned int usec, void* data) {}

int DRM_UpdateWindowFramebuffer(_THIS, SDL_Window* window,
                                const SDL_Rect* rects, int numrects) {
  SDL_DRM_VideoData*  driver = (SDL_DRM_VideoData*)_this->driverdata;
  SDL_DRM_WindowData* data   = (SDL_DRM_WindowData*)window->driverdata;
  fd_set              fds;
  drmEventContext     event;

  if (drmModePageFlip(driver->fd, data->crtc->crtc_id,
                      data->framebuffer.buffer_id, DRM_MODE_PAGE_FLIP_EVENT,
                      data) < 0)
    return SDL_SetError("Page flip request failed");

  SDL_zero(event);
  event.version           = 2;
  event.page_flip_handler = DRM_PageFlipHandler;

  FD_ZERO(&fds);
  FD_SET(driver->fd, &fds);
  if (select(driver->fd + 1, &fds, NULL, NULL, NULL) < 0) {
    return SDL_SetError("Wait for VSYNC interrupted");
  } else if (FD_ISSET(driver->fd, &fds)) {
    drmHandleEvent(driver->fd, &event);
  }

  return 0;
}

void DRM_DestroyWindowFramebuffer(_THIS, SDL_Window* window) {
  SDL_DRM_VideoData*  driver = (SDL_DRM_VideoData*)_this->driverdata;
  SDL_DRM_WindowData* data   = (SDL_DRM_WindowData*)window->driverdata;

  drmModeSetCrtc(driver->fd, data->prev_crtc->crtc_id,
                 data->prev_crtc->buffer_id, data->prev_crtc->x,
                 data->prev_crtc->y, &data->conn_id, 1, &data->prev_crtc->mode);
  drmModeFreeCrtc(data->prev_crtc);

  DRM_DestroyFramebuffer(_this, &data->framebuffer);
}

#endif /* SDL_VIDEO_DRIVER_DRM */

/* vi: set ts=4 sw=4 expandtab: */
