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
        uv_async_t async;
        uv_mutex_t commandMutex;
        uv_mutex_t dataMutex;
        
        int currentCommand;
        int currentData;
        
        TSharedData()
        {
            currentCommand = 1;
            currentData = 0;
            
            // Init mutexes
            uv_mutex_init( &commandMutex );
            uv_mutex_init( &dataMutex );
        }
    };

    uv_thread_t bgThread;
    
    // Data shared between worker thread and main thread
    TSharedData m_shared;


    void PrintSignal( uv_async_t *handle, int status ) 
    {
        fprintf(stderr, "Thread signaled update\n" );
        
        // Update the command value
        uv_mutex_lock( &m_shared.dataMutex );
        fprintf( stderr, "Current Command: %d\n", m_shared.currentCommand );
        fprintf( stderr, "Current Data: %d\n", m_shared.currentData );
        uv_mutex_unlock( &m_shared.dataMutex );
        
    }

    void ThreadLoop( void *arg ) 
    {
        // Get shared data pointer
        TSharedData* shared = ((TSharedData*) arg );
    
        fprintf( stderr, "BG Thread running...\n" );
        
        int command = 1;
        
        while( true ) 
        {
            // Update command
            uv_mutex_lock( &shared->commandMutex );
            command = shared->currentCommand;
            uv_mutex_unlock( &shared->commandMutex );
            
            // Update data
            uv_mutex_lock( &shared->commandMutex );
            
            // Force break if value is 10
            if( command != 10 )
            {
                shared->currentData = command * 10;
            }
            else
            {
                shared->currentData = 999;
                break;
            }
            
            uv_mutex_unlock( &shared->commandMutex );
            
            // Signal main thread that we did some work
            uv_async_send( &shared->async );
            
            sleep( 1 );
        }
        
        // Thread is over, close the async watcher on the main thread
        uv_close( (uv_handle_t*)&shared->async, NULL );
        
        fprintf( stderr, "BG Thread finished running!\n");
    }
    
    
    void StartThread(const Nan::FunctionCallbackInfo<v8::Value>& info) 
    {
        info.GetReturnValue().Set( Nan::New("Started Thread").ToLocalChecked() );

        uv_async_init( uv_default_loop(), &m_shared.async, PrintSignal );

        uv_thread_create( &bgThread, ThreadLoop, &m_shared );
    }
    
    void SendCommandToThread( const Nan::FunctionCallbackInfo<v8::Value>& info)  
    {
        if( info.Length() < 1 ) 
        {
            Nan::ThrowTypeError( "Wrong number of arguments!" );
            return;
        }
        
        if( !info[0]->IsNumber() )
        {
            Nan::ThrowTypeError( "Wrong argument type!" );
            return;
        }
        
        // Update the command value
        uv_mutex_lock( &m_shared.commandMutex );
        m_shared.currentCommand = info[0]->NumberValue();;
        uv_mutex_unlock( &m_shared.commandMutex );
    }
    
    void Init(v8::Local<v8::Object> exports) 
    {
        exports->Set( Nan::New("StartThread").ToLocalChecked(), Nan::New<v8::FunctionTemplate>( StartThread )->GetFunction() );
        exports->Set( Nan::New("SendCommandToThread").ToLocalChecked(), Nan::New<v8::FunctionTemplate>( SendCommandToThread )->GetFunction() );
    }
    
    NODE_MODULE(skeppleton, Init)
}