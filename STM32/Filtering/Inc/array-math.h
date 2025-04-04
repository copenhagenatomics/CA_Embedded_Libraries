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

typedef struct {
    double_cbuf_t cbuf_t;
    double  sum;
    double  varSum;
    double  mean;
} moving_avg_cbuf_t;

typedef moving_avg_cbuf_t * moving_avg_cbuf_handle_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Note: Function nomenclature loosely chosen to fit with C++ algorithm.h library */
int max_element(double arr[], unsigned len, double* result);
int min_element(double arr[], unsigned len, double* result);
int mean_element(double arr[], unsigned len, double* result);
int sum_element(double arr[], unsigned len, double* result);

/* Functions for moving average filters */
int maInit(moving_avg_cbuf_handle_t p_ma, double* buf, unsigned len);
double maMean(moving_avg_cbuf_handle_t p_ma, double new_val);
double maVariance(moving_avg_cbuf_handle_t p_ma, double new_val);
double maStdDeviation(moving_avg_cbuf_handle_t p_ma, double new_val);
int cbInit(double_cbuf_handle_t p_cb, double* buf, unsigned len);
void cbPush(double_cbuf_handle_t p_cb, double new_val);
int cbMean(double_cbuf_handle_t p_cb, int elements, double* result);
int cbMax(double_cbuf_handle_t p_cb, int elements, double* result);

double cbGetHead(double_cbuf_handle_t p_cb);
int cbGetIdx(double_cbuf_handle_t p_cb, int idx, double* ret);
double cbGetTail(double_cbuf_handle_t p_cb);

#ifdef __cplusplus
}
#endif

#endif /* ARRAY_MATH_H_ */