#ifndef _SPOTIFY_H_
#define _SPOTIFY_H_

#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FMediaAudioOut.h>
#include <FBaseRtTimer.h>

extern "C" {
#include <despotify.h>
}

class DecoderThread;

/**
 * [spotify] application must inherit from Application class
 * which provides basic features necessary to define an application.
 */
class spotify :
	public Osp::App::Application,
	public Osp::System::IScreenEventListener,
	public Osp::Ui::IKeyEventListener,
	public Osp::Media::IAudioOutEventListener,
	public Osp::Base::Runtime::ITimerEventListener
{
public:

	/**
	 * [spotify] application must have a factory method that creates an instance of itself.
	 */
	static Osp::App::Application* CreateInstance(void);


public:
	spotify();
	~spotify();

	void tick(void);
private:
	despotify_session *ds;

	DecoderThread *decoder;

	Osp::Media::AudioOut ao;
	Osp::Base::ByteBuffer writeBuffer;
	Osp::Base::Runtime::Mutex aobMutex;
	Osp::Base::Runtime::Monitor aobMonitor;
	bool buffered;
	bool prefilled;

	/* debug */
	int last_reads;
	int last_yields;
	long long last_fill;
	long long last_flush;
	/* debug */

	Osp::Base::Runtime::Timer timer;
	int timer_period;

	void flushBuffer(void);
public:

	// Called when the application is initializing.
	bool OnAppInitializing(Osp::App::AppRegistry& appRegistry);

	// Called when the application is terminating.
	bool OnAppTerminating(Osp::App::AppRegistry& appRegistry, bool forcedTermination = false);


	// Called when the application's frame moves to the top of the screen.
	void OnForeground(void);


	// Called when this application's frame is moved from top of the screen to the background.
	void OnBackground(void);

	// Called when the system memory is not sufficient to run the application any further.
	void OnLowMemory(void);

	// Called when the battery level changes.
	void OnBatteryLevelChanged(Osp::System::BatteryLevel batteryLevel);

	// Called when the screen turns on.
	void OnScreenOn (void);

	// Called when the screen turns off.
	void OnScreenOff (void);

	void OnKeyPressed(const Osp::Ui::Control& source, Osp::Ui::KeyCode keyCode);
	void OnKeyReleased(const Osp::Ui::Control& source, Osp::Ui::KeyCode keyCode);
	void OnKeyLongPressed(const Osp::Ui::Control& source, Osp::Ui::KeyCode keyCode);

	void OnAudioOutBufferEndReached(Osp::Media::AudioOut &src);

	void OnAudioOutInterrupted(Osp::Media::AudioOut &src);

	void OnAudioOutReleased(Osp::Media::AudioOut &src);

	void OnTimerExpired(Osp::Base::Runtime::Timer &timer);
};

#endif
