#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <stdlib.h>
#include <string.h>
#include "order.h"

/* Customer struct */
typedef struct {
  int id;             /* key */
  char *name;
  float balance;
  char *address;
  char *state;
  char *zip;
  order *successful_orders;
  order *failed_orders;

  pthread_mutex_t lock;
} customer;


#endif
