#ifndef _FORM1_H_
#define _FORM1_H_

#include <FBase.h>
#include <FUi.h>

#include "spotify.h"

class Form1 :
	public Osp::Ui::Controls::Form,
	public Osp::Ui::IActionEventListener
{

// Construction
public:
	Form1(despotify_session *ds);
	virtual ~Form1(void);
	bool Initialize(void);

// Implementation
protected:
	static const int ID_BUTTON_LOGIN = 101;
	Osp::Ui::Controls::Button *__pButtonLogin;
	static const int ID_BUTTON_PLAY = 102;
	Osp::Ui::Controls::Button *__pButtonPlay;

	despotify_session *ds;
	Osp::Base::Runtime::Thread *thread;

public:
	virtual result OnInitializing(void);
	virtual result OnTerminating(void);
	virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);
};

#endif	//_FORM1_H_
