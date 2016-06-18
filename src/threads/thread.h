#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <threads/synch.h>
/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/*fixed point Arithemtic*/
typedef int fixedPoint;
#define fpExp (1 << 14)
#define fpConvert(A) ((fixedPoint)(A * fpExp))
#define fpAdd(A,B) (A + B)
#define fpSub(A,B) (A - B)
#define fpMult(A,B) (A * B)
#define fpDiv(A,B) (A / B)
#define fpGetInt(A) (A / fpExp)
#define fpRoundOff(A) (A >= 0 ? (A + (fpExp) / 2) / (fpExp) : (A - (fpExp) / 2) / (fpExp))
/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int self_priority;                   /* Rachna : The original priority of the thread */
    int priority;                       /* Rachna : Priority also takes into account priority donation now*/
    struct list_elem allelem;           /* List element for all threads list. */
    int64_t ticks_to_sleep;
    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */
    struct list_elem elem2; 
    struct list priority_donation_list; /* Rachna : List of all the donations given to this thread in sorted order ( high to low) */
    struct list_elem donating_thread;  /* Rachna : An element which recognizes which thread donates to a current thread */
    struct lock *lock_waited_upon;  /* Rachna : Refers to the lock the thread is waiting for acquiring, can be NULL */
    int nice;
    fixedPoint recent_cpu;
#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
  
    struct task_struct *info;
    
    struct list open_files;
    int fd;

//    struct list children;
//    struct lock child_changed_lock;
//    struct condition child_changed;
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

  struct task_struct
  {
    struct semaphore parent_block;

    tid_t tid;

    struct list children;
    int exit_status;         
    bool thread_status;     /*true if thread is alive */
    bool isParentAlive;
    bool canParentWait;

    bool loadSuccessStatus;
    struct semaphore fileLoad;

    struct list_elem elem;     /* adding to parent threads child list*/
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);


/* Rachna : Prototypes added for Priority Scheduler */
void thread_yield_to_max_priority(void);
bool thread_priority_comparison(const struct list_elem *a,const struct list_elem *b, void * aux UNUSED);
void thread_calculate_priority (struct thread *t);
void thread_donate_priority (struct thread *t);
void thread_recall_donation (struct thread *t);

/* Fuctions for mlfqs start here*/
int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);
void thread_inc_recent_cpu(void);
void thread_calc_recent_cpu(struct thread *);
void thread_update_priority(struct thread *);
void thread_mlfqs_calc(void);

#endif /* threads/thread.h */
