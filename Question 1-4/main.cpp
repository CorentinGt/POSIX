#include "Incr.h"
#include "Thread.h"
#include "Mutex.h"
#include "Lock.h"
#include <vector>
#include <iostream>
#include <stdio.h>

using namespace std;

int main(int argc, char* argv[])
{
	int nLoops = 0;
	int nTask=0;
	if(argc > 2)
	{
		sscanf(argv[1], "%d", &nLoops);
		sscanf(argv[2], "%d", &nTask);
		
		Incr::Counter counter(nLoops, true);
		
		int schedPolicy;
		schedPolicy = SCHED_RR;
		vector<Incr*> myVect;
		
		for(int i=0; i<nTask; i++)
		{	
			Incr* ptrIncr = new Incr(&counter, schedPolicy); 
			myVect.push_back(ptrIncr);
		}
		
		for(int i=0; i < nTask; i++)
		{
			cout << "main(): creating thread, " << i << endl;
			myVect[i]->start(42);
		}
		
		for(int i=0; i < nTask; i++)
		{
			myVect[i]->join();
		}
		
		for(int i=0; i < nTask; i++)
		{
			delete myVect[i];
		}
		

		myVect.clear();
		cout << "Le compteur vaut: " << counter.getValue() << endl;
		return 0;
	}
	return -1;
}


  
  
  
  
  
