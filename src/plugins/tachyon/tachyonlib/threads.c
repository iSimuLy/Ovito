/*
 * threads.c - code for spawning threads on various platforms.
 *
 *  $Id: threads.c,v 1.63 2011/02/07 19:28:04 johns Exp $
 */ 

/*
   To use the WorkForce threading routines outside of Tachyon, 
   run the following sed commands to generate the WKF variants of the
   function names and macros.  This method supercedes the old hand-edited
   versions that I previously maintained, and retains the support for
   UI threads etc.  These are written in c-shell (/bin/csh) syntax:
     sed -e 's/rt_/wkf_/g' threads.c >! /tmp/tmp1
     sed -e 's/defined(THR)/defined(WKFTHREADS)/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/#ifdef THR/#ifdef WKFTHREADS/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/#ifndef THR/#ifndef WKFTHREADS/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/RTUSE/WKFUSE/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/RTFORCE/WKFFORCE/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/threads.h/WKFThreads.h/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/RT_/WKF_/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/STAWKF_ROUTINE/START_ROUTINE/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/RTTHREAD/WKFTHREAD/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/ THR / WKFTHREADS /g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/THRUSE/WKFUSE/g' /tmp/tmp1 >! /tmp/WKFThreads.C

     sed -e 's/rt_/wkf_/g' threads.h >! /tmp/tmp1
     sed -e 's/defined(THR)/defined(WKFTHREADS)/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/#ifdef THR/#ifdef WKFTHREADS/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/#ifndef THR/#ifndef WKFTHREADS/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/RTUSE/WKFUSE/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/RTFORCE/WKFFORCE/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/threads.h/WKFThreads.h/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/RT_/WKF_/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/STAWKF_ROUTINE/START_ROUTINE/g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/RTTHREAD/WKFTHREAD/g' /tmp/tmp1 >! /tmp/tmp2
     sed -e 's/ THR / WKFTHREADS /g' /tmp/tmp2 >! /tmp/tmp1
     sed -e 's/THRUSE/WKFUSE/g' /tmp/tmp1 >! /tmp/WKFThreads.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * If compiling on Linux, enable the GNU CPU affinity functions in both
 * libc and the libpthreads
 */
#if defined(__linux)
#define _GNU_SOURCE 1
#include <sched.h>
#endif

#include "threads.h"

#ifdef WIN32
#if 0
#define RTUSENEWWIN32APIS 1
#define _WIN32_WINNT 0x0400 /**< needed for TryEnterCriticalSection(), etc */
#define  WINVER      0x0400 /**< needed for TryEnterCriticalSection(), etc */
#endif
#include <windows.h>        /**< main Win32 APIs and types */
#include <winbase.h>        /**< system services headers */
#endif

#if defined(_AIX) || defined(_CRAY) || defined(__irix) || defined(__linux) || defined(__osf__) || defined(__sun)
#include <unistd.h>         /**< sysconf() headers, used by most systems */
#endif

#if defined(__APPLE__) && defined(THR)
#include <Carbon/Carbon.h>  /**< Carbon APIs for Multiprocessing */
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if defined(__hpux)
#include <sys/mpctl.h>      /**< HP-UX Multiprocessing headers */
#endif


#ifdef __cplusplus
extern "C" {
#endif

int rt_thread_numphysprocessors(void) {
  int a=1;

#ifdef THR
#if defined(__APPLE__)
  int nm[2];
  size_t len = 4;
  uint32_t count;

  nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
  sysctl(nm, 2, &count, &len, NULL, 0);

  if(count < 1) {
	  nm[1] = HW_NCPU;
	  sysctl(nm, 2, &count, &len, NULL, 0);
	  if(count < 1) { count = 1; }
  }
  a = (int)count;       /**< Number of active/running CPUs */
#endif

#ifdef WIN32
  struct _SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  a = sysinfo.dwNumberOfProcessors;  /**< total number of CPUs */
#endif /* WIN32 */

#if defined(__PARAGON__) 
  a=2; /**< Threads-capable Paragons have 2 CPUs for computation */
#endif /* __PARAGON__ */ 

#if defined(_CRAY)
  a = sysconf(_SC_CRAY_NCPU);        /**< total number of CPUs */
#endif

#if defined(__sun) || defined(__linux) || defined(__osf__) || defined(_AIX)
  a = sysconf(_SC_NPROCESSORS_ONLN); /**< Number of active/running CPUs */
#endif /* SunOS */

#if defined(__irix)
  a = sysconf(_SC_NPROC_ONLN);       /**< Number of active/running CPUs */
#endif /* IRIX */

#if defined(__hpux)
  a = mpctl(MPC_GETNUMSPUS, 0, 0);   /**< total number of CPUs */
#endif /* HPUX */
#endif /* THR */

  return a;
}


int rt_thread_numprocessors(void) {
  int a=1;

#ifdef THR
  /* Allow the user to override the number of CPUs for use */
  /* in scalability testing, debugging, etc.               */
  char *forcecount = getenv("RTFORCECPUCOUNT");
  if (forcecount != NULL) {
    if (sscanf(forcecount, "%d", &a) == 1) {
      return a; /* if we got a valid count, return it */
    } else {
      a=1;      /* otherwise use the real available hardware CPU count */
    }
  }

  /* otherwise return the number of physical processors currently available */
  a = rt_thread_numphysprocessors();

  /* XXX we should add checking for the current CPU affinity masks here, */
  /* and return the min of the physical processor count and CPU affinity */
  /* mask enabled CPU count.                                             */
#endif /* THR */

  return a;
}


int * rt_cpu_affinitylist(int *cpuaffinitycount) {
  int *affinitylist = NULL;
  *cpuaffinitycount = -1; /* return count -1 if unimplemented or err occurs */

/* Win32 process affinity mask query */
#if 0 && defined(WIN32)
  /* XXX untested, but based on the linux code, may work with a few tweaks */
  HANDLE myproc = GetCurrentProcess(); /* returns a psuedo-handle */
  DWORD affinitymask, sysaffinitymask;

  if (!GetProcessAffinityMask(myproc, &affinitymask, &sysaffinitymask)) {
    /* count length of affinity list */
    int affinitycount=0;
    int i;
    for (i=0; i<31; i++) {
      affinitycount += (affinitymask >> i) & 0x1;
    }

    /* build affinity list */
    if (affinitycount > 0) {
      affinitylist = (int *) malloc(affinitycount * sizeof(int));
      if (affinitylist == NULL)
        return NULL;

      int curcount = 0;
      for (i=0; i<CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &affinitymask)) {
          affinitylist[curcount] = i;
          curcount++;
        }
      }
    }

    *cpuaffinitycount = affinitycount; /* return final affinity list */
  }
