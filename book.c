#include "headers.h"

#define	FALSE		0
#define TRUE		1
#define MAXSIZE		5

pthread_mutex_t 	printLock 		= PTHREAD_MUTEX_INITIALIZER;		/* lock for printf, don't think they are sigsafe */
pthread_mutex_t 	openLock 		= PTHREAD_MUTEX_INITIALIZER;		/* lock for printf, don't think they are sigsafe */
int 				file_open 		= 	FALSE;							/* 1 for open; 0 for close */
int 				max 			= 	MAXSIZE;						/* Max size of the queue */
int 				custCount 		= 	0;								/* amount of customers, calculated in producerfnc */
int 				catCount		=	0;								/* amount of categories, calculated in main*/


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

	/* Creates thread object and attributes */
 	pthread_t producerThread;

	//pthread_mutex_init( &printLock , 0 );

	/* Declare and initialize customer list */
	customer **customerList = customerList_init();
	customerList_fill( customerList, argv[1] );
	//printCustomers( customerList );

	/* Declare and initialize category list */
	category **categoryList = categoryList_init( );
	categoryList_fill( categoryList, argv[3]);
	//printCategories( categoryList );

	/* Declare and Initialize shared data struct */
	shared *data = shared_init( );
	data->custFile = (char*)malloc(strlen(argv[1]) + 1);
		strcpy( data->custFile, argv[1] );
	data->orderFile = (char*)malloc(strlen(argv[2]) + 1);
		strcpy( data->orderFile, argv[2] );
	data->catFile = (char*)malloc(strlen(argv[3]) + 1);
		strcpy( data->catFile, argv[3] );
	data->customerList = customerList;
	data->categoryList = categoryList;

	catCount = getThreadCount( data->catFile );
	custCount = get_customerCount( data->customerList );

	/* Start the producer thread */
	pthread_create( &producerThread, 0, producerfnc, ( void * ) data );
	pthread_join( producerThread, NULL );
}

/*
*	producerfnc		producer function... couldn't tell?
*/
void * producerfnc( void *arg )
{
	shared 			*data 						= 	( shared* )arg;
	pthread_t 		consumerThread[catCount];
	threadShared 	*threadData[catCount];
	int 			i 							=	0;
	int 			j 							=	0;
	FILE 			*fp 						= 	fopen( data->orderFile, "r" );
	char 			line[2048]					=	"";
	char 			buffer[2048]				=	"";
	order 			*o 							=	NULL;

	/* creates consumer threads */
	for( i = 0; i < catCount; i++ )
	{
		threadData[i] = threadShared_init( i + 1 );
		pthread_create( &consumerThread[i], 0, consumerfnc, (void *)threadData[i] );
	}

	/* If the file is readable, set global file_open variable to true. otherwise exit the program */
	if ( fp == NULL )
	{
		fprintf(stderr, "\nOrder file could not be read.\n\n");
		exit(1);
	}
	else
	{
		file_open = TRUE;
	}

	/* 	Goes to the end of the orders file. 
	*	Create a new struct for each order
	*	wait if the order's category thread is full
	*	insert the new order into the queue
	*/
	while ( fgets( line, 2048, fp ) )
	{
		o = order_format( line );
		o->cat_id = getCat_id( o->category, data->categoryList );
		
		//pthread_mutex_lock( &threadData[o->cat_id]->orderLock );
		pthread_mutex_lock(&threadData[o->cat_id]->orderLock);
		{
			while( threadData[o->cat_id]->curCount ==  max )
			{
				printf("\nProducer Waiting");
				pthread_cond_signal( &threadData[o->cat_id]->dataAvailable );
				pthread_cond_wait( &threadData[o->cat_id]->spaceAvailable, &threadData[o->cat_id]->orderLock );
			}

			insertOrder( o, threadData[o->cat_id]->orderList );
			threadData[o->cat_id]->curCount += 1;
			
			pthread_mutex_unlock( &threadData[o->cat_id]->orderLock );
			pthread_cond_signal( &threadData[o->cat_id]->dataAvailable );
		}

	}




	FCLOSE(fp);

	if(o == NULL)
	{
		printf("\nNo Orders Found\n\n");
		end(1);
	}
	for( i = 0; i < catCount; i++ )
	{		
//		printOrders( threadData[i]->orderList );
		if( *(threadData[i]->orderList) == NULL )
		{
			printf("\ntwo");
			threadData[i]->isdone = TRUE;
		}
		else
		{
			pthread_mutex_lock( &openLock );
			printf("\none");
			pthread_cond_wait( &threadData[i]->started, &openLock );
			pthread_mutex_unlock( &openLock );
		}
	}

	file_open = FALSE;
/*
	int tests[ catCount ];

	for( i = 0; i < catCount; i++ )
	{
		tests[i] = FALSE;
	}


	//while( checkJoined( tests ) == FALSE)
	while(0)
	{
		//printf("\nwhile");
		for( i = 0; i < catCount; i++)
		{
			pthread_cond_signal( &threadData[i]->dataAvailable);
			if(tests[i] == FALSE)
			{
				if(pthread_mutex_trylock( &threadData[i]->orderLock ) == 0 )
				{
					if( (pthread_tryjoin_np( consumerThread[i], NULL) ) == 0 )
					{
						tests[i] = TRUE;
					}
					//pthread_cond_signal( &threadData[i]->dataAvailable );
					pthread_mutex_unlock( &threadData[i]->orderLock );
				}
			}
		}

	}
*/




}

