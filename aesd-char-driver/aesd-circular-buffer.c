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
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

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
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    // Check if inputs are valid pointers
    if (buffer == NULL || entry_offset_byte_rtn == NULL) {
        return NULL;
    }
    
    // Track accumulated size as we traverse the buffer
    size_t accumulated_size = 0;
    size_t current_index = buffer->out_offs;
    size_t entry_size;
    
    // Count filled entries to limit our search
    size_t entries_to_check = 0;
    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        if (buffer->entry[(buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED].buffptr != NULL) {
            entries_to_check++;
        }
    }
    
    // Traverse the buffer looking for the correct entry
    for (size_t i = 0; i < entries_to_check; i++) {
        entry_size = strlen(buffer->entry[current_index].buffptr);
        
        if (char_offset >= accumulated_size && char_offset < accumulated_size + entry_size) {
            // Found the entry containing our target position
            *entry_offset_byte_rtn = char_offset - accumulated_size;
            return &buffer->entry[current_index];
        }
        
        // Move to next entry
        accumulated_size += entry_size;
        current_index = (current_index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    
    // Position not found in buffer
    *entry_offset_byte_rtn = 0;
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
       // Check if inputs are valid pointers
       if (buffer == NULL || add_entry == NULL) {
        return;
    }
    
    // Store current state before modification
    size_t current_in = buffer->in_offs;
    
    // Calculate next input position
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    
    // Copy the entry to the buffer at the current input position
    buffer->entry[current_in] = *add_entry;
    
    // Determine if we need to update out_offs based on buffer state
    if (buffer->full) {
        // Buffer was already full, so move the output pointer
        buffer->out_offs = buffer->in_offs;
    } else if (buffer->in_offs == buffer->out_offs) {
        // We've just filled the buffer completely
        buffer->full = true;
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
