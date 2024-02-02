# Project README: Skip List-Based Process Scheduler

## Overview

This project is an operating system kernel that incorporates a skip list-based process scheduler. The primary goal is to efficiently manage processes and their execution order within the operating system. The skip list data structure is employed to optimize process insertion, deletion, and search operations.

## Key Features

- **Skip List Implementation:** The skip list is utilized to maintain an ordered structure of processes based on their virtual deadlines.

- **Priority-Based Scheduling:** Processes are scheduled for execution based on their virtual deadlines, determined by their nice values and the current system ticks.

- **Dynamic Level Assignment:** The skip list dynamically assigns levels to newly inserted nodes, ensuring a balanced structure.

- **Process Creation and Termination:** The project includes functionalities for creating and terminating processes, with appropriate handling of process states and resources.

- **Scheduler Logging:** The scheduler logs information about process states and scheduling events, aiding in debugging and analysis.

## In-Depth Explanation

### Skip List Initialization

The `init_skiplist` function initializes a skip list data structure. It allocates memory for the skip list and its header nodes, setting up the necessary pointers for each level.

```c
struct skiplist * init_skiplist() {
  // Implementation details...
}
```

### Process Insertion into Skip List

The `insert_node` function is responsible for inserting a new process node into the skip list. It uses a dynamic level assignment approach to determine the appropriate level for the new node based on its `max_level` attribute.

```c
void insert_node(struct skiplist * skiplist, struct proc * p) {

  p->max_level = randomize_max_level(skiplist->levels - 1); // TO DO: randomize

  struct node * prev_nodes[p->max_level + 1];

  int current_level = skiplist->levels - 1;

  struct node * current_node = skiplist->headers[current_level];

  // Find previous nodes to insert per level
  while (current_level >= 0) {
    // Find previous node in a level
    while (current_node->next != NULL && p->virtual_deadline >= current_node->next->virtual_deadline) {
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

    forward = insert_to_level(p->pid, p->virtual_deadline, prev_nodes[current_level], forward);

    current_level++;
  }
  cprintf("inserted|[%d]%d\n", p->pid, p->max_level);
}
```

### Virtual Deadline Calculation

The `compute_virtual_deadline` function calculates the virtual deadline of a process based on its nice value and the default quantum. This deadline is used for determining the order of process execution.

```c
int compute_virtual_deadline(int nice_value) {
  int priority_ratio = nice_value - BFS_NICE_FIRST_LEVEL + 1;
  return ticks + (BFS_DEFAULT_QUANTUM * priority_ratio);
}
```

### Scheduler Functionality

The `scheduler` function represents the core scheduler logic. It selects the next process to run based on its virtual deadline, updates its state, and performs context switching.

```c
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
      delete_node(skiplist, p);

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
      
      if (p->state == RUNNABLE) {
        if (p->ticks_left == 0) {
          p->virtual_deadline = compute_virtual_deadline(p->nice_value);
        }
        insert_node(skiplist, p);
      } 


      c->proc = 0;
    }
    release(&ptable.lock);

  }
}
```

### Process Creation (Nice Fork)

The `nicefork` function is an extension of the traditional `fork` operation, allowing the specification of a nice value for the newly created process. It computes the virtual deadline for the process and inserts it into the skip list.

```c
int nicefork(int nice_value) {
  // Implementation details...
}
```

## Getting Started

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/Qubits-01/bf-os-scheduler/tree/main
   ```

2. **Build and Run:**
   ```bash
   cd bf-os-scheduler
   make
   make qemu-nox
   ```

3. **Explore and Contribute:**
   Explore the codebase, run the operating system in a virtual environment, and contribute to the project by opening issues or submitting pull requests.

## Contributions

Contributions are welcome! If you find a bug, have an enhancement idea, or want to contribute in any way, feel free to open an issue or submit a pull request.

## License

This project is licensed under the [MIT License](LICENSE). Feel free to use, modify, and distribute the code as per the license terms.

---

[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/8HsS6-oF)
xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern x86-based multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

xv6 borrows code from the following sources:
    JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
    Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
    FreeBSD (ioapic.c)
    NetBSD (console.c)

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by Silas
Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, eyalz800,
Nelson Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter
Froehlich, Yakir Goaron,Shivam Handa, Bryan Henry, Jim Huang, Alexander
Kapshuk, Anders Kaseorg, kehao95, Wolfgang Keller, Eddie Kohler, Austin
Liew, Imbar Marinescu, Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi
Merimovich, Mark Morrissey, mtasm, Joel Nider, Greg Price, Ayan Shafqat,
Eldar Sehayek, Yongming Shen, Cam Tenny, tyfkda, Rafael Ubal, Warren
Toomey, Stephen Tu, Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas
Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy Zheng, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2018 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

We switched our focus to xv6 on RISC-V; see the mit-pdos/xv6-riscv.git
repository on github.com.

BUILDING AND RUNNING XV6

To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
"make". On non-x86 or non-ELF machines (like OS X, even on x86), you
will need to install a cross-compiler gcc suite capable of producing
x86 ELF binaries (see https://pdos.csail.mit.edu/6.828/).
Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
simulator and run "make qemu".
