#include "Form1.h"

#include <stdlib.h>

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

char *strdup (const char *s) {
    char *d = (char *)(malloc (strlen (s) + 1)); // Space for length plus nul
    if (d == NULL) return NULL;                  // No memory
    strcpy (d,s);                                // Copy the characters
    return d;                                    // Return the new string
}

class AuthenticationThread : public virtual Osp::Base::Runtime::Thread
{
private :
	despotify_session *ds;
	char *username;
	char *password;

public:
	AuthenticationThread(despotify_session *ds, char *username, char *password) {
		this->ds = ds;
		this->username = strdup(username);
		this->password = strdup(password);
	}

	Osp::Base::Object* Run(void) {
	    AppLog("Authenticating");
	    if (!despotify_authenticate(ds, username, password)) {
	        AppLog( "Authentication failed: %s\n", despotify_get_error(ds));
	        return new Boolean(false);
	    }

	    AppLog("Getting track");
	    struct track* t = despotify_link_get_track(ds, despotify_link_from_uri("spotify:track:0RaSazqzf6a90Kn6WgFcuA"));
	    if(t == 0) return new Boolean(false);

	    // Fake it til you make it:
	    strncpy(t->title, "dummy", strlen("dummy"));
	    strncpy((char*)t->track_id, "1c2e2540e4284c388a1f673d385cff10", 33);
	    strncpy((char*)t->file_id, "8371bfa80cbff81887881648e5346d0192309d83", 41);
	    AppLog("Playing track");
	    despotify_play(ds, t, false);

	    return new Boolean(true);
	}
	bool OnStart(void) {
		return true;
	}
	void OnStop(void) {
		free(username);
		free(password);
	}
};


Form1::Form1(despotify_session *ds)
{
	this->ds = ds;
}

Form1::~Form1(void)
{
}

bool
Form1::Initialize()
{
	// Construct an XML form
	Construct(L"IDF_FORM1");

	return true;
}

result
Form1::OnInitializing(void)
{
	result r = E_SUCCESS;

	// TODO: Add your initialization code here

	// Get a button via resource ID
	__pButtonLogin = static_cast<Button *>(GetControl(L"IDC_BUTTON_LOGIN"));
	if (__pButtonLogin != null)
	{
		__pButtonLogin->SetActionId(ID_BUTTON_LOGIN);
		__pButtonLogin->AddActionEventListener(*this);
	}
	__pButtonPlay = static_cast<Button *>(GetControl(L"IDC_BUTTON_PLAY"));
	if (__pButtonPlay != null)
	{
		__pButtonPlay->SetActionId(ID_BUTTON_PLAY);
		__pButtonPlay->AddActionEventListener(*this);
	}

#if 1
#error "Set username and password"
#endif
	thread = new AuthenticationThread(ds, "<username>", "<password>");
	thread->Construct(Runtime::THREAD_TYPE_WORKER);
	thread->Start();

	return r;
}

result
Form1::OnTerminating(void)
{
	result r = E_SUCCESS;

	// TODO: Add your termination code here

	return r;
}

void
Form1::OnActionPerformed(const Osp::Ui::Control& source, int actionId)
{
	switch(actionId)
	{
	case ID_BUTTON_LOGIN:
		{
			__pButtonLogin->SetEnabled(false);
			this->RequestRedraw(true);
			AppLog("This button does nothing! \n");
		}
		break;
	case ID_BUTTON_PLAY:
		{
			AppLog("This button does nothing! \n");
		}
		break;
	default:
		break;
	}
}
