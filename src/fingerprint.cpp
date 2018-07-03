#include <iomanip>
#include <sstream>
#include <zlib.h>

#include "fingerprint.h"

#include "enroll.h"
#include "verify.h"
#include "identify.h"
#include <vector>


#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

using namespace v8;
using namespace std;
using v8::FunctionTemplate;

typedef struct __POLL_DATA__ {
    uv_thread_t thread;
    bool exit;
} POLL_DATA;

int initalized = -1;
static POLL_DATA *polldata = NULL;
static vector<struct fp_dev*> devices;

int fromFPDev(struct fp_dev *dev)
{
    int idx = devices.size();
    devices.resize(idx + 1);
    devices[idx] = dev;
    return idx;
}

struct fp_dev* toFPDev(int idx)
{
    return devices[idx];
}

#define UNCOMPRESSED_SIZE 12050
unsigned char* fromString(std::string hex, unsigned long *size)
{
	unsigned char *buffer = NULL;
	unsigned char *uncompressed = NULL;
	unsigned long length;
	unsigned long i = 0;
	int tmp;

	if(!size)
		return NULL;

	if(hex.length() % 2 != 0 || !size)
		goto error;

	length = hex.length() / 2;
	*size = UNCOMPRESSED_SIZE;
	buffer = (unsigned char*)malloc(length * sizeof(unsigned char));
	uncompressed = (unsigned char*)malloc(*size * sizeof(unsigned char));

	for(std::string::iterator it=hex.begin(); it!=hex.end(); it++) {
		std::string data;

		if(i >= length)
			goto error;

		data += *it++;
		if(it==hex.end())
			goto error;

		data += *it;

		std::stringstream converter(data);
		converter >> std::hex >> tmp;
		buffer[i++] = (unsigned char)tmp;
	}

	if(uncompress(uncompressed, size, buffer, length) == Z_OK) {
		free(buffer);
		return uncompressed;
	}

error:
	if(buffer != NULL)
		free(buffer);

	if(uncompressed != NULL)
		free(uncompressed);

	*size = 0;
	return NULL;
}

std::string toString(unsigned char* buffer, unsigned long size)
{
    std::stringstream converter;
	unsigned long compressedLength = 0;

	compressedLength = compressBound(size);
	unsigned char *compressed = (unsigned char*)malloc(compressedLength);

	if(compressed && compress(compressed, &compressedLength, buffer, size) == Z_OK) {
		converter << std::setfill('0');
		for(unsigned long l = 0; l < compressedLength; l++) {
			converter << std::hex << std::setw(2) << static_cast<int>(compressed[l]);
		}

		free(compressed);
		return converter.str();
	}
	return "";
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

    dev = toFPDev(Nan::To<v8::Number>(info[0]).ToLocalChecked()->Value());
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
			return;
        }
    }
    info.GetReturnValue().Set(Nan::Null());
}

NAN_METHOD(closeDevice)
{
    if(info.Length() == 1) {
        struct fp_dev *dev;
        int i = Nan::To<v8::Number>(info[0]).ToLocalChecked()->Value();
        dev = toFPDev(i);
        if(initalized != 0)
            return;

        if(dev) {
            fp_dev_close(dev);
            devices[i] = NULL;
        }
    }
}

void poll_fp(void *d)
{
    POLL_DATA *polldata = (POLL_DATA*)d;
    struct timeval zerotimeout = {
        .tv_sec = 0,
        .tv_usec = 10000,
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
	NAN_EXPORT(target, identifyStart);
	NAN_EXPORT(target, identifyStop);
    NAN_EXPORT(target, setDebug);
}

NODE_MODULE(fingerprint, module_init)
