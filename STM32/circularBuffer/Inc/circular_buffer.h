#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <stdio.h>       // Needed for size_t
#include <stdbool.h>     // Needed for bool

#ifdef __cplusplus
  extern "C" {
#endif

/// Circular buffer structure
typedef struct circular_buf_t {
    uint8_t * buffer;
    size_t head;
    size_t tail;
    size_t max;
    bool full;
} circular_buf_t;

/// Handle type, the way users interact with the API
typedef circular_buf_t* cbuf_handle_t;

/// Pass in a storage buffer and size, returns a circular buffer handle
/// Requires: buffer is not NULL, size > 0
/// Ensures: cbuf has been created and is returned in an empty state
cbuf_handle_t circular_buf_init(size_t size);

/// Pass in a storage buffer and size, returns a circular buffer handle
/// Requires: buffer is not NULL, size > 0
/// Ensures: cbuf has been created and is returned in an empty state
cbuf_handle_t circular_buf_init_static(circular_buf_t* cb, uint8_t* buf, size_t size);

/// Free a circular buffer structure
/// Requires: cbuf is valid and created by circular_buf_init
/// Does not free data buffer; owner is responsible for that
void circular_buf_free(cbuf_handle_t cbuf);

/// Reset the circular buffer to empty, head == tail. Data not cleared
/// Requires: cbuf is valid and created by circular_buf_init
void circular_buf_reset(cbuf_handle_t cbuf);

/// Put new data if the buffer is full.
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns 0 on success, -1 if buffer is full
int circular_buf_put(cbuf_handle_t cbuf, uint8_t data);

/// Retrieve a value from the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns 0 on success, -1 if the buffer is empty
int circular_buf_get(cbuf_handle_t cbuf, uint8_t * data);

/// CHecks if the buffer is empty
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns true if the buffer is empty
bool circular_buf_empty(cbuf_handle_t cbuf);

/// Checks if the buffer is full
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns true if the buffer is full
bool circular_buf_full(cbuf_handle_t cbuf);

/// Check the capacity of the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the maximum capacity of the buffer
size_t circular_buf_capacity(cbuf_handle_t cbuf);

/// Check the number of elements stored in the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the current number of elements in the buffer
size_t circular_buf_size(cbuf_handle_t cbuf);

#ifdef __cplusplus
  }
#endif

#endif //CIRCULAR_BUFFER_H_
