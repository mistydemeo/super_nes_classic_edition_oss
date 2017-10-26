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

#ifdef SDL_JOYSTICK_LINUX

#ifndef SDL_INPUT_LINUXEV
#error SDL now requires a Linux 2.4+ kernel with /dev/input/event support.
#endif

/* This is the Linux implementation of the SDL joystick API */

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <limits.h>             /* For the definition of PATH_MAX */
#include <linux/joystick.h>

#include "SDL_assert.h"
#include "SDL_joystick.h"
#include "SDL_endian.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#include "SDL_sysjoystick_c.h"
#include "SDL_thread.h"

/* !!! FIXME: move this somewhere else. */
#if !SDL_EVENTS_DISABLED
#include "../../events/SDL_events_c.h"
#endif

/* This isn't defined in older Linux kernel headers */
#ifndef SYN_DROPPED
#define SYN_DROPPED 3
#endif

#include "../../core/linux/SDL_udev.h"

static int MaybeAddDevice(const char *path);
#if SDL_USE_LIBUDEV
static int MaybeRemoveDevice(const char *path);
void joystick_udev_callback(SDL_UDEV_deviceevent udev_type, int udev_class, const char *devpath);
#endif /* SDL_USE_LIBUDEV */

#if SDL_USE_LIBUDEV && SDL_USE_UDEV_EVENT_THREAD
/*
SDL_DEV_EVENT_THREAD makes all close()s and the initial open() performed on device files
asynchronous. They are performed by a dedicated thread, and in the case of open() the result is then
retrieved by the main thread to carry on with execution.

This is useful when rendering is performed on the main thread, meaning that relatively long blocking operations
will produce visible or audible glitches. open() and close() on controller files have been
observed to take >100ms, an unacceptably long time for a device that should handle plugging and unplugging
controllers as part of its normal operation.

The implementation is very simple - we hook on the main thread and call ProcessDeviceEvents() from it
every time we poll devices for events. This method manages a work queue where requests for aysnc open()
and close() can be appended. If the queue is not empty, its first item is submitted to the worker
thread for execution.

At most one work item is submitted by the main thread at a time. Besides from making the implementation
simple, the upshot is that this guarantees that when detecting a controller device, it is asynchronously
close()d following its initial open() only after it has been reopened by the application. That is, instead
of the usual (and costly):
   open() -> [query device info] -> close() -> open(),
the order becomes:
   async open() -> open() -> async close()
The second, non-async open therefore opens an already-opened file and avoids whichever setup cost would
be required to open a closed device file. The following async close merely closes the initial file descriptor.

Note that the second open() cannot be made asynchronous, since it is called by code which expects to be able
to use the device as soon as the call returns.
*/
#define SDL_DEV_EVENT_THREAD 1
#endif

#if SDL_DEV_EVENT_THREAD
    SDL_Thread *device_event_thread;

    typedef int (* SDL_Async_Continuation) (int fd, const char* path, __dev_t st_dev);

    struct SDL_DE_Open_Job {
        const char *path;
        SDL_Async_Continuation cont;
        int fd;
        __dev_t st_dev;
    };

    struct SDL_DE_Close_Job {
        int fd;
    };

    struct SDL_DE_Quit_Job {};

    struct SDL_Device_Event_Job {
        enum {DE_OPEN_JOB, DE_CLOSE_JOB, DE_QUIT} type;
        enum {DE_NOJOB, DE_PROCESSED, DE_WAITING} status;
        union {
            struct SDL_DE_Open_Job openjob;
            struct SDL_DE_Close_Job closejob;
            struct SDL_DE_Quit_Job quitjob;
        } job;
    };

    #define MAX_DE_JOBS 16
    struct SDL_Device_Event_Job dejobs[MAX_DE_JOBS];
    static int dejob_begin = 0, dejob_end = 0;

    #define MAX_DEV_NAME_LEN 256
    typedef char SDL_Dev_Name[MAX_DEV_NAME_LEN];
    SDL_Dev_Name openjob_paths[MAX_DE_JOBS];

    struct SDL_Device_Event_Job curdejob;
    SDL_mutex *demutex;
    SDL_cond *decond;
#endif


/* A linked list of available joysticks */
typedef struct SDL_joylist_item
{
    int device_instance;
    char *path;   /* "/dev/input/event2" or whatever */
    char *name;   /* "SideWinder 3D Pro" or whatever */
    SDL_JoystickGUID guid;
    dev_t devnum;
    struct joystick_hwdata *hwdata;
    struct SDL_joylist_item *next;
} SDL_joylist_item;

