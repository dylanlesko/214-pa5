#ifndef CATEGORY_H
#define CATEGORY_H

typedef struct category_{
  char *cat;
  void *val;
  struct category_ *next;
  struct category_ *prev;

  int producing;
  pthread_mutex_t lock;
  pthread_cond_t waiter;

} category;

#endif
