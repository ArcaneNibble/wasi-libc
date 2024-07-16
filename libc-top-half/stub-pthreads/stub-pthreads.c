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
	return pthread_mutex_lock(m);
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
