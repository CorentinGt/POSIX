#ifndef Thread_h_INCLUDED
#define Thread_h_INCLUDED

#include <pthread.h>
#include <time.h>
#include "Condition.h"
#include "Lock.h"

class Thread
{
public:	
	Thread(int); // constructeur 
	bool start(int);
	void join();
	void join(double);
	static void sleep(double);
	void setStackSize(size_t);
	
protected:
	virtual void run() = 0;	// fonction virtuelle pure. C'est la sous classe de thread qui l'implémente. 
	
private: 
	static void* call_run(void*); // interface de la classe abstraite thread poru appeler run. 
	pthread_t m_tid;
	pthread_attr_t m_attr; 
	int m_schedPolicy;
	bool started;
	Condition condition;
	
};

#endif
