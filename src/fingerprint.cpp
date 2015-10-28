#include <nan.h>
#include <libfprint/fprint.h>

using namespace v8;
using v8::FunctionTemplate;

using namespace std;

static int initalized = -1;

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

NAN_METHOD(enroll) {
    struct fp_print_data *enrolled_print = NULL;
    int r;
    int stages;
    int stage = 1;
    struct fp_dev *dev;

    if(info.Length() != 1)
        return;

    dev = toFPDev(info[0]->ToNumber()->Value());
    if(initalized != 0)
        return;


    stages = fp_dev_get_nr_enroll_stages(dev);
    do {
        printf("\nScan your finger now (%d/%d)\n", stage, stages);

        r = fp_enroll_finger(dev, &enrolled_print);
        if (r < 0) {
            printf("Enroll failed with error %d\n", r);
            return;
        }

        switch (r) {
        case FP_ENROLL_COMPLETE:
            printf("Enroll complete!\n");
            break;
        case FP_ENROLL_FAIL:
            printf("Enroll failed, something wen't wrong :(\n");
            return;
        case FP_ENROLL_PASS:
            stage++;
            printf("Enroll stage passed. Yay!\n");
            break;
        case FP_ENROLL_RETRY:
            printf("Didn't quite catch that. Please try again.\n");
            break;
        case FP_ENROLL_RETRY_TOO_SHORT:
            printf("Your swipe was too short, please try again.\n");
            break;
        case FP_ENROLL_RETRY_CENTER_FINGER:
            printf("Didn't catch that, please center your finger on the "
                "sensor and try again.\n");
            break;
        case FP_ENROLL_RETRY_REMOVE_FINGER:
            printf("Scan failed, please remove your finger and then try "
                "again.\n");
            break;
        }
    } while (r != FP_ENROLL_COMPLETE);

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    if(enrolled_print) {
        unsigned char *data;
        size_t size;
        size = fp_print_data_get_data(enrolled_print, &data);
        Nan::Set(obj, Nan::New("fingerprint").ToLocalChecked(), Nan::CopyBuffer((const char*)data, size).ToLocalChecked());
        Nan::Set(obj, Nan::New("result").ToLocalChecked(), Nan::New(true));
        fp_print_data_free(enrolled_print);
    }
    else
        Nan::Set(obj, Nan::New("result").ToLocalChecked(), Nan::New(false));

    info.GetReturnValue().Set(obj);
}

NAN_METHOD(verify) {
    struct fp_print_data *data = NULL;
    int r;
    struct fp_dev *dev;
    Local<Object> obj;

    if(info.Length() != 2)
        return;

    dev = toFPDev(info[0]->ToNumber()->Value());
    obj = info[1]->ToObject();
    if(initalized != 0)
        return;

    data = fp_print_data_from_data((unsigned char*)node::Buffer::Data(obj), (size_t)node::Buffer::Length(obj));
	do {
		printf("\nScan your finger now.\n");
		r = fp_verify_finger(dev, data);
		if (r < 0) {
			printf("verification failed with error %d :(\n", r);
			return;
		}
		switch (r) {
		case FP_VERIFY_NO_MATCH:
			printf("NO MATCH!\n");
			return;
		case FP_VERIFY_MATCH:
			printf("MATCH!\n");
			return;
		case FP_VERIFY_RETRY:
			printf("Scan didn't quite work. Please try again.\n");
			break;
		case FP_VERIFY_RETRY_TOO_SHORT:
			printf("Swipe was too short, please try again.\n");
			break;
		case FP_VERIFY_RETRY_CENTER_FINGER:
			printf("Please center your finger on the sensor and try again.\n");
			break;
		case FP_VERIFY_RETRY_REMOVE_FINGER:
			printf("Please remove finger from the sensor and try again.\n");
			break;
		}
	} while (1);
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

NAN_METHOD(init)
{
    initalized = fp_init();
    info.GetReturnValue().Set(initalized == 0);
}
NAN_METHOD(exit)
{
    fp_exit();
    initalized = -1;
}

NAN_MODULE_INIT(module_init) {
    Nan::Set(target, Nan::New("init").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(init)).ToLocalChecked());
    Nan::Set(target, Nan::New("exit").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(exit)).ToLocalChecked());
    Nan::Set(target, Nan::New("discoverDevices").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(discoverDevices)).ToLocalChecked());
    Nan::Set(target, Nan::New("openDevice").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(openDevice)).ToLocalChecked());
    Nan::Set(target, Nan::New("closeDevice").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(closeDevice)).ToLocalChecked());
    Nan::Set(target, Nan::New("enroll").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(enroll)).ToLocalChecked());
    Nan::Set(target, Nan::New("verify").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(verify)).ToLocalChecked());
    Nan::Set(target, Nan::New("setDebug").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(setDebug)).ToLocalChecked());
}

NODE_MODULE(fingerprint, module_init)
