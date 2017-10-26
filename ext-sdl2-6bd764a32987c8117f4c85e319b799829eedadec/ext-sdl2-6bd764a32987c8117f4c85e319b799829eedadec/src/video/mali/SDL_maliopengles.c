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

#if SDL_VIDEO_DRIVER_MALI && SDL_VIDEO_OPENGL_EGL

#include "SDL_maliopengles.h"
#include "SDL_malivideo.h"

int MALI_GLES_LoadLibrary(_THIS, const char *path) {
    if (_this->egl_data) {
        return SDL_SetError("OpenGL ES context already created");
    }

    _this->egl_data = (struct SDL_EGL_VideoData *) SDL_calloc(1, sizeof(SDL_EGL_VideoData));
    if (!_this->egl_data) {
        return SDL_OutOfMemory();
    }

    _this->egl_data->eglGetDisplay = &eglGetDisplay;
    _this->egl_data->eglInitialize = &eglInitialize;
    _this->egl_data->eglTerminate = &eglTerminate;
    // _this->egl_data->eglGetProcAddress = (void *(EGLAPIENTRY*) (const char*)) &eglGetProcAddress;
    _this->egl_data->eglChooseConfig = &eglChooseConfig;
    _this->egl_data->eglGetConfigAttrib = &eglGetConfigAttrib;
    _this->egl_data->eglCreateContext = &eglCreateContext;
    _this->egl_data->eglDestroyContext = &eglDestroyContext;
    _this->egl_data->eglCreateWindowSurface = &eglCreateWindowSurface;
    _this->egl_data->eglDestroySurface = &eglDestroySurface;
    _this->egl_data->eglMakeCurrent = &eglMakeCurrent;
    _this->egl_data->eglSwapBuffers = &eglSwapBuffers;
    _this->egl_data->eglSwapInterval = &eglSwapInterval;
    _this->egl_data->eglWaitNative = &eglWaitNative;
    _this->egl_data->eglWaitGL = &eglWaitGL;
    _this->egl_data->eglBindAPI = &eglBindAPI;
    _this->egl_data->eglQueryString = &eglQueryString;

    _this->egl_data->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!_this->egl_data->egl_display) {
        return SDL_SetError("Could not get EGL display");
    }
    
    if (eglInitialize(_this->egl_data->egl_display, NULL, NULL) != EGL_TRUE) {
        return SDL_SetError("Could not initialize EGL");
    }

    _this->gl_config.driver_loaded = 1;
    *_this->gl_config.driver_path = '\0';

    return 0;
}

void MALI_GLES_UnloadLibrary(_THIS) {
    if (_this->egl_data) {
        if (_this->egl_data->egl_display) {
            eglTerminate(_this->egl_data->egl_display);
            _this->egl_data->egl_display = NULL;
        }
        
        SDL_free(_this->egl_data);
        _this->egl_data = NULL;
    }
}

/* EGL implementation of SDL OpenGL support */
SDL_EGL_SwapWindow_impl(MALI)
SDL_EGL_MakeCurrent_impl(MALI)
SDL_EGL_CreateContext_impl(MALI)

#endif /* SDL_VIDEO_DRIVER_MALI && SDL_VIDEO_OPENGL_EGL */

/* vi: set ts=4 sw=4 expandtab: */

