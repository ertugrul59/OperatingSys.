/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for put_user */
#include <linux/slab.h>
#include <linux/sched.h>
#include "charDeviceDriver.h"

#define RESET_SIZE 0

MODULE_LICENSE("GPL");

int lastWritten = 0;
int lastRead = 0;
/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */

struct msg_t {
    char *buf;
    size_t size;
    struct msg_t *next;
};


#define MSGSIZE 4096 // 4K

size_t max_size = 1024*1024*2; // 2MB
size_t overall_size = 0;
struct msg_t *messages = NULL;
struct msg_t *lastMsg = NULL;



DECLARE_WAIT_QUEUE_HEAD(Wait_read_queue); // queue for readers 
DECLARE_WAIT_QUEUE_HEAD(Wait_write_queue); // queue for writers 
spinlock_t msg_lock; // exclusive access to message lists 

static long device_ioctl(struct file *file,	/* see include/linux/fs.h */
		 unsigned int ioctl_num,	/* number and param for ioctl */
		 unsigned long ioctl_param)
{

	/* 
	 * Switch according to the ioctl called 
	 */
	if (ioctl_num == RESET_SIZE) {
	    spin_lock(&Wait_write_queue.lock);
	    if (ioctl_param <= overall_size) {
		spin_unlock(&Wait_write_queue.lock);
		return -EINVAL;
	    }

	    max_size = ioctl_param;
	    spin_unlock(&Wait_write_queue.lock);
	    return 0;

	}

	else {
	    /* no operation defined - return failure */
	    return -EINVAL;

	}
}


/*
 * This function is called when the module is loaded
 */
int init_module(void)
{

    spin_lock_init (&msg_lock);
    Major = register_chrdev(0, DEVICE_NAME, &fops);

    if (Major < 0) {
	printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	return Major;
    }

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
    printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
    printk(KERN_INFO "the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");

    return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
    struct msg_t *tmp;

    /*  Unregister the device */
    unregister_chrdev(Major, DEVICE_NAME);

    /* discard all leftover messages */
    while (messages) {
	tmp = messages->next;
	kfree (messages->buf);
	kfree (messages);
	messages = tmp;
    }
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
    
    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "Closing device \n");

    return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char __user *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
    struct msg_t *tmp;
    int res;
    
    spin_lock(&Wait_read_queue.lock);
    res = wait_event_interruptible_locked (Wait_read_queue, messages);
    if (res == -ERESTARTSYS) {
	spin_unlock(&Wait_read_queue.lock);
	return -EINTR;
    }

    // extract message at the head 
    spin_lock (&msg_lock);
    tmp = messages;
    messages = messages->next;
    overall_size = overall_size - tmp->size;
    if (messages == NULL) {
	lastMsg = NULL;
    }
    spin_unlock (&msg_lock);
    spin_unlock (&Wait_read_queue.lock);

    // wake up waiting writers 
    spin_lock (&Wait_write_queue.lock);
    wake_up_locked (&Wait_write_queue);
    spin_unlock (&Wait_write_queue.lock);


    // now copy message to user space
    res = tmp->size;
    if (length < tmp->size) {
	res = -EFAULT;
	goto out;
    }

    if (copy_to_user (buffer, tmp->buf, tmp->size)) {
	res = -EFAULT;
    }

 out: kfree (tmp->buf);
    kfree (tmp);
    return res;
}
    
    
/* Called when a process writes to dev file */
static ssize_t
device_write(struct file *filp, const char __user *buff, size_t len, loff_t * off)
{
    char *kernelBuffer;
    struct msg_t *msg;
    int res;

    printk(KERN_INFO "write %d\n", (int)len);
    
    if (len > MSGSIZE) {
	return -EINVAL;
    }

    // prepare new list element first 
    kernelBuffer = kmalloc (len, GFP_KERNEL);
    if (!kernelBuffer) {
	return -ENOMEM;
    }

    msg = kmalloc (sizeof (struct msg_t), GFP_KERNEL);
    if (!msg) {
	kfree (kernelBuffer);
	return -ENOMEM;
    }
    
    msg->buf = kernelBuffer;
    if (copy_from_user (msg->buf, buff, len)) {
	kfree (msg);
	kfree (kernelBuffer);
	return -EFAULT;
    }
    msg->next = NULL;
    msg->size = len;

    // wait for free space 
    spin_lock(&Wait_write_queue.lock);
    res = wait_event_interruptible_locked (Wait_write_queue, len + overall_size <= max_size);
    if (res == -ERESTARTSYS) {
	spin_unlock(&Wait_write_queue.lock);
	kfree (msg);
	kfree (kernelBuffer);
	return -EINTR;
    }

    // add element to the list 
    spin_lock (&msg_lock);
    if (lastMsg == NULL) {
	messages = msg;
    }
    else {
	lastMsg->next = msg;
    }
    lastMsg  = msg;
    overall_size = overall_size +len;
    spin_unlock (&msg_lock);
    spin_unlock (&Wait_write_queue.lock);

    // wake up possibly waiting readers 
    spin_lock (&Wait_read_queue.lock);
    wake_up_locked(&Wait_read_queue);
    spin_unlock(&Wait_read_queue.lock);


    return len;

}