static SDL_joylist_item *SDL_joylist = NULL;
static SDL_joylist_item *SDL_joylist_tail = NULL;
static int numjoysticks = 0;
static int instance_counter = 0;

#define test_bit(nr, addr) \
    (((1UL << ((nr) % (sizeof(long) * 8))) & ((addr)[(nr) / (sizeof(long) * 8)])) != 0)
#define NBITS(x) ((((x)-1)/(sizeof(long) * 8))+1)

static int
IsJoystick(int fd, char *namebuf, const size_t namebuflen, SDL_JoystickGUID *guid)
{
    struct input_id inpid;
    Uint16 *guid16 = (Uint16 *) ((char *) &guid->data);

#if !SDL_USE_LIBUDEV
    /* When udev is enabled we only get joystick devices here, so there's no need to test them */
    unsigned long evbit[NBITS(EV_MAX)] = { 0 };
    unsigned long keybit[NBITS(KEY_MAX)] = { 0 };
    unsigned long absbit[NBITS(ABS_MAX)] = { 0 };

    if ((ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
        (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) ||
        (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) < 0)) {
        return (0);
    }

    if (!(test_bit(EV_KEY, evbit) && test_bit(EV_ABS, evbit) &&
          test_bit(ABS_X, absbit) && test_bit(ABS_Y, absbit))) {
        return 0;
    }
#endif

    if (ioctl(fd, EVIOCGNAME(namebuflen), namebuf) < 0) {
        return 0;
    }

    if (ioctl(fd, EVIOCGID, &inpid) < 0) {
        return 0;
    }

#ifdef DEBUG_JOYSTICK
    printf("Joystick: %s, bustype = %d, vendor = 0x%x, product = 0x%x, version = %d\n", namebuf, inpid.bustype, inpid.vendor, inpid.product, inpid.version);
#endif

    SDL_memset(guid->data, 0, sizeof(guid->data));

    /* We only need 16 bits for each of these; space them out to fill 128. */
    /* Byteswap so devices get same GUID on little/big endian platforms. */
    *(guid16++) = SDL_SwapLE16(inpid.bustype);
    *(guid16++) = 0;

    if (inpid.vendor && inpid.product && inpid.version) {
        *(guid16++) = SDL_SwapLE16(inpid.vendor);
        *(guid16++) = 0;
        *(guid16++) = SDL_SwapLE16(inpid.product);
        *(guid16++) = 0;
        *(guid16++) = SDL_SwapLE16(inpid.version);
        *(guid16++) = 0;
    } else {
        SDL_strlcpy((char*)guid16, namebuf, sizeof(guid->data) - 4);
    }

    return 1;
}

#if SDL_USE_LIBUDEV
void joystick_udev_callback(SDL_UDEV_deviceevent udev_type, int udev_class, const char *devpath)
{
    if (devpath == NULL) {
        return;
    }

    switch (udev_type) {
        case SDL_UDEV_DEVICEADDED:
            if (!(udev_class & SDL_UDEV_DEVICE_JOYSTICK)) {
                return;
            }
            MaybeAddDevice(devpath);
            break;
            
        case SDL_UDEV_DEVICEREMOVED:
            MaybeRemoveDevice(devpath);
            break;
            
        default:
            break;
    }
    
}
#endif /* SDL_USE_LIBUDEV */

#if SDL_DEV_EVENT_THREAD

static int
DeviceEventLoop(void *data) {

    int quit = 0;
    struct SDL_Device_Event_Job job;
    struct SDL_DE_Close_Job *closejob;
    struct SDL_DE_Open_Job *openjob;

    SDL_mutexP(demutex);
    while (!quit && SDL_CondWait(decond, demutex) == 0) {
        job = curdejob;
        if (job.status != DE_WAITING) {
            SDL_assert(0);
            continue;
        }
        switch (curdejob.type) {
        case DE_QUIT:
            quit = 1;
            break;
        case DE_OPEN_JOB:
            SDL_mutexV(demutex);
            openjob = &job.job.openjob;
            openjob->fd = open(openjob->path, O_RDONLY, 0);
            SDL_mutexP(demutex);
            curdejob = job;
            break;
        case DE_CLOSE_JOB:
            SDL_mutexV(demutex);
            closejob = &job.job.closejob;
            close(closejob->fd);
            SDL_mutexP(demutex);
            break;
        }
        if (curdejob.type == DE_QUIT) {
            quit = 1;
        }
        curdejob.status = DE_PROCESSED;
    }
    SDL_mutexV(demutex);

    return 0;
}

