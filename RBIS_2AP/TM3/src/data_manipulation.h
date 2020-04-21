  
#ifndef __DATA_MANIPULATION__ // must be unique name in the project
#define __DATA_MANIPULATION__

/* 
Contains functions to sort arrays, find the median, calculate a linear regression,...
 */

#include <Arduino.h>

void swap(int *a, int *b);
void printArray(int arr[], int size);
void quicksort(int arr[], int l, int r);
int findMedian(int arr[], int n) ;


#endif 
