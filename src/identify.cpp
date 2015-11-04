#include "fingerprint.h"

using namespace v8;
using v8::FunctionTemplate;

typedef struct __IDENTIFY_STOP__ {
    uv_async_t async;
    Nan::Persistent<Function> callback;
} IDENTIFY_STOP;

typedef struct __IDENTIFY_START__ {
    uv_async_t async;
    Nan::Persistent<Function> callback;
    int result;
    int index;
} IDENTIFY_START;

static const char *identify_result_to_name(int result)
{
    switch (result) {
    case FP_VERIFY_MATCH: return "identify-succeeded";
	case FP_VERIFY_NO_MATCH: return "identify-failed";
	case FP_VERIFY_RETRY: return "identify-retry-scan";
	case FP_VERIFY_RETRY_TOO_SHORT: return "identify-swipe-too-short";
	case FP_VERIFY_RETRY_CENTER_FINGER: return "identify-finger-not-centered";
	case FP_VERIFY_RETRY_REMOVE_FINGER: return "identify-remove-and-retry";
    default: return "identify-unknown-error";
	}
}

void identify_stop_after(uv_handle_t* handle)
{
    IDENTIFY_STOP *data = container_of((uv_async_t *)handle, IDENTIFY_STOP, async);

    if(!data)
        return;

    delete data;
}

#ifdef OLD_UV_RUN_SIGNATURE
void report_identify_stop(uv_async_t *handle)
#else
void report_identify_stop(uv_async_t *handle, int status)
#endif
{
    IDENTIFY_STOP *data = container_of(handle, IDENTIFY_STOP, async);
    Nan::HandleScope();

    if(!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    callback.Call(0, NULL);
    uv_close((uv_handle_t*)&data->async, identify_stop_after);
}

static void identify_stop_cb(struct fp_dev *dev, void *user_data)
{
    IDENTIFY_STOP *data = (IDENTIFY_STOP*)user_data;

    if(!data)
        return;

    uv_async_send(&data->async);
}

NAN_METHOD(identifyStop) {
    bool ret = false;
    struct fp_dev *dev;
    IDENTIFY_STOP *data;

    if(info.Length() < 2)
        goto error;

    dev = toFPDev(info[0]->ToNumber()->Value());
    if(initalized != 0 || dev == NULL)
        goto error;

    data = new IDENTIFY_STOP;
    data->callback.Reset(v8::Local<v8::Function>::Cast(info[1]));
    uv_async_init(uv_default_loop(), &data->async, report_identify_stop);
    ret = fp_async_identify_stop(dev, identify_stop_cb, data) == 0;

error:
    info.GetReturnValue().Set(Nan::New(ret));
}

void identify_start_after(uv_handle_t* handle)
{
    IDENTIFY_START *data = container_of((uv_async_t *)handle, IDENTIFY_START, async);

    if(!data)
        return;

    delete data;
}

#ifdef OLD_UV_RUN_SIGNATURE
void report_identify_start(uv_async_t *handle)
#else
void report_identify_start(uv_async_t *handle, int status)
#endif
{
    IDENTIFY_START *data = container_of(handle, IDENTIFY_START, async);
    Nan::HandleScope();

    if(!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    Local<Value> argv[3];
    argv[0] = Nan::New(data->result);
    argv[1] = Nan::Null();
    argv[2] = Nan::Null();

    if(identify_result_to_name(data->result))
        argv[1] = Nan::New(identify_result_to_name(data->result)).ToLocalChecked();

    if(data->result == FP_VERIFY_MATCH)
        argv[2] = Nan::New(data->index);

    callback.Call(3, argv);
    uv_close((uv_handle_t*)&data->async, identify_start_after);
}

static void identify_start_cb(struct fp_dev *dev, int result, size_t match_offset, struct fp_img *img, void *user_data)
{
    IDENTIFY_START *data = (IDENTIFY_START*)user_data;

    if(!data)
        return;

    data->result = result;
    data->index = match_offset;

    uv_async_send(&data->async);
    if(img) fp_img_free(img);
}

NAN_METHOD(identifyStart) {
    bool ret = false;
    struct fp_dev *dev;
    IDENTIFY_START *data;
    int fingerprints;
    struct fp_print_data **fpdata = NULL;
    Local<Object> obj;
    int i;

    if(info.Length() < 3)
        goto error;

    dev = toFPDev(info[0]->ToNumber()->Value());
    if(initalized != 0 || dev == NULL)
        goto error;

    data = new IDENTIFY_START;
    obj = info[1]->ToObject();
    data->callback.Reset(v8::Local<v8::Function>::Cast(info[2]));

    fingerprints = obj->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    fpdata = (struct fp_print_data **)calloc(fingerprints + 1, sizeof(struct fp_print_data *));
    for(i = 0; i < fingerprints; i++) {
        fpdata[i] = fp_print_data_from_data((unsigned char*)node::Buffer::Data(obj->Get(i)), (size_t)node::Buffer::Length(obj->Get(i)));
    }
    fpdata[i] = NULL;

    uv_async_init(uv_default_loop(), &data->async, report_identify_start);
   ret = fp_async_identify_start(dev, fpdata, identify_start_cb, data) == 0;

error:
    info.GetReturnValue().Set(Nan::New(ret));
}
