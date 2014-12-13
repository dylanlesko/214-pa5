#include "headers.h"

#define	FALSE		0
#define TRUE		1
#define MAXSIZE		5


/*
 *	argv1	=	customer input file 
 * 	argv2	=	order input file
 *	argv3	=	categories file
 * 
 */
int main ( int argc, char *argv[ ] )
{
	/* checks argument count */
	if (argc != 4) 
	{
		printf("Usage: incorrect arg count\n\n");
		exit(1);
	}

	/* Clears standard output */
	system("clear");

	printf("\nmem: %.2f gigs", (double)(getTotalSystemMemory())/1000000000);

	/* Creates thread object and attributes */
	pthread_attr_t attr;
 	pthread_t producerThread;

	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/* Declare and initialize customer list */
	customer **customerList = customerList_init();
	customerList_fill( customerList, argv[1] );
	//printCustomers( customerList );

	/* Declare and initialize category list */
	category **categoryList = categoryList_init( );
	categoryList_fill( categoryList, argv[3]);
	//printCategories( categoryList );

	shared **test = malloc(sizeof(shared));

	/* Declare and Initialize shared data struct */
	shared *data = shared_init();
	data->custFile = (char*)malloc(strlen(argv[1]) + 1);
		strcpy( data->custFile, argv[1] );
	data->orderFile = (char*)malloc(strlen(argv[2]) + 1);
		strcpy( data->orderFile, argv[2] );
	data->catFile = (char*)malloc(strlen(argv[3]) + 1);
		strcpy( data->catFile, argv[3] );
	data->customerList = customerList;
	data->categoryList = categoryList;
	data->catCount = getThreadCount( data->catFile );

	/* Start the producer thread */
	pthread_create( &producerThread, &attr, producerfnc, (void *) data );
	pthread_join( producerThread, NULL );


}

/*
*	producerfnc		producer function... couldn't tell?
*/
void * producerfnc( void *arg )
{
	shared *data = (shared*)arg;
	pthread_t consumerThread[data->catCount];
	threadShared *threadData[data->catCount];
	pthread_attr_t attr;

	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int i = 0;
	for( i = 0; i < data->catCount; i++ )
	{
		threadData[i] = (threadShared*)malloc(sizeof(threadShared));
		threadData[i]->queueMax = MAXSIZE;
		threadData[i]->curCount = 0;
		threadData[i]->customerList = data->customerList;
		threadData[i]->isopen = 1;
		threadData[i]->orders = (order**)malloc(sizeof(order));

		pthread_cond_init(&(threadData[i]->fullSignal), 0);
		pthread_cond_init(&(threadData[i]->emptySignal), 0);
		pthread_mutex_init( &threadData[i]->orderLock, 0 );

		pthread_create( &consumerThread[i], &attr, consumerfnc, (void *)threadData[i] );
	}

	FILE *fp = fopen( data->orderFile, "r" );
	char line[2048];
	char buffer[2048];
	order *o;

	/* Checks to see if the file is valid, exits program is it cannot be read */
	if ( fp == NULL )
	{
		fprintf(stderr, "\nCustomer file not found\n\n");
		exit(1);
	}

	//pthread_detach( pthread_self() );
	while (fgets(line, 2048, fp))
	{
		o = (order*)malloc(sizeof(order));

		strcpy(buffer, strtok(line,"\""));
		o->title = malloc(strlen(buffer) + 1);
		strcpy(o->title, buffer);

		o->price = atof(strtok(NULL, "|"));

		o->customer_id = atoi(strtok(NULL, "|"));

		strcpy(buffer, strtok(NULL, "\n"));
		o->category = malloc(strlen(buffer) + 1);
		strcpy(o->category, buffer);

		o->isProcessed = 0;

		o->cat_id = getCat_id( o->category, data->categoryList );
		//printf("\norder: %s", o->title);
		
		pthread_mutex_lock( &threadData[o->cat_id]->orderLock );

		while( threadData[o->cat_id]->curCount == threadData[o->cat_id]->queueMax )
		{
			//printf("\nProducer Waiting");
			pthread_cond_signal( &threadData[o->cat_id]->fullSignal );
			pthread_cond_wait( &threadData[o->cat_id]->emptySignal, &threadData[o->cat_id]->orderLock );
		}

		insertOrder( o, threadData[o->cat_id]->orders );
		threadData[o->cat_id]->curCount += 1;
		
		pthread_mutex_unlock( &threadData[o->cat_id]->orderLock );
		pthread_cond_signal( &threadData[o->cat_id]->fullSignal );


	}

	FCLOSE(fp);

	for( i = 0; i < data->catCount; i++ )
	{
		//printf("\nProducer Thread Joined With Thread %d", (i+1) );
		//printOrders( threadData[i]->orders);
		//printf("\ncurcount: %d", threadData[i]->curCount );
		//printf("\n");
		threadData[i]->isopen = 0;
		pthread_join( consumerThread[i], NULL );
	}



}