/*
Sends a job, overriding whatever result there might have been waiting.
Only safe for DE_QUIT right now.
*/
static void SendDEJob(struct SDL_Device_Event_Job *job) {
    SDL_mutexP(demutex);
    curdejob = *job;
    curdejob.status = DE_WAITING;
    SDL_mutexV(demutex);
    SDL_CondSignal(decond);
}

static int DEJobQueueEmpty() {
    return dejob_begin == dejob_end;
}

static void QuitDEJobs() {
    struct SDL_Device_Event_Job quitjob;
    quitjob.type = DE_QUIT;
    SendDEJob(&quitjob);
    while (!DEJobQueueEmpty()) {
        struct SDL_Device_Event_Job* job = &dejobs[dejob_begin];
        if (job->type == DE_CLOSE_JOB) {
            close(job->job.closejob.fd);
        }
        dejob_begin = (dejob_begin + 1) % MAX_DE_JOBS;
    }
}

static int
PushDEJob(struct SDL_Device_Event_Job *job) {
    if ((dejob_end + 1) % MAX_DE_JOBS == dejob_begin) {
#ifdef DEBUG_INPUT_EVENTS
        printf("Device event queue is full\n");
#endif
        return 0;
    }

    dejobs[dejob_end] = *job;
    dejobs[dejob_end].status = DE_WAITING;
    if (job->type == DE_OPEN_JOB) {
        strncpy(openjob_paths[dejob_end], job->job.openjob.path, MAX_DEV_NAME_LEN);
        dejobs[dejob_end].job.openjob.path = openjob_paths[dejob_end];
    }
    dejob_end = (dejob_end + 1) % MAX_DE_JOBS;
    return 1;
}

static int
PopFirstDEJob(struct SDL_Device_Event_Job *job) {
    if (DEJobQueueEmpty()) {
        SDL_assert(0);
        return 0;
    }

    *job = dejobs[dejob_begin];
    dejob_begin = (dejob_begin + 1) % MAX_DE_JOBS;
    return 1;
}

/*
Processes incoming open / close device events on the main SDL thread, offloading the actual work to the
device event thread.
*/
static void
ProcessDeviceEvents(void)
{
    struct SDL_Device_Event_Job oldjob;
    struct SDL_DE_Open_Job *openjob;
    int signal = 0;

    SDL_mutexP(demutex);
    
    if (curdejob.status == DE_WAITING) {
        SDL_mutexV(demutex);
        return;
    }

    oldjob = curdejob;

    if (DEJobQueueEmpty()) {
        curdejob.status = DE_NOJOB;
    } else {
        PopFirstDEJob(&curdejob);
        signal = 1;
    }

    SDL_mutexV(demutex);

    if (signal) {
        SDL_CondSignal(decond);
    }

    if (oldjob.status == DE_PROCESSED && oldjob.type == DE_OPEN_JOB) {
        openjob = &oldjob.job.openjob;
        openjob->cont(openjob->fd, openjob->path, openjob->st_dev);
    }
}

/*
Schedules the asynchronous opening of a device, and continues execution from the main thread
at cont.

Returns 1 if the file open was successfully scheduled, 0 otherwise.
*/
static int
AsyncOpenDevice(const char *path, SDL_Async_Continuation cont, __dev_t st_dev)
{
    struct SDL_Device_Event_Job job;
    struct SDL_DE_Open_Job *openjob = &job.job.openjob;

    job.type = DE_OPEN_JOB;
    openjob->path = path;
    openjob->cont = cont;
    openjob->st_dev = st_dev;

    return PushDEJob(&job);
}

/*
Schedules the asynchronous closing of a device.

Returns 1 if the file close was successfully scheduled, a 0 otherwise.
*/
static int
AsyncCloseDevice(int fd)
{
    struct SDL_Device_Event_Job job;
    struct SDL_DE_Close_Job *closejob = &job.job.closejob;

    job.type = DE_CLOSE_JOB;
    closejob->fd = fd;

    return PushDEJob(&job);
}

#endif

