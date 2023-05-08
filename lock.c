#include <stdatomic.h>

atomic_bool LOCK;

void lock()
{
    while (atomic_store_explicit(&LOCK, true, memory_order_acquire))
    {
        relax();
    }
}

void unlock()
{
    atomic_store_explicit(&LOCK, false, memory_order_release);
}