#endif

/* Linux process affinity mask query */
#if defined(__linux)

/* protect ourselves from some older Linux distros */
#if defined(CPU_SETSIZE)
  int i;
  cpu_set_t affinitymask;
  int affinitycount=0;

  /* PID 0 refers to the current process */
  if (sched_getaffinity(0, sizeof(affinitymask), &affinitymask) < 0) {
    perror("rt_cpu_affinitylist: sched_getaffinity");
    return NULL;
  }

  /* count length of affinity list */
  for (i=0; i<CPU_SETSIZE; i++) {
    affinitycount += CPU_ISSET(i, &affinitymask);
  }

  /* build affinity list */
  if (affinitycount > 0) {
    affinitylist = (int *) malloc(affinitycount * sizeof(int));
    if (affinitylist == NULL)
      return NULL;

    int curcount = 0;
    for (i=0; i<CPU_SETSIZE; i++) {
      if (CPU_ISSET(i, &affinitymask)) {
        affinitylist[curcount] = i;
        curcount++;
      }
    }
  }

  *cpuaffinitycount = affinitycount; /* return final affinity list */
#endif
#endif

  /* MacOS X 10.5.x has a CPU affinity query/set capability finally      */
  /* http://developer.apple.com/releasenotes/Performance/RN-AffinityAPI/ */

  /* Solaris and HP-UX use pset_bind() and related functions, and they   */
  /* don't use the single-level mask-based scheduling mechanism that     */
  /* the others, use.  Instead, they use a hierarchical tree of          */
  /* processor sets and processes float within those, or are tied to one */
  /* processor that's a member of a particular set.                      */

  return affinitylist;
}


int rt_thread_set_self_cpuaffinity(int cpu) {
  int status=-1; /* unsupported by default */

#ifdef THR

#if defined(__linux) && defined(CPU_ZERO) && defined(CPU_SET)
#if 0
  /* XXX this code is too new even for RHEL4, though it runs on Fedora 7 */
  /* and other newer revs.                                               */
  /* NPTL systems can assign per-thread affinities this way              */
  cpu_set_t affinitymask;
  CPU_ZERO(&affinitymask);
  CPU_SET(cpu, &affinitymask);
  status = pthread_setaffinity_np(pthread_self(), sizeof(affinitymask), &affinitymask);
#else
  /* non-NPTL systems based on the clone() API must use this method      */
  cpu_set_t affinitymask;
  CPU_ZERO(&affinitymask);
  CPU_SET(cpu, &affinitymask);

  /* PID 0 refers to the current process */
  if ((status=sched_setaffinity(0, sizeof(affinitymask), &affinitymask)) < 0) {
    perror("rt_thread_set_self_cpuaffinitylist: sched_setaffinity");
    return status;
  }
#endif

  /* call sched_yield() so new affinity mask takes effect immediately */
  sched_yield();
#endif /* linux */

  /* MacOS X 10.5.x has a CPU affinity query/set capability finally      */
  /* http://developer.apple.com/releasenotes/Performance/RN-AffinityAPI/ */

  /* Solaris and HP-UX use pset_bind() and related functions, and they   */
  /* don't use the single-level mask-based scheduling mechanism that     */
  /* the others, use.  Instead, they use a hierarchical tree of          */
  /* processor sets and processes float within those, or are tied to one */
  /* processor that's a member of a particular set.                      */
#endif

  return status;
}


int rt_thread_setconcurrency(int nthr) {
  int status=0;

#ifdef THR
#if defined(__sun)
#ifdef USEPOSIXTHREADS
  status = pthread_setconcurrency(nthr);
#else
  status = thr_setconcurrency(nthr);
#endif
#endif /* SunOS */

#if defined(__irix) || defined(_AIX)
  status = pthread_setconcurrency(nthr);
#endif
#endif /* THR */

  return status;
}


/*
 * Thread creation/management
 */
/** Typedef to eliminate compiler warning caused by C/C++ linkage conflict. */
typedef void * (*RTTHREAD_START_ROUTINE)(void *);

int rt_thread_create(rt_thread_t * thr, void * fctn(void *), void * arg) {
  int status=0;

#ifdef THR
#ifdef WIN32
  DWORD tid; /* thread id, msvc only */
  *thr = CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE) fctn, arg, 0, &tid);
  if (*thr == NULL) {
    status = -1;
  }
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS 
#if defined(_AIX)
  /* AIX schedule threads in system scope by default, have to ask explicitly */
  {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    status = pthread_create(thr, &attr, (RTTHREAD_START_ROUTINE)fctn, arg);
    pthread_attr_destroy(&attr);
  }
#elif defined(__PARAGON__)
  status = pthread_create(thr, pthread_attr_default, fctn, arg);
#else   
  status = pthread_create(thr, NULL, (RTTHREAD_START_ROUTINE)fctn, arg);
#endif 
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS 
  status = thr_create(NULL, 0, (RTTHREAD_START_ROUTINE)fctn, arg, 0, thr); 
#endif /* USEUITHREADS */
#endif /* THR */
 
  return status;
}