/* !!! FIXME: I would love to dump this code and use libudev instead. */
static int
_MaybeAddDevice(int fd, const char *path, __dev_t st_dev)
{
    int isstick = 0;
    char namebuf[128];
    SDL_JoystickGUID guid;
    SDL_joylist_item *item;
#if !SDL_EVENTS_DISABLED
    SDL_Event event;
#endif
    int res;

    if (fd < 0) {
        return -1;
    }

#ifdef DEBUG_INPUT_EVENTS
    printf("Checking %s\n", path);
#endif

    isstick = IsJoystick(fd, namebuf, sizeof (namebuf), &guid);

#if SDL_DEV_EVENT_THREAD
    res = AsyncCloseDevice(fd);
#else
    res = 0;
#endif
    if (!res) {
        close(fd);
    }

    if (!isstick) {
        return -1;
    }

    item = (SDL_joylist_item *) SDL_malloc(sizeof (SDL_joylist_item));
    if (item == NULL) {
        return -1;
    }

    SDL_zerop(item);
    item->devnum = st_dev;
    item->path = SDL_strdup(path);
    item->name = SDL_strdup(namebuf);
    item->guid = guid;

    if ( (item->path == NULL) || (item->name == NULL) ) {
         SDL_free(item->path);
         SDL_free(item->name);
         SDL_free(item);
         return -1;
    }

    item->device_instance = instance_counter++;
    if (SDL_joylist_tail == NULL) {
        SDL_joylist = SDL_joylist_tail = item;
    } else {
        SDL_joylist_tail->next = item;
        SDL_joylist_tail = item;
    }

    /* Need to increment the joystick count before we post the event */
    ++numjoysticks;

    /* !!! FIXME: Move this to an SDL_PrivateJoyDeviceAdded() function? */
#if !SDL_EVENTS_DISABLED
    event.type = SDL_JOYDEVICEADDED;

    if (SDL_GetEventState(event.type) == SDL_ENABLE) {
        event.jdevice.which = (numjoysticks - 1);
        if ( (SDL_EventOK == NULL) ||
             (*SDL_EventOK) (SDL_EventOKParam, &event) ) {
            SDL_PushEvent(&event);
        }
    }
#endif /* !SDL_EVENTS_DISABLED */

    return numjoysticks;
}

static int
MaybeAddDevice(const char *path)
{
    struct stat sb;
    int fd = -1;
    int res;
    SDL_joylist_item *item;

    if (path == NULL) {
        return -1;
    }

    if (stat(path, &sb) == -1) {
        return -1;
    }

    /* Check to make sure it's not already in list. */
    for (item = SDL_joylist; item != NULL; item = item->next) {
        if (sb.st_rdev == item->devnum) {
            return -1;  /* already have this one */
        }
    }

#if SDL_DEV_EVENT_THREAD
    res = AsyncOpenDevice(path, _MaybeAddDevice, sb.st_dev);
    /* if we fail to schedule an asynchronous file open, we fall back to synchronous open */
#else
    res = 0;
#endif

    if (!res) {
        fd = open(path, O_RDONLY, 0);
        return _MaybeAddDevice(fd, path, sb.st_dev);
    }

    /* Dummy value, doesn't seem to be used */
    return 1;
}

#if SDL_USE_LIBUDEV
/* !!! FIXME: I would love to dump this code and use libudev instead. */
static int
MaybeRemoveDevice(const char *path)
{
    SDL_joylist_item *item;
    SDL_joylist_item *prev = NULL;
#if !SDL_EVENTS_DISABLED
    SDL_Event event;
#endif

    if (path == NULL) {
        return -1;
    }

    for (item = SDL_joylist; item != NULL; item = item->next) {
        /* found it, remove it. */
        if (SDL_strcmp(path, item->path) == 0) {
            const int retval = item->device_instance;
            if (item->hwdata) {
                item->hwdata->item = NULL;
            }
            if (prev != NULL) {
                prev->next = item->next;
            } else {
                SDL_assert(SDL_joylist == item);
                SDL_joylist = item->next;
            }
            if (item == SDL_joylist_tail) {
                SDL_joylist_tail = prev;
            }

            /* Need to decrement the joystick count before we post the event */
            --numjoysticks;

            /* !!! FIXME: Move this to an SDL_PrivateJoyDeviceRemoved() function? */
#if !SDL_EVENTS_DISABLED
            event.type = SDL_JOYDEVICEREMOVED;

            if (SDL_GetEventState(event.type) == SDL_ENABLE) {
                event.jdevice.which = item->device_instance;
                if ( (SDL_EventOK == NULL) ||
                     (*SDL_EventOK) (SDL_EventOKParam, &event) ) {
                    SDL_PushEvent(&event);
                }
            }
#endif /* !SDL_EVENTS_DISABLED */

            SDL_free(item->path);
            SDL_free(item->name);
            SDL_free(item);
            return retval;
        }
        prev = item;
    }

    return -1;
}
#endif

