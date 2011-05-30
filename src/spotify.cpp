/**
 * Name        : spotify
 * Version     : 
 * Vendor      : 
 * Description : 
 */


#include "spotify.h"
#include "Form1.h"

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::System;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::Media;

class DecoderThread : public virtual Osp::Base::Runtime::Thread
{
private :
	despotify_session *ds;
	spotify *app;
	int seconds;

public:
	bool running;

	DecoderThread(despotify_session *ds, spotify *app) {
		this->ds = ds;
		this->app = app;
		running = true;
		long long ticks;
		Osp::System::SystemTime::GetTicks(ticks);
		seconds = ticks/1000;
	}

	Osp::Base::Object* Run(void) {
		AppLog("Decoding thread started");

		while(running) {
			app->tick();
		}

		AppLog("Decoding thread stopped");
		return NULL;
	}
};

extern "C" void callback(struct despotify_session* ds, int signal, void* data, void* callback_data)
{
    static int seconds = -1;
    (void)ds; (void)callback_data; /* don't warn about unused parameters */

//	spotify *app = (spotify*)callback_data;

    switch (signal) {
    case DESPOTIFY_NEW_TRACK: {
    	struct track* t = (track*)data;
    	AppLog("New track: %s / %s (%d:%02d) %d kbit/s",
    			t->title, t->artist->name,
    			t->length / 60000, t->length % 60000 / 1000,
    			t->file_bitrate / 1000);
    	break;
    }

    case DESPOTIFY_TIME_TELL:
    	if ((int)(*((double*)data)) != seconds) {
    		seconds = *((double*)data);
    		AppLog("Time: %d:%02d", seconds / 60, seconds % 60);
    	}
    	break;

    case DESPOTIFY_END_OF_PLAYLIST:
    	AppLog("End of playlist");
    	//            thread_pause();
    	break;

    default:
    	AppLog("callback %d", signal);
    	break;
    }
}

spotify::spotify()
{
	aobMutex.Create();
	aobMonitor.Construct();
	last_flush = 0;
	last_fill = 0;
	last_reads = 0;
	last_yields = 0;
	buffered = false;
	prefilled = false;
}

spotify::~spotify()
{
}

void spotify::tick(){
	if(!ds) return;
	aobMutex.Acquire();
	if(buffered) {
		aobMutex.Release();
		aobMonitor.Enter();
		aobMonitor.Wait();
		aobMonitor.Exit();
		// New fill
		Osp::System::SystemTime::GetTicks(last_fill);
		if(buffered) return;
		else aobMutex.Acquire();
	}

	long long init;
	Osp::System::SystemTime::GetTicks(init);

	if(writeBuffer.GetPosition() < writeBuffer.GetCapacity() - 4096) {
		// Fill back buffer to brim
		pcm_data pcm;
		int ret;
		do {
			ret = despotify_get_pcm(ds, &pcm);
		} while(ret == 0 && pcm.len == 0);
		if(ret < 0) {
			AppLog("ret %d", ret);
			AppAssert(1 == 2);
		}
		if(pcm.len > 0) {
			last_reads++;
			if(pcm.channels != 2 || pcm.samplerate != 44100) {
				AppLog("Pcm %d %d", pcm.channels, pcm.samplerate);
				AppAssert(0 == 1);
			}
			else {
				writeBuffer.SetArray((const byte*)pcm.buf, 0, pcm.len);
			}
		}
	} else {
		long long tocks;
		Osp::System::SystemTime::GetTicks(tocks);
		if(last_fill > 0) {
			int dur = (int)(tocks-last_fill);
			int rate = (int)(writeBuffer.GetPosition()*1000/dur);
			if(rate < 44100 * 2 * 2) {
				AppLog("fill took %dms, %d Bps (%d, %d)", dur, rate, last_reads, last_yields);
			}
		}
		if(!prefilled) {
			AppLog("Prefilling");
			flushBuffer();
			prefilled = true;
		} else {
			flushBuffer();
			// Wait for AudioOutBufferEndReached
			buffered = true;
		}

		last_fill = tocks;
		last_reads = 0;
		last_yields = 0;
	}
	aobMutex.Release();

	long long ticks;
	Osp::System::SystemTime::GetTicks(ticks);
	if(ticks - init > 50) AppLog("tick took %dms", (int)(ticks - init));
}

Application*
spotify::CreateInstance(void)
{
	// Create the instance through the constructor.
	return new spotify();
}

bool
spotify::OnAppInitializing(AppRegistry& appRegistry)
{
	// TODO:
	// Initialize UI resources and application specific data.
	// The application's permanent data and context can be obtained from the appRegistry.
	//
	// If this method is successful, return true; otherwise, return false.
	// If this method returns false, the application will be terminated.

	// Uncomment the following statement to listen to the screen on/off events.
	//PowerManager::SetScreenEventListener(*this);

	// NOTE: Libraries must be linked in spotify, zlib, vorbis order for app to work on target device.

	if (!despotify_init())
	{
		AppLog("despotify_init() failed\n");
        return false;
	}

	ds = despotify_init_client(callback, this, true, true);
	if (!ds) {
		AppLog("despotify_init_client() failed\n");
        return false;
	}

	ao.Construct(*this);
	ao.Prepare(AUDIO_TYPE_PCM_S16_LE, AUDIO_CHANNEL_TYPE_STEREO, 44100);
	ao.SetVolume(15);
	ao.Start();	ao.Stop(); // State: PREPARED -> STOPPED
	writeBuffer.Construct(ao.GetMaxBufferSize());
	AppLog("%dB audio buffer", writeBuffer.GetCapacity());

	timer.Construct(*this);
	timer_period = 5000;
	timer.Start(timer_period);

	decoder = new DecoderThread(ds, this);
	decoder->Construct(Runtime::THREAD_TYPE_WORKER);
	decoder->Start();

	// Create a form
	Form1 *pForm1 = new Form1(ds);
	pForm1->Initialize();
	pForm1->AddKeyEventListener(*this);

	// Add the form to the frame
	Frame *pFrame = GetAppFrame()->GetFrame();
	pFrame->AddControl(*pForm1);

	// Set the current form
	pFrame->SetCurrentForm(*pForm1);

	// Draw and Show the form
	pForm1->Draw();
	pForm1->Show();

	return true;
}

