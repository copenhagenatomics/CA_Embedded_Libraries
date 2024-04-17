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
** @brief Returns the average value of an array of double
*/
int mean_element(double arr[], unsigned len, double* result) {
    if(len != 0) {
        double ret_val = arr[0];
        for(int i = 1; i < len; i++) {
            ret_val += arr[i];
        }
        *result = ret_val / len;
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
int cbMean(double_cbuf_handle_t p_cb, int elements, double* result) {
    if(elements > p_cb->len) {
        return -1;
    }
    else if(elements == p_cb->len) {
        return mean_element(p_cb->buffer, p_cb->len, result);
    }
    else if(elements <= p_cb->idx) {
        /* The current index is un-occupied, so start from the previous one */
        return mean_element(&p_cb->buffer[p_cb->idx - elements], elements, result);
    }
    else {
        double p1, p2;
        int err = mean_element(p_cb->buffer, p_cb->idx, &p1);

        if(err != 0) {
            return err;
        }

        int remaining = elements - p_cb->idx;
        err = mean_element(&p_cb->buffer[p_cb->len - remaining], remaining, &p2);

        *result = p1 + p2;
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