/*
*	consumerfnc		fuck comments
*/
void * consumerfnc( void *arg )
{
	threadShared *test = (threadShared*)arg;
	order **queue = test->orders;
	customer **customerList = test->customerList;
	order *currentOrder = (*queue);
	customer *temp;

	pthread_detach( pthread_self() );
	while ( (test)->isopen == 1 || currentOrder != NULL )
	{
		pthread_mutex_lock( &test->orderLock );

		while(test->curCount == 0 )
		{
			//printf("\nconsumer waiting");
			pthread_cond_signal( &test->emptySignal );
			pthread_cond_wait( &test->fullSignal, &test->orderLock );
		
		}
		if(test->curCount > 0)
		{
			if(currentOrder == NULL)
			{
				currentOrder = *test->orders;
				//printf("\nif");
			}
			while(currentOrder != NULL)
			{
				if(currentOrder->next != NULL)
				{
					if( currentOrder->isProcessed == 0)
					{
						//printf("\ncount: %d", test->curCount);
						temp = getCustomer( currentOrder->customer_id, customerList);
						pthread_mutex_lock( &temp->customerLock );
						printf(MAKE_RED"\ncat: %s", currentOrder->title);
						printf(MAKE_BLUE"\n\tneeds: %.2f", currentOrder->price);
						printf(MAKE_GREEN"\n\thas: %.2f"RESET_FORMAT, temp->balance);
						currentOrder->isProcessed = 1;
						test->curCount -= 1;
						pthread_mutex_unlock( &temp->customerLock  );
					}
					currentOrder = currentOrder->next;
				}
				else
				{

					//if( (test)->isopen == 1)
					if( test->curCount > 0 )
					{
						temp = getCustomer( currentOrder->customer_id, customerList);
						pthread_mutex_lock( &temp->customerLock );
						printf(MAKE_RED"\ncat: %s", currentOrder->title);
						printf(MAKE_BLUE"\n\tneeds: %.2f", currentOrder->price);
						printf(MAKE_GREEN"\n\thas: %.2f"RESET_FORMAT, temp->balance);
						currentOrder->isProcessed = 1;
						test->curCount -= 1;
						pthread_mutex_unlock( &temp->customerLock  );
					}
					break;

				}

			}
		}

		//sleep(1);
		pthread_mutex_unlock( &test->orderLock );
		pthread_cond_signal( &test->emptySignal );


	}
}

/*
*	shared_init
*
*	returns a sharec struct with dynamically allocated memory
*/
struct shared* shared_init( )
{
	shared *newShared = (shared*)malloc(sizeof(shared));

	return newShared; 
}

struct order** order_init( )
{
	struct order **newQueue = (order**)malloc(sizeof(order));
}

struct threadShared** threadShared_init( )
{

}

/*
*	customerList_init
*	returns a customerList struct with dynamic memory allocated
*
*/
struct customer** customerList_init( )
{
	customer **newList = (customer**)malloc(sizeof(customer));
	(*newList) = NULL;