static int
JoystickInitWithoutUdev(void)
{
    int i;
    char path[PATH_MAX];

    /* !!! FIXME: only finds sticks if they're called /dev/input/event[0..31] */
    /* !!! FIXME:  we could at least readdir() through /dev/input...? */
    /* !!! FIXME:  (or delete this and rely on libudev?) */
    for (i = 0; i < 32; i++) {
        SDL_snprintf(path, SDL_arraysize(path), "/dev/input/event%d", i);
        MaybeAddDevice(path);
    }

    return numjoysticks;
}

#if SDL_USE_LIBUDEV
static int
JoystickInitWithUdev(void)
{
    if (SDL_UDEV_Init() < 0) {
        return SDL_SetError("Could not initialize UDEV");
    }

    /* Set up the udev callback */
    if (SDL_UDEV_AddCallback(joystick_udev_callback) < 0) {
        SDL_UDEV_Quit();
        return SDL_SetError("Could not set up joystick <-> udev callback");
    }
    
#if SDL_DEV_EVENT_THREAD
    decond = SDL_CreateCond();
    demutex = SDL_CreateMutex();
    /* Using the mutex for its barrier */
    SDL_mutexP(demutex);
    curdejob.status = DE_NOJOB;
    SDL_mutexV(demutex);
    device_event_thread = SDL_CreateThread(DeviceEventLoop, "devevent", NULL);
    if (!device_event_thread) {
        SDL_UDEV_DelCallback(joystick_udev_callback);
        return SDL_SetError("Could not create device event thread");
    }
#endif

    /* Force a scan to build the initial device list */
    SDL_UDEV_Scan();

    return numjoysticks;
}
#endif

int
SDL_SYS_JoystickInit(void)
{
    /* First see if the user specified one or more joysticks to use */
    if (SDL_getenv("SDL_JOYSTICK_DEVICE") != NULL) {
        char *envcopy, *envpath, *delim;
        envcopy = SDL_strdup(SDL_getenv("SDL_JOYSTICK_DEVICE"));
        envpath = envcopy;
        while (envpath != NULL) {
            delim = SDL_strchr(envpath, ':');
            if (delim != NULL) {
                *delim++ = '\0';
            }
            MaybeAddDevice(envpath);
            envpath = delim;
        }
        SDL_free(envcopy);
    }

#if SDL_USE_LIBUDEV
    return JoystickInitWithUdev();
#endif

    return JoystickInitWithoutUdev();
}

int SDL_SYS_NumJoysticks()
{
    return numjoysticks;
}

void SDL_SYS_JoystickDetect()
{
#if SDL_USE_LIBUDEV
    SDL_UDEV_Poll();
#endif
#if SDL_DEV_EVENT_THREAD
    ProcessDeviceEvents();
#endif
}

