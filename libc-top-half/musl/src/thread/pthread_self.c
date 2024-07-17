#include "pthread_impl.h"
#include <threads.h>

#if !defined(__wasilibc_unmodified_upstream) && defined(__wasm__)
#ifdef _REENTRANT
_Thread_local struct pthread __wasilibc_pthread_self;
#else
struct pthread __wasilibc_pthread_self;
#endif
#endif

static pthread_t __pthread_self_internal()
{
	return __pthread_self();
}

weak_alias(__pthread_self_internal, pthread_self);
weak_alias(__pthread_self_internal, thrd_current);
