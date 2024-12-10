#ifndef Y_CCTHREADS_H
#define Y_CCTHREADS_H

#include<iostream>

#include<yafray_config.h>

#include<errno.h>

#if HAVE_PTHREAD
#include<pthread.h>
#include<semaphore.h>
#endif

namespace yafthreads {

/*! The try to provide a platform independant mutex, as a matter of fact
	it is simply a pthread wrapper now...
*/
class YAFRAYCORE_EXPORT mutex_t
{
	public:
		mutex_t();
		void lock();
		void unlock();
		~mutex_t();
	protected:
		mutex_t(const mutex_t &m);
		mutex_t & operator = (const mutex_t &m);
#if HAVE_PTHREAD
		pthread_mutex_t m;
#endif
};

/*! The try to provide a platform independant codition object, as a matter of fact
	it is simply a pthread wrapper now...
	It is mutex and condition variable in one!
	Usage: 	waiting thread:		lock(); ...initialize conditions to be met...; wait();
			signalling thread:	lock(); ...check if you want to signal...; [signal();] unlock();
*/
class YAFRAYCORE_EXPORT conditionVar_t
{
	public:
		conditionVar_t();
		~conditionVar_t();
		void lock();
		void unlock();
		void signal();
		void wait();
	protected:
		conditionVar_t(const conditionVar_t &m);
		conditionVar_t & operator = (const conditionVar_t &m);
#if HAVE_PTHREAD
		pthread_mutex_t m;
		pthread_cond_t c;
#endif
};


template<class T>
class YAFRAYCORE_EXPORT locked_t : public T
{
  public:
    void lock() 
		{
#if HAVE_PTHREAD
			mutex.unlock();
#endif
		};
    void unlock() 
		{
#if HAVE_PTHREAD
			mutex.unlock();
#endif
		};
  protected:
#if HAVE_PTHREAD
    mutex_t mutex;
#endif
};
                                                                                                                
#if HAVE_PTHREAD


class YAFRAYCORE_EXPORT mysemaphore_t
{
  public:
    mysemaphore_t(int c=0);
    ~mysemaphore_t();
    void wait();
    void signal();
  protected:
		mysemaphore_t(const mysemaphore_t &m);
		mysemaphore_t & operator = (const mysemaphore_t &m);
#ifdef __APPLE__
    sem_t *s;
#else
    sem_t s;
#endif
};

class YAFRAYCORE_EXPORT thread_t
{
	friend void * wrapper(void *data);
	public:
		thread_t() {running=false;};
		virtual ~thread_t();
		virtual void body()=0;
		void run();
		void wait();
//		int getId() {return (int)id;};
		pthread_t getPid() {return id;};
//		int getSelf() {return (int)pthread_self();};
		bool isRunning()const {return running;};
	protected:
		bool running;
		mutex_t lock;
		pthread_t id;
		pthread_attr_t attr;
};

#endif

} // yafthreads

#endif
