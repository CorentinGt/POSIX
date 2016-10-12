#include "Condition.h"


Condition::Condition(): Mutex(false)
{
	//pthread_condattr_t cattr;
	//pthread_condattr_init(&cattr);
	pthread_cond_init(&cid, NULL);
	//ret = pthread_cond_init(&cid, &cattr)
	
}

Condition::~Condition()
{
	pthread_cond_destroy(&cid);
}


void Condition::wait()
{
	pthread_cond_wait(&cid, &mid);
}

bool Condition::wait(double timeout_ms)
{
	struct timespec abstime;
	clock_gettime(CLOCK_REALTIME, &abstime);
	long secs = timeout_ms / 1000;
	abstime.tv_sec += secs;
	abstime.tv_nsec += (timeout_ms - secs *1000) * 1000000;
	if(abstime.tv_nsec >= 1000000000)
	{	
		abstime.tv_sec += 1; 
		abstime.tv_nsec %= 1000000000; //TODO utiliser classe Clock
	}
	if (pthread_cond_timedwait(&cid, &mid, &abstime) == ETIMEDOUT)
	{
		return false;//TODO faire un throw plutôt que return false. Verifier que dans bloc try 
	}
	return true;
}

void Condition::notify()
{
	pthread_cond_signal(&cid);
}

void Condition::notifyAll()
{
	pthread_cond_broadcast(&cid);
}
