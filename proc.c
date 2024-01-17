#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "skiplist.h"
#include "bfs.h"

// Skiplist Start
#define NULL 0
#define SKIPLIST_LEVELS 4

int seed = 1234567; // To Do: move to bfs.h
void insert_node(struct skiplist * skiplist, int pid, int virtual_deadline, struct proc * p);

struct skiplist * init_skiplist() {
  struct skiplist * skiplist = (struct skiplist *) kalloc();
  skiplist->levels = SKIPLIST_LEVELS;
  skiplist->headers = (struct node **) kalloc(); // allocate N header;

  // Initialize header nodes
  for (int i = 0; i < SKIPLIST_LEVELS; i++) {
    skiplist->headers[i] = (struct node *) kalloc();
    skiplist->headers[i]->pid = -1;
    skiplist->headers[i]->virtual_deadline = -1;
    skiplist->headers[i]->next = NULL;
    skiplist->headers[i]->prev = NULL;
    if (i > 0) {
      skiplist->headers[i]->forward = skiplist->headers[i-1];
    }
  }

  return skiplist;
}



struct node * insert_to_level(int pid, int virtual_deadline, struct node * prev_node, struct node * forward) {
  struct node * new_node = (struct node *) kalloc();
  new_node->pid = pid;
  new_node->virtual_deadline = virtual_deadline;

  struct node * next_node = prev_node->next;

  prev_node->next = new_node;

  new_node->prev = prev_node;
  new_node->next = next_node;
  new_node->forward = forward;

  if (next_node != NULL) {
    next_node->prev = new_node;
  }

  return new_node;
}

unsigned int random(int max) {
  seed ^= seed << 17;
  seed ^= seed >> 7;
  seed ^= seed << 5;
  return seed % max;
}

int randomize_max_level(int skiplist_max_level) {
  int insertion_max_level = 0;
  while (insertion_max_level <= skiplist_max_level) {
    int x = random(4);
    if (x == 0) {
      insertion_max_level++;
    } else {
      break;
    }
  }
  return insertion_max_level;
}

void print_skiplist(struct skiplist * skiplist) {
  int current_level = skiplist->levels - 1;

  while (current_level >= 0) {
    struct node * current_node = skiplist->headers[current_level];
    cprintf("level %d: ", current_level);
    while (current_node != NULL) {
      cprintf("%d(pid:%d) -> ", current_node->virtual_deadline, current_node->pid);
      current_node = current_node->next;
    }
    cprintf("\n");
    current_level--;
  }

}

void insert_node(struct skiplist * skiplist, int pid, int virtual_deadline, struct proc * p) {

  p->max_level = randomize_max_level(skiplist->levels - 1); // TO DO: randomize

  struct node * prev_nodes[p->max_level + 1];

  int current_level = skiplist->levels - 1;

  struct node * current_node = skiplist->headers[current_level];

  // Find previous nodes to insert per level
  while (current_level >= 0) {
    // Find previous node in a level
    while (current_node->next != NULL && virtual_deadline >= current_node->next->virtual_deadline) {
      current_node = current_node->next;
    }

    // Store previous node if within max level
    if (p->max_level >= current_level) {
      prev_nodes[current_level] = current_node;
    }

    // Set current node to next level
    current_node = current_node->forward;

    current_level--;
  }

  // Insert new nodes given previous nodees per level
  current_level = 0;
  struct node * forward = NULL;
  while (current_level <= p->max_level) {

    forward = insert_to_level(pid, virtual_deadline, prev_nodes[current_level], forward);

    current_level++;
  }
  cprintf("inserted|[%d]%d\n", pid, p->max_level);
  print_skiplist(skiplist);
}

void delete_from_levels(struct node * node) {
  struct node * current_node = node;

  while (current_node != NULL) {
    struct node * next_to_delete = current_node->forward;
    struct node * next = current_node->next;
    struct node * prev = current_node->prev;

    prev->next = next;
    if (next != NULL) {
      next->prev = prev;
    }

    kfree((char*)current_node);

    current_node = next_to_delete;
  }
}

void delete_node(struct skiplist * skiplist, int pid, int virtual_deadline) {


  // Start at header of top level
  int current_level = skiplist->levels - 1;
  struct node * current_node = skiplist->headers[skiplist->levels - 1];

  while (current_level >= 0) {
    while (current_node->virtual_deadline < virtual_deadline && current_node->pid != pid) { // TO DO: handle null case
      if (current_node->virtual_deadline > virtual_deadline  || current_node->next == NULL) {
        break;
      }
      current_node = current_node->next;
    }

    if (current_node->pid == pid && current_node->virtual_deadline == virtual_deadline) {
      // Case where node to delete is already found
      break;
    } else {
      while (current_node->virtual_deadline >= virtual_deadline) {
        current_node = current_node->prev;
      }
      current_node = current_node->forward;
      current_level--;
    }
  }
  if (current_level >= 0) {
    delete_from_levels(current_node);
    cprintf("removed|[%d]%d\n", pid, current_level);
  }
  cprintf("delete virtual deadline: %d process: %d\n", virtual_deadline, pid);
  print_skiplist(skiplist);
  
}

