#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>  // For file_operations

struct scull_dev {
    int size;  // Simulated device size
};

static int driver_open(struct inode *device_file, struct file *instance) {
	printk("lseek - open was called!\n");
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance) {
	printk("lseek - close was called!\n");
	return 0;
}



// static int test_open(struct inode *inode, struct file *filp) {
//     struct scull_dev *dev = container_of(inode->i_cdev, struct scull_dev, cdev);
//     filp->private_data = dev;
//     return 0;
// }




static loff_t my_lseek(struct file *filp, loff_t offset, int cmd){
    struct scull_dev *dev = filp -> private_data;
    int new_pos;

    switch(cmd){
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = filp->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = dev -> size + offset;
            break;
        default:
            return -EINVAL;
            break;
    }
    if(new_pos < 0) return -EINVAL;
    filp->f_pos = new_pos;
    return new_pos;
}

struct file_operations test_fops = {
    .owner =    THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .llseek = my_lseek
    
};

static int __init test_module_init(void) {
    printk(KERN_INFO "Test module: Loaded\n");
    return 0;
}

static void __exit test_module_exit(void) {
    printk(KERN_INFO "Test module: Unloaded\n");
}

module_init(test_module_init);
module_exit(test_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple test module");