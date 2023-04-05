/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/console.h>
#include <kern/env.h>
#include <kern/kclock.h>
#include <kern/pmap.h>
#include <kern/sched.h>
#include <kern/syscall.h>
#include <kern/trap.h>
#include <kern/traceopt.h>

/* Print a string to the system console.
 * The string is exactly 'len' characters long.
 * Destroys the environment on memory errors. */
static int
sys_cputs(const char *s, size_t len) {
    // LAB 8: Your code here

    /* Check that the user has permission to read memory [s, s+len).
    * Destroy the environment if not. */
    user_mem_assert(curenv, s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", (int)len, s);

    return 0;
}

/* Read a character from the system console without blocking.
 * Returns the character, or 0 if there is no input waiting. */
static int
sys_cgetc(void) {
    // LAB 8: Your code here

    return cons_getc();
}

/* Returns the current environment's envid. */
static envid_t
sys_getenvid(void) {
    // LAB 8: Your code here

    return curenv->env_id;
}

/* Destroy a given environment (possibly the currently running environment).
 *
 *  Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 */
static int
sys_env_destroy(envid_t envid) {
    // LAB 8: Your code here.
    int r;
	struct Env *env;

    cprintf("start\n");
	if ((r = envid2env(envid, &env, 1)) < 0)
		return r;
    cprintf("end\n");

#if 1 /* TIP: Use this snippet to log required for passing grade tests info */
    if (trace_envs) {
        cprintf(env == curenv ?
                        "[%08x] exiting gracefully\n" :
                        "[%08x] destroying %08x\n",
                curenv->env_id, env->env_id);
    }
#endif
    env_destroy(env);
    
    return 0;
}

/* Deschedule current environment and pick a different one to run. */
static void
sys_yield(void) {
    sched_yield();
    // LAB 9: Your code here
}

/* Allocate a new environment.
 * Returns envid of new environment, or < 0 on error.  Errors are:
 *  -E_NO_FREE_ENV if no free environment is available.
 *  -E_NO_MEM on memory exhaustion. */
static envid_t
sys_exofork(void) {
    /* Create the new environment with env_alloc(), from kern/env.c.
     * It should be left as env_alloc created it, except that
     * status is set to ENV_NOT_RUNNABLE, and the register set is copied
     * from the current environment -- but tweaked so sys_exofork
     * will appear to return 0. */

    // LAB 9: Your code here

    struct Env* env = NULL;
    int err = env_alloc (&env, sys_getenvid(), ENV_TYPE_USER);
    if (err)
    {
        return err;
    }
    env->env_status = ENV_NOT_RUNNABLE;
    env->env_tf = curenv->env_tf;
    env->env_pgfault_upcall = curenv->env_pgfault_upcall;
    env->env_parent_id = curenv->env_id;

    env->env_tf.tf_regs.reg_rax = 0;

    return env->env_id;
}

/* Set envid's env_status to status, which must be ENV_RUNNABLE
 * or ENV_NOT_RUNNABLE.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if status is not a valid status for an environment. */
static int
sys_env_set_status(envid_t envid, int status) {
    /* Hint: Use the 'envid2env' function from kern/env.c to translate an
     * envid to a struct Env.
     * You should set envid2env's third argument to 1, which will
     * check whether the current environment has permission to set
     * envid's status. */

    // LAB 9: Your code here
    if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
    {
        return -E_INVAL;
    }

    int r;
	struct Env *env;

	if ((r = envid2env(envid, &env, 1)) < 0)
		return r;

    env->env_status = status;
    return 0;
}

/* Set the page fault upcall for 'envid' by modifying the corresponding struct
 * Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
 * kernel will push a fault record onto the exception stack, then branch to
 * 'func'.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid. */
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func) {
    // LAB 9: Your code here:
    struct Env *e;
    if (envid2env(envid, &e, 1) < 0) {
        return -E_BAD_ENV;
    }
    e->env_pgfault_upcall = func;
    return 0;
}

static int check_perm(int perm) {
  int must_set = PTE_U | PTE_P;
  if ((perm & must_set) != must_set)
    return -E_INVAL;
  if (perm & ~PTE_U)
    return -E_INVAL;
  return 0;
}

/* Allocate a region of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 * The page's contents are set to 0.
 * If a page is already mapped at 'va', that page is unmapped as a
 * side effect.
 * 
 * This call should work with or without ALLOC_ZERO/ALLOC_ONE flags
 * (set them if they are not already set)
 * 
 * It allocates memory lazily so you need to use map_region
 * with PROT_LAZY and ALLOC_ONE/ALLOC_ZERO set.
 * 
 * Don't forget to set PROT_USER_
 * 
 * PROT_ALL is useful for validation.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned.
 *  -E_INVAL if perm is inappropriate (see above).
 *  -E_NO_MEM if there's no memory to allocate the new page,
 *      or to allocate any necessary page tables. */
