/**********************************************/
/* CPSC 340				      */
/* File: get_path.c			      */
/* 					      */
/* This code parses the PATH env var and      */
/* returns a linked list.		      */
/* For a given node n, n->element is one      */
/* dir in the PATH.                           */
/* Head node is empty.			      */
/* Head->next points to first node with data. */
/* Next of last node with data points to NULL */
/* See man -s3 getenv for more info           */
/**********************************************/

#include "get_path.h"

path_t
*getPath ( void ) {
   /* path is a copy of the PATH environment variable */
   /* p is a temp pointer */
   char *path, *p;

   /* tmp is a temp pointer used to create a new node in the list */
   /* pathlist is a pointer to the head of the list */
   path_t *tmp, *pathlist = NULL;

   /* get a pointer to the PATH env var.
      We make a copy of it, to work with, strtok modifies the
      string that it tokenizes */
   p = getenv ( "PATH" );

   path = malloc ( ( strlen( p ) + 1 ) * sizeof ( char ) );
   strncpy ( path, p, strlen( p ) );
   path[ strlen( p ) ] = '\0';      /* make sure NULL terminated! */

   /* Colon : separates entires in PATH */
   /* for each entry in PATH, create node */
   /* to build a linked list of dirs. */
   
   p = strtok ( path, ":" ); 	
   do {				
   				
      if ( !pathlist )	{	           /* Empty list? Create head of list */
	 tmp = calloc( 1, sizeof ( path_t ) );
	 pathlist = tmp;
      } else {		  	           /* add on next element */
	 tmp->next = calloc ( 1, sizeof ( path_t ) );
	 tmp = tmp->next;
      }
      tmp->element = malloc ( strlen( p ) + 1 );
      strcpy ( tmp->element, p );
      tmp->next = NULL;
   } while ( ( p = strtok( NULL, ":" ) ) );

   free ( path );

   return pathlist;
}
