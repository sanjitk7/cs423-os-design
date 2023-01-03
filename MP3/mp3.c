#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp3_given.h"
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>	
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sanjitk3");
MODULE_DESCRIPTION("CS-423 MP3");

#define DEBUG 1


struct mp3_task_struct {
	struct task_struct *linux_task;
	struct list_head list;
	unsigned long pid;
};

LIST_HEAD(registered_process_list);

// declaring proc entry file and dir pointers
static struct proc_dir_entry *dir_entry;
static struct proc_dir_entry *mp3_dir;

//lock
spinlock_t list_mutex;

// workqueue
static struct workqueue_struct *my_workqueue;
void my_workqueue_function(struct work_struct *work);
DECLARE_DELAYED_WORK(my_work,my_workqueue_function);
// static struct work_struct *my_work;

// declare callback
static int cdev_mmap_callback(struct file *f, struct vm_area_struct *vma);

//vmalloc buf
unsigned long* buf;
unsigned long buf_pos = 0;
int count=0;

// workqueue callback
void my_workqueue_function(struct work_struct *work){
	printk(KERN_INFO "wWorkqueue Function Called!\n");
	
	//get stats from get_cpu_use()
	unsigned long maj_pf=0,min_pf=0, utime=0,stime=0,t_maj_pf = 0, t_min_pf = 0,t_cpu_time = 0;
	struct mp3_task_struct *pos;
	if (list_empty(&registered_process_list)){
		printk(KERN_INFO "Inside WQ - List Empty!\n");
	}
	
	// sample data
	list_for_each_entry(pos,&registered_process_list,list){
		//printk(KERN_INFO "for PID: %lu\n",pos->pid);
		if(get_cpu_use(pos->pid,&min_pf,&maj_pf,&utime,&stime)!=-1){
			t_maj_pf += maj_pf;
			t_min_pf += min_pf;
			t_cpu_time = t_cpu_time + stime + utime;
			//printk(KERN_INFO "stime:%lu\tutime:%lu\n",stime,utime);
		} else {
			printk(KERN_INFO "get_cpu_use() failed!\n");
		}
	}
	
	printk(KERN_INFO "STATS:maj_pf:%lu, min_pf:%lu, cpu_util: %lu",t_maj_pf, t_min_pf,t_cpu_time);
	
	// write to shared mem area
	printk(KERN_INFO "buf_pos before: %lu",buf_pos);
	buf[buf_pos + 0] = (unsigned long) jiffies;
	buf[buf_pos + 1] = (unsigned long) t_min_pf;
	buf[buf_pos + 2] = (unsigned long) t_maj_pf;
	buf[buf_pos + 3]  = (unsigned long) t_cpu_time; // nanoseconds
	
	printk(KERN_INFO "buf_pos after 1: %lu",buf_pos);
	buf_pos+=4;
	printk(KERN_INFO "buf_pos after 2: %lu",buf_pos);
	if (4 * buf_pos >= 128*4*1024) buf_pos = 0;
	printk(KERN_INFO "buf_pos after 3: %lu",buf_pos);
	
	
	//checking buffer values
	int i=0;
	if(count<4){
	printk(KERN_INFO "****CHECK BUF VALUES ON 1 SAMPLE WRITE START****\n");
		while(i<buf_pos){
			printk(KERN_INFO "%lu %lu %lu %lu\n",buf[i],buf[i+1],buf[i+2],buf[i+3]);
			i+=4;
		}
	printk(KERN_INFO "****CHECK BUF VALUES ON 1 SAMPLE WRITE END****\n");
	count+=1;
	}
 	
	// delay is 1000ms/20samplespersec = 50ms intervals
	queue_delayed_work(my_workqueue,&my_work,msecs_to_jiffies(50));
	printk(KERN_INFO "wWorkqueue Function End!\n");
}

static ssize_t mywrite(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos){

	printk(KERN_INFO "Write Callback Called! \n");
	
	char *mybuf = (char *)kmalloc(count,GFP_KERNEL);
	if(copy_from_user(mybuf,ubuf,count)!=0){
		printk(KERN_INFO "copy_from_user failed!\n");
		return -1;
	}
	
	char op;
	unsigned int pid;
	
	if(sscanf(strsep(&mybuf," "),"%c",&op)!=1){
		printk(KERN_INFO "op scan failed!\n");
	}
	
	if(sscanf(strsep(&mybuf," "),"%u",&pid)!=1){
		printk(KERN_INFO "pid scan failed!\n");
	}
	
	switch(op){
		case 'R':
			// add new node to registerd process linked list
			printk(KERN_INFO "Register Section Start!\n");
			struct mp3_task_struct *tmp = (struct mp3_task_struct *)kmalloc(sizeof(struct mp3_task_struct),GFP_KERNEL);
			tmp->pid = pid;
			tmp->linux_task=find_task_by_pid(pid);
			
			INIT_LIST_HEAD(&tmp->list);
			
			if (list_empty(&registered_process_list)){
				printk(KERN_INFO "List is empty! Creating Workqueue Job!\n");
				queue_delayed_work(my_workqueue,&my_work,msecs_to_jiffies(50));
			}
			
			spin_lock(&list_mutex);
			list_add_tail(&tmp->list, &registered_process_list);
			spin_unlock(&list_mutex);
			
			printk(KERN_INFO "Register Section End!\n");	
			break;
			
		case 'U':
			printk(KERN_INFO "Unregister Section Start\n");
			struct mp3_task_struct *pos, *n;
    			// iterate though list till correct task via pid is found and delete entry from list
    			spin_lock(&list_mutex);
			list_for_each_entry_safe(pos,n,&registered_process_list,list){
				if(pos->pid == pid){
					list_del(&pos->list);
					kfree(pos);
				}
			}
			if (list_empty(&registered_process_list)){
				printk(KERN_INFO "List is empty! Deleting Workqueue Job!\n");
				cancel_delayed_work(&my_work);
				flush_workqueue(my_workqueue);
			}
			spin_unlock(&list_mutex);
			break;
			
	
	}
	kfree(mybuf);
	return count;
}


