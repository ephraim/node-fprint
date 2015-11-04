#include "fingerprint.h"
#include "enroll.h"
#include "verify.h"

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

using namespace v8;
using v8::FunctionTemplate;

using namespace std;

typedef struct __POLL_DATA__ {
    uv_thread_t thread;
    bool exit;
} POLL_DATA;

Nan::Persistent<Object> module_handle;

int initalized = -1;
static POLL_DATA *polldata = NULL;

unsigned int fromFPDev(struct fp_dev *dev)
{
    union {
        struct fp_dev *dev;
        unsigned int value;
    } fpDevice;

    fpDevice.dev = dev;
    return fpDevice.value;
}

struct fp_dev* toFPDev(unsigned int value)
{
    union {
        struct fp_dev *dev;
        unsigned int value;
    } fpDevice;

    fpDevice.value = value;
    return fpDevice.dev;
}


NAN_METHOD(setDebug)
{
    int debug;

    if(info.Length() == 1) {
        debug = info[0]->ToInteger()->Value();

        if(initalized != 0)
            return;

        if(debug > 0)
            fp_set_debug(debug);
    }
}

NAN_METHOD(getEnrollStages) {
    struct fp_dev *dev;

    if(info.Length() != 1)
        return;

    dev = toFPDev(info[0]->ToNumber()->Value());
    if(initalized != 0 || dev == NULL)
        return;

    info.GetReturnValue().Set(fp_dev_get_nr_enroll_stages(dev));
}

NAN_METHOD(discoverDevices)
{
    struct fp_dscv_dev **discovered_devs;
    struct fp_driver *drv;
    int devCount = 0;

    if(initalized != 0)
        return;

    discovered_devs = fp_discover_devs();
    for(int i = 0; discovered_devs[i] != NULL; i++)
        devCount++;

    v8::Local<v8::Array> arr = Nan::New<v8::Array>(devCount);
    for(int i = 0; discovered_devs[i] != NULL; i++) {
        drv = fp_dscv_dev_get_driver(discovered_devs[i]);
        Nan::Set(arr, i, Nan::New(fp_driver_get_full_name(drv)).ToLocalChecked());
    }
    fp_dscv_devs_free(discovered_devs);
    info.GetReturnValue().Set(arr);
}

NAN_METHOD(openDevice)
{
    if(info.Length() == 1) {
        struct fp_dscv_dev **discovered_devs;
        struct fp_driver *drv;
        struct fp_dev *dev = NULL;
        string drivername;

        drivername = string(*v8::String::Utf8Value(info[0]->ToString()));
        if(initalized != 0)
            return;

        discovered_devs = fp_discover_devs();
        for(int i = 0; discovered_devs[i] != NULL; i++) {
            drv = fp_dscv_dev_get_driver(discovered_devs[i]);
            if(drivername.compare(fp_driver_get_full_name(drv)) == 0) {
                dev = fp_dev_open(discovered_devs[i]);
                break;
            }
        }
        fp_dscv_devs_free(discovered_devs);
        if(dev) {
            info.GetReturnValue().Set(fromFPDev(dev));
        }
    }
}

NAN_METHOD(closeDevice)
{
    if(info.Length() == 1) {
        struct fp_dev *dev;
        dev = toFPDev(info[0]->ToNumber()->Value());
        if(initalized != 0)
            return;

        if(dev)
            fp_dev_close(dev);
    }
}

void poll_fp(void *d)
{
    POLL_DATA *polldata = (POLL_DATA*)d;
    struct timeval zerotimeout = {
        .tv_sec = 0,
        .tv_usec = 0,
    };

    if(!polldata)
        return;

    while(!polldata->exit) {
        fp_handle_events_timeout(&zerotimeout);
    }
    delete polldata;
    polldata = NULL;
}

NAN_METHOD(init)
{
    initalized = fp_init();

    if(initalized == 0) {
        polldata = new POLL_DATA;
        polldata->exit = false;
        uv_thread_create(&polldata->thread, poll_fp, (void*)polldata);
    }
    info.GetReturnValue().Set(initalized == 0);
}

NAN_METHOD(exit)
{
    if(polldata)
        polldata->exit = true;

    fp_exit();
    initalized = -1;
}

NAN_MODULE_INIT(module_init) {
    module_handle.Reset(Nan::New(target));

    NAN_EXPORT(target, init);
    NAN_EXPORT(target, exit);
    NAN_EXPORT(target, discoverDevices);
    NAN_EXPORT(target, openDevice);
    NAN_EXPORT(target, closeDevice);
    NAN_EXPORT(target, getEnrollStages);
    NAN_EXPORT(target, enrollStart);
    NAN_EXPORT(target, enrollStop);
	NAN_EXPORT(target, verifyStart);
	NAN_EXPORT(target, verifyStop);
	/*NAN_EXPORT(target, identifyStart);
	NAN_EXPORT(target, identifyStop);*/
    NAN_EXPORT(target, setDebug);
}

NODE_MODULE(fingerprint, module_init)
