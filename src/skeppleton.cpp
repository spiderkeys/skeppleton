// #include <unistd.h>
// #include <node.h>
// #include <string.h>
// #include <v8.h>

// using namespace v8;

// unsigned long long count = 0;

// // native blocking/compute intensive function
// void delay(int seconds) 
// {
//     int i;
//     int j;

//     // a long computation
//     for(i=0;i<2000000;++i) 
//     {
//         for(j=0;j<400;++j)   
//         {
//             count = count * seconds;
//         }
//     }
// }


// // the 'baton' is the carrier for data between functions
// struct DelayBaton
// {
//     // required
//     uv_work_t request;                  // libuv
//     Persistent<Function> callback;      // javascript callback

//     // optional : data goes here.
//     // data that doesn't go back to javascript can be any typedef
//     // data that goes back to javascript needs to be a supported type
//     int         seconds;
//     char        greeting[256];
// };

// // called by libuv worker in separate thread
// static void DelayAsync(uv_work_t *req)
// {
//     DelayBaton *baton = static_cast<DelayBaton *>(req->data);
//     delay(baton->seconds);
// }

// // called by libuv in event loop when async function completes
// static void DelayAsyncAfter(uv_work_t *req,int status)
// {
//     // get the reference to the baton from the request
//     DelayBaton *baton = static_cast<DelayBaton *>(req->data);

//     // set up return arguments
//     Handle<Value> argv[] =
//         {
//             Handle<Value>(Int32::New(baton->seconds)),
//             Handle<Value>(String::New(baton->greeting))
//         };

//     // execute the callback
//     baton->callback->Call(Context::GetCurrent()->Global(),2,argv);

//     // dispose the callback object from the baton
//     baton->callback.Dispose();

//     // delete the baton object
//     delete baton;
// }

// // javascript callable function
// Handle<Value> Delay(const Arguments &args)
// {
//     // create 'baton' data carrier
//     DelayBaton *baton = new DelayBaton;

//     // get callback argument
//     Handle<Function> cb = Handle<Function>::Cast(args[2]);

//     // attach baton to uv work request
//     baton->request.data = baton;

//     // assign incoming arguments to baton
//     baton->seconds =   args[0]->Int32Value();

//     // point at the argument as a string, then copy it to the baton
//     v8::String::Utf8Value str(args[1]);
//     strncpy(baton->greeting,*str,sizeof(baton->greeting));

//     // assign callback to baton
//     baton->callback = Persistent<Function>::New(cb);

//     // queue the async function to the event loop
//     // the uv default loop is the node.js event loop
//     uv_queue_work(uv_default_loop(),&baton->request,DelayAsync,DelayAsyncAfter);

//     // nothing returned
//     return Undefined();
// }

// void init( Handle<Object> exports ) 
// {

//   // add the async function to the exports for this object
//   exports->Set(
//                 String::NewSymbol("delay"),                          // javascript function name
//                 FunctionTemplate::New(Delay)->GetFunction()          // attach 'Delay' function to javascript name
//               );
// }

// NODE_MODULE(skeppleton, init)

#include <nan.h>
#include <unistd.h>

namespace skep
{
    struct TSharedData
    {
        
    };
    
    uv_thread_t bgThread;
    uv_async_t async;


    void PrintSignal( uv_async_t *handle, int status ) 
    {
        fprintf(stderr, "Thread signaled step\n" );
    }

    void ThreadLoop( void *arg ) 
    {
        uv_async_t* shared = ((uv_async_t *) arg);
    
        fprintf( stderr, "Thread running..." );
        
        int intArg = 10;
        
        while( intArg > 0 ) 
        {
            intArg--;
            sleep(1);
            
            uv_async_send( shared );
        }
        
        uv_close((uv_handle_t*)shared, NULL);
        fprintf( stderr, "BG Thread finished running!\n");
    }
    
    
    void StartThread(const Nan::FunctionCallbackInfo<v8::Value>& info) 
    {
        info.GetReturnValue().Set( Nan::New("Started Thread").ToLocalChecked() );

        uv_async_init( uv_default_loop(), &async, PrintSignal );

        uv_thread_create(&bgThread, ThreadLoop, &async);
    }
    
    void Init(v8::Local<v8::Object> exports) 
    {
        exports->Set( Nan::New("StartThread").ToLocalChecked(), Nan::New<v8::FunctionTemplate>( StartThread )->GetFunction() );
    }
    
    NODE_MODULE(skeppleton, Init)
}