static SDL_joylist_item *
JoystickByDevIndex(int device_index)
{
    SDL_joylist_item *item = SDL_joylist;

    if ((device_index < 0) || (device_index >= numjoysticks)) {
        return NULL;
    }

    while (device_index > 0) {
        SDL_assert(item != NULL);
        device_index--;
        item = item->next;
    }

    return item;
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickNameForDeviceIndex(int device_index)
{
    return JoystickByDevIndex(device_index)->name;
}

/* Function to perform the mapping from device index to the instance id for this index */
SDL_JoystickID SDL_SYS_GetInstanceIdOfDeviceIndex(int device_index)
{
    return JoystickByDevIndex(device_index)->device_instance;
}

static int
allocate_hatdata(SDL_Joystick * joystick)
{
    int i;

    joystick->hwdata->hats =
        (struct hwdata_hat *) SDL_malloc(joystick->nhats *
                                         sizeof(struct hwdata_hat));
    if (joystick->hwdata->hats == NULL) {
        return (-1);
    }
    for (i = 0; i < joystick->nhats; ++i) {
        joystick->hwdata->hats[i].axis[0] = 1;
        joystick->hwdata->hats[i].axis[1] = 1;
    }
    return (0);
}

static int
allocate_balldata(SDL_Joystick * joystick)
{
    int i;

    joystick->hwdata->balls =
        (struct hwdata_ball *) SDL_malloc(joystick->nballs *
                                          sizeof(struct hwdata_ball));
    if (joystick->hwdata->balls == NULL) {
        return (-1);
    }
    for (i = 0; i < joystick->nballs; ++i) {
        joystick->hwdata->balls[i].axis[0] = 0;
        joystick->hwdata->balls[i].axis[1] = 0;
    }
    return (0);
}

static void
ConfigJoystick(SDL_Joystick * joystick, int fd)
{
    int i, t;
    unsigned long keybit[NBITS(KEY_MAX)] = { 0 };
    unsigned long absbit[NBITS(ABS_MAX)] = { 0 };
    unsigned long relbit[NBITS(REL_MAX)] = { 0 };

    /* See if this device uses the new unified event API */
    if ((ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) &&
        (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0) &&
        (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit) >= 0)) {

        /* Get the number of buttons, axes, and other thingamajigs */
        for (i = BTN_JOYSTICK; i < KEY_MAX; ++i) {
            if (test_bit(i, keybit)) {
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has button: 0x%x\n", i);
#endif
                joystick->hwdata->key_map[i - BTN_MISC] = joystick->nbuttons;
                ++joystick->nbuttons;
            }
        }
        for (i = BTN_MISC; i < BTN_JOYSTICK; ++i) {
            if (test_bit(i, keybit)) {
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has button: 0x%x\n", i);
#endif
                joystick->hwdata->key_map[i - BTN_MISC] = joystick->nbuttons;
                ++joystick->nbuttons;
            }
        }
        for (i = 0; i < ABS_MAX; ++i) {
            /* Skip hats */
            if (i == ABS_HAT0X) {
                i = ABS_HAT3Y;
                continue;
            }
            if (test_bit(i, absbit)) {
                struct input_absinfo absinfo;

                if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0) {
                    continue;
                }
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has absolute axis: 0x%.2x\n", i);
                printf("Values = { %d, %d, %d, %d, %d }\n",
                       absinfo.value, absinfo.minimum, absinfo.maximum,
                       absinfo.fuzz, absinfo.flat);
#endif /* DEBUG_INPUT_EVENTS */
                joystick->hwdata->abs_map[i] = joystick->naxes;
                if (absinfo.minimum == absinfo.maximum) {
                    joystick->hwdata->abs_correct[i].used = 0;
                } else {
                    joystick->hwdata->abs_correct[i].used = 1;
                    joystick->hwdata->abs_correct[i].coef[0] =
                        (absinfo.maximum + absinfo.minimum) - 2 * absinfo.flat;
                    joystick->hwdata->abs_correct[i].coef[1] =
                        (absinfo.maximum + absinfo.minimum) + 2 * absinfo.flat;
                    t = ((absinfo.maximum - absinfo.minimum) - 4 * absinfo.flat);
                    if (t != 0) {
                        joystick->hwdata->abs_correct[i].coef[2] =
                            (1 << 28) / t;
                    } else {
                        joystick->hwdata->abs_correct[i].coef[2] = 0;
                    }
                }
                ++joystick->naxes;
            }
        }
        for (i = ABS_HAT0X; i <= ABS_HAT3Y; i += 2) {
            if (test_bit(i, absbit) || test_bit(i + 1, absbit)) {
                struct input_absinfo absinfo;

                if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0) {
                    continue;
                }
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has hat %d\n", (i - ABS_HAT0X) / 2);
                printf("Values = { %d, %d, %d, %d, %d }\n",
                       absinfo.value, absinfo.minimum, absinfo.maximum,
                       absinfo.fuzz, absinfo.flat);
#endif /* DEBUG_INPUT_EVENTS */
                ++joystick->nhats;
            }
        }
        if (test_bit(REL_X, relbit) || test_bit(REL_Y, relbit)) {
            ++joystick->nballs;
        }

        /* Allocate data to keep track of these thingamajigs */
        if (joystick->nhats > 0) {
            if (allocate_hatdata(joystick) < 0) {
                joystick->nhats = 0;
            }
        }
        if (joystick->nballs > 0) {
            if (allocate_balldata(joystick) < 0) {
                joystick->nballs = 0;
            }
        }
    }
}

/* Function to open a joystick for use.
   The joystick to open is specified by the device index.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick, int device_index)
{
    SDL_joylist_item *item = JoystickByDevIndex(device_index);
    char *fname = NULL;
    int fd = -1;
    int res;

    if (item == NULL) {
        return SDL_SetError("No such device");
    }

    fname = item->path;
    /* Can't make this one async, the device is usable once this returns */
    fd = open(fname, O_RDONLY, 0);
    if (fd < 0) {
        return SDL_SetError("Unable to open %s", fname);
    }


    joystick->instance_id = item->device_instance;
    joystick->hwdata = (struct joystick_hwdata *)
        SDL_malloc(sizeof(*joystick->hwdata));
    if (joystick->hwdata == NULL) {
#if SDL_DEV_EVENT_THREAD
        res = AsyncCloseDevice(fd);
#else
        res = 0;
#endif
        if (!res) {
            close(fd);
        }
        return SDL_OutOfMemory();
    }
    SDL_memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));
    joystick->hwdata->item = item;
    joystick->hwdata->guid = item->guid;
    joystick->hwdata->fd = fd;
    joystick->hwdata->fname = SDL_strdup(item->path);
    if (joystick->hwdata->fname == NULL) {
        SDL_free(joystick->hwdata);
        joystick->hwdata = NULL;
#if SDL_DEV_EVENT_THREAD
        res = AsyncCloseDevice(fd);
#else
        res = 0;
#endif
        if (!res) {
            close(fd);
        }
        return SDL_OutOfMemory();
    }

    SDL_assert(item->hwdata == NULL);
    item->hwdata = joystick->hwdata;

    /* Set the joystick to non-blocking read mode */
    fcntl(fd, F_SETFL, O_NONBLOCK);

    /* Get the number of buttons and axes on the joystick */
    ConfigJoystick(joystick, fd);

    /* mark joystick as fresh and ready */
    joystick->hwdata->fresh = 1;

    return (0);
}

