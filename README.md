#Compte rendu du TD4 - Classes de base pour la programmation multitâches


---

Co-TD: Benjamin Poilvé 

Ce TD a pour but d'encapsuler la gestion des tâches Posix dans des classes. Cela permet d'automatiser au maximum le fonctionnement multitâche, de rendre le tout plus agréable et moins verbeux pour l'utilisateur, et également d'automatiser les constructions et destructions de ressources, et en particulier de **mutex**.



##Question a. Classe Thread

On programme une classe Thread. On utilise le constructeur pour mettre en place la politique d'ordonnancement Posix:


	Thread::Thread(int schedPolicy) : m_schedPolicy(schedPolicy)
	{
		pthread_t m_tid;
		pthread_attr_t m_attr;
		pthread_attr_init(&m_attr);
		pthread_attr_setschedpolicy(&m_attr, schedPolicy);
		pthread_attr_setinheritsched(&m_attr, PTHREAD_EXPLICIT_SCHED);
	}


La classe thread ainsi créée est une **classe abstraite**. Elle représente uniquement le concept de thread. En effet, nous avons déclaré une fonction virtuelle pure, `run()`. Ainsi, on impose aux classes dérivées de redéfinir et de fournir une implémentation concrète de la fonction. 


	protected:
		virtual void run() = 0;	

Nous avons toutefois besoin d'une  interface commune. Nous implémentons donc une méthode `call_run()` qui permet depuis l'objet Thread d'appeler la fonction `run()`. La signature de la fonction est telle que l'on puisse la passer à un pthread, d'où l'utilisation d'un *cast* ensuite.


	void* Thread::call_run(void* thread)
	{
		Thread* ptr_thread= (Thread*)thread;
		ptr_thread->run();
	}

Ce `call_run()` est comme indiqué passé au thread lors de sa création, dans la fonction `start()` de Thread. 

Pour mettre en place notre incrémenteur, nous développons une classe Incr, classe dérivée de Thread ``class Incr : public Thread``.

C'est donc dans cette fonction que sera implémenté la fonction `run()` virtuelle pure de Thread.

A ce stade, la fonction run() ressemble à cela: 

	void Incr::run()
	{
		if(m_pCounter->getMutexUse() == true)
		{
			for(int i=0; i <  m_pCounter->get_nLoops(); i++)
			{
				m_pCounter->incrementSafe();		
			}
		}
		else
		{
			for(int i=0; i < m_pCounter->get_nLoops(); i++)
			{
				m_pCounter->incrementUnsafe();
			}		
		}
	}


On ajoute à la classe Thread un ensemble de fonctions de paramètrage:

* Un join avec Timeout qui vient surcharger le join habituel :


		void Thread::join(double timeout_ms)
		{
			struct timespec abstime;
			clock_gettime(CLOCK_REALTIME, &abstime);
			long secs = timeout_ms / 1000;
			abstime.tv_sec += secs;
			abstime.tv_nsec += (timeout_ms - secs *1000) * 1000000;
			if(abstime.tv_nsec >= 1000000000)
			{	
				abstime.tv_sec += 1; 
				abstime.tv_nsec %= 1000000000; 
			}
			pthread_timedjoin_np(m_tid, NULL, &abstime);
		}


* Une option de paramètrage de la taille de la pile:

		void Thread::setStackSize(size_t stackSize)
		{
			pthread_attr_setstacksize(&m_attr, stackSize);
			//printf("Thread stack size successfully set to %li bytes\n",stackSize);
		}


* Une fonction qui permet simplement d'endormir le thread appelant durant le temps spécifié:

		void Thread::sleep(double delay_ms)
		{
			const double mille = 1000;
			struct timespec tim;
			tim.tv_sec = delay_ms / mille;
			tim.tv_nsec = (delay_ms - (delay_ms / mille) * mille) * mille * mille;
			nanosleep(&tim, NULL);
		}

Ceci étant fait, nous testons notre classe en refaisant le TD-1 dans un contexte orienté objet. Nous déclarons un compteur, et un vecteur de pointeur sur Incr. Nous appelons la méthode `start()` de chacun de ces threads *Incr*, avant de *join*. Nous n'oublions pas d'appeler delete pour les pointeurs dans le vecteur et d'appeler la méthode `clear()` du vecteur.
	
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




##Classes Mutex et Lock


Nous commençons par coder une classe Mutex qui propose les méthodes lock(), lock() avec timeout, trylock() et unlock(). 

	class Mutex
	{
	public:
		Mutex(bool isInversionSafe);
		~Mutex();
		void lock();
		bool lock(double timeout_ms);
		bool trylock();
		void unlock();
	protected:
		pthread_mutex_t mid;
	};


A la création d'un mutex, on donne l'option de se protéger de l'inversion de priorité par héritage de priorité en passant un booléen en argument. 
Notre constructeur est donc le suivant:

	Mutex::Mutex(bool isInversionSafe) 
	{
		pthread_mutex_t mid;
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		if(isInversionSafe == true)
		{
			pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);	
		}
		pthread_mutex_init(&mid, &attr);
		pthread_mutexattr_destroy(&attr);
	}

