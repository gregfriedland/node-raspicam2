#include <node.h>
#include <v8.h>
#include <nan.h>
#include <node_buffer.h>

#include <iostream>
#include <raspicam.h>

using namespace v8;
using namespace node;

raspicam::RaspiCam Camera;
unsigned char *rawData = NULL;
double width, height;
bool isReadingAsync = false;
//node::Buffer *buffer = NULL;

void copyDataToImageData(const unsigned char* data, unsigned char* imgData) {
  int i = 0, j = 0;
  for (int x = 0; x < width; x++)
    for (int y = 0; y < height; y++) {
      for (int c = 0; c < 3; c++) 
	imgData[i++] = data[j++];
      imgData[i++] = 255;
    }
}

void allocateData() {
  if (rawData != NULL) {
    delete rawData;
  }
  unsigned int n = Camera.getImageBufferSize( );
  rawData = new unsigned char[ n ];

  //buffer = node::Buffer::New(n);
  std::cout << "created buffer of size: " << n << std::endl;
}


void open(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() != 0) {
    Nan::ThrowTypeError("Wrong arguments");
    return;
  }

  width = 1280;
  height = 960;

  Camera.setWidth ( 1280 );
  Camera.setHeight ( 960 );
  Camera.setBrightness ( 50 );
  Camera.setSharpness ( 0 );
  Camera.setContrast ( 0 );
  Camera.setSaturation ( 0 );
  Camera.setShutterSpeed( 0 );
  Camera.setISO ( 400 );
  Camera.setExposureCompensation ( 0 );
  Camera.setAWB_RB(1, 1);


  if ( !Camera.open() ) {
    std::cerr<<"Error opening camera"<<std::endl;
  } else {
    allocateData();
  }
}

void release(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() != 0) {
    Nan::ThrowTypeError("Wrong arguments");
    return;
  }

  Camera.release();
}

void setSize(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() != 2 || !info[0]->IsNumber() || !info[1]->IsNumber()) {
    Nan::ThrowTypeError("Wrong arguments");
   return;
  }

  width = info[0]->NumberValue();
  height = info[1]->NumberValue();

  Camera.setWidth(width);
  Camera.setHeight(height);
  allocateData();
}

void read(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Camera.grab();
  Camera.retrieve(rawData);
  //std::cout << "raw0: " << (int)rawData[0] << std::endl;

  Local<Object> buffer = info[0]->ToObject();

  unsigned int n = Camera.getImageBufferSize( );
  unsigned char* data = (unsigned char*) node::Buffer::Data(buffer);
  memcpy(data, rawData, n);
}


class AsyncReadWorker : public Nan::AsyncWorker {
private:
  unsigned char* data;

public:
  AsyncReadWorker(unsigned char* _data, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), data(_data) {
    Camera.grab();
  }
  ~AsyncReadWorker() {}

  void Execute () {
    //Camera.retrieve(rawData);
    Camera.retrieve(data);
    //std::cout << "raw0: " << (int)rawData[0] << std::endl;
  }

  void HandleOKCallback () {
    //Nan::Scope();



//     unsigned int n = Camera.getImageBufferSize( );
//     node::Buffer* buffer = node::Buffer::New(width * height * 3);

//     unsigned char* bufData = (unsigned char*) node::Buffer::Data(buffer);  
    
// //std::cout << "raw0: " << (int)rawData[0] << std::endl;
//     memcpy(bufData, rawData, n);

//     v8::Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
//     v8::Local<v8::Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
//     v8::Handle<v8::Value> constructorArgs[3] = { buffer->handle_, v8::Integer::New(node::Buffer::Length(buffer)), v8::Integer::New(0) };
//     v8::Local<v8::Object> bufferObj = bufferConstructor->NewInstance(3, constructorArgs);

    Local<Value> argv[] = {
      //bufferObj
    };

    TryCatch try_catch;
    callback->Call(0, argv);
    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    isReadingAsync = false;
  }
};

void readAsync(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (!isReadingAsync) {
    isReadingAsync = true;
    v8::Local<v8::Object> buffer = info[0]->ToObject();
    unsigned char* data = (unsigned char*) node::Buffer::Data(buffer);

    v8::Local<v8::Function> cb = info[1].As<v8::Function>();
    Nan::Callback *callback = new Nan::Callback(cb.As<Function>());
    Nan::AsyncQueueWorker(new AsyncReadWorker(data, callback));
  } else
    std::cout << "Skipping frame\n";
}

void getData(const Nan::FunctionCallbackInfo<v8::Value>& args) {
  Local<Object> buffer = args[0]->ToObject();

  unsigned int n = Camera.getImageBufferSize( );
  unsigned char* data = (unsigned char*) node::Buffer::Data(buffer);
  memcpy(data, rawData, n);
}

void getImageData(const Nan::FunctionCallbackInfo<v8::Value>& args) {
  Local<Object> buffer = args[0]->ToObject();

  unsigned int n = node::Buffer::Length(buffer);
  std::cout << "image buffer length: " << n << std::endl;
  assert(n == width * height * 4);
  unsigned char* data = (unsigned char*) node::Buffer::Data(buffer);

  copyDataToImageData(rawData, data);
}

void getPixel(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() != 1 || !info[0]->IsNumber()) {
    Nan::ThrowTypeError("Wrong arguments");
    return;
  }

  int i = info[0]->NumberValue();
  v8::Local<v8::Number> val = Nan::New(rawData[i]);

  info.GetReturnValue().Set(val);
}

void init(v8::Local<v8::Object> exports) {
exports->Set(Nan::New("open").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(open)->GetFunction());
  exports->Set(Nan::New("setSize").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(setSize)->GetFunction());
  exports->Set(Nan::New("read").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(read)->GetFunction());
  exports->Set(Nan::New("readAsync").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(readAsync)->GetFunction());
  exports->Set(Nan::New("getPixel").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(getPixel)->GetFunction());
  exports->Set(Nan::New("getData").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(getData)->GetFunction());
 exports->Set(Nan::New("getImageData").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(getImageData)->GetFunction());
 exports->Set(Nan::New("release").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(release)->GetFunction());
}

NODE_MODULE(raspicam2, init)
