#include <yafraycore/ccthreads.h>
#include <iostream>
#include <stdexcept>

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

using namespace std;

namespace yafthreads {

mutex_t::mutex_t() 
{
#if HAVE_PTHREAD
	int error=pthread_mutex_init(&m, NULL);
	switch(error)
	{
		case EINVAL: throw std::runtime_error("pthread_mutex_init error EINVAL"); break;
		case ENOMEM: throw std::runtime_error("pthread_mutex_init error ENOMEM"); break;
		case EAGAIN: throw std::runtime_error("pthread_mutex_init error EAGAIN"); break;
		default: break;
	}
#endif
}

void mutex_t::lock() 
{
#if HAVE_PTHREAD
	if(pthread_mutex_lock(&m))
	{
		throw std::runtime_error("Error mutex lock");
	}
#endif
}

void mutex_t::unlock() 
{
#if HAVE_PTHREAD
	if(pthread_mutex_unlock(&m))
	{
		throw std::runtime_error("Error mutex lock");
	}
#endif
}

mutex_t::~mutex_t() 
{
#if HAVE_PTHREAD
	pthread_mutex_destroy(&m);
#endif
}

/* condition object */

conditionVar_t::conditionVar_t() 
{
#if HAVE_PTHREAD
	int error=pthread_mutex_init(&m, NULL);
	switch(error)
	{
		case EINVAL: throw std::runtime_error("pthread_mutex_init error EINVAL"); break;
		case ENOMEM: throw std::runtime_error("pthread_mutex_init error ENOMEM"); break;
		case EAGAIN: throw std::runtime_error("pthread_mutex_init error EAGAIN"); break;
		default: break;
	}
	error = pthread_cond_init (&c, NULL);
	if(error != 0)
	{
		throw std::runtime_error("pthread_cond_init error\n");
	}
#endif
}

void conditionVar_t::lock() 
{
#if HAVE_PTHREAD
	if(pthread_mutex_lock(&m))
	{
		throw std::runtime_error("Error mutex lock");
	}
#endif
}

void conditionVar_t::unlock() 
{
#if HAVE_PTHREAD
	if(pthread_mutex_unlock(&m))
	{
		throw std::runtime_error("Error mutex lock");
	}
#endif
}

void conditionVar_t::signal()
{
#if HAVE_PTHREAD
	if(pthread_cond_signal(&c))
	{
		throw std::runtime_error("Error condition signal");
	}	
#endif
}

void conditionVar_t::wait()
{
#if HAVE_PTHREAD
	if(pthread_cond_wait(&c, &m))
	{
		throw std::runtime_error("Error condition wait");
	}
#endif
}

conditionVar_t::~conditionVar_t() 
{
#if HAVE_PTHREAD
	pthread_mutex_destroy(&m);
	pthread_cond_destroy(&c);
#endif
}

#if HAVE_PTHREAD
void * wrapper(void *data)
{
	thread_t *obj=(thread_t *)data;
	obj->lock.lock();
	try{ obj->body(); }
	catch(std::exception &e)
	{
		std::cout << "exception occured: " << e.what() << std::endl;
	}
	obj->running=false;
	obj->lock.unlock();
	pthread_exit(0);
	return NULL;
}

void thread_t::run()
{
	lock.lock();
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	pthread_create(&id,&attr,wrapper,this);
	running=true;
	lock.unlock();
}

void thread_t::wait()
{
	if(running)
		pthread_join(id,NULL);
	running=false;
}

thread_t::~thread_t()
{
	wait();
}

mysemaphore_t::mysemaphore_t(int c) 
{
#ifdef __APPLE__
	static int id=0;
	char name[32];
	snprintf(name,32,"stupidosxsem%d",id);
	id++;
	s=sem_open(name, O_CREAT, 0777, c);
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_4
	if(s==SEM_FAILED) std::cout<<"Error sem_open"<<std::endl;
#else
	if((int)s==SEM_FAILED) std::cout<<"Error sem_open"<<std::endl;
#endif
	sem_unlink(name);
#else
	int error=sem_init (&s,0,c);
	if(error!=0)
	{
		if(errno==EINVAL) std::cout<<"sem_init EINVAL"<<std::endl;
		if(errno==ENOSYS) std::cout<<"sem_init ENOSYS"<<std::endl;
	}
#endif
}

void mysemaphore_t::wait() 
{ 
#ifdef __APPLE__
	if(sem_wait(s))
	{
		std::string err("Error sem_wait ");
		switch(errno)
		{
			case EAGAIN: err += "EAGAIN"; break;
			case EINVAL: err += "EINVAL"; break;
			case EDEADLK: err += "EDEADLK"; break;
			case EINTR: err += "EINTR"; break;
			default: err += "NOSE"; break;
		}
		throw std::runtime_error(err);
	}
#else
	sem_wait(&s); 
#endif
}

void mysemaphore_t::signal() 
{
#ifdef __APPLE__
	if(sem_post(s))
	{
		throw std::runtime_error("Error sem_post");
	}
#else
	sem_post(&s);
#endif
}

mysemaphore_t::~mysemaphore_t() 
{
#ifdef __APPLE__
  #if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_4
	if(s!=SEM_FAILED) sem_close(s);
  #else
	if((int)s!=SEM_FAILED) sem_close(s);
  #endif
#else
	sem_destroy (&s); 
#endif
}

#endif

} // yafthreads