	return newList;
}

/*
*	categoryList_init
*
*	returns a categoryList struct with dynamically allocated memory
*/
struct category** categoryList_init( )
{
	category **newList = (category**)malloc(sizeof(category));
	(*newList) = NULL;

	return newList;
}

/*
*	customerList_fill	function that generates the customerList database
*
*	arg1:				customerList pointer that will contain all the customer data
*	arg2:				string for the file to open and gather the data from
*
*/
void customerList_fill( struct customer **customerList, char *custFile )
{
	FILE 		*fp 			=	fopen(custFile, "r");
	char 		line[2048];
	char		buffer[2048];
	customer 	*c;

	/* Checks to see if the file is valid, exits program is it cannot be read */
	if ( fp == NULL )
	{
		fprintf(stderr, "\nCustomer file not found\n\n");
		exit(1);
	}

	/* Add every customer to the database */
	while (fgets(line, 2048, fp))
	{
		c = (customer*)malloc(sizeof(customer));

		strcpy(buffer, strtok(line, "\""));			/* Assigns Name		*/
		c->name = malloc(strlen(buffer) + 1);		// Malloc
		strcpy(c->name, buffer);

		c->id = atoi(strtok(NULL, "|"));			/* Assigns ID		*/

		c->balance = atof(strtok(NULL, "|"));		/* Assigns Balance	*/

		strcpy(buffer, strtok(NULL, "\""));			/* Assigns Address	*/
		c->address = malloc(strlen(buffer) + 1);	// Malloc
		strcpy(c->address, buffer);

		strcpy(buffer, strtok(NULL, "\"|"));		/* Assigns State	*/
		c->state = malloc(strlen(buffer) + 1);		// Malloc
		strcpy(c->state, buffer);

		strcpy(buffer, strtok(NULL, "\"|"));		/* Assigns Zip		*/
		c->zip = malloc(strlen(buffer) + 1);		// Malloc
		strcpy(c->zip, buffer);

		insertCustomer( c, customerList );
	}

	/* Nulls temp struct and close file */
	c = NULL;
	FCLOSE( fp ); 
}

/*
*	categoryList_fill	function that will create a category list, using the insertCategory function call
*
*	arg1:				categoryList pointer to contain the entire list
*	arg2:				string for the file to open and gather the data from
*/
void categoryList_fill( struct category **categoryList, char *catFile )
{
	int 		count			=	0;
	FILE 		*fp 			=	fopen( catFile, "r" );
	char		line[2048];
	char		buffer[2048];
	category 	*c;
	
	/* Checks to see if the file is valid, exits program is it cannot be read */
	if ( fp == NULL )
	{
		fprintf(stderr, "\nCustomer file not found\n\n");
		exit(1);
	}

	/* Add every category to the category list, and assign a unique id to it */
	while (fgets(line, 2048, fp))
	{
		c = (category*)malloc(sizeof(category));

		strcpy(buffer, strtok(line, "\n"));

		c->category = (char*)malloc(strlen(buffer) + 1);
		strcpy(c->category, buffer);

		c->next = NULL;

		c->id = count;
		count++;

		insertCategory( c, categoryList );
	}

	/* Nulls temp struct and close file */
	c= NULL;
	FCLOSE(fp);
}

/*
*	insertCustomer	function that recursively adds item to the end of a list
*
*	arg1:			customer struct to be added
*	arg2:			the customer list pointer
*
*	returns:		1 on success, 0 on failure
*/
int insertCustomer( customer *newCust, customer **list )
{
	/* when the list is null, you reached the end. add the new struct to the list */
	if( (*list) == NULL )
	{
		(*list) = newCust;
		return 1;
	}
	else
	{
		/* recursively passes the pointer to the next object in the struct */
		insertCustomer( newCust, &((*list)->next) );
	}
	return 0;
}

