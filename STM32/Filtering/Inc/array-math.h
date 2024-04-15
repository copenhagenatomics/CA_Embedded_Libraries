/*! 
**  \brief     Array math functions
**  \details   Functions to find the max/average of arrays
**  \author    Luke Walker
**  \date      09/04/2024
**/

#ifndef ARRAY_MATH_H_
#define ARRAY_MATH_H_

/***************************************************************************************************
** TYPE DEFINITIONS
***************************************************************************************************/

typedef struct {
    double* buffer;
    int     len;
    int     idx;
} double_cbuf_t;

typedef double_cbuf_t * double_cbuf_handle_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

/* Note: Function nomenclature loosely chosen to fit with C++ algorithm.h library */
double max_element(double arr[], int len);
double mean_element(double arr[], int len);

/* Functions for moving average filters */
void cbInit(double_cbuf_handle_t p_cb, double* buf, int len);
void cbPush(double_cbuf_handle_t p_cb, double new_val);
double cbMean(double_cbuf_handle_t p_cb, int elements);
double cbMax(double_cbuf_handle_t p_cb, int elements);

#endif /* ARRAY_MATH_H_ */