Ensuite, les méthodes lock, unlock, et trylock font simplement usage des méthodes posix sous-jacentes `pthread_mutex_XXXX(&mid)` où mid est le pthread_mutex_tL On implémente un lock avec time-out comme on l'a déjà détaillé pour le join() avec time-out de la classe Thread.
Pas de difficulté particulière donc.

La classe Lock est intéressante.
Nous l'implémentons pour exploiter les possibilités du C++ qui empile le constructeur et le destructeur. Ainsi, dans un bloc try catch, quoi qu'il arrive, en sortie de bloc, le destructeur est appelé et le jeton rendu. Nous n'avons pas besoin de new et delete dans la mesure où nous faisons des déclarations sur la pile. 

L'utilisation suivra le principe suivant:

	void f()
	{
		....
		{ 
			Lock l(&mutex);
			...; // section critique
		} // fin de la section critique: rendu de jeton car fermeture bloc
	}


Cette classe ne possède donc que trois méthodes qui vont appeler les méthodes équivalentes de la classe Mutex. Notons que c'est dans le destructeur qu'on appelera `Mutex::unlock()`:

	Lock::~Lock()
	{
		m->unlock();
	}

Pour l'utiliser, il suffit de modifier légèrement la classe Incr, et de lui donner un objet mutex en argument.

Dès lors, nous pouvons protéger l'incrémentation par un mutex en modèle objet: 
	
	double Incr::Counter::incrementSafe()
	{
		try
		{
			Lock lock(&mutex);
			value += 1;
		}
		catch(std::exception& e)
		{
			std::cerr << "Error:" << e.what() << std::endl;
		} 
		return value;
	}


##Classe Condition

Il s'agit ici d'implémenter une classe condition dérivée de la classe Mutex. 
Les méthodes principales de cette classe sont l'attente de notification, l'attente avec timeout, la notification, et le broadcast de notification

Les méthodes `wait()` et `wait(timeout_ms)` font simplement usage des méthodes posix sous-jacentes pthread_cond_wait et pthread_cond_timedwait, en oubliant pas que l'on utilise des temps absolus, et la normalisation du temps. 

Les fonction `notify()` et `notifyAll()` font usage des appels posix respectifs pthread_cond_signal et pthread_cond_broadcast. Il suffit de leur passer un pointeur sur la condition. 

De la même manière que pour Lock, on utilise le destructeur pour automatiquement detruire l'objet posix:


	Condition::~Condition()
	{
		pthread_cond_destroy(&cid);
	}

On va maintenant pouvoir utiliser un objet Lock avec une condition pour protéger le champs started de la classe Thread. 

Cela nous permet de ne pas risque de relancer une tâche inachevée de manière élégante:


	bool Thread::start(int priority) 
	{
		Lock lock(&condition);
		if(started)
		{
			return false; 
		}
		started = true;
		sched_param schedParam;
		schedParam.sched_priority = priority;
		pthread_attr_setschedparam(&m_attr, &schedParam);
		pthread_create(&m_tid, &m_attr, call_run, this); 
		return true;	
	}





##Classe Semaphore

Le semaphore se comporte comme une"boite à jetons". Il implémente les méthodes suivantes:

* La méthode `give()` permet d'ajouter un jeton à son stock:


	void Semaphore::give()
	{
		Lock lock(&condition);
		if(counter < maxCount)
		{
			counter += 1;
			condition.notifyAll();
		}
		else
		{
			while (counter >= maxCount)
			{
				condition.wait(); //wait libère le mutex en étant bloqué :)
			}
			counter += 1;
			condition.notifyAll();
		}	
	}

Nous n'oublions pas de "locker" avant l'accès au compteur, que l'on incrémente avant de notifier les tâches en attente à l'aide de `notifyAll()`. Si le compteur est plein, l'appel est bloquant. Une particularité de condition.wait() est qu'il libère le mutex en étant bloqué, donc nous évitons les problèmes. 
Enfin, lock est détruit en sorti de bloc.

* La méthode `take()` permet de prendre un jeton dans le sémaphore.
 
Une nouvelle fois nous en donnons une version avec et sans time-out.
Nous avons implémenté un mécanisme permetant de compter le nombre de tâches dans l'état bloqué, puisqu'elles ont fait un appel à `take()` alors que le sémaphore était vide. Ce nombre est stocké dans bkdTasks. 

	void Semaphore::take()
	{	
		Lock lock(&condition);
		if(counter > 0)
		{
			counter -= 1;
			condition.notifyAll();
		}
		else
		{	
			bkdTasks += 1;
			while (counter == 0)
			{ 
				condition.wait(); //wait libère le mutex en étant bloqué :)
			}
			bkdTasks -= 1; 
			counter -= 1;
			condition.notifyAll();
		}	
	}


* `flush()` permet de libérer l'ensemble des jetons

