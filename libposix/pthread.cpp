/*
 Bada pthread
 Copyright (c) 2010 Markovtsev Vadim

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include <FApp.h>
#include <FBaseRtThread.h>

#define PTHREAD_INTERNAL
#include "pthread.h"
#undef PTHREAD_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

typedef void*(*pthread_func)(void *);

using namespace Osp::Base;
using namespace Osp::Base::Runtime;

class RunnableProxy :
		public Object,
  		public virtual IRunnable
{
private:
	pthread_func _func;
	void * _parameter;

public:
	void SetFunc(pthread_func func)
	{
		_func = func;
	}

	pthread_func GetFunc()
	{
		return _func;
	}

	void SetParameter(void *parameter)
	{
		_parameter = parameter;
	}

	void *GetParameter()
	{
		return _parameter;
	}

	Object* Run(void)
	{
		void *result = _func(_parameter);
		return (Object *)result;
	}
};

int pthread_create(pthread_t*__threadarg,
			const pthread_attr_t*__attr,
			void*(*__start_routine)(void *),
			void*__arg)
{
	Thread *thread = new Thread();
	RunnableProxy *proxy = new RunnableProxy();
	proxy->SetFunc(__start_routine);
	proxy->SetParameter(__arg);
	thread->Construct(*proxy);
	*__threadarg = thread;
	return (int)thread->Start();
}

void pthread_exit(void*__retval)
{
	Thread::Exit(-1);
}

int pthread_join(pthread_t __th,void**__thread_return)
{
	if (!__th)
	{
		return -1;
	}
	return __th->Join();
}

int pthread_cancel(pthread_t thread)
{
	if (!thread)
	{
		return -1;
	}
	return thread->Exit();
}

int pthread_detach(pthread_t __th)
{
	if (__th)
	{
		__th->Exit();
		delete __th;
	}
	return 0;
}

int pthread_equal(pthread_t __thread1,pthread_t __thread2)
{
	if (!__thread1 || !__thread2)
	{
		return (void *)__thread1 == (void *)__thread2;
	}
	return __thread1->Equals(*__thread2);
}

int pthread_kill(pthread_t thread,int sig)
{
	if (!thread)
	{
		return -1;
	}
	return thread->Exit(sig);
}

int pthread_attr_init(pthread_attr_t*attr)
{
	attr = new pthread_attr_t();
	attr->__stacksize = Thread::DEFAULT_STACK_SIZE;
	struct sched_param param;
	param.sched_priority = THREAD_PRIORITY_MID;
	attr->__schedparam = param;
	return 0;
}

int pthread_attr_destroy(pthread_attr_t*attr)
{
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t*attr,const int detachstate)
{
	attr->__detachstate = detachstate;
	return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t*attr,int*detachstate)
{
	*detachstate = attr->__detachstate;
	return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t*attr)
{
	attr = new pthread_mutexattr_t();
	return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t*attr)
{
	return 0;
}

int pthread_mutex_init(pthread_mutex_t*mutex,
		const pthread_mutexattr_t*mutexattr)
{
	if (!mutex)
	{
		return -1;
	}
	mutex->lock = new Mutex();
	mutex->acquired = false;
	return mutex->lock->Create();
}

int pthread_mutex_lock(pthread_mutex_t*mutex)
{
	if (!mutex || !mutex->lock)
	{
		return -1;
	}
	int res = mutex->lock->Acquire();
	if (res == E_SUCCESS)
	{
		mutex->acquired = true;
	}
	return res;
}

int pthread_mutex_unlock(pthread_mutex_t*mutex)
{
	if (!mutex || !mutex->lock)
	{
		return -1;
	}
	int res = mutex->lock->Release();
	if (res == E_SUCCESS)
	{
		mutex->acquired = false;
	}
	return res;
}

int pthread_mutex_trylock(pthread_mutex_t*mutex)
{
	if (!mutex || !mutex->lock)
	{
		return -1;
	}
	if (!mutex->acquired)
	{
		int res = mutex->lock->Acquire();
		if (res == E_SUCCESS)
		{
			mutex->acquired = true;
		}
		return res;
	}
	return -1;
}

int pthread_mutex_destroy(pthread_mutex_t*mutex)
{
	delete mutex->lock;
	mutex->acquired = false;
	return 0;
}

int pthread_cond_init(pthread_cond_t*cond,pthread_condattr_t*cond_attr)
{
	if (!cond)
	{
		return -1;
	}
	cond->lock = new Monitor();
	cond->lock->Construct();
	return 0;
}

int pthread_cond_destroy(pthread_cond_t*cond)
{
	delete cond->lock;
	return 0;
}

int pthread_cond_signal(pthread_cond_t*cond)
{
	if (!cond)
	{
		return -1;
	}
	cond->lock->Enter();
	cond->lock->Notify();
	cond->lock->Exit();
	return 0;
}

int pthread_cond_broadcast(pthread_cond_t*cond)
{
	if (!cond)
	{
		return -1;
	}
	return cond->lock->NotifyAll();
}

int pthread_cond_timedwait(pthread_cond_t*cond,pthread_mutex_t*mutex,
			   const struct timespec*abstime)
{
	if (!cond)
	{
		return -1;
	}
	cond->lock->Enter();
	mutex->lock->Release();
	cond->lock->Wait();
	cond->lock->Exit();
	mutex->lock->Acquire();

	return 0;
}

int pthread_cond_enter(pthread_cond_t*cond)
{
	if (!cond)
	{
		return -1;
	}
	return cond->lock->Enter();
}

int pthread_cond_exit(pthread_cond_t*cond)
{
	if (!cond)
	{
		return -1;
	}
	return cond->lock->Exit();
}

int pthread_cond_wait(pthread_cond_t*cond,pthread_mutex_t*mutex)
{
	if (!cond)
	{
		return -1;
	}

	cond->lock->Enter();
	mutex->lock->Release();
	cond->lock->Wait();
	cond->lock->Exit();
	mutex->lock->Acquire();

	return 0;
}

int pthread_condattr_init(pthread_condattr_t*attr)
{
	return 0;
}

int pthread_condattr_destroy(pthread_condattr_t*attr)
{
	return 0;
}

int pthread_condattr_getpshared(const pthread_condattr_t*attr,int*pshared)
{
	return 0;
}

int pthread_condattr_setpshared(pthread_condattr_t*attr,int pshared)
{
	return 0;
}

#ifdef __cplusplus
}
#endif
