/**********************************************/
/* CPSC 340				      */
/* File: get_path.h			      */
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifndef GET_PATH_H
#define GET_PATH_H

struct pathElement {
   char *element;			/* one directory in the path */
   struct pathElement *next;		/* pointer to next path_t node */
} typedef path_t;

/*  Returns a pointer to a linked list of path elements. */
path_t *getPath(  );

#endif