int rt_thread_join(rt_thread_t thr, void ** stat) {
  int status=0;  

#ifdef THR
#ifdef WIN32
  DWORD wstatus = 0;
 
  wstatus = WAIT_TIMEOUT;
 
  while (wstatus != WAIT_OBJECT_0) {
    wstatus = WaitForSingleObject(thr, INFINITE);
  }
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_join(thr, stat);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = thr_join(thr, NULL, stat);
#endif /* USEPOSIXTHREADS */
#endif /* THR */

  return status;
}  


/*
 * Mutexes
 */
int rt_mutex_init(rt_mutex_t * mp) {
  int status=0;

#ifdef THR
#ifdef WIN32
  InitializeCriticalSection(mp);
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_mutex_init(mp, 0);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS 
  status = mutex_init(mp, USYNC_THREAD, NULL);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


int rt_mutex_lock(rt_mutex_t * mp) {
  int status=0;

#ifdef THR
#ifdef WIN32
  EnterCriticalSection(mp);
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_mutex_lock(mp);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = mutex_lock(mp);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


int rt_mutex_trylock(rt_mutex_t * mp) {
  int status=0;

#ifdef THR
#ifdef WIN32
#if defined(THRUSENEWWIN32APIS)
  /* TryEnterCriticalSection() is only available on newer */
  /* versions of Win32: _WIN32_WINNT/WINVER >= 0x0400     */
  status = (!(TryEnterCriticalSection(mp)));
#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = (pthread_mutex_lock(mp) != 0);
#endif /* USEPOSIXTHREADS */
#endif /* THR */

  return status;
}


int rt_mutex_spin_lock(rt_mutex_t * mp) {
  int status=0;

#ifdef THR
#ifdef WIN32
#if defined(THRUSENEWWIN32APIS)
  /* TryEnterCriticalSection() is only available on newer */
  /* versions of Win32: _WIN32_WINNT/WINVER >= 0x0400     */
  while (!TryEnterCriticalSection(mp));
#else
  EnterCriticalSection(mp);
#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  while ((status = pthread_mutex_trylock(mp)) != 0);
#endif /* USEPOSIXTHREADS */
#endif /* THR */

  return status;
}


int rt_mutex_unlock(rt_mutex_t * mp) {
  int status=0;

#ifdef THR  
#ifdef WIN32
  LeaveCriticalSection(mp);
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_mutex_unlock(mp);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = mutex_unlock(mp);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


int rt_mutex_destroy(rt_mutex_t * mp) {
  int status=0;

#ifdef THR
#ifdef WIN32
  DeleteCriticalSection(mp);
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_mutex_destroy(mp);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = mutex_destroy(mp);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


/*
 * Condition variables
 */
int rt_cond_init(rt_cond_t * cvp) {
  int status=0;

#ifdef THR
#ifdef WIN32
#if defined(RTUSEWIN2008CONDVARS)
  InitializeConditionVariable(cvp);
#else
  /* XXX not implemented */
  cvp->waiters = 0;

  /* Create an auto-reset event. */
  cvp->events[RT_COND_SIGNAL] = CreateEvent(NULL,  /* no security */
                                            FALSE, /* auto-reset event */
                                            FALSE, /* non-signaled initially */
                                            NULL); /* unnamed */

  /* Create a manual-reset event. */
  cvp->events[RT_COND_BROADCAST] = CreateEvent(NULL,  /* no security */
                                               TRUE,  /* manual-reset */
                                               FALSE, /* non-signaled initially */
                                               NULL); /* unnamed */
#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_cond_init(cvp, NULL);
#endif /* USEPOSIXTHREADS */
#ifdef USEUITHREADS
  status = cond_init(cvp, USYNC_THREAD, NULL);
#endif
#endif /* THR */

  return status;
}

int rt_cond_destroy(rt_cond_t * cvp) {
  int status=0;

#ifdef THR
#ifdef WIN32
#if defined(RTUSEWIN2008CONDVARS)
  /* XXX not implemented */
#else
  CloseHandle(cvp->events[RT_COND_SIGNAL]);
  CloseHandle(cvp->events[RT_COND_BROADCAST]);
#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_cond_destroy(cvp);
#endif /* USEPOSIXTHREADS */
#ifdef USEUITHREADS
  status = cond_destroy(cvp);
#endif
#endif /* THR */

  return status;
}

int rt_cond_wait(rt_cond_t * cvp, rt_mutex_t * mp) {
  int status=0;
#if defined(THR) && defined(WIN32)
  int result=0;
  LONG last_waiter;
  LONG my_waiter;
#endif

#ifdef THR
#ifdef WIN32
#if defined(RTUSEWIN2008CONDVARS)
  SleepConditionVariableCS(cvp, mp, INFINITE)
#else
#if !defined(RTUSEINTERLOCKEDATOMICOPS)
  EnterCriticalSection(&cvp->waiters_lock);
  cvp->waiters++;
  LeaveCriticalSection(&cvp->waiters_lock);
#else
  InterlockedIncrement(&cvp->waiters);
#endif

  LeaveCriticalSection(mp); /* SetEvent() keeps state, avoids lost wakeup */

  /* Wait either a single or broadcast even to become signalled */
  result = WaitForMultipleObjects(2, cvp->events, FALSE, INFINITE);

#if !defined(RTUSEINTERLOCKEDATOMICOPS)
  EnterCriticalSection (&cvp->waiters_lock);
  cvp->waiters--;
  last_waiter =
    ((result == (WAIT_OBJECT_0 + RT_COND_BROADCAST)) && cvp->waiters == 0);
  LeaveCriticalSection (&cvp->waiters_lock);
#else
  my_waiter = InterlockedDecrement(&cvp->waiters);
  last_waiter =
    ((result == (WAIT_OBJECT_0 + RT_COND_BROADCAST)) && my_waiter == 0);
#endif

  /* Some thread called cond_broadcast() */
  if (last_waiter)
    /* We're the last waiter to be notified or to stop waiting, so */
    /* reset the manual event.                                     */
    ResetEvent(cvp->events[RT_COND_BROADCAST]);

  EnterCriticalSection(mp);
#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_cond_wait(cvp, mp);
#endif /* USEPOSIXTHREADS */
#ifdef USEUITHREADS
  status = cond_wait(cvp, mp);
#endif
#endif /* THR */

  return status;
}

int rt_cond_signal(rt_cond_t * cvp) {
  int status=0;

#ifdef THR
#ifdef WIN32
#if defined(RTUSEWIN2008CONDVARS)
  WakeConditionVariable(cvp);
#else
#if !defined(RTUSEINTERLOCKEDATOMICOPS)
  EnterCriticalSection(&cvp->waiters_lock);
  int have_waiters = (cvp->waiters > 0);
  LeaveCriticalSection(&cvp->waiters_lock);
  if (have_waiters)
    SetEvent (cvp->events[RT_COND_SIGNAL]);
#else
  if (InterlockedExchangeAdd(&cvp->waiters, 0) > 0)
    SetEvent(cvp->events[RT_COND_SIGNAL]);
#endif
#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_cond_signal(cvp);
#endif /* USEPOSIXTHREADS */
#ifdef USEUITHREADS
  status = cond_signal(cvp);
#endif
#endif /* THR */

  return status;
}

int rt_cond_broadcast(rt_cond_t * cvp) {
  int status=0;

#ifdef THR
#ifdef WIN32
#if defined(RTUSEWIN2008CONDVARS)
  WakeAllConditionVariable(cvp);
#else
#if !defined(RTUSEINTERLOCKEDATOMICOPS)
  EnterCriticalSection(&cvp->waiters_lock);
  int have_waiters = (cvp->waiters > 0);
  LeaveCriticalSection(&cvp->waiters_lock);
  if (have_waiters)
    SetEvent(cvp->events[RT_COND_BROADCAST]);
#else
  if (InterlockedExchangeAdd(&cvp->waiters, 0) > 0)
    SetEvent(cvp->events[RT_COND_BROADCAST]);
#endif

#endif
#endif /* WIN32 */

#ifdef USEPOSIXTHREADS
  status = pthread_cond_broadcast(cvp);
#endif /* USEPOSIXTHREADS */
#ifdef USEUITHREADS
  status = cond_broadcast(cvp);
#endif
#endif /* THR */

  return status;
}


/*
 * Reader/Writer locks -- slower than mutexes but good for some purposes
 */
int rt_rwlock_init(rt_rwlock_t * rwp) {
  int status=0;

#ifdef THR  
#ifdef WIN32
  rt_mutex_init(&rwp->lock);
  rt_cond_init(&rwp->rdrs_ok);
  rt_cond_init(&rwp->wrtr_ok);
  rwp->rwlock = 0;
  rwp->waiting_writers = 0;
#endif

#ifdef USEPOSIXTHREADS
  pthread_mutex_init(&rwp->lock, NULL);
  pthread_cond_init(&rwp->rdrs_ok, NULL);
  pthread_cond_init(&rwp->wrtr_ok, NULL);
  rwp->rwlock = 0;
  rwp->waiting_writers = 0;
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = rwlock_init(rwp, USYNC_THREAD, NULL);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


int rt_rwlock_readlock(rt_rwlock_t * rwp) {
  int status=0;

#ifdef THR  
#ifdef WIN32
  rt_mutex_lock(&rwp->lock);
  while (rwp->rwlock < 0 || rwp->waiting_writers) 
    rt_cond_wait(&rwp->rdrs_ok, &rwp->lock);   
  rwp->rwlock++;   /* increment number of readers holding the lock */
  rt_mutex_unlock(&rwp->lock);
#endif

#ifdef USEPOSIXTHREADS
  pthread_mutex_lock(&rwp->lock);
  while (rwp->rwlock < 0 || rwp->waiting_writers) 
    pthread_cond_wait(&rwp->rdrs_ok, &rwp->lock);   
  rwp->rwlock++;   /* increment number of readers holding the lock */
  pthread_mutex_unlock(&rwp->lock);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = rw_rdlock(rwp);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


int rt_rwlock_writelock(rt_rwlock_t * rwp) {
  int status=0;

#ifdef THR  
#ifdef WIN32
  rt_mutex_lock(&rwp->lock);
  while (rwp->rwlock != 0) {
    rwp->waiting_writers++;
    rt_cond_wait(&rwp->wrtr_ok, &rwp->lock);
    rwp->waiting_writers--;
  }
  rwp->rwlock=-1;
  rt_mutex_unlock(&rwp->lock);
#endif

#ifdef USEPOSIXTHREADS
  pthread_mutex_lock(&rwp->lock);
  while (rwp->rwlock != 0) {
    rwp->waiting_writers++;
    pthread_cond_wait(&rwp->wrtr_ok, &rwp->lock);
    rwp->waiting_writers--;
  }
  rwp->rwlock=-1;
  pthread_mutex_unlock(&rwp->lock);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = rw_wrlock(rwp);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


int rt_rwlock_unlock(rt_rwlock_t * rwp) {
  int status=0;

#ifdef THR  
#ifdef WIN32
  int ww, wr;
  rt_mutex_lock(&rwp->lock);
  if (rwp->rwlock > 0) {
    rwp->rwlock--;
  } else {
    rwp->rwlock = 0;
  } 
  ww = (rwp->waiting_writers && rwp->rwlock == 0);
  wr = (rwp->waiting_writers == 0);
  rt_mutex_unlock(&rwp->lock);
  if (ww) 
    rt_cond_signal(&rwp->wrtr_ok);
  else if (wr)
    rt_cond_signal(&rwp->rdrs_ok);
#endif

#ifdef USEPOSIXTHREADS
  int ww, wr;
  pthread_mutex_lock(&rwp->lock);
  if (rwp->rwlock > 0) {
    rwp->rwlock--;
  } else {
    rwp->rwlock = 0;
  } 
  ww = (rwp->waiting_writers && rwp->rwlock == 0);
  wr = (rwp->waiting_writers == 0);
  pthread_mutex_unlock(&rwp->lock);
  if (ww) 
    pthread_cond_signal(&rwp->wrtr_ok);
  else if (wr)
    pthread_cond_signal(&rwp->rdrs_ok);
#endif /* USEPOSIXTHREADS */

#ifdef USEUITHREADS
  status = rw_unlock(rwp);
#endif /* USEUITHREADS */
#endif /* THR */

  return status;
}


/*
 * Simple counting barrier primitive
 */
rt_barrier_t * rt_thread_barrier_init(int n_clients) {
  rt_barrier_t *barrier = (rt_barrier_t *) malloc(sizeof(rt_barrier_t));

#ifdef THR
  if (barrier != NULL) {
    barrier->n_clients = n_clients;
    barrier->n_waiting = 0;
    barrier->phase = 0;
    barrier->sum = 0;
    rt_mutex_init(&barrier->lock);
    rt_cond_init(&barrier->wait_cv);
  }
#endif

  return barrier;
}


void rt_thread_barrier_destroy(rt_barrier_t *barrier) {
#ifdef THR
  rt_mutex_destroy(&barrier->lock);
  rt_cond_destroy(&barrier->wait_cv);
#endif
  free(barrier);
}


int rt_thread_barrier(rt_barrier_t *barrier, int increment) {
#ifdef THR
  int my_phase;
  int my_result;

  rt_mutex_lock(&barrier->lock);
  my_phase = barrier->phase;
  barrier->sum += increment;
  barrier->n_waiting++;

  if (barrier->n_waiting == barrier->n_clients) {
    barrier->result = barrier->sum;
    barrier->sum = 0;
    barrier->n_waiting = 0;
    barrier->phase = 1 - my_phase;
    rt_cond_broadcast(&barrier->wait_cv);
  }

  while (barrier->phase == my_phase) {
    rt_cond_wait(&barrier->wait_cv, &barrier->lock);
  }

  my_result = barrier->result;

  rt_mutex_unlock(&barrier->lock);

  return my_result; 
#else 
  return 0;
#endif
}


/*
 * Barriers used for sleepable thread pools
 */
/* symmetric run barrier for use within a single process */
int rt_thread_run_barrier_init(rt_run_barrier_t *barrier, int n_clients) {
#ifdef THR
  if (barrier != NULL) {
    barrier->n_clients = n_clients;
    barrier->n_waiting = 0;
    barrier->phase = 0;
    barrier->fctn = NULL;

    rt_mutex_init(&barrier->lock);
    rt_cond_init(&barrier->wait_cv);
  }
#endif

  return 0;
}

void rt_thread_run_barrier_destroy(rt_run_barrier_t *barrier) {
#ifdef THR
  rt_mutex_destroy(&barrier->lock);
  rt_cond_destroy(&barrier->wait_cv);
#endif
}


/**
 * Wait until all threads reach barrier, and return the function
 * pointer passed in by the master thread.
 */
void * (*rt_thread_run_barrier(rt_run_barrier_t *barrier,
                               void * fctn(void*),
                               void * parms,
                               void **rsltparms))(void *) {
#if defined(THR)
  int my_phase;
  void * (*my_result)(void*);

  rt_mutex_lock(&barrier->lock);
  my_phase = barrier->phase;
  if (fctn != NULL)
    barrier->fctn = fctn;
  if (parms != NULL)
    barrier->parms = parms;
  barrier->n_waiting++;

  if (barrier->n_waiting == barrier->n_clients) {
    barrier->rslt = barrier->fctn;
    barrier->rsltparms = barrier->parms;
    barrier->fctn = NULL;
    barrier->parms = NULL;
    barrier->n_waiting = 0;
    barrier->phase = 1 - my_phase;
    rt_cond_broadcast(&barrier->wait_cv);
  }

  while (barrier->phase == my_phase) {
    rt_cond_wait(&barrier->wait_cv, &barrier->lock);
  }

  my_result = barrier->rslt;
  if (rsltparms != NULL)
    *rsltparms = barrier->rsltparms;

  rt_mutex_unlock(&barrier->lock);
#else
  void * (*my_result)(void*) = fctn;
  if (rsltparms != NULL)
    *rsltparms = parms;
#endif

  return my_result;
}


/** non-blocking poll to see if peers are already at the barrier */
int rt_thread_run_barrier_poll(rt_run_barrier_t *barrier) {
  int rc=0;
#if defined(THR)
  rt_mutex_lock(&barrier->lock);
  if (barrier->n_waiting == (barrier->n_clients-1)) {
    rc=1;
  }
  rt_mutex_unlock(&barrier->lock);
#endif
  return rc;
}


/*
 * task tile stack
 */
int rt_tilestack_init(rt_tilestack_t *s, int size) {
  if (s == NULL)
    return -1;

#if defined(THR)
  rt_mutex_init(&s->mtx);
#endif

  s->growthrate = 512;
  s->top = -1;

  if (size > 0) {
    s->size = size;
    s->s = (rt_tasktile_t *) malloc(s->size * sizeof(rt_tasktile_t));
  } else {
    s->size = 0;
    s->s = NULL;
  }

  return 0;
}


void rt_tilestack_destroy(rt_tilestack_t *s) {
#if defined(THR)
  rt_mutex_destroy(&s->mtx);
#endif
  free(s->s);
  s->s = NULL; /* prevent access after free */
}


int rt_tilestack_compact(rt_tilestack_t *s) {
#if defined(THR)
  rt_mutex_lock(&s->mtx);
#endif
  if (s->size > (s->top + 1)) {
    int newsize = s->top + 1;
    rt_tasktile_t *tmp = (rt_tasktile_t *) realloc(s->s, newsize * sizeof(rt_tasktile_t));
    if (tmp == NULL) {
#if defined(THR)
      rt_mutex_unlock(&s->mtx);
#endif
      return -1; /* out of space! */
    }
    s->s = tmp;
    s->size = newsize;
  }
#if defined(THR)
  rt_mutex_unlock(&s->mtx);
#endif

  return 0;
}


int rt_tilestack_push(rt_tilestack_t *s, const rt_tasktile_t *t) {
#if defined(THR)
  rt_mutex_lock(&s->mtx);
#endif
  s->top++;
  if (s->top >= s->size) {
    int newsize = s->size + s->growthrate;
    rt_tasktile_t *tmp = (rt_tasktile_t *) realloc(s->s, newsize * sizeof(rt_tasktile_t));
    if (tmp == NULL) {
      s->top--;
#if defined(THR)
      rt_mutex_unlock(&s->mtx);
#endif
      return -1; /* out of space! */
    }
    s->s = tmp;
    s->size = newsize;
  }

  s->s[s->top] = *t; /* push onto the stack */

#if defined(THR)
  rt_mutex_unlock(&s->mtx);
#endif

  return 0;
}


int rt_tilestack_pop(rt_tilestack_t *s, rt_tasktile_t *t) {
#if defined(THR)
  rt_mutex_lock(&s->mtx);
#endif

  if (s->top < 0) {
#if defined(THR)
    rt_mutex_unlock(&s->mtx);
#endif
    return RT_TILESTACK_EMPTY; /* empty stack */
  }

  *t = s->s[s->top];
  s->top--;

#if defined(THR)
  rt_mutex_unlock(&s->mtx);
#endif

  return 0;
}


int rt_tilestack_popall(rt_tilestack_t *s) {
#if defined(THR)
  rt_mutex_lock(&s->mtx);
#endif

  s->top = -1;

#if defined(THR)
  rt_mutex_unlock(&s->mtx);
#endif

  return 0;
}


int rt_tilestack_empty(rt_tilestack_t *s) {
#if defined(THR)
  rt_mutex_lock(&s->mtx);
#endif

  if (s->top < 0) {
#if defined(THR)
    rt_mutex_unlock(&s->mtx);
#endif
    return 1;
  }

#if defined(THR)
  rt_mutex_unlock(&s->mtx);
#endif

  return 0;
}


/*
 * shared iterators
 */

/** initialize a shared iterator */
int rt_shared_iterator_init(rt_shared_iterator_t *it) {
  memset(it, 0, sizeof(rt_shared_iterator_t));
#if defined(THR)
  rt_mutex_init(&it->mtx);
#endif
  return 0;
}


/** destroy a shared iterator */
int rt_shared_iterator_destroy(rt_shared_iterator_t *it) {
#if defined(THR)
  rt_mutex_destroy(&it->mtx);
#endif
  return 0;
}


/** set shared iterator parameters */
int rt_shared_iterator_set(rt_shared_iterator_t *it,
                           rt_tasktile_t *tile) {
#if defined(THR)
  rt_mutex_lock(&it->mtx);
#endif
  it->start = tile->start;
  it->current = tile->start;
  it->end = tile->end;
  it->fatalerror = 0;
#if defined(THR)
  rt_mutex_unlock(&it->mtx);
#endif
  return 0;
}


/** iterate the shared iterator, over a requested half-open interval */
int rt_shared_iterator_next_tile(rt_shared_iterator_t *it, int reqsize,
                                 rt_tasktile_t *tile) {
  int rc=RT_SCHED_CONTINUE;

#if defined(THR)
  rt_mutex_spin_lock(&it->mtx);
#endif
  if (!it->fatalerror) {
    tile->start=it->current; /* set start to the current work unit    */
    it->current+=reqsize;    /* increment by the requested tile size  */
    tile->end=it->current;   /* set the (exclusive) endpoint          */

    /* if start is beyond the last work unit, we're done */
    if (tile->start >= it->end) {
      tile->start=0;
      tile->end=0;
      rc = RT_SCHED_DONE;
    }

    /* if the endpoint (exclusive) for the requested tile size */
    /* is beyond the last work unit, roll it back as needed     */
    if (tile->end > it->end) {
      tile->end = it->end;
    }
  } else {
    rc = RT_SCHED_DONE;
  }
#if defined(THR)
  rt_mutex_unlock(&it->mtx);
#endif

  return rc;
}


/** worker thread calls this to indicate a fatal error */
int rt_shared_iterator_setfatalerror(rt_shared_iterator_t *it) {
#if defined(THR)
  rt_mutex_spin_lock(&it->mtx);
#endif
  it->fatalerror=1;
#if defined(THR)
  rt_mutex_unlock(&it->mtx);
#endif
  return 0;
}


/** master thread calls this to query for fatal errors */
int rt_shared_iterator_getfatalerror(rt_shared_iterator_t *it) {
  int rc=0;
#if defined(THR)
  rt_mutex_lock(&it->mtx);
#endif
  if (it->fatalerror)
    rc = -1;
#if defined(THR)
  rt_mutex_unlock(&it->mtx);
#endif
  return rc;
}


#if defined(THR)
/*
 * Thread pool.
 */
static void * rt_threadpool_workerproc(void *voidparms) {
  void *(*fctn)(void*);
  rt_threadpool_workerdata_t *workerdata = (rt_threadpool_workerdata_t *) voidparms;
  rt_threadpool_t *thrpool = (rt_threadpool_t *) workerdata->thrpool;

  while ((fctn = rt_thread_run_barrier(&thrpool->runbar, NULL, NULL, &workerdata->parms)) != NULL) {
    (*fctn)(workerdata);
  }

  return NULL;
}


static void * rt_threadpool_workersync(void *voidparms) {
  return NULL;
}
#endif


rt_threadpool_t * rt_threadpool_create(int workercount, int *devlist) {
  int i;
  rt_threadpool_t *thrpool = NULL;
  thrpool = (rt_threadpool_t *) malloc(sizeof(rt_threadpool_t));
  if (thrpool == NULL)
    return NULL;

  memset(thrpool, 0, sizeof(rt_threadpool_t));

#if !defined(THR)
  workercount=1;
#endif

  /* if caller provides a device list, use it, otherwise we assume */
  /* all workers are CPU cores */
  thrpool->devlist = (int *) malloc(sizeof(int) * workercount);
  if (devlist == NULL) {
    for (i=0; i<workercount; i++)
      thrpool->devlist[i] = -1; /* mark as a CPU core */
  } else {
    memcpy(thrpool->devlist, devlist, sizeof(int) * workercount);
  }

  /* initialize shared iterator */
  rt_shared_iterator_init(&thrpool->iter);

  /* initialize tile stack for error handling */
  rt_tilestack_init(&thrpool->errorstack, 64);

  /* create a run barrier with N+1 threads: N workers, 1 master */
  thrpool->workercount = workercount;
  rt_thread_run_barrier_init(&thrpool->runbar, workercount+1);

  /* allocate and initialize thread pool */
  thrpool->threads = (rt_thread_t *) malloc(sizeof(rt_thread_t) * workercount);
  thrpool->workerdata = (rt_threadpool_workerdata_t *) malloc(sizeof(rt_threadpool_workerdata_t) * workercount);
  memset(thrpool->workerdata, 0, sizeof(rt_threadpool_workerdata_t) * workercount);

  /* setup per-worker data */
  for (i=0; i<workercount; i++) {
    thrpool->workerdata[i].iter=&thrpool->iter;
    thrpool->workerdata[i].errorstack=&thrpool->errorstack;
    thrpool->workerdata[i].threadid=i;
    thrpool->workerdata[i].threadcount=workercount;
    thrpool->workerdata[i].devid=thrpool->devlist[i];
    thrpool->workerdata[i].devspeed=1.0f; /* must be reset by dev setup code */
    thrpool->workerdata[i].thrpool=thrpool;
  }

#if defined(THR)
  /* launch thread pool */
  for (i=0; i<workercount; i++) {
    rt_thread_create(&thrpool->threads[i], rt_threadpool_workerproc, &thrpool->workerdata[i]);
  }
#endif

  return thrpool;
}


int rt_threadpool_launch(rt_threadpool_t *thrpool,
                          void *fctn(void *), void *parms, int blocking) {
  if (thrpool == NULL)
    return -1;

#if defined(THR)
  /* wake sleeping threads to run fctn(parms) */
  rt_thread_run_barrier(&thrpool->runbar, fctn, parms, NULL);
  if (blocking)
    rt_thread_run_barrier(&thrpool->runbar, rt_threadpool_workersync, NULL, NULL);
#else
  thrpool->workerdata[0].parms = parms;
  (*fctn)(&thrpool->workerdata[0]);
#endif
  return 0;
}


int rt_threadpool_wait(rt_threadpool_t *thrpool) {
#if defined(THR)
  rt_thread_run_barrier(&thrpool->runbar, rt_threadpool_workersync, NULL, NULL);
#endif
  return 0;
}


int rt_threadpool_poll(rt_threadpool_t *thrpool) {
#if defined(THR)
  return rt_thread_run_barrier_poll(&thrpool->runbar);
#else
  return 1;
#endif
}


int rt_threadpool_destroy(rt_threadpool_t *thrpool) {
#if defined(THR)
  int i;
#endif

  /* wake threads and tell them to shutdown */
  rt_thread_run_barrier(&thrpool->runbar, NULL, NULL, NULL);

#if defined(THR)
  /* join the pool of worker threads */
  for (i=0; i<thrpool->workercount; i++) {
    rt_thread_join(thrpool->threads[i], NULL);
  }
#endif

  /* destroy the thread barrier */
  rt_thread_run_barrier_destroy(&thrpool->runbar);

  /* destroy the shared iterator */
  rt_shared_iterator_destroy(&thrpool->iter);

  /* destroy tile stack for error handling */
  rt_tilestack_destroy(&thrpool->errorstack);

  free(thrpool->devlist);
  free(thrpool->threads);
  free(thrpool->workerdata);
  free(thrpool);

  return 0;
}


/** return the number of worker threads currently in the pool */
int rt_threadpool_get_workercount(rt_threadpool_t *thrpool) {
  return thrpool->workercount;
}


/** worker thread can call this to get its ID and number of peers */
int rt_threadpool_worker_getid(void *voiddata, int *threadid, int *threadcount) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voiddata;
  if (threadid != NULL)
    *threadid = worker->threadid;

  if (threadcount != NULL)
    *threadcount = worker->threadcount;

  return 0;
}


/** worker thread can call this to get its CPU/GPU device ID */
int rt_threadpool_worker_getdevid(void *voiddata, int *devid) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voiddata;
  if (devid != NULL)
    *devid = worker->devid;

  return 0;
}


/**
 * worker thread calls this to set relative speed of this device
 * as determined by the SM/core count and clock rate
 * Note: this should only be called once, during the worker's
 * device initialization process
 */
int rt_threadpool_worker_setdevspeed(void *voiddata, float speed) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voiddata;
  worker->devspeed = speed;
  return 0;
}


/**
 * worker thread calls this to get relative speed of this device
 * as determined by the SM/core count and clock rate
 */
int rt_threadpool_worker_getdevspeed(void *voiddata, float *speed) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voiddata;
  if (speed != NULL)
    *speed = worker->devspeed;
  return 0;
}


/**
 * worker thread calls this to scale max tile size by worker speed
 * as determined by the SM/core count and clock rate
 */
int rt_threadpool_worker_devscaletile(void *voiddata, int *tilesize) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voiddata;
  if (tilesize != NULL) {
    int scaledtilesize;
    scaledtilesize = (int) (worker->devspeed * ((float) (*tilesize)));
    if (scaledtilesize < 1)
      scaledtilesize = 1;

    *tilesize = scaledtilesize;
  }

  return 0;
}


/** worker thread can call this to get its client data pointer */
int rt_threadpool_worker_getdata(void *voiddata, void **clientdata) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voiddata;
  if (clientdata != NULL)
    *clientdata = worker->parms;

  return 0;
}


/** Set shared iterator state to half-open interval defined by tile */
int rt_threadpool_sched_dynamic(rt_threadpool_t *thrpool, rt_tasktile_t *tile) {
  if (thrpool == NULL)
    return -1;
  return rt_shared_iterator_set(&thrpool->iter, tile);
}


/** iterate the shared iterator over the requested half-open interval */
int rt_threadpool_next_tile(void *voidparms, int reqsize,
                            rt_tasktile_t *tile) {
  int rc;
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voidparms;
  rc = rt_shared_iterator_next_tile(worker->iter, reqsize, tile);
  if (rc == RT_SCHED_DONE) {
    /* if the error stack is empty, then we're done, otherwise pop */
    /* a tile off of the error stack and retry it                  */
    if (rt_tilestack_pop(worker->errorstack, tile) != RT_TILESTACK_EMPTY)
      return RT_SCHED_CONTINUE;
  }

  return rc;
}


/**
 * worker thread calls this when a failure occurs on a tile it has
 * already taken from the scheduler
 */
int rt_threadpool_tile_failed(void *voidparms, rt_tasktile_t *tile) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voidparms;
  return rt_tilestack_push(worker->errorstack, tile);
}