/* Function to determine if this joystick is attached to the system right now */
SDL_bool SDL_SYS_JoystickAttached(SDL_Joystick *joystick)
{
    return joystick->hwdata->item != NULL;
}

static SDL_INLINE void
HandleHat(SDL_Joystick * stick, Uint8 hat, int axis, int value)
{
    struct hwdata_hat *the_hat;
    const Uint8 position_map[3][3] = {
        {SDL_HAT_LEFTUP, SDL_HAT_UP, SDL_HAT_RIGHTUP},
        {SDL_HAT_LEFT, SDL_HAT_CENTERED, SDL_HAT_RIGHT},
        {SDL_HAT_LEFTDOWN, SDL_HAT_DOWN, SDL_HAT_RIGHTDOWN}
    };

    the_hat = &stick->hwdata->hats[hat];
    if (value < 0) {
        value = 0;
    } else if (value == 0) {
        value = 1;
    } else if (value > 0) {
        value = 2;
    }
    if (value != the_hat->axis[axis]) {
        the_hat->axis[axis] = value;
        SDL_PrivateJoystickHat(stick, hat,
                               position_map[the_hat->
                                            axis[1]][the_hat->axis[0]]);
    }
}

static SDL_INLINE void
HandleBall(SDL_Joystick * stick, Uint8 ball, int axis, int value)
{
    stick->hwdata->balls[ball].axis[axis] += value;
}


static SDL_INLINE int
AxisCorrect(SDL_Joystick * joystick, int which, int value)
{
    struct axis_correct *correct;

    correct = &joystick->hwdata->abs_correct[which];
    if (correct->used) {
        value *= 2;
        if (value > correct->coef[0]) {
            if (value < correct->coef[1]) {
                return 0;
            }
            value -= correct->coef[1];
        } else {
            value -= correct->coef[0];
        }
        value *= correct->coef[2];
        value >>= 13;
    }

    /* Clamp and return */
    if (value < -32768)
        return -32768;
    if (value > 32767)
        return 32767;

    return value;
}

static SDL_INLINE void
PollAllValues(SDL_Joystick * joystick)
{
    struct input_absinfo absinfo;
    int i, a, b = 0;

    /* Poll all axis */
    for (a = ABS_X; b < ABS_MAX; a++) {
        switch (a) {
        case ABS_HAT0X:
        case ABS_HAT0Y:
        case ABS_HAT1X:
        case ABS_HAT1Y:
        case ABS_HAT2X:
        case ABS_HAT2Y:
        case ABS_HAT3X:
        case ABS_HAT3Y:
            /* ingore hats */
            break;
        default:
            if (joystick->hwdata->abs_correct[b].used) {
                if (ioctl(joystick->hwdata->fd, EVIOCGABS(a), &absinfo) >= 0) {
                    absinfo.value = AxisCorrect(joystick, b, absinfo.value);

#ifdef DEBUG_INPUT_EVENTS
                    printf("Joystick : Re-read Axis %d (%d) val= %d\n",
                        joystick->hwdata->abs_map[b], a, absinfo.value);
#endif
                    SDL_PrivateJoystickAxis(joystick,
                            joystick->hwdata->abs_map[b],
                            absinfo.value);
                }
            }
            b++;
        }
    }

    /* Poll all keys */
    uint8_t keyvalue[KEY_MAX / 8 + 1] = { 0 };
    ioctl(joystick->hwdata->fd, EVIOCGKEY(sizeof(keyvalue)), keyvalue);
    for (i = BTN_MISC; i < KEY_MAX; ++i) {
        if (keyvalue[i / 8] & (1 << (i % 8)))
            SDL_PrivateJoystickButton(joystick, joystick->hwdata->key_map[i - BTN_MISC], SDL_PRESSED);
        else
            SDL_PrivateJoystickButton(joystick, joystick->hwdata->key_map[i - BTN_MISC], SDL_RELEASED);
    }
}

