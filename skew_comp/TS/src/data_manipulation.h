  
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
void calculateDeviation(int arr[], int n, int median, int deviations[]);
int removeOutliers(int arr[], int n, int scalefactor);
inline int sqr(int x);
int linreg(int n, const uint64_t time[], const int y[], double* m, double* b, double* r);
#endif 
