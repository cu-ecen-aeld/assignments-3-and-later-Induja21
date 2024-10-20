/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer implementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset. Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset. This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn)
{
    // Check if inputs are valid pointers
    if (buffer == NULL || entry_offset_byte_rtn == NULL) {
        return NULL;
    }

    // Initialize variables
    int lengthOfString = 0;
    int total_entries_visited = 0;
    char_offset++; // Adjusting for zero-referenced index


    // First check the out_offs buffer entry and check the length of buffer. 
    // 1) If length is < actual offset move to next buffer and reduce the length -= strlen(buffer); Go to next circular before pos
    // else 2) retrun len -1 ;(which will be the pos of corresponding buffer)
    // Iterate over the circular buffer
    for (int seek_off = buffer->out_offs; total_entries_visited < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; 
         total_entries_visited++, seek_off = (seek_off + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
    {

        lengthOfString = strlen(buffer->entry[seek_off].buffptr);
        if (lengthOfString < char_offset) {
            char_offset -= lengthOfString;
        } else {
            *entry_offset_byte_rtn = char_offset - 1;
            return &buffer->entry[seek_off];
        }
    }

    *entry_offset_byte_rtn = (size_t)-1; // Indicate no valid entry found
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location. Any necessary locking must be handled by the caller.
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    // Check if inputs are valid pointers
    if (buffer == NULL || add_entry == NULL) {
        return;
    }

    // Add the entry to the buffer
    buffer->entry[buffer->in_offs] = *add_entry;

    // Check if buffer is full
    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = true;
    }

    // If buffer is full, update the out_offs offset
    if (buffer->full) {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Update the in_offs offset
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
}
