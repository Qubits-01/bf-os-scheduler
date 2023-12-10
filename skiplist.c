
#include "bfs.h"
#include "defs.h"
#include "skiplist.h"

#define NULL 0
#define SKIPLIST_LEVELS 4

int seed = 1234567;

struct skiplist * init_skiplist() {
  struct skiplist * skiplist = (struct skiplist *) malloc(sizeof(struct skiplist));
  skiplist->levels = SKIPLIST_LEVELS;
  skiplist->headers = (struct node **) malloc(sizeof(struct node *) * (SKIPLIST_LEVELS)); // allocate N header;

  // Initialize header nodes
  for (int i = 0; i < SKIPLIST_LEVELS; i++) {
    skiplist->headers[i] = (struct node *) malloc(sizeof(struct node));
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
  struct node * new_node = (struct node *) malloc(sizeof(struct node));
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

void insert_node(struct skiplist * skiplist, int pid, int virtual_deadline) {

  int insertion_max_level = randomize_max_level(skiplist->levels - 1); // TO DO: randomize

  struct node * prev_nodes[insertion_max_level + 1];

  int current_level = skiplist->levels - 1;

  struct node * current_node = skiplist->headers[current_level];
  
  // Find previous nodes to insert per level
  while (current_level >= 0) {
    // Find previous node in a level
    while (!(current_node->next == NULL || virtual_deadline <= current_node->next->virtual_deadline)) { 
      current_node = current_node->next;
    }
    
    // Store previous node if within max level
    if (insertion_max_level >= current_level) {
      prev_nodes[current_level] = current_node;
    }

    // Set current node to next level
    current_node = current_node->forward;

    current_level--;
  }
  
  // Insert new nodes given previous nodees per level
  current_level = 0;
  struct node * forward = NULL;
  while (current_level <= insertion_max_level) {
    
    forward = insert_to_level(pid, virtual_deadline, prev_nodes[current_level], forward);

    current_level++;
  }
  cprintf("inserted|[%d]%d\n", pid, insertion_max_level);
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

    free(current_node);
    
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
      while (current_node->virtual_deadline > virtual_deadline) {
        current_node = current_node->prev;
      }
      current_node = current_node->forward;
      current_level--;
    }
  }

  delete_from_levels(current_node);
  cprintf("removed|[%d]%d\n", pid, current_level);
}

int get_minimum(struct skiplist * skiplist) {
  return skiplist->headers[0]->next->pid;
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

// int main() {
//   // Initialize Skiplist
//   struct skiplist * skiplist = init_skiplist();

//   print_skiplist(skiplist);
//   printf("\n");

//   // Insert Nodes
//   insert_node(skiplist, 3, 1000);
//   print_skiplist(skiplist);
//   printf("\n");

//   insert_node(skiplist, 4, 1002);
//   print_skiplist(skiplist);
//   printf("\n");

//   insert_node(skiplist, 5, 1001);
//   print_skiplist(skiplist);
//   printf("\n");

//   insert_node(skiplist, 6, 100);
//   print_skiplist(skiplist);
//   printf("\n");

//   insert_node(skiplist, 7, 1008);
//   print_skiplist(skiplist);
//   printf("\n");

//   insert_node(skiplist, 8, 1007);
//   print_skiplist(skiplist);
//   printf("\n");



//   // Delete Nodes
//   delete_node(skiplist, 5, 1001);
//   print_skiplist(skiplist);
//   printf("\n");

//   printf("minimum pid: %d\n", get_minimum(skiplist));
//   printf("\n");

//   delete_node(skiplist, 6, 100);
//   print_skiplist(skiplist);
//   printf("\n");
// }