/* worker thread calls this to indicate that an unrecoverable error occured */
int rt_threadpool_setfatalerror(void *voidparms) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voidparms;
  rt_shared_iterator_setfatalerror(worker->iter);
  return 0;
}


/* worker thread calls this to indicate that an unrecoverable error occured */
int rt_threadpool_getfatalerror(void *voidparms) {
  rt_threadpool_workerdata_t *worker = (rt_threadpool_workerdata_t *) voidparms;
  /* query error status for return to caller */
  return rt_shared_iterator_getfatalerror(worker->iter);
}


/* launch up to numprocs threads using shared iterator as a load balancer */
int rt_threadlaunch(int numprocs, void *clientdata, void * fctn(void *),
                    rt_tasktile_t *tile) {
  rt_shared_iterator_t iter;
  rt_threadlaunch_t *parms=NULL;
  rt_thread_t * threads=NULL;
  int i, rc;

  /* XXX have to ponder what the right thing to do is here */
#if !defined(THR)
  numprocs=1;
#endif

  /* initialize shared iterator and set the iteration and range */
  rt_shared_iterator_init(&iter);
  if (rt_shared_iterator_set(&iter, tile))
    return -1;

  /* allocate array of threads */
  threads = (rt_thread_t *) calloc(numprocs * sizeof(rt_thread_t), 1);
  if (threads == NULL)
    return -1;

  /* allocate and initialize array of thread parameters */
  parms = (rt_threadlaunch_t *) malloc(numprocs * sizeof(rt_threadlaunch_t));
  if (parms == NULL) {
    free(threads);
    return -1;
  }
  for (i=0; i<numprocs; i++) {
    parms[i].iter = &iter;
    parms[i].threadid = i;
    parms[i].threadcount = numprocs;
    parms[i].clientdata = clientdata;
  }

#if defined(THR)
  if (numprocs == 1) {
    /* XXX we special-case the single worker thread  */
    /*     scenario because this greatly reduces the */
    /*     GPU kernel launch overhead since a new    */
    /*     contexts doesn't have to be created, and  */
    /*     in the simplest case with a single-GPU we */
    /*     will just be using the same device anyway */
    /*     Ideally we shouldn't need to do this....  */
    /* single thread does all of the work */
    fctn((void *) &parms[0]);
  } else {
    /* spawn child threads to do the work */
    for (i=0; i<numprocs; i++) {
      rt_thread_create(&threads[i], fctn, &parms[i]);
    }

    /* join the threads after work is done */
    for (i=0; i<numprocs; i++) {
      rt_thread_join(threads[i], NULL);
    }
  }
#else
  /* single thread does all of the work */
  fctn((void *) &parms[0]);
#endif

  /* free threads/parms */
  free(parms);
  free(threads);

  /* query error status for return to caller */
  rc=rt_shared_iterator_getfatalerror(&iter);

  /* destroy the shared iterator */
  rt_shared_iterator_destroy(&iter);

  return rc;
}