/*
*	insertCategory	function that recursively adds items to the end of a list
*
*	arg1:			category struct to be added
*	arg2:			the category list pointer
*
*	returns:		1 on success, 0 on failure
*/
int insertCategory( category *newCat, category **list)
{
	/* when the list is null, you reached the end. add the new struct to the list */
	if( (*list) == NULL)
	{
		(*list) = newCat;
		return 1;
	}
	else
	{
		/* recursively passes the pointer to the next object in the struct */
		insertCategory( newCat, &((*list)->next) );
	}
	return 0;
}

int insertOrder( order *newOrder, order **list)
{

	/* when the list is null, you reached the end. add the new struct to the list */
	if( (*list) == NULL)
	{
		(*list) = newOrder;
		return 1;
	}
	else
	{
		/* recursively passes the pointer to the next object in the struct */
		insertOrder( newOrder, &((*list)->next) );
	}
	return 0;}

/*
*	printCustomers	function that outputs all elements in a customerList to stdout
*
*	arg1:			the customer list to output
*/
void printCustomers( customer **list )
{
	/* checks if the passed paramenter is the end of the list */
	if( (*list) == NULL)
	{
		return;
	}
	
	/* prints all the fields of the struct */
	printf("\nid: %d", (*list)->id );
	printf("\nbal: %.2f", (*list)->balance);
	printf("\nadd: %s", (*list)->address);
	printf("\nstate: %s", (*list)->state);
	printf("\nzip: %s", (*list)->zip);
	printf("\n");
		
	/* recursively pass the pointer to the next object in the list */	
	printCustomers( &((*list)->next) );
}

/*
*	printCategories	function that outputs all elements in a categoryList to stdout
*
*	arg1:			the category list to output
*/
void printCategories( category **list )
{
	/* checks if the passed paramenter is the end of the list */
	if( (*list) == NULL )
	{
		return;
	}

	/* prints all the fields of the struct */
	printf("\ncat: %s", (*list)->category);
	printf("\nid: %d", (*list)->id );

	/* recursively pass the pointer to the next object in the list */	
	printCategories( &((*list)->next) );
}

void printOrders( order **orderList)
{
	/* checks if the passed paramenter is the end of the list */
	if( (*orderList) == NULL )
	{
		return;
	}

	printf(MAKE_BLUE"\ntitle: %s", (*orderList)->title );
	printf("\n\tid: %d", (*orderList)->customer_id);
	printf("\n\tprice: %f", (*orderList)->price);
	printf("\n\tbal: %f", (*orderList)->remaining_balance);
	printf("\n\tprocessed: %d"RESET_FORMAT, (*orderList)->isProcessed);

	/* recursively pass the pointer to the next object in the list */	
	printOrders( &((*orderList)->next) );

}

/* 
*	getCat_id	function for getting the id of a cetegory
*
*	arg1:		string of category you want to find id of
*	arg2:		list of all the categories
*	returns:	id for the category
*/
int getCat_id( char *category, struct category **categories )
{
	if ( (*categories) == NULL )
	{
		return -1;
	}
	if ( strncmp( category, (*categories)->category, strlen( (*categories)->category )  ) == 0 )
	{
		return ((*categories)->id);
	}
	else
	{
		getCat_id ( category, &((*categories)->next) );
	}
}

/*
*	getThreadCount	function that reads a category file to count the amount of categories
*
*	arg1:			string of the category file
*
*	returns:		int for the number of threads to generate
*/
int getThreadCount( char *categoryFile)
{
	FILE *fp = fopen(categoryFile, "r");
	char line[2048];
	int count = 0;

	/* reads to end of the category file */
	while( fgets(line, 2048, fp) )
	{
		count++;
	}

	/* if there are less than 1 thread, return 0 */
	if( count > 0 )
		return ( count );
	else
		return 0;
}

customer* getCustomer( int id, customer **customerList )
{
	if ( (*customerList) == NULL )
	{
		return NULL;
	}
	if( (*customerList)->id == id )
	{
		return (*customerList);
	}
	else
	{
		getCustomer( id, &((*customerList)->next) );
	}
}

size_t getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}