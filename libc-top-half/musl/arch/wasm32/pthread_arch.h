#ifdef _REENTRANT
extern _Thread_local struct __pthread __wasilibc_pthread_self;
#else
extern struct __pthread __wasilibc_pthread_self;
#endif

static inline uintptr_t __get_tp() {
  return (uintptr_t)&__wasilibc_pthread_self;
}
