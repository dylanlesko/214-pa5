#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "customer.h"
#include "order.h"
#include "category.h"

int main(int argc, char **argv)
{
	
	customer *c;
	category *q;
	order *o;
	FILE *f;
	char line[2048];
	int err;
	pthread_t *tid;
	char buffer[2048];

	if (argc != 4) 
	{
		printf("Usage: whore\n\n");
		exit(1);
	}


	//Start Customers
	f = fopen(argv[1], "r");
	if (f == NULL)
	{
		fprintf(stderr, "Customer file not found\n");
		exit(1);
	}

	/* Add customers to database */
	while (fgets(line, 2048, f)) 
	{
		c = malloc(sizeof(customer));

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


		//Outputs database
		printf("\nname: %s", c->name);
		printf("\nid: %d", c->id);
		printf("\nbal: %.2f", c->balance);
		printf("\nadd: %s", c->address);
		printf("\nstate: %s", c->state);
		printf("\nzip: %s", c->zip);
		printf("\n");
	
	}
	fclose(f);										/* Close database	*/
	//End Customers
	
	//Start Orders	
	/* Open Orders */
	f = fopen(argv[2], "r");
	if (f == NULL) 
	{
		fprintf(stderr, "Order file not found\n");
		exit(1);
	}
	
	/* Load orders */
	while (fgets(line, 2048, f)) 
	{
		o = malloc(sizeof(order));

		strcpy(buffer, strtok(line, "\""));
		o->title = malloc(strlen(buffer) + 1);
		strcpy(o->title, buffer);

		o->price = atof(strtok(NULL, "|"));

		o->customer_id = atoi(strtok(NULL, "|"));

		strcpy(buffer, strtok(NULL, "|"));
		o->category = malloc(strlen(buffer) + 1);
		strcpy(o->category, buffer);

		//Outputs orders
		printf("\ntitle: %s", o->title);
		printf("\nprice: %.2f", o->price);
		printf("\nid: %d", o->customer_id);
		printf("\ncategory: %s", o->category);
		printf("\n");
	  
	}
	fclose(f);										/* Close orders		*/
	//End Orders
	
	//Start Categories
	/* Open Categories */
	f = fopen(argv[3], "r");
	if (!f)
	{
		fprintf(stderr, "Categories file not found");
		exit(1);
	}
	
	/* Load categories */
	while(fgets(line, 2048, f))
	{
		q = malloc(sizeof(category));
		
		q->cat = malloc(strlen(line) + 1);
		strcpy(q->cat, line);
		
		printf("\ncat: %s", q->cat);
		printf("\n");
	}
	fclose(f);										/* Close categories	*/	
	//End Catagories



	printf("\n\n");

}
