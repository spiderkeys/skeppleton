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
	bool keepRunning = true;
        
        while( keepRunning ) 
        {
	    //////////////////////////////////////////
	    // Background processing stuff goes here!
	    //////////////////////////////////////////
	  
            // Get latest command
            uv_mutex_lock( &shared->commandMutex );
            command = shared->currentCommand;
            uv_mutex_unlock( &shared->commandMutex );
            
            // Update shared data based on command value
            uv_mutex_lock( &shared->commandMutex );
            
            if( command != 10 )
            {
                shared->currentData = command * 10;
            }
            else
            {
		// Force break if value is 10
                shared->currentData = 999;
                keepRunning = false;
            }
            
            uv_mutex_unlock( &shared->commandMutex );
            
            // Signal main thread that we did some work
            uv_async_send( &shared->async );
	    
	    if( !keepRunning )
	    {
		break;
	    }
            
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