int get_minimum(struct skiplist * skiplist) {
  if (skiplist->headers[0]->next != NULL) {
    return skiplist->headers[0]->next->pid;
  }
  return -69;

}


// Skiplist End


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct skiplist * skiplist;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);



int compute_virtual_deadline(int nice_value) {
  int priority_ratio = nice_value - BFS_NICE_FIRST_LEVEL + 1;
  return ticks + (BFS_DEFAULT_QUANTUM * priority_ratio);
}

int schedlog_active = 0;
int schedlog_lasttick = 0;

void schedlog(int n) {
  schedlog_active = 1;
  schedlog_lasttick = ticks + n;
}


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");

  // Initialize Skip List
  skiplist = init_skiplist();
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  //cprintf("--allocproc called\n");
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  // // Add to skiplist
  // struct Node * node = (struct Node *) malloc(sizeof(struct Node));
  // node->pid = p->pid;
  // insert_node(head, p->pid, p->nice_value);


  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;



  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);

  // Skip List
  p->nice_value = 0;
  p->virtual_deadline = compute_virtual_deadline(0);
  insert_node(skiplist, p->pid, p->virtual_deadline, p);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.


int
nicefork(int nice_value)
{
  //cprintf("--nicefork called\n");
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;


  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  // Skip List
  np->nice_value = nice_value;
  np->virtual_deadline = compute_virtual_deadline(nice_value);
  insert_node(skiplist, np->pid, np->virtual_deadline, np);

  return pid;
}

int fork(void) {
  //cprintf("--fork called\n");
  return nicefork(0);
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;


  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    int next_process_pid = get_minimum(skiplist);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if (p->pid != next_process_pid) {
        continue;
      }


      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->ticks_left = BFS_DEFAULT_QUANTUM;

      // Schedlog
      if (schedlog_active) {
        if (ticks > schedlog_lasttick) {
          schedlog_active = 0;
        } else {
          cprintf("%d|", ticks);
          struct proc *pp;
          int highest_idx = -1;
          for (int k = 0; k < NPROC; k++) {
            pp = &ptable.proc[k];
            if (pp->state != UNUSED) {
              highest_idx = k;
            }
          }
          for (int k = 0; k <= highest_idx; k++) {
            pp = &ptable.proc[k];
            if (pp->state == UNUSED) cprintf("[-]---:0:-(-)(-)(-)");
            else cprintf("[%d]%s:%d:%d(%d)(%d)(%d)", pp->pid, pp->name, pp->state, pp->nice_value, pp->max_level, pp->virtual_deadline, pp->ticks_left);
            if (k <= highest_idx - 1) {
              cprintf(",");
            }
          }
          cprintf("\n");
        }
      }

      //cprintf("Context switching from scheduler to %s [scheduler]\n", p->name);
      swtch(&(c->scheduler), p->context);
      //cprintf("Context switch to scheduler complete [scheduler]\n");
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      
      if (p->state == RUNNABLE && p->ticks_left == 0) {
        delete_node(skiplist, p->pid, p->virtual_deadline);
        p->virtual_deadline = compute_virtual_deadline(p->nice_value);
        
        cprintf("Reinserting since runnuble| pid: %d, ticks_left: %d\n", p->pid, p->ticks_left);
        insert_node(skiplist, p->pid, p->virtual_deadline, p);
      } else if (p->state == SLEEPING) {
        delete_node(skiplist, p->pid, p->virtual_deadline);
      } else if (p->state == ZOMBIE) {
        delete_node(skiplist, p->pid, p->virtual_deadline);
      }


      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  //cprintf("Context switching from %s to scheduler [sched]\n", p->name);
  swtch(&p->context, mycpu()->scheduler);
  //cprintf("Context switch to %s complete [sched]\n", p->name);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{

  struct proc *p = myproc();
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
  cprintf("will sleep pid %d\n", p->pid);

  sched();

  // Tidy up.
  p->chan = 0;
  
  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      cprintf("\n Process woken up: %d\n\n", p->pid);
      insert_node(skiplist, p->pid, p->virtual_deadline, p);
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
        insert_node(skiplist, p->pid, p->virtual_deadline, p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
