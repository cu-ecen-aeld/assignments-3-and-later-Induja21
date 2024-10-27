/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/printk.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"
#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdcharbuffer: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif
/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(
    struct aesd_circular_buffer *buffer,
    size_t char_offset,
    size_t *entry_offset_byte_rtn)
{
    // Reset the output variable
    *entry_offset_byte_rtn = 0;

    // Check for an empty buffer condition
    if (buffer->full == false && (buffer->in_offs == buffer->out_offs)) {
        return NULL;
    }

    size_t sum_size = 0; // To accumulate the sizes of entries
    int total_entries_visited = 0; // To track the number of entries visited

    // Start looking for the specified character offset in the buffer
    for (int seek_off = buffer->out_offs; 
         total_entries_visited < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; 
         total_entries_visited++, seek_off = (seek_off + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
    {
        size_t entry_size = buffer->entry[seek_off].size;

        if (entry_size <= char_offset) {
            // Offset exceeds the current entry, move to the next
            char_offset -= entry_size;
        } else {
            // Found the entry containing the requested offset
            *entry_offset_byte_rtn = char_offset; // This is the position within the found entry
            return &buffer->entry[seek_off];
        }
    }

    // If we get here, the offset was not found in any entry
    *entry_offset_byte_rtn = (size_t)-1; // Indicate that no valid entry was found
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    const char *retval = NULL;

    if (buffer == NULL || add_entry == NULL) {
        return retval;
    }

    // Place new entry at the current in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;
    PDEBUG("Buffer entry contentc after copying:");
    for(int i=0;i<buffer->entry[buffer->in_offs].size;i++)
    {
        PDEBUG("%c",buffer->entry[buffer->in_offs].buffptr[i]);
    }
    // If buffer is full, prepare to return the oldest entry (currently at out_offs)
    if (buffer->full) {
        retval = buffer->entry[buffer->out_offs].buffptr;
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Advance in_offs and check if we've filled up the buffer
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // If in_offs catches up with out_offs, the buffer is now full
    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = true;
    } else {
        buffer->full = false;  // Reset full flag if there's still free space
    }

    return retval;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    printk("Init successful\n");
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
