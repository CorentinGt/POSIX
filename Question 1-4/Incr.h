#ifndef Incr_h_INCLUDED
#define Incr_h_INCLUDED
#include "Lock.h"
#include <iostream>
#include "Thread.h"
#include "Mutex.h"


class Incr : public Thread
{
public:
	class Counter 
	{	
	public:
		Counter(double, bool);
		double getValue();
		double incrementSafe();
		double incrementUnsafe();
		bool getMutexUse();
		double get_nLoops();
	private:
		int value;
		Mutex mutex;
		bool mutexUse;
		double nLoops;
	};
public:
	Incr(Counter* pCounter, int schedPolicy);
protected:
	void run();
private:
	Counter* m_pCounter;	

};


#endif
