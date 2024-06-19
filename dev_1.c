/****************************
 * Includes
 ***************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/fs.h>
//#include <linux/types.h>
//#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <asm/io.h>
MODULE_LICENSE("GPL");
/**************************
 * Constants
 * ***********************/
#define BASE  			0x1f00000000 
#define PIN			17		  
#define PAD4_CTRL 		0xf0014
#define RIO_ADDRESS  		0xe0000
#define GPIO4_STATUS 		0xd0020
#define GPIO4_CTRL  		0xd0024 
#define WIDTH 			0x100000		
#define DEVICE_NAME 		"GPIO_dev"
#define NUMBER_OF_MAJORS 	1
#define NUMBER_OF_MINORS	50
#define MINOR_NUMBER_BASE 	0
#define MAJOR_NUMBER_BASE 	234
#define SUCCESS 		0

/**************************
 * Function declairations
 **************************/
static int device_open(struct inode* i_node, struct file* flip);
static int device_release(struct inode* i_node, struct file* flip);
static ssize_t device_read(struct file* filp, char* __user, size_t s, loff_t* ptr);
static ssize_t device_write(struct file* filp, const char* __user, size_t s, loff_t* ptr);
static ssize_t device_write_1(struct file* filp, const char* __user, size_t s, loff_t* ptr);

/**************************
 * Globals
 **************************/
static void* mem_ptr = NULL;
static int opened = 0;
static int has_mem = 0;
static struct{
	dev_t dev; 
	struct cdev cdev;
}toy_dev;

static struct file_operations fpos = {
	.open 		= device_open,
	.release	= device_release,
	.read		= device_read,
	.write		= device_write
	
};

static struct file_operations fpos_1 = {
	.open 		= device_open,
	.release	= device_release,
	.read		= device_read,
	.write		= device_write_1
	
};


/***************************
 * Function Definitons
 ***************************/
static int init(void){
	printk(KERN_INFO"Init_function.\n");
	toy_dev.dev = MKDEV(MAJOR_NUMBER_BASE, MINOR_NUMBER_BASE);	
	alloc_chrdev_region(&toy_dev.dev, 0, NUMBER_OF_MINORS, DEVICE_NAME);	
		
	cdev_init(&(toy_dev.cdev), &fpos);
	toy_dev.cdev.owner = THIS_MODULE;

	if(cdev_add(&(toy_dev.cdev), toy_dev.dev, NUMBER_OF_MINORS)){
		goto failed;
	}
	printk(KERN_INFO " cdev struct connected to kernel.\n Major number: %d.\n", MAJOR(toy_dev.dev));	
	//printk(KERN_INFO);
	has_mem = 1;
	if(!request_mem_region(BASE, WIDTH, DEVICE_NAME)){
		printk(KERN_INFO"Requested memory region failed.\n");
		has_mem = 0;
	//	goto failed;
	}
	

//	printk(KERN_INFO"memrequest success.\n");

	mem_ptr = ioremap(BASE, WIDTH);
	if (mem_ptr == NULL){
		printk(KERN_INFO"REMAP_FAILED.\n");
		goto map_failed;
	};
	printk(KERN_INFO"here -> 0x%lx\n", *(unsigned long*)(mem_ptr) );
	u32 func = 0x05; 
	iowrite32(func, mem_ptr + 0xd0000 + PIN*8 + 4 ); /* Set the funtion within the GPIOCTRL0 register to RIO */
	iowrite32(0x02, mem_ptr + 0xf0000 + 4 + PIN*4);   /* Set up pad_ctrl */

	//iowrite32(0x01<<PIN , mem_ptr + RIO_ADDRESS + 0x2000); /* Set the RIO to output a 3.3 volts on pin. */	
	//iowrite32(0x01<<PIN, mem_ptr + RIO_ADDRESS + 0x2000 + 0x04); /* Set to output driver. */


	u32 rio_data = *(u32*)(mem_ptr + RIO_ADDRESS + 0x08); /* Read from RIO register*/
	u32 gpio_status = 0;
	gpio_status = ioread32(mem_ptr + (0xd0000 + PIN*8)); /* 0xd0000 is the offset of the gpio registers. PIN*8 since the ctrl registers are 8 bis appart. */
	printk(KERN_INFO"HERE is the status of gpio%d -> 0x%x\n", PIN, gpio_status);
	printk(KERN_INFO"HERE is RIO  -> 0x%x\n",rio_data);

	return SUCCESS;
map_failed:
	
	release_mem_region(BASE,WIDTH);

failed:
	printk(KERN_ALERT"ERROR: alloc_chardev_region failed, OR request_mem_region failed\n");
	unregister_chrdev_region(toy_dev.dev, NUMBER_OF_MINORS);
	return -1;
}

void clean_up(void){
	cdev_del(&toy_dev.cdev);
	unregister_chrdev_region(toy_dev.dev, NUMBER_OF_MINORS);
	if (mem_ptr)
		iounmap(mem_ptr);
	printk(KERN_INFO"FUNCTION: dev_1_exit(void). \n");
	
	if(has_mem)                             /* We will have a has_mem if the alloc_mem_region was successfull.*/
		release_mem_region(BASE,WIDTH); /* We probably didn't get the go ahead from the kernel*/
                                                /* to do this, but we did it anyway. Thus we are conditionally releasing.*/

}

static int device_open(struct inode* i_node, struct file* flip){
	opened++;
	if(!iminor(i_node))
		flip->f_op = &fpos_1;  /* One file is on, the other is off. */
	else
		flip->f_op = &fpos;

	printk(KERN_INFO "Inside open.\n");
	return 0;
}
static int device_release(struct inode* i_node, struct file* flip){

	printk(KERN_INFO "Inside release.\n");
	return 0;
}
static ssize_t device_read(struct file* filp, char* __user, size_t count, loff_t* ptr)
{
	printk(KERN_INFO "Inside read.\n");
	return 0;
}
static ssize_t device_write(struct file* filp, const char* __user, size_t count, loff_t* ptr)
{
	iowrite32(0x01<<PIN , mem_ptr + RIO_ADDRESS + 0x2000); /* set the RIO to output a 3.3 volts on pin */	
	iowrite32(0x01<<PIN, mem_ptr + RIO_ADDRESS + 0x2000 + 0x04); /* set to output driver */
	iowrite32(0x10, mem_ptr + 0xf0000 + 4 + PIN*4);   /* set up pad_ctrl */
	printk(KERN_INFO "Inside write.\n");
	return count;
}
static ssize_t device_write_1(struct file* filp, const char* __user, size_t count, loff_t* ptr)
{
	
	iowrite32(0x01<<PIN , mem_ptr + RIO_ADDRESS + 0x3000); /* set the RIO to output a 3.3 volts on pin */	
	iowrite32(0x01<<PIN, mem_ptr + RIO_ADDRESS + 0x3000 + 0x04); /* set to output driver */
	iowrite32(0x10, mem_ptr + 0xf0000 + 4 + PIN*4);   /* set up pad_ctrl */
	printk(KERN_INFO "Inside write_1.\n");
	return count;
}
module_init(init);
module_exit(clean_up);