bool
spotify::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	AppLog("Terminating");
	decoder->running = false;
	aobMonitor.Notify();
	decoder->Stop();
	decoder->Join();
	delete decoder;

	timer.Cancel();
	if(ao.GetState() == AUDIOOUT_STATE_PLAYING) ao.Stop();

	// NOTE: Networking thread may block here on socket.
	despotify_exit(ds);

	if (!despotify_cleanup())
	{
		AppLog("despotify_cleanup() failed\n");
		return false;
	}

	// TODO:
	// Deallocate resources allocated by this application for termination.
	// The application's permanent data and context can be saved via appRegistry.
	return true;
}

void
spotify::OnForeground(void)
{
	// TODO:
	// Start or resume drawing when the application is moved to the foreground.
}

void
spotify::OnBackground(void)
{
	// TODO:
	// Stop drawing when the application is moved to the background.
}

void
spotify::OnLowMemory(void)
{
	AppLog("Low memory");
	// TODO:
	// Free unused resources or close the application.
}

void
spotify::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
	// TODO:
	// Handle any changes in battery level here.
	// Stop using multimedia features(camera, mp3 etc.) if the battery level is CRITICAL.
}

void
spotify::OnScreenOn (void)
{
	// TODO:
	// Get the released resources or resume the operations that were paused or stopped in OnScreenOff().
}

void
spotify::OnScreenOff (void)
{
	// TODO:
	//  Unless there is a strong reason to do otherwise, release resources (such as 3D, media, and sensors) to allow the device to enter the sleep mode to save the battery.
	// Invoking a lengthy asynchronous method within this listener method can be risky, because it is not guaranteed to invoke a callback before the device enters the sleep mode.
	// Similarly, do not perform lengthy operations in this listener method. Any operation must be a quick one.
}

void
spotify::OnKeyPressed(const Osp::Ui::Control& source, Osp::Ui::KeyCode keyCode)
{
	int volume;
	switch(keyCode) {
	case KEY_SIDE_UP:
		volume = ao.GetVolume() + 1;
		if(volume > 100) volume = 100;
		ao.SetVolume(volume);
		break;
	case KEY_SIDE_DOWN:
		volume = ao.GetVolume() -1;
		if(volume < 0) volume = 0;
		ao.SetVolume(volume);
		break;
	default:
		break;
	}
}

void
spotify::OnKeyReleased(const Osp::Ui::Control& source, Osp::Ui::KeyCode keyCode)
{
}

void
spotify::OnKeyLongPressed(const Osp::Ui::Control& source, Osp::Ui::KeyCode keyCode)
{
}

void
spotify::OnAudioOutBufferEndReached(Osp::Media::AudioOut &src) {
	aobMutex.Acquire();
	if(buffered) {
		// Continue buffering
		buffered = false;
	} else {
		AppLog("No buffer to flip");
		prefilled = false;
		if(ao.GetState() ==  AUDIOOUT_STATE_PLAYING) src.Stop();
	}
	aobMutex.Release();
	aobMonitor.Notify();
}

void
spotify::OnAudioOutInterrupted(Osp::Media::AudioOut &src) {
	AppLog("OnAudioOutInterrupted");
}

void
spotify::OnAudioOutReleased(Osp::Media::AudioOut &src) {
	AppLog("OnAudioOutReleased");
}

void
spotify::OnTimerExpired(Osp::Base::Runtime::Timer &timer) {
	bool start = false;
	if(prefilled && buffered && ao.GetState() == AUDIOOUT_STATE_STOPPED) {
		start = true;
		AppLog("Starting");
		ao.Start();
	}
	timer.Start(timer_period);
}

void
spotify::flushBuffer(void) {
	long long init;
	Osp::System::SystemTime::GetTicks(init);

	// Write buffer to device
	int bytes = writeBuffer.GetPosition();
	if(bytes < ao.GetMinBufferSize()) AppLog("Buffer only %d bytes", bytes);
	writeBuffer.Flip();
	// Compact manually since the device is retarded.
	Osp::Base::ByteBuffer *compact = new ByteBuffer();
	compact->Construct(writeBuffer.GetLimit());
	compact->CopyFrom(writeBuffer);
	result r = ao.WriteBuffer(*compact);
	delete compact;
	if(IsFailed(r)) {
		AppLog("WriteBuffer failed: %s", GetErrorMessage(r));
		ao.Stop();
	}
	writeBuffer.Clear();

	long long ticks;
	Osp::System::SystemTime::GetTicks(ticks);
	if(ticks - init > 10) AppLog("WriteBuffer took %dms", (int)(ticks - init));

	if(last_flush > 0) {
		int dur = (int)(ticks-last_flush);
		int rate = (int)(bytes*1000/dur);
		if(rate < 44100 * 2 * 2) {
//			AppLog("flush took %dms, %d Bps", dur, rate);
		}
	}
	last_flush = ticks;
}

