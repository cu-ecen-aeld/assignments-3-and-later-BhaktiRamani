/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd_ioctl.h"
#include <linux/kernel.h>
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Bhakti Ramani"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

int aesd_release(struct inode *inode, struct file *filp);
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static int __init aesd_init_module(void);
static void __exit aesd_cleanup_module(void);
int aesd_open(struct inode *inode, struct file *filp);


struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
   
    /**
     * TODO: handle open
     */
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);

    printk(KERN_INFO "aesdchar:Open file\n");
  
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{

    /**
     * TODO: handle release
     */
    filp -> private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
    


    if (!buf || !filp || !f_pos || *f_pos < 0 || count <= 0) {
        PDEBUG("Invalid input parameters for read syscall");
        return -EINVAL;
    }
    
    struct aesd_dev *dev = filp->private_data;
    size_t entry_offset = 0;
    size_t bytes_copied = 0;
    size_t remaining_count = count;
    //*f_pos = 0;
    
    retval = mutex_lock_interruptible(&dev->buffer_lock);
    if (retval != 0) {
        PDEBUG("Error: Unable to acquire mutex");
        return -ERESTARTSYS;
    }
    
    // Keep reading until we've filled the user buffer or run out of data
    while (remaining_count > 0) {
        struct aesd_buffer_entry *entry = aesd_circular_buffer_find_entry_offset_for_fpos(
                                           &dev->buffer, *f_pos, &entry_offset);
        
        if (!entry || !entry->buffptr) {
            // No more data to read
            break;
        }
        
        // Calculate how many bytes we can read from this entry
        size_t bytes_to_read = entry->size - entry_offset;
        if (bytes_to_read > remaining_count) {
            bytes_to_read = remaining_count;
        }
        
        // Copy data to user space
        if (copy_to_user(buf + bytes_copied, entry->buffptr + entry_offset, bytes_to_read)) {
            PDEBUG("ERROR: Failed to copy data to user space");
            retval = -EFAULT;
            goto unlock_exit;
        }
        
        // Update counters
        *f_pos += bytes_to_read;
        bytes_copied += bytes_to_read;
        remaining_count -= bytes_to_read;
        
        // If we've read to the end of this entry but still have space in the user buffer,
        // we'll continue with the next entry in the next iteration
    }
    
    // If we've read any data, return success
    if (bytes_copied > 0) {
        retval = bytes_copied;
    }


 
    
unlock_exit:
    mutex_unlock(&dev->buffer_lock);
    return retval;
}



static int val = 1;

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld string %d", count, *f_pos, val);
    val += 1;
  
    
    // Validate parameters
    if (!buf || !filp || !f_pos || *f_pos < 0 || count <= 0) {
        PDEBUG("Invalid input parameters for write syscall");
        return -EINVAL;
    }
    
    struct aesd_dev *dev = filp->private_data;
    if (dev == NULL) {
        PDEBUG("ERROR: Invalid filp ptr");
        return -EINVAL;
    }
    
    // Allocate buffer for this write operation
    char *tmp_buf = kmalloc(count, GFP_KERNEL);
    if (tmp_buf == NULL) {
        PDEBUG("ERROR: kmalloc failed for write operation");
        return -ENOMEM;
    }
    
    // Copy data from user space
    if (copy_from_user(tmp_buf, buf, count)) {
        PDEBUG("ERROR: Copy from user space failed");
        kfree(tmp_buf);
        return -EFAULT;
    }

    // Lock the buffer
    retval = mutex_lock_interruptible(&dev->buffer_lock);
    if (retval != 0) {
        PDEBUG("ERROR: Failed to acquire mutex lock");
        kfree(tmp_buf);
        return -ERESTARTSYS;
    }
    
    // Check for newline in the current write
    bool has_newline = memchr(tmp_buf, '\n', count) != NULL;
    
    // Case 1: No partial write and this write has a newline
    if (dev->partial_write == NULL && has_newline) {
        // Add directly to the circular buffer
        struct aesd_buffer_entry new_entry;
        new_entry.buffptr = tmp_buf;
        new_entry.size = count;
        
        aesd_circular_buffer_add_entry(&dev->buffer, &new_entry);
      
        
        retval = count;
        tmp_buf = NULL; // Prevent freeing as it's now owned by the buffer
    }
    // Case 2: We have a partial write or this write doesn't have a newline
    else {
        char *combined_buf;
        size_t combined_size;
        
        if (dev->partial_write) {
            // Append this write to existing partial
            PDEBUG("Partial Write");
            combined_size = dev->partial_size + count;
            combined_buf = krealloc(dev->partial_write, combined_size, GFP_KERNEL);
            
            if (!combined_buf) {
                PDEBUG("ERROR: krealloc failed");
                retval = -ENOMEM;
                goto unlock_exit;
            }
            
            memcpy(combined_buf + dev->partial_size, tmp_buf, count);
            kfree(tmp_buf); // Free tmp_buf as we've copied it
            tmp_buf = NULL;
        } else {
            // This is the first part of a partial write
            combined_buf = tmp_buf;
            combined_size = count;
            tmp_buf = NULL; // Prevent freeing as we're keeping it
        }
        
        if (has_newline) {
            // We now have a complete entry to add
            struct aesd_buffer_entry new_entry;
            new_entry.buffptr = combined_buf;
            new_entry.size = combined_size;
            
            aesd_circular_buffer_add_entry(&dev->buffer, &new_entry);

            // Reset partial tracking
            dev->partial_write = NULL;
            dev->partial_size = 0;
        } else {
            // Still accumulating
            dev->partial_write = combined_buf;
            dev->partial_size = combined_size;
        }
        
        retval = count;
    }

unlock_exit:
    mutex_unlock(&dev->buffer_lock);
    
    // Free tmp_buf if we didn't store it
    if (tmp_buf) {
        kfree(tmp_buf);
    }
    
    return retval;
}


static loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    loff_t new_fpos = 0;
    loff_t total_size = 0;

    PDEBUG("Inside llseek");
    mutex_lock(&dev->buffer_lock);

    // Calculate total bytes stored in buffer
    struct aesd_buffer_entry *entry;
    uint8_t index;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, index) {
        total_size += entry->size;
    }

    switch (whence) {
        case SEEK_SET:
            new_fpos = offset;
            break;
        case SEEK_CUR:
            new_fpos = filp->f_pos + offset;
            break;
        case SEEK_END:
            new_fpos = total_size + offset;
            break;
        default:
            mutex_unlock(&dev->buffer_lock);
            return -EINVAL;
    }

    // Validate bounds
    if (new_fpos < 0 || new_fpos > total_size) {
        mutex_unlock(&dev->buffer_lock);
        return -EINVAL;
    }

    filp->f_pos = new_fpos;

    mutex_unlock(&dev->buffer_lock);

    PDEBUG("llseek: offset=%lld whence=%d -> new pos=%lld", offset, whence, new_fpos);
    return new_fpos;
}

static long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    loff_t new_fpos = 0;
    uint8_t index;
    uint32_t i;

    PDEBUG("Inside ioctl");
    if (cmd != AESDCHAR_IOCSEEKTO)
        return -ENOTTY;

    // Copy struct from userspace
    if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)))
        return -EFAULT;

    mutex_lock(&dev->buffer_lock);

    // Check if command index is out of bounds
    uint8_t total_commands = dev->buffer.full ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED :
                                                dev->buffer.in_offs;

    if (seekto.write_cmd >= total_commands) {
        mutex_unlock(&dev->buffer_lock);
        return -EINVAL;
    }

    // circular buffer overwrite condition check
    index = (dev->buffer.out_offs + seekto.write_cmd) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    struct aesd_buffer_entry *entry = &dev->buffer.entry[index];

    if (!entry->buffptr || seekto.write_cmd_offset >= entry->size) {
        mutex_unlock(&dev->buffer_lock);
        return -EINVAL;
    }

    // Calculate byte offset 
    for (i = 0; i < seekto.write_cmd; i++) {
        uint8_t idx = (dev->buffer.out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        new_fpos += dev->buffer.entry[idx].size;
    }

    new_fpos += seekto.write_cmd_offset;
    filp->f_pos = new_fpos;
    PDEBUG("IOCTL new file position : %d", filp->f_pos);

    mutex_unlock(&dev->buffer_lock);
    return 0;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_unlocked_ioctl,
    .release =  aesd_release,

};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev\n", err);
    }
    return err;
}



static int __init aesd_init_module(void)
{
   
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    // Initialize the AESD specific portion of the device
    aesd_circular_buffer_init(&aesd_device.buffer);
    mutex_init(&aesd_device.buffer_lock);
    aesd_device.partial_write = NULL;
    aesd_device.partial_size = 0;

    result = aesd_setup_cdev(&aesd_device);
    if (result) {
        unregister_chrdev_region(dev, 1);
    }
 
    return result;
}

static void __exit aesd_cleanup_module(void)
{
  
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    // Cleanup AESD specific portions
    if (aesd_device.partial_write) {
        kfree(aesd_device.partial_write);
        aesd_device.partial_write = NULL;
    }

    struct aesd_buffer_entry *entry;
    uint8_t index;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
        if (entry->buffptr) {
            kfree((void *)entry->buffptr);
            entry->buffptr = NULL;
        }
    }
    
    mutex_destroy(&aesd_device.buffer_lock);
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);




 