static int
sys_alloc_region(envid_t envid, uintptr_t addr, size_t size, int perm) {
    // LAB 9: Your code here:
    int r;
	struct Env *env;
	if ((r = envid2env(envid, &env, 1)) < 0)
		return r;

    //r = check_perm(perm);
    //if (r)
    //    return r;

    if (addr >= MAX_USER_ADDRESS || PAGE_OFFSET(addr))
        return -E_INVAL;

    r = map_region(&env->address_space, addr, NULL, 0, size, PROT_LAZY | ALLOC_ZERO | PROT_USER_ | perm);
    if (r < 0)
    {
        return r;
    }
    return 0;
}

/* Map the region of memory at 'srcva' in srcenvid's address space
 * at 'dstva' in dstenvid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_alloc_region, except
 * that it also does not supprt ALLOC_ONE/ALLOC_ONE flags.
 * 
 * You only need to check alignment of addresses, perm flags and
 * that addresses are a part of user space. Everything else is
 * already checked inside map_region().
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
 *      or the caller doesn't have permission to change one of them.
 *  -E_INVAL if srcva >= MAX_USER_ADDRESS or srcva is not page-aligned,
 *      or dstva >= MAX_USER_ADDRESS or dstva is not page-aligned.
 *  -E_INVAL if srcva is not mapped in srcenvid's address space.
 *  -E_INVAL if perm is inappropriate (see sys_page_alloc).
 *  -E_INVAL if (perm & PROT_W), but srcva is read-only in srcenvid's
 *      address space.
 *  -E_NO_MEM if there's no memory to allocate any necessary page tables. */

static int
sys_map_region(envid_t srcenvid, uintptr_t srcva,
               envid_t dstenvid, uintptr_t dstva, size_t size, int perm) {
    // LAB 9: Your code here

    int r;
	struct Env *srcenv;
	if ((r = envid2env(srcenvid, &srcenv, 1)) < 0)
		return r;
	struct Env *dstenv;
	if ((r = envid2env(dstenvid, &dstenv, 1)) < 0)
		return r;
    /*if (perm & ~PROT_ALL)
        return -E_INVAL;*/
    //r = check_perm(perm);
    //if (r)
    //    return r;

    if (dstva >= MAX_USER_ADDRESS || PAGE_OFFSET(dstva) || srcva >= MAX_USER_ADDRESS || PAGE_OFFSET(srcva))
        return -E_INVAL;

    /*if (user_mem_check(srcenv, ))
    struct Page* page = page_lookup_virtual(srcenv->address_space.root, (uintptr_t)srcva, 0, 0);
    if (!page)
    {
        return -E_INVAL;
    }
    if ((page->state & PROT_R & ~PROT_W) && (perm & PROT_W))
    {
        return -E_INVAL;
    }*/
    
    r = map_region(&dstenv->address_space, dstva, &srcenv->address_space, srcva, size, PROT_LAZY | perm | PROT_USER_ );
    if (r < 0)
    {
        return r;
    }
    return 0;
}

/* Unmap the region of memory at 'va' in the address space of 'envid'.
 * If no page is mapped, the function silently succeeds.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned. */
static int
sys_unmap_region(envid_t envid, uintptr_t va, size_t size) {
    /* Hint: This function is a wrapper around unmap_region(). */
    int r;
	struct Env *env;
	if ((r = envid2env(envid, &env, 1)) < 0)
		return r;
	if (va >= MAX_USER_ADDRESS || PAGE_OFFSET(va))
        return -E_INVAL;
    /*
    struct Page* page = page_lookup_virtual(env->address_space.root, (uintptr_t)va, 0, 0);
    if (!page)
    {
        return 0;
    }*/
    unmap_region(&env->address_space, va, size);
    // LAB 9: Your code here

    return 0;
}

/* Try to send 'value' to the target env 'envid'.
 * If srcva < MAX_USER_ADDRESS, then also send region currently mapped at 'srcva',
 * so receiver also gets mapping.
 *
 * The send fails with a return value of -E_IPC_NOT_RECV if the
 * target is not blocked, waiting for an IPC.
 *
 * The send also can fail for the other reasons listed below.
 *
 * Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends;
 *    env_ipc_maxsz is set to min of size and it's current vlaue;
 *    env_ipc_from is set to the sending envid;
 *    env_ipc_value is set to the 'value' parameter;
 *    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
 * The target environment is marked runnable again, returning 0
 * from the paused sys_ipc_recv system call.  (Hint: does the
 * sys_ipc_recv function ever actually return?)
 *
 * If the sender wants to send a page but the receiver isn't asking for one,
 * then no page mapping is transferred, but no error occurs.
 * Send region size is the minimum of sized specified in sys_ipc_try_send() and sys_ipc_recv()
 * 
 * The ipc only happens when no errors occur.
 *
 * Returns 0 on success, < 0 on error.
 * Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist.
 *      (No need to check permissions.)
 *  -E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
 *      or another environment managed to send first.
 *  -E_INVAL if srcva < MAX_USER_ADDRESS but srcva is not page-aligned.
 *  -E_INVAL if srcva < MAX_USER_ADDRESS and perm is inappropriate
 *      (see sys_page_alloc).
 *  -E_INVAL if srcva < MAX_USER_ADDRESS but srcva is not mapped in the caller's
 *      address space.
 *  -E_INVAL if (perm & PTE_W), but srcva is read-only in the
 *      current environment's address space.
 *  -E_NO_MEM if there's not enough memory to map srcva in envid's
 *      address space. */
