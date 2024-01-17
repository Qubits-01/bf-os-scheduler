struct node {
  int pid;
  int virtual_deadline;
  struct node * prev;
  struct node * next;
  struct node * forward;
};

struct skiplist {
  int levels;
  struct node ** headers;
};

struct skiplist * init_skiplist();
void insert_node(struct skiplist * skiplist, struct proc * p);
void delete_node(struct skiplist * skiplist, struct proc * p);
int get_minimum(struct skiplist * skiplist);
void print_skiplist(struct skiplist * skiplist);