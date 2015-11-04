#include "fingerprint.h"

using namespace v8;
using v8::FunctionTemplate;

extern int initalized;

NAN_METHOD(identify) {
    struct fp_print_data **data = NULL;
    int r, i, length;
    struct fp_dev *dev;
    Local<Object> obj;
    Nan::Callback callback;
    string msg;
    size_t matchIdx;

    if(info.Length() != 3)
        return;

    dev = toFPDev(info[0]->ToNumber()->Value());
    obj = info[1]->ToObject();
    callback.SetFunction(v8::Local<v8::Function>::Cast(info[1]));
    if(initalized != 0 || dev == NULL)
        return;

    length = obj->Get(Nan::New("length").ToLocalChecked())->ToObject()->Uint32Value();
    data = (struct fp_print_data **)calloc(length + 1, sizeof(struct fp_print_data *));
    for(i = 0; i < length; i++) {
        data[i] = fp_print_data_from_data((unsigned char*)node::Buffer::Data(obj->Get(i)), (size_t)node::Buffer::Length(obj->Get(i)));
    }
    data[i] = NULL;

    do {
        printf("\nScan your finger now.\n");
        r = fp_identify_finger(dev, data, &matchIdx);
        printf("got r: %d\n", r);
        if (r < 0) {
            printf("verification failed with error %d :(\n", r);
            break;
        }
        switch (r) {
        case FP_VERIFY_NO_MATCH:
        case FP_VERIFY_MATCH:
            break;
        case FP_VERIFY_RETRY: msg = "Scan didn't quite work. Please try again."; break;
        case FP_VERIFY_RETRY_TOO_SHORT: msg = "Swipe was too short, please try again."; break;
        case FP_VERIFY_RETRY_CENTER_FINGER: msg = "Please center your finger on the sensor and try again."; break;
        case FP_VERIFY_RETRY_REMOVE_FINGER: msg = "Please remove finger from the sensor and try again."; break;
        default: msg = "unknown"; break;
        }

        if(r != FP_VERIFY_NO_MATCH && r != FP_VERIFY_MATCH) {
            Local<Value> argv[] = { Nan::New(r), Nan::New(msg).ToLocalChecked() };;
            callback.Call(2, argv);
        }
    } while (r != FP_VERIFY_NO_MATCH && r != FP_VERIFY_MATCH);

    for(i = 0; data[i] != NULL; i++)
        fp_print_data_free(data[i]);
    free(data);

    obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("matchedIndex").ToLocalChecked(), Nan::New((int)matchIdx));
    Nan::Set(obj, Nan::New("result").ToLocalChecked(), Nan::New(Nan::New(r == FP_VERIFY_MATCH)));
    info.GetReturnValue().Set(obj);
}
