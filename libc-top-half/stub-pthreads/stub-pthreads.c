#include "pthread_impl.h"

static void dummy_0()
{
}
weak_alias(dummy_0, __acquire_ptc);
weak_alias(dummy_0, __release_ptc);

/*
	Note on PTHREAD_PROCESS_SHARED:
	because WASM doesn't have virtual memory nor subprocesses,
	there isn't any way for the same synchronization object to have multiple mappings.
	Thus we can completely ignore it here.
*/

/* Barriers */

int pthread_barrier_init(pthread_barrier_t *restrict b, const pthread_barrierattr_t *restrict a, unsigned count)
{
	if (count-1 > INT_MAX-1) return EINVAL;
	*b = (pthread_barrier_t){ ._b_limit = count-1 };
	return 0;
}
int pthread_barrier_destroy(pthread_barrier_t *b)
{
	return 0;
}
int pthread_barrier_wait(pthread_barrier_t *b)
{
	if (!b->_b_limit) return PTHREAD_BARRIER_SERIAL_THREAD;
	__builtin_trap();
}

/* Mutex */

/*
	Musl mutex
	
	_m_type:
	b[7]	- process shared
	b[3]	- priority inherit
	b[2] 	- robust
	b[1:0] 	- type
		0 - normal
		1 - recursive
		2 - errorcheck

	_m_lock:
	b[30]	- owner dead, if robust
	b[29:0]	- tid, if not normal
	b[4]	- locked?, if normal
*/

int pthread_mutex_consistent(pthread_mutex_t *m)
{
	/* cannot be a robust mutex */
	return EINVAL;
}
int pthread_mutex_lock(pthread_mutex_t *m)
{
	if (m->_m_type&3 != PTHREAD_MUTEX_RECURSIVE) {
		if (m->_m_count) return EDEADLK;
		m->_m_count = 1;
	} else {
		if ((unsigned)m->_m_count >= INT_MAX) return EAGAIN;
		m->_m_count++;
	}
	return 0;
}
int pthread_mutex_trylock(pthread_mutex_t *m)
{
	if (m->_m_type&3 != PTHREAD_MUTEX_RECURSIVE) {
		if (m->_m_count) return EBUSY;
		m->_m_count = 1;
	} else {
		if ((unsigned)m->_m_count >= INT_MAX) return EAGAIN;
		m->_m_count++;
	}
	return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *m)
{
	if (!m->_m_count) return EPERM;
	m->_m_count--;
	return 0;
}
int pthread_mutex_timedlock(pthread_mutex_t *restrict m, const struct timespec *restrict at)
{
	/* "The pthread_mutex_timedlock() function may fail if: A deadlock condition was detected." */
	/* This means that we don't have to wait and then return timeout, we can just detect deadlock. */
	return pthread_mutex_lock(m);
}

/* Condvar */

int pthread_cond_init(pthread_cond_t *restrict c, const pthread_condattr_t *restrict a)
{
	return 0;
}
int pthread_cond_destroy(pthread_cond_t *c)
{
	return 0;
}
int pthread_cond_broadcast(pthread_cond_t *c)
{
	return 0;
}
int pthread_cond_signal(pthread_cond_t *c)
{
	return 0;
}
int pthread_cond_wait(pthread_cond_t *restrict c, pthread_mutex_t *restrict m)
{
	/* Because there is no other thread that can signal us, this is a deadlock immediately.
	The other possible choice is to return immediately (spurious wakeup), but that is likely to
	just result in the program spinning forever on a condition that cannot become true. */
	__builtin_trap();
}
int pthread_cond_timedwait(pthread_cond_t *restrict c, pthread_mutex_t *restrict m, const struct timespec *restrict ts)
{
	__builtin_trap();
}

/* RWLock */

/* Musl uses bit31 to mark "has waiters", bit[30:0] all 1s to indicate writer */

/* These functions have the __ prefix just to stub out thread-specific data */

int __pthread_rwlock_rdlock(pthread_rwlock_t *rw)
{
	if (rw->_rw_lock == 0x7fffffff) return EDEADLK;
	if (rw->_rw_lock == 0x7ffffffe) return EAGAIN;
	rw->_rw_lock++;
	return 0;
}
weak_alias(__pthread_rwlock_rdlock, pthread_rwlock_rdlock);
int __pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rw, const struct timespec *restrict at)
{
	return pthread_rwlock_rdlock(rw);
}
weak_alias(__pthread_rwlock_timedrdlock, pthread_rwlock_timedrdlock);
int __pthread_rwlock_tryrdlock(pthread_rwlock_t *rw)
{
	if (rw->_rw_lock == 0x7fffffff) return EBUSY;
	if (rw->_rw_lock == 0x7ffffffe) return EAGAIN;
	rw->_rw_lock++;
	return 0;
}
weak_alias(__pthread_rwlock_tryrdlock, pthread_rwlock_tryrdlock);
int __pthread_rwlock_wrlock(pthread_rwlock_t *rw)
{
	if (rw->_rw_lock) return EDEADLK;
	rw->_rw_lock = 0x7fffffff;
	return 0;
}
weak_alias(__pthread_rwlock_wrlock, pthread_rwlock_wrlock);
int __pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rw, const struct timespec *restrict at)
{
	return pthread_rwlock_wrlock(rw);
}
weak_alias(__pthread_rwlock_timedwrlock, pthread_rwlock_timedwrlock);
int __pthread_rwlock_trywrlock(pthread_rwlock_t *rw)
{
	if (rw->_rw_lock) return EBUSY;
	rw->_rw_lock = 0x7fffffff;
	return 0;
}
weak_alias(__pthread_rwlock_trywrlock, pthread_rwlock_trywrlock);
int __pthread_rwlock_unlock(pthread_rwlock_t *rw)
{
	if (rw->_rw_lock == 0x7fffffff)
		rw->_rw_lock = 0;
	else
		rw->_rw_lock--;
	return 0;
}
weak_alias(__pthread_rwlock_unlock, pthread_rwlock_unlock);

/* Spinlocks */

/* The only reason why we have to do anything is trylock */

int pthread_spin_lock(pthread_spinlock_t *s)
{
	if (*s) return EDEADLK;
	*s = 1;
	return 0;
}
int pthread_spin_trylock(pthread_spinlock_t *s)
{
	if (*s) return EBUSY;
	*s = 1;
	return 0;
}
int pthread_spin_unlock(pthread_spinlock_t *s)
{
	*s = 0;
	return 0;
}

/* Actual thread-related stuff */

int pthread_create(pthread_t *restrict res, const pthread_attr_t *restrict attrp, void *(*entry)(void *), void *restrict arg)
{
	/*
		"The system lacked the necessary resources to create another thread,
	    or the system-imposed limit on the total number of threads in a process
		{PTHREAD_THREADS_MAX} would be exceeded."
	*/
	return EAGAIN;
}
int pthread_detach(pthread_t t)
{
	/*
		"The behavior is undefined if the value specified by the thread argument
		to pthread_detach() does not refer to a joinable thread."
	*/
	return 0;
}
int pthread_join(pthread_t t, void **res)
{
	/*
		"The behavior is undefined if the value specified by the thread argument
		to pthread_join() does not refer to a joinable thread.

		The behavior is undefined if the value specified by the thread argument
		to pthread_join() refers to the calling thread."
	*/
	return 0;
}
int pthread_tryjoin_np(pthread_t t, void **res)
{
	return 0;
}
int pthread_timedjoin_np(pthread_t t, void **res, const struct timespec *at)
{
	return 0;
}