int checkJoined( int tests[] )
{
	int i = 0;

	for( i = 0; i < catCount; i++ )
	{
		if(tests[i] == FALSE)
		{
			return FALSE;
		}
	}
	return TRUE;

}

/*
*	consumerfnc		fuck comments
*/
void * consumerfnc( void *arg )
{
	order 			*newOrder 			=	NULL;
	customer 		*currentCustomer	=	NULL;
	order 			*currentOrder		=	NULL;
	threadShared 	*data 				= 	(threadShared*)arg;
	order 			**orderList 		= 	data->orderList;
	customer 		**customerList 		= 	data->customerList;

	while( *(data->isopen) == TRUE)
	{

		currentOrder = *(data->orderList);
		if( *(data->orderList) == NULL)
		{
			continue;
		}
		else
		{
			pthread_cond_signal( &data->started );

			while( currentOrder->next != NULL )
			{
				newOrder = malloc(sizeof(order));
				(*newOrder) = (*currentOrder);
				currentOrder = currentOrder->next;
				data->curCount--;
			}
		}
	}


	//while( 1 )
	{	
		//printf("\ndsf");

	}


}

/*
*	shared_init
*
*	returns a sharec struct with dynamically allocated memory
*/
struct shared* shared_init(  )
{
	shared *newShared = (shared*)malloc(sizeof(shared));

	return newShared; 
}

struct order** order_init( )
{
	struct order **newQueue = (order**)malloc(sizeof(order*));
	(*newQueue) = NULL;

	return newQueue;
}

struct threadShared* threadShared_init( int id )
{
	threadShared *newShared = (threadShared*)malloc(sizeof(threadShared));
	newShared->isopen = (int*)malloc(sizeof(int));
		(newShared->isopen) = &file_open;
	newShared->queueMax = (int*)malloc(sizeof(int));
		(newShared->queueMax) = &max;
	newShared->curCount = 0;
	newShared->id = id;
	//newShared->started	=	FALSE;
	newShared->isdone	=	FALSE;

	newShared->orderList = order_init( );
	newShared->customerList = customerList_init( );

	pthread_cond_init(&(newShared->dataAvailable), 0);
	pthread_cond_init(&(newShared->spaceAvailable), 0);
	pthread_cond_init(&(newShared->started), 0);
	pthread_mutex_init( &newShared->orderLock, 0 );

	return newShared;
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
	FILE 		*fp 			=	fopen( custFile, "r" );
	char 		line[2048];
	char		buffer[2048];
	customer 	*c;
	order 		**success 		=	NULL;
	order 		**failure 		=	NULL;

	/* Checks to see if the file is valid, exits program is it cannot be read */
	if ( fp == NULL )
	{
		fprintf(stderr, "\nDatabase file could not be read.\n\n");
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

		success = order_init( );
		failure = order_init( );

		c->successful_orders = success;
		c->failed_orders = failure;

		c->next = NULL;

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
		fprintf(stderr, "\nCategory could not be read.\n\n");
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

struct order* order_format( char* line)
{
	char 	buffer[2048]	=	"";
	order*	newOrder		=	(order*)malloc(sizeof(order));

	if(line == NULL)
	{
		return NULL;
	}
		strcpy(buffer, strtok(line,"\""));
		newOrder->title = malloc(strlen(buffer) + 1);
		strcpy(newOrder->title, buffer);

		newOrder->price = atof(strtok(NULL, "|"));

		newOrder->customer_id = atoi(strtok(NULL, "|"));

		strcpy(buffer, strtok(NULL, "\n"));
		newOrder->category = malloc(strlen(buffer) + 1);
		strcpy(newOrder->category, buffer);

		newOrder->isProcessed = FALSE;

		return newOrder;
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

	if(list == NULL)
	{
		return 0;
	}
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
	return 0;
}

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

	printf("\ntitle: %s", (*orderList)->title );
	printf("\n\tid: %d", (*orderList)->customer_id);
	printf("\n\tprice: %f", (*orderList)->price);
	printf("\n\tbal: %f", (*orderList)->remaining_balance);
	printf("\n\tprocessed: %d", (*orderList)->isProcessed);

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

int get_customerCount( customer **customerList )
{
	int count = 0;

	customer *p = (*customerList);

	while( p != NULL )
	{
		count++;
		p = p->next;
	}

	return count;
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