static ssize_t myread(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){
	printk(KERN_INFO "Read Callback Called!\n");
	
	int copy_offset = 0;
	char *mybuf = (char *)kmalloc(count,GFP_KERNEL);
	static bool flag_check_one_loop = false;


	// fixes infinite loop of iterating doubly linked list by cheking if myread only works once
	if (flag_check_one_loop==true){
		flag_check_one_loop = false;
		return 0;
	} else {
		flag_check_one_loop = true;
	}
	

	
	struct mp3_task_struct *pos;
	
    	spin_lock(&list_mutex);
    	printk(KERN_INFO "Entering MP3 Read CS\n");
    	// Critical Section
	list_for_each_entry(pos, &registered_process_list,list){
   		copy_offset += sprintf( mybuf + copy_offset, "******\npid: %lu exists in list!\n******\n", pos->pid);
   }
   	printk(KERN_INFO "Exiting MP3 Read CS\n");
   	spin_unlock(&list_mutex);
   	if(copy_to_user(ubuf, mybuf, copy_offset)!=0){
   		printk("copy_to_user failed!\n");
   		return -1;
   	}
   	

   	
   	kfree(mybuf);
	return copy_offset;
}

// set proc_operations to pass callback functions
static struct proc_ops file_ops = {
	.proc_read = myread,
	.proc_write = mywrite,
};

// set ops for character device
static const struct file_operations cdev_fops = {
    .owner = THIS_MODULE,
    .open = NULL,
    .release = NULL,
    .mmap = cdev_mmap_callback
};

// character device callback called when user monitor process uses mmap that maps to shared memory buffer.

static int cdev_mmap_callback(struct file *f, struct vm_area_struct *vma) {
    unsigned long pg;
    unsigned long itr;
    unsigned long str  = vma->vm_start;
    unsigned long total_size = vma->vm_end - vma->vm_start;

    itr = (unsigned long)buf;
    unsigned long curr_fill = 0;
    while (curr_fill < total_size) {
        pg = vmalloc_to_pfn((void *)itr); 
        if (remap_pfn_range(vma, str, pg, PAGE_SIZE, PAGE_SHARED))
            return -1;

        str += PAGE_SIZE;
        itr += PAGE_SIZE;
        if ((total_size - curr_fill) > PAGE_SIZE)
            curr_fill += PAGE_SIZE;
        else
            curr_fill = total_size;
    }

    return 0;
}


//character device

dev_t device;
struct cdev mp3_cdev;

// mp1_init - Called when module is loaded
int __init mp3_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP3 MODULE LOADING\n");
   #endif
   // Insert your code here ...
   
   // initialising proc interface
   printk(KERN_INFO "Hello Workl from Kernel.");
   mp3_dir = proc_mkdir("mp3",NULL);
   dir_entry = proc_create("status",0666, mp3_dir, &file_ops);
   
   // init spin lock
   spin_lock_init(&list_mutex);
   
   //create workqueue
   my_workqueue = create_singlethread_workqueue("my_workqueue");
   
   //create common memory buffer
   // not setting PG_reserved bc it is handled by VMA in recent kernels
   buf = (unsigned long*) vmalloc(128*4*1024); //(>=128x4kb)
   if(buf==0){
   	printk(KERN_INFO "vmalloc failed :(\n");
   }
   memset(buf,0,128*4*1024);
   printk(KERN_INFO "buf sizeof: %lu",sizeof(buf));
   
   // character device init
   int check;
   printk(KERN_INFO "major number is statically defined as 423!\n");
   device = MKDEV(423, 0);
   check = register_chrdev_region(device, 1, "node");
   if (check) {
	    printk(KERN_ERR "alloc_chrdev_region failed :(\n");
	    return -1;
   }
   
    int dev_major = MAJOR(device);
    printk(KERN_INFO "major number: %d\n",dev_major);
    cdev_init(&mp3_cdev, &cdev_fops);
    mp3_cdev.owner = THIS_MODULE;
    mp3_cdev.ops = &cdev_fops;
    if(cdev_add(&mp3_cdev, device, 1)<0){
        printk(KERN_INFO "cdev_add failed!\n");
    }
   
   printk(KERN_ALERT "MP3 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp3_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP3 MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
   proc_remove(dir_entry);
   proc_remove(mp3_dir);
   
   // delete linked list of tasks
   struct mp3_task_struct *pos, *n;
   spin_lock(&list_mutex);
   list_for_each_entry_safe(pos,n, &registered_process_list,list){
   	printk(KERN_INFO "0\n");
   	list_del(&pos->list);
   	printk(KERN_INFO "000\n");
   }
     spin_unlock(&list_mutex);
     
     //character device del
     cdev_del(&mp3_cdev);
     unregister_chrdev_region(device, 1);
     
   // delete workqueue
   destroy_workqueue(my_workqueue);
   //kfree(my_work);
   
   // free vmalloc buf   
   
   vfree(buf);
   printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
 
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);