/** worker thread can call this to get its ID and number of peers */
int rt_threadlaunch_getid(void *voidparms, int *threadid, int *threadcount) {
  rt_threadlaunch_t *worker = (rt_threadlaunch_t *) voidparms;
  if (threadid != NULL)
    *threadid = worker->threadid;

  if (threadcount != NULL)
    *threadcount = worker->threadcount;

  return 0;
}


/** worker thread can call this to get its client data pointer */
int rt_threadlaunch_getdata(void *voidparms, void **clientdata) {
  rt_threadlaunch_t *worker = (rt_threadlaunch_t *) voidparms;
  if (clientdata != NULL)
    *clientdata = worker->clientdata;

  return 0;
}


/** iterate the shared iterator over the requested half-open interval */
int rt_threadlaunch_next_tile(void *voidparms, int reqsize,
                              rt_tasktile_t *tile) {
  rt_threadlaunch_t *worker = (rt_threadlaunch_t *) voidparms;
  return rt_shared_iterator_next_tile(worker->iter, reqsize, tile);
}


/** worker thread calls this to indicate that an unrecoverable error occured */
int rt_threadlaunch_setfatalerror(void *voidparms) {
  rt_threadlaunch_t *worker = (rt_threadlaunch_t *) voidparms;
  return rt_shared_iterator_setfatalerror(worker->iter);
}


#ifdef __cplusplus
}
#endif

