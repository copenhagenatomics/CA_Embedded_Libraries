/*! 
**  \brief     Array math functions
**  \details   Functions to find the max/average of arrays
**  \author    Luke Walker
**  \date      09/04/2024
**/

#include "array-math.h"
#include <math.h>
#include <stdio.h>

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Returns the maximum value in an array of double
*/
int max_element(double arr[], unsigned len, double* result) {
    if(len != 0) {
        double ret_val = arr[0];
        for(unsigned i = 1; i < len; i++) {
            if(arr[i] > ret_val) ret_val = arr[i];
        }
        *result = ret_val;
        return 0;
    }

    return -1;
}

/*!
** @brief Returns the minimum value in an array of double
*/
int min_element(double arr[], unsigned len, double* result) {
    if(len != 0) {
        double ret_val = arr[0];
        for(unsigned i = 1; i < len; i++) {
            if(arr[i] < ret_val) ret_val = arr[i];
        }
        *result = ret_val;
        return 0;
    }

    return -1;
}

/*!
** @brief Returns the average value of an array of double
*/
int mean_element(double arr[], unsigned len, double* result) {
    double tmp;
    int ret = sum_element(arr, len, &tmp);
    
    if(ret == 0) {
        *result = tmp / len;
    }

    return ret;
}

/*!
** @brief Returns the sum of an array of double
*/
int sum_element(double arr[], unsigned len, double* result) {
    if(len != 0) {
        double ret_val = arr[0];
        for(int i = 1; i < len; i++) {
            ret_val += arr[i];
        }
        *result = ret_val;
        return 0;
    }

    return -1;
}

/*!
** @brief Initialises an empty circular buffer
*/
int cbInit(double_cbuf_handle_t p_cb, double* buf, unsigned len) {
    if(len != 0) {
        p_cb->buffer = buf;
        p_cb->len    = len;
        p_cb->idx    = 0;

        for(int i = 0; i < p_cb->len; i++) {
            p_cb->buffer[i] = 0.0;
        }

        return 0;
    }

    return -1;
}

/*!
** @brief Initialises an empty moving average buffer 
*/
int maInit(moving_avg_cbuf_handle_t p_ma, double* buf, unsigned len) {
    if (cbInit(&p_ma->cbuf_t, buf, len) != 0)
    {
        return -1;
    }

    p_ma->sum = 0;
    p_ma->varSum = 0;
    p_ma->mean = 0;

    return 0;
}

/*!
** @brief Add a new element to the circular buffer and discard the oldest element
*/
void cbPush(double_cbuf_handle_t p_cb, double new_val) {
    p_cb->buffer[p_cb->idx++] = new_val;

    if(p_cb->idx >= p_cb->len) {
        p_cb->idx = 0;
    }
}

/*!
** @brief Get the newest element of the circular buffer
*/
double cbGetHead(double_cbuf_handle_t p_cb) {
    if (p_cb->idx == 0)
    {
        return p_cb->buffer[p_cb->len - 1];
    }
    return p_cb->buffer[p_cb->idx - 1];
}

/*!
** @brief Get the element at index X of the circular buffer. 
**
** 0 is the tail, len-1 = head
*/
int cbGetIdx(double_cbuf_handle_t p_cb, int idx, double* ret) {
    if(idx >= p_cb->len) {
        return -1;
    }
    else {
        idx += p_cb->idx;

        if(idx >= p_cb->len) {
            idx -= p_cb->len;
        }

        *ret = p_cb->buffer[idx];

        return 0;
    }
}

/*!
** @brief Get the oldest element of the circular buffer
*/
double cbGetTail(double_cbuf_handle_t p_cb) {
    return p_cb->buffer[p_cb->idx];
}

/*!
** @brief Computes the moving average of an array in a circular fashion.
*/
double maMean(moving_avg_cbuf_handle_t p_ma, double new_val)
{
    // Update sum of array
    p_ma->sum = p_ma->sum - cbGetTail(&p_ma->cbuf_t) + new_val;
    
    // Push new value onto buffer
    cbPush(&p_ma->cbuf_t, new_val);

    // Return average
    p_ma->mean = p_ma->sum / p_ma->cbuf_t.len;
    return p_ma->mean;
}

/*!
** @brief Computes the variance of an array using Welford's online algorithm.
** @note The variance algorithm is equivalent to 
**              var (X) = (1 / (N-1)) * SUM_i ((X(i) - mean(X))^2)
**       Hence, it uses the unbiased (N-1) sample variance (Bessel's correction), rather than the population variance 
**       used in the references below. This will yield a more conservative estimate especially for smaller sample size.
**
**       Information about the algorithm can be found here: 
**              Theory: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
**              Implementation: https://stackoverflow.com/a/6664212
*/
double maVariance(moving_avg_cbuf_handle_t p_ma, double new_val)
{
    double x_old = cbGetTail(&p_ma->cbuf_t);
    double current_mean = p_ma->mean;

    // Moving average (new_val is pushed onto circular buffer in maMean)
    double new_mean = maMean(p_ma, new_val);

    /* Variance numerator term 
     * Standard notation is p_ma->varSum += (new_val-avg)*(new_val-newMean)-(x_old-avg)*(x_old-newMean)
     * The expression below is a computationally lighter equivalent.  
     */
    p_ma->varSum += (new_val + x_old - current_mean - new_mean) * (new_val - x_old);

    // Return sample variance (using Bessel's correction)
    return (p_ma->varSum / (p_ma->cbuf_t.len - 1));
}

/*!
** @brief Computes the moving standard deviation of an array in a circular fashion.
**        Details of implementation described in maVariance.
*/
double maStdDeviation(moving_avg_cbuf_handle_t p_ma, double new_val)
{
    return sqrt(maVariance(p_ma, new_val));
}

/*!
** @brief Take the mean of the n most recent elements in the circular buffer
*/
int cbMean(double_cbuf_handle_t p_cb, int elements, double* result) {
    if(elements > p_cb->len) {
        return -1;
    }
    else if(elements == p_cb->len) {
        return mean_element(p_cb->buffer, p_cb->len, result);
    }
    else if(0 == p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return mean_element(&p_cb->buffer[p_cb->len - elements], elements, result);
    }
    else if(elements <= p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return mean_element(&p_cb->buffer[p_cb->idx - elements], elements, result);
    }
    else {
        double p1, p2;
        int err = sum_element(p_cb->buffer, p_cb->idx, &p1);

        if(err != 0) {
            return err;
        }

        int remaining = elements - p_cb->idx;
        err = sum_element(&p_cb->buffer[p_cb->len - remaining], remaining, &p2);

        *result = (p1 + p2) / elements;
        return 0;
    }
}

/*!
** @brief Take the max of the n most recent elements in the circular buffer
*/
int cbMax(double_cbuf_handle_t p_cb, int elements, double* result) {
    if(elements > p_cb->len) {
        return -1;
    }
    else if(elements == p_cb->len) {
        return max_element(p_cb->buffer, p_cb->len, result);
    }
    else if(0 == p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return max_element(&p_cb->buffer[p_cb->len - elements], elements, result);
    }
    else if(elements <= p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return max_element(&p_cb->buffer[p_cb->idx - elements], elements, result);
    }
    else {
        double  p1, p2;
        int err = max_element(p_cb->buffer, p_cb->idx, &p1);

        if(err != 0) {
            return err;
        }

        int     remaining = elements - p_cb->idx;
        err = max_element(&p_cb->buffer[p_cb->len - remaining], remaining, &p2);

        *result = (p1 > p2) ? p1 : p2;
        return 0;
    }
}