C'est ici que nous avons besoin de bkdTasks. 
Grâce à lui, on peut écrire simplement:

	void Semaphore::flush() 
	{
		Lock lock(&condition);
		counter += bkdTasks;
		condition.notifyAll();
	}

Nous "lockons" la condition, ajoutons un nombre de jeton correspondant au nombre de tâches bloquées, puis nous faisons un `notifyAll()`.



##Classe Fifo multitâches

Pour finir, nous implémentons une FIFO, qui fait usage de std::queue pour stocker nos futures requêtes.
Nous développons le tout dans un .hpp, en effet, nous développons  un template, qui permettra de préciser le type d'éléments contenus dans la FIFO a posteriori.
On la dote d'une classe EmptyException en public que l'on pourra utiliser quand la Fifo est vide:

	public:
		class EmptyException : public std::exception
			{
			public:
				const char* what() throw();
			};

Elle prend comme attributs une condition et une std::queue<T> que l'on nomme elements. 

Nous implémentons ensuite ses quatre méthodes, à savoir:

*  `isEmpty()`, qui renvoie simplement un booléen
*  `push()` ajoute un élément sur la Fifo

		void push(T msg)
		{
			elements.push(msg);
			condition.notify();
		}
* `pop()` est  plus intéressante. Dans sa version avec timeout, elle verrouille le mutex. Si la Fifo est vide, elle lance un wait avec timeout sur la condition. En cas de timeout, elle lance l'erreur EmptyException. Sinon, elle rejoint le cas normal, récupère le premier élément de la Fifo, le retourne et l'enlève:

		T pop(double timeout_ms)
		{
			Lock lock(&condition);
			while(elements.empty())
				if(!condition.wait(timeout_ms))
				{
					throw EmptyException();
				}
			T popped = elements.front();
			elements.pop();
			return popped;
		}

Nous avons ensuite entrepris de faire des tests. Pour cela, nous avons découpé notre ancienne classe *Incr* en deux classes dérivées de Thread, l'une nommée Consume et l'autre nommée Produce. 

La classe Produce est simple. elle prend un entier nPush, une schedulePolicy et un pointeur sur la Fifo. Elle construit l'objet Thread et push *nPush* fois le nombre 42 sur la Fifo. 

La classe Consume est plus proche de notre ancienne classe Incr. On modifie principalement l'implémentation de la fonction `run()`, qui utilise toujours `incrementSafe()`

	void Consume::run()
	{
		while(!(canFinish && m_pFifo->isEmpty()))
		{
			int res;	
			try
			{
			 	res = m_pFifo->pop(20);
				if(m_pCounter->getMutexUse() == true)
				{
					m_pCounter->incrementSafe();		
				}
				else
				{
					m_pCounter->incrementUnsafe();
				}
			}
			catch (const std::exception& ex)
			{ 
				std::cout << "Exception" << std::endl;
			}
			std::cout << "je suis tjs dans la boucle" << std::endl;
		}
		std::cout << "je suis sorti de la boucle" << std::endl;
	}

Nous avons voulu que l'on puisse sortir du while, mais ce uniquement lorsque la Fifo est vide et qu'il n'est plus possible que de nouveaux éléments y soient ajoutés. 
Ceci explique notre `while(!(canFinish && m_pFifo->isEmpty()))`. canFinish est mis à true dans le main(), après que l'on n'a fini de joindre les thread Produce. Ainsi, aucun nouvel élément ne peut être ajouté sur la Fifo. et l'on peut sortir du while une fois vide. 
Ensuite, au sein d'un bloc try-catch, nous appelons la méthode pop de la Fifo, et incrémentons le compteur. J'ai laissé les indications de débugage car il reste encore des problèmes que nous n'avons pas su résoudre. 

Nous avons modifé le main() afin de pouvoir dire en ligne de commande combien nous voulions lancer de thread producteurs et de thread consommateurs de cette façon:

**./main [Nombre de threads PRODUCTEURS] [Nombre de threads CONSOMMATEURS]**

L'utilisation du mutex est activée par défaut donc nous n'utilisons qu'`incrementSafe()`

Ensuite, à l'intérieur du main, nous créons un vecteur de pointeurs sur des *Consume* et un vecteur de pointeur sur des *Produce*. Nous lancons les *produce* et les *consume* avec leur méthode `start()`. Nous appelons ensuite `join()` sur les Produce. Une fois les Produce gérés, nous passons les attributs `canFinish` des *Consume* à `true` en appelant `allowFinish()`. Ceci étant fait, nous appelons `join()` sur les *Consume*. 
A la toute fin, nous imprimons la valeur du compteur, elle doit être égale à **10 fois le nombre de tâches productrices**, puisqu'ici chaque tâche productrice pousse dix éléments sur la FIFO, et que l'on incrémente le compteur à chaque appel à `pop()`. 

Après test, les résultats semblent corrects, mais le code bugué avec d'occasionnels seg fault, et des deadlock après l'impression de la valeur du compteur que nous n'avons su résoudre avant la fin de la classe. 