static int
sys_ipc_try_send(envid_t envid, uint32_t value, uintptr_t srcva, size_t size, int perm) {
    // LAB 9: Your code here

    int r;
	struct Env *env;

	if ((r = envid2env(envid, &env, 0)) < 0)
		return r;

    if (!env->env_ipc_recving)
        return -E_IPC_NOT_RECV;

    uintptr_t dst = env->env_ipc_dstva;

    if (srcva < MAX_USER_ADDRESS)
    {
        if (PAGE_OFFSET(srcva) || perm & ~PROT_ALL)
        {
            return -E_INVAL;
        }

        if ((r = map_region(&env->address_space, dst, &curenv->address_space, srcva, MIN(env->env_ipc_maxsz, size), perm)))
        {
            env->env_ipc_perm = 0;
            return r;
        }
        env->env_ipc_perm = perm;
    }
    else
    {
        env->env_ipc_perm = 0;
    }

    env->env_ipc_recving = 0;
    env->env_ipc_maxsz = MIN(env->env_ipc_maxsz, size);
    env->env_ipc_from = curenv->env_id;
    env->env_ipc_value = value;
    env->env_status = ENV_RUNNABLE;
    

    return 0;
}

/* Block until a value is ready.  Record that you want to receive
 * using the env_ipc_recving, env_ipc_maxsz and env_ipc_dstva fields of struct Env,
 * mark yourself not runnable, and then give up the CPU.
 *
 * If 'dstva' is < MAX_USER_ADDRESS, then you are willing to receive a page of data.
 * 'dstva' is the virtual address at which the sent page should be mapped.
 *
 * This function only returns on error, but the system call will eventually
 * return 0 on success.
 * Return < 0 on error.  Errors are:
 *  -E_INVAL if dstva < MAX_USER_ADDRESS but dstva is not page-aligned;
 *  -E_INVAL if dstva is valid and maxsize is 0,
 *  -E_INVAL if maxsize is not page aligned.
 */
static int
sys_ipc_recv(uintptr_t dstva, uintptr_t maxsize) {
    // LAB 9: Your code here
    if ((uintptr_t)dstva < MAX_USER_ADDRESS)
    {
        if (PAGE_OFFSET(dstva))
        {
            return -E_INVAL;
        }
        if (PAGE_OFFSET(maxsize) || maxsize == 0)
        {
            return -E_INVAL;
        }
    }
    curenv->env_ipc_dstva = dstva;
    curenv->env_ipc_recving = 1;
    curenv->env_ipc_maxsz = maxsize;

    curenv->env_status = ENV_NOT_RUNNABLE;
    curenv->env_tf.tf_regs.reg_rax = 0; //equal return 0
    sys_yield();

    return 0;
}

/*
 * This function return the difference between maximal
 * number of references of regions [addr, addr + size] and [addr2,addr2+size2]
 * if addr2 is less than MAX_USER_ADDRESS, or just
 * maximal number of references to [addr, addr + size]
 * 
 * Use region_maxref() here.
 */
static int
sys_region_refs(uintptr_t addr, size_t size, uintptr_t addr2, uintptr_t size2) {
    // LAB 10: Your code here
    return 0;
}

/* Dispatches to the correct kernel function, passing the arguments. */
uintptr_t
syscall(uintptr_t syscallno, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
    /* Call the function corresponding to the 'syscallno' parameter.
     * Return any appropriate return value. */

    // LAB 8: Your code here
    if (syscallno == SYS_cputs) 
    {
        sys_cputs((const char *) a1, (size_t) a2);
        return 0;
    } 
    else if (syscallno == SYS_cgetc) 
    {
        return sys_cgetc();
    } 
    else if (syscallno == SYS_getenvid) 
    {
        return sys_getenvid();
    } 
    else if (syscallno == SYS_env_destroy) 
    {
        return sys_env_destroy((envid_t) a1);
    }
    else if (syscallno == SYS_exofork) 
    {
        return sys_exofork();
    }
    else if(syscallno == SYS_env_set_status)
    {
        return sys_env_set_status(a1, a2);
    }
    else if (syscallno == SYS_alloc_region)
    {
        return sys_alloc_region(a1, a2, a3, a4);
    }
    else if (syscallno == SYS_map_region)
    {
        return sys_map_region(a1, a2, a3, a4, a5, a6);
    }
    else if (syscallno == SYS_yield) 
    {
        sys_yield();
        return 0;
    }
    else if (syscallno == SYS_env_set_pgfault_upcall) 
    {
        return sys_env_set_pgfault_upcall((envid_t) a1, (void *) a2);
    }
    else if (syscallno == SYS_ipc_try_send) 
    {
        return sys_ipc_try_send((envid_t) a1, (uint32_t) a2, a3, a4, a5);
    } 
    else if (syscallno == SYS_ipc_recv) 
    {
        return sys_ipc_recv(a1, a2);
    }
    // LAB 9: Your code here

    return -E_NO_SYS;
}