#ifndef BOOK_H
#define BOOK_H

#include "headers.h"


/* linkedlist struct to contain the order queues for each thread */
typedef struct order
{
	int 				customer_id;						/* key */
	char 				*title;
	float 				price;
	float 				remaining_balance;
	int 				isProcessed;						/* 0 for unprocessed */

	int 				cat_id;								/* category reference */
	char				*category;

	struct order 		*next;
} order;

/* linkedlist struct to contain customer data base*/
typedef struct customer
{
	int 				id;									/* key */
	char 				*name;
	float 				balance;
	char 				*address;
	char 				*state;
	char 				*zip;

	pthread_mutex_t 	customerLock;

	struct order 		**successful_orders;
	struct order 		**failed_orders;

	struct customer 	*next;
} customer;

/* reference table for categories */
typedef struct category
{
	int 				id;									/* key */
	char 				*category;

	struct category 	*next;
} category;

/* the big daddy struct */
typedef struct shared
{
	struct customer 	**customerList;
	struct category 	**categoryList;
	char 				*custFile;
	char 				*orderFile;
	char 				*catFile;


} shared;

/* struct to pass necesarry data to consumer threads*/
typedef struct threadShared
{
	int 				*isopen;							/* 1 for open; 0 for closed */
	int 				*queueMax;
	int  				curCount;
	int 				*custCount;
	int 				id;

	int 				is_joined;
	int 				is_started;

	struct order 		**orderList;
	struct customer 	**customerList;	

	pthread_cond_t 		spaceAvailable;
	pthread_cond_t 		dataAvailable;


	pthread_mutex_t 	orderLock;
} threadShared;


void * producerfnc( void *arg );
void * consumerfnc( void *arg );


int getCat_id( char *category, struct category **categories );
int get_customerCount( customer **customerList );

int getThreadCount( char *categoryFile);
customer* getCustomer( int id, customer **customerList );

struct order** order_init( );
struct threadShared* threadShared_init( int id  , customer** customerList );
struct shared* shared_init( );
struct customer** customerList_init();
struct category** categoryList_init( );

void customerList_fill( struct customer **customerList, char *custFile);
void categoryList_fill( struct category **categoryList, char *catFile );
struct order* order_format( char* line );

int insertOrder( order *newOrder, order **orderList);
int insertCustomer( customer *newCust, customer **list );
int insertCategory( category *newCat, category **list );

void printCustomers( customer **list );
void printCategories( category **list );
void printReport( shared *data );
void printOrders( order **orderList);


int checkJoined( int tests[] );



int countCustomers( customer **list );


#endif