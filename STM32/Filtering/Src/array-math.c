/*! 
**  \brief     Array math functions
**  \details   Functions to find the max/average of arrays
**  \author    Luke Walker
**  \date      09/04/2024
**/

#include "array-math.h"

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Returns the maximum value in an array of double
*/
double max_element(double arr[], int len) {
    if(len != 0) {
        double ret_val = arr[0];
        for(int i = 1; i < len; i++) {
            if(arr[i] > ret_val) ret_val = arr[i];
        }
        return ret_val;
    }

    return -1.0;
}

/*!
** @brief Returns the average value of an array of double
*/
double mean_element(double arr[], int len) {
    if(len != 0) {
        double ret_val = arr[0];
        for(int i = 1; i < len; i++) {
            ret_val += arr[i];
        }
        return ret_val / (double)len;
    }

    return -1.0;
}

/*!
** @brief Initialises an empty circular buffer
*/
void cbInit(double_cbuf_handle_t p_cb, double* buf, int len) {
    p_cb->buffer = buf;
    p_cb->len    = len;
    p_cb->idx    = 0;

    for(int i = 0; i < p_cb->len; i++) {
        p_cb->buffer[i] = 0.0;
    }
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
** @brief Take the mean of the n most recent elements in the circular buffer
*/
double cbMean(double_cbuf_handle_t p_cb, int elements) {
    if(elements == p_cb->len) {
        return mean_element(p_cb->buffer, p_cb->len);
    }
    else if(elements <= p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return mean_element(&p_cb->buffer[p_cb->idx - 1 - elements], elements);
    }
    else {
        double  p1 = mean_element(p_cb->buffer, p_cb->idx);
        int     remaining = elements - p_cb->idx;
        double  p2 = mean_element(&p_cb->buffer[p_cb->len - 1 - remaining], remaining);

        return p1 + p2;
    }
}

/*!
** @brief Take the max of the n most recent elements in the circular buffer
*/
double cbMax(double_cbuf_handle_t p_cb, int elements) {
    if(elements == p_cb->len) {
        return max_element(p_cb->buffer, p_cb->len);
    }
    else if(elements <= p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return max_element(&p_cb->buffer[p_cb->idx - 1 - elements], elements);
    }
    else {
        double  p1 = max_element(p_cb->buffer, p_cb->idx);
        int     remaining = elements - p_cb->idx;
        double  p2 = max_element(&p_cb->buffer[p_cb->len - 1 - remaining], remaining);

        return p1 > p2 ? p1 : p2;
    }
}