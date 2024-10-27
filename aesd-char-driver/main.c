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
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Induja Narayanan"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    filp->private_data =NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
    
    // Validate input
    if (!filp || !buf || !f_pos || *f_pos < 0) {
        PDEBUG("Improper arguments");
        return -EINVAL;
    }

    struct aesd_dev *dev_buffer = filp->private_data;
    size_t offset = 0;
    size_t remaining_bytes = 0;

    // Lock
    retval = mutex_lock_interruptible(&dev_buffer->lock);
    if (retval != 0) {
        return -ERESTART;
    }

    struct aesd_buffer_entry *temp = aesd_circular_buffer_find_entry_offset_for_fpos(&dev_buffer->buffer, *f_pos, &offset);
    if (!temp) {
        PDEBUG("Error: Entry for given position not found");
        retval = -EINVAL;
        goto unlock;
    }

    remaining_bytes = temp->size - offset;
    if (remaining_bytes > count) {
        remaining_bytes = count; // Prevent overflow
    }

    retval = copy_to_user(buf, temp->buffptr + offset, remaining_bytes);
    if (retval != 0) {
        remaining_bytes -= retval; // Adjust remaining bytes
        PDEBUG("Copying data to user space failed");
        retval = -EFAULT;
        goto unlock;
    }

    *f_pos += remaining_bytes; // Update position
    retval = remaining_bytes; // Successfully read this many bytes

unlock:
    mutex_unlock(&dev_buffer->lock);
    PDEBUG("Mutex unlocked");
    return retval;
}
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char *mem_ptr = NULL;
    char *newline_ptr = NULL;
    size_t size_to_fill_in_buffer = 0, new_size = 0;
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev_buffer = filp->private_data; // Pointer to dev_buffer

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    // Validate input parameters
    if (filp == NULL || buf == NULL || f_pos == NULL || count <= 0 || *f_pos<0)
    {
        retval = -EINVAL;
        goto exit;
    }

    // Allocate memory for the incoming data
    mem_ptr = kmalloc(count, GFP_KERNEL);
    if (mem_ptr == NULL)
    {
        PDEBUG("Memory allocation failed in kernel\n");
        retval = -ENOMEM;
        goto exit;
    }

    // Copy data from user space to kernel space
    if (copy_from_user(mem_ptr, buf, count))
    {
        PDEBUG("Copy from user space failed in kernel\n");
        retval = -EFAULT;
        goto free_and_exit;
    }
    PDEBUG("String copied is:");
    for(int i=0;i<count;i++)
    {
         PDEBUG("%c",mem_ptr[i]);
    }
    // Check for newline character in the buffer
    newline_ptr = memchr(mem_ptr, '\n', count);
    if (newline_ptr != NULL)
    {
        // Newline character found
        size_to_fill_in_buffer = newline_ptr - mem_ptr + 1;
         PDEBUG("Size to fill in buffer is %ld",size_to_fill_in_buffer);
    }

    // Locking to ensure safe access to shared resources
    retval = mutex_lock_interruptible(&dev_buffer->lock);
    if (retval != 0)
    {
        retval = -ERESTART;
        PDEBUG("Unable to acquire lock\n");
        goto free_and_exit;
    }

    if (size_to_fill_in_buffer > 0)
    {
        dev_buffer->new_entry.buffptr = mem_ptr;
        dev_buffer->new_entry.size = size_to_fill_in_buffer;
        // If a newline was found, add the pointer to circular buffer
        struct aesd_buffer_entry *entry = aesd_circular_buffer_add_entry(&dev_buffer->buffer, &dev_buffer->new_entry);

        // Free entry
        if (entry != NULL) {
            PDEBUG("Freeing entry\n");
            kfree(entry); // Free old entry if necessary
        }

        PDEBUG("Resetting buffptr\n");

        // Reset temp buffer after successfully processing the command
        dev_buffer->new_entry.buffptr = NULL;  // Reset pointer
        dev_buffer->new_entry.size = 0;         // Reset size

        retval = size_to_fill_in_buffer; // Number of bytes written
    }
    else
    {
        // No newline character found, buffer the data for future writes
        new_size = dev_buffer->new_entry.size + count; // New size for temp buffer

        // Reallocate memory for the temporary buffer
        char *temp_realloc_ptr = krealloc(dev_buffer->new_entry.buffptr, new_size, GFP_KERNEL);
        if (temp_realloc_ptr == NULL)
        {
            PDEBUG("Realloc failed during write.");
            retval = -ENOMEM;
            goto free_mutex; // Free mutex before exiting
        }

        dev_buffer->new_entry.buffptr = temp_realloc_ptr; // Update pointer to new memory
        memcpy(dev_buffer->new_entry.buffptr + dev_buffer->new_entry.size, mem_ptr, count); // Copy new data to the end of the buffer
        dev_buffer->new_entry.size += count; // Update the size of the temporary buffer

        PDEBUG("Buffered %zu bytes; total size now %zu", count, dev_buffer->new_entry.size);
        retval = count; // Return the number of bytes written
    }


free_mutex:
    mutex_unlock(&dev_buffer->lock); // Unlock the mutex
free_and_exit:
    kfree(mem_ptr); // Free the allocated memory for incoming data
exit:
    return retval; // Return the result
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
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
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");

    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    printk(KERN_INFO, "AESD Init module");
    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.buffer);
    mutex_init(&aesd_device.lock); // Initialize the mutex


    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);
    struct aesd_buffer_entry *entry;
    uint8_t index = 0;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index){
    if(entry->buffptr != NULL){
        kfree(entry->buffptr);
    }
    }

    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);