static SDL_INLINE void
HandleInputEvents(SDL_Joystick * joystick)
{
    struct input_event events[32];
    int i, len;
    int code;

    if (joystick->hwdata->fresh) {
        PollAllValues(joystick);
        joystick->hwdata->fresh = 0;
    }

    while ((len = read(joystick->hwdata->fd, events, (sizeof events))) > 0) {
        len /= sizeof(events[0]);
        for (i = 0; i < len; ++i) {
            code = events[i].code;
            switch (events[i].type) {
            case EV_KEY:
                if (code >= BTN_MISC) {
                    code -= BTN_MISC;
                    SDL_PrivateJoystickButton(joystick,
                                              joystick->hwdata->key_map[code],
                                              events[i].value);
                }
                break;
            case EV_ABS:
                switch (code) {
                case ABS_HAT0X:
                case ABS_HAT0Y:
                case ABS_HAT1X:
                case ABS_HAT1Y:
                case ABS_HAT2X:
                case ABS_HAT2Y:
                case ABS_HAT3X:
                case ABS_HAT3Y:
                    code -= ABS_HAT0X;
                    HandleHat(joystick, code / 2, code % 2, events[i].value);
                    break;
                default:
                    events[i].value =
                        AxisCorrect(joystick, code, events[i].value);
                    SDL_PrivateJoystickAxis(joystick,
                                            joystick->hwdata->abs_map[code],
                                            events[i].value);
                    break;
                }
                break;
            case EV_REL:
                switch (code) {
                case REL_X:
                case REL_Y:
                    code -= REL_X;
                    HandleBall(joystick, code / 2, code % 2, events[i].value);
                    break;
                default:
                    break;
                }
                break;
            case EV_SYN:
                switch (code) {
                case SYN_DROPPED :
#ifdef DEBUG_INPUT_EVENTS
                    printf("Event SYN_DROPPED detected\n");
#endif
                    PollAllValues(joystick);
                    break;
                default:
                    break;
                }
            default:
                break;
            }
        }
    }
}

void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
    int i;

    HandleInputEvents(joystick);

    /* Deliver ball motion updates */
    for (i = 0; i < joystick->nballs; ++i) {
        int xrel, yrel;

        xrel = joystick->hwdata->balls[i].axis[0];
        yrel = joystick->hwdata->balls[i].axis[1];
        if (xrel || yrel) {
            joystick->hwdata->balls[i].axis[0] = 0;
            joystick->hwdata->balls[i].axis[1] = 0;
            SDL_PrivateJoystickBall(joystick, (Uint8) i, xrel, yrel);
        }
    }
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
    int fd = -1;
    int res;

    if (joystick->hwdata) {
        fd = joystick->hwdata->fd;
#if SDL_DEV_EVENT_THREAD
        res = AsyncCloseDevice(fd);
#else
        res = 0;
#endif
        if (!res) {
            close(fd);
        }
        if (joystick->hwdata->item) {
            joystick->hwdata->item->hwdata = NULL;
        }
        SDL_free(joystick->hwdata->hats);
        SDL_free(joystick->hwdata->balls);
        SDL_free(joystick->hwdata->fname);
        SDL_free(joystick->hwdata);
    }
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    SDL_joylist_item *item = NULL;
    SDL_joylist_item *next = NULL;
    for (item = SDL_joylist; item; item = next) {
        next = item->next;
        SDL_free(item->path);
        SDL_free(item->name);
        SDL_free(item);
    }

    SDL_joylist = SDL_joylist_tail = NULL;

    numjoysticks = 0;
    instance_counter = 0;

#if SDL_DEV_EVENT_THREAD
    QuitDEJobs();
    SDL_WaitThread(device_event_thread, NULL);
    SDL_DestroyMutex(demutex);
    SDL_DestroyCond(decond);
#endif

#if SDL_USE_LIBUDEV
    SDL_UDEV_DelCallback(joystick_udev_callback);
    SDL_UDEV_Quit();
#endif
}

SDL_JoystickGUID SDL_SYS_JoystickGetDeviceGUID( int device_index )
{
    return JoystickByDevIndex(device_index)->guid;
}

SDL_JoystickGUID SDL_SYS_JoystickGetGUID(SDL_Joystick * joystick)
{
    return joystick->hwdata->guid;
}

#endif /* SDL_JOYSTICK_LINUX */

/* vi: set ts=4 sw=4 expandtab: */
