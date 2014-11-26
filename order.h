#ifndef ORDER_H
#define ORDER_H

#include <stdlib.h>
#include <string.h>

/* Order struct */
typedef struct order {
  char *title;
  float price;
  float remaining_balance;
  int customer_id;
  char *category;  /* hash key for queues */
  int success;        /* 1: successful, -1: failed, 0: pending */
  struct order *next;

} order;

#endif
