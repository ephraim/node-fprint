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
    struct fp_print_data **fpdata;
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

#ifndef OLD_UV_RUN_SIGNATURE
void report_identify_stop(uv_async_t *handle)
#else
void report_identify_stop(uv_async_t *handle, int status)
#endif
{
    IDENTIFY_STOP *data = container_of(handle, IDENTIFY_STOP, async);
    Nan::HandleScope scope;

    if(!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    Nan::AsyncResource asyncResource("identifyStopped");
    callback.Call(0, NULL, &asyncResource);
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

    dev = toFPDev(Nan::To<v8::Number>(info[0]).ToLocalChecked()->Value());
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
    int i;

    if(!data)
        return;

    if(data->fpdata != NULL) {
        for(i = 0; data->fpdata[i] != NULL; i++)
        {
            fp_print_data_free(data->fpdata[i]);
        }
        free(data->fpdata);
    }

    delete data;
}

#ifndef OLD_UV_RUN_SIGNATURE
void report_identify_start(uv_async_t *handle)
#else
void report_identify_start(uv_async_t *handle, int status)
#endif
{
    IDENTIFY_START *data = container_of(handle, IDENTIFY_START, async);
    Nan::HandleScope scope;

    if(!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    Nan::AsyncResource asyncResource("identifyStarted");
    Local<Value> argv[3];
    argv[0] = Nan::New(data->result);
    argv[1] = Nan::Null();
    argv[2] = Nan::Null();

    if(identify_result_to_name(data->result))
        argv[1] = Nan::New(identify_result_to_name(data->result)).ToLocalChecked();

    if(data->result == FP_VERIFY_MATCH)
        argv[2] = Nan::New(data->index);

    callback.Call(3, argv, &asyncResource);
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
    struct fp_dev *dev = NULL;
    IDENTIFY_START *data = NULL;
    int fingerprints = 0;
    Local<Object> obj;
    int i = 0;

    if(info.Length() < 3)
        goto error;

    dev = toFPDev(Nan::To<v8::Number>(info[0]).ToLocalChecked()->Value());
    if(initalized != 0 || dev == NULL)
        goto error;

    data = new IDENTIFY_START;
    obj = info[1]->ToObject();
    data->callback.Reset(v8::Local<v8::Function>::Cast(info[2]));

    fingerprints = obj->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    data->fpdata = (struct fp_print_data **)calloc(fingerprints + 1, sizeof(struct fp_print_data *));
    if(data->fpdata) {
        for(i = 0; i < fingerprints; i++) {
            std::string s(*String::Utf8Value(obj->Get(i)->ToString()));
            unsigned char *tmp;
            unsigned long length;

            tmp = fromString(s, &length);
            if(tmp) {
                data->fpdata[i] = fp_print_data_from_data(tmp, length);
            }
            else {
                data->fpdata[i] = NULL;
                goto error;
            }
        }
        data->fpdata[i] = NULL;

        uv_async_init(uv_default_loop(), &data->async, report_identify_start);
        ret = fp_async_identify_start(dev, data->fpdata, identify_start_cb, data) == 0;
    }
error:
    if(!ret && data && data->fpdata != NULL) {
        for(i = 0; data->fpdata[i] != NULL; i++)
        {
            fp_print_data_free(data->fpdata[i]);
        }
        free(data->fpdata);
    }
    info.GetReturnValue().Set(Nan::New(ret));
}
