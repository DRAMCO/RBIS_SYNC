#include "data_manipulation.h"

// Function to swap two pointers
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Function to print an array of integers
void printArray(int arr[], int size)
{
    for (int i = 0; i < size; i++)
    {
      Serial.print(arr[i]);
      Serial.print(" , ");
    }

    Serial.println(" ");
}


// Function to run quicksort on an array of integers
// l is the leftmost starting index, which begins at 0
// r is the rightmost starting index, which begins at array length - 1
void quicksort(int arr[], int l, int r)
{
    // Base case: No need to sort arrays of length <= 1
    if (l >= r)
    {
        return;
    }
    
    // Choose pivot to be the last element in the subarray
    int pivot = arr[r];

    // Index indicating the "split" between elements smaller than pivot and 
    // elements greater than pivot
    int cnt = l;

    // Traverse through array from l to r
    for (int i = l; i <= r; i++)
    {
        // If an element less than or equal to the pivot is found...
        if (arr[i] <= pivot)
        {
            // Then swap arr[cnt] and arr[i] so that the smaller element arr[i] 
            // is to the left of all elements greater than pivot
            swap(&arr[cnt], &arr[i]);

            // Make sure to increment cnt so we can keep track of what to swap
            // arr[i] with
            cnt++;
        }
    }
    
    // NOTE: cnt is currently at one plus the pivot's index 
    // (Hence, the cnt-2 when recursively sorting the left side of pivot)
    quicksort(arr, l, cnt-2); // Recursively sort the left side of pivot
    quicksort(arr, cnt, r);   // Recursively sort the right side of pivot
}

// Function for calculating median 
int findMedian(int arr[],int n) 
{ 
    int temp[n];
    for(int i=0; i<n ;i++){// copy array to temp so the original array doesn't change
        temp[i] = arr[i];
    }
    quicksort(temp, 0, n-1);
    // check for even case 
    if (n % 2 != 0){
       return temp[n/2]; 
    } else{
        return (int)(temp[(n-1)/2] + temp[n/2])/2;
    }    
} 

// Function to calculate the deviation 
void calculateDeviation(int arr[], int n, int median, int deviations[]){

    for(int i=0;i<n;i++){
        deviations[i] = abs(arr[i] - median);
    }

}

// function to remove the outliers (outliers will be replaced with the median value)
int removeOutliers(int arr[], int n, int scalefactor){
    int median = findMedian(arr, n); // calculate median of datapoints
    int deviations[n];
    calculateDeviation(arr,n,median,deviations); // calculate the deviation of each datapoint
    int mediandeviation = findMedian(deviations,n); // calculate the median of the deviations
    int upperlimit = median + scalefactor*mediandeviation; // calculate upperlimit
    int lowerlimit = median - scalefactor*mediandeviation; // calculate lowerlimit
    for(int i=0;i<n;i++){
        if(arr[i] > upperlimit || arr[i] < lowerlimit){
            arr[i] = median;// replace outlier with median
        }
    }
    return median;
}

// Function to calculate the square
inline int sqr(int x) {
    return x*x;
}

// Function to perform linear regression: y = mx + b
int linreg(int n, const uint64_t time[], const int y[], double* m, double* b, double* r){
    double x[n];
    for(int i=0;i<n;i++){
        x[i] = time[i]/1000000;
    }

    double   sumx = 0.0;                      /* sum of x     */
    double   sumx2 = 0.0;                     /* sum of x**2  */
    double   sumxy = 0.0;                     /* sum of x * y */
    double   sumy = 0.0;                      /* sum of y     */
    double   sumy2 = 0.0;                     /* sum of y**2  */


    for (int i=0;i<n;i++){ 
        sumx  += x[i];       
        sumx2 += sqr(x[i]);  
        sumxy += x[i] * y[i];
        sumy  += y[i];      
        sumy2 += sqr(y[i]); 
    } 

    double denom = (n * sumx2 - sqr(sumx));
    if (denom == 0) {
        // singular matrix. can't solve the problem.
        *m = 0;
        *b = 0;
        if (r) *r = 0;
            return 1;
    }

    *m = (n * sumxy  -  sumx * sumy) / denom;
    *b = (sumy * sumx2  -  sumx * sumxy) / denom;
    if (r!=NULL) {
        *r = (sumxy - sumx * sumy / n) /    /* compute correlation coeff */
              sqrt((sumx2 - sqr(sumx)/n) *
              (sumy2 - sqr(sumy)/n));
    }

    return 0; 
}