#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp1_given.h"
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sanjitk3");
MODULE_DESCRIPTION("CS-423 MP1");



#define DEBUG 1

// declaring proc entry file and dir pointers
static struct proc_dir_entry *dir_entry;
static struct proc_dir_entry *mp1_dir;

// declaring timer
static struct timer_list cpu_timer;

// declare workqueue
static struct workqueue_struct *my_workqueue;
static struct work_struct *my_work;

// process linked list structure and init
struct process_struc {
	struct list_head list;
	int pid;
	unsigned long user_cpu_time;
};

LIST_HEAD(registered_process_list);

struct list_head process_list;


static ssize_t mywrite(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos){

	printk(KERN_INFO "Write Callback Called! \n");
	
	char *mybuf = (char *)kmalloc(count,GFP_KERNEL);
	copy_from_user(mybuf,ubuf,count);
	
	// add new node to registerd process linked list
	
	struct process_struc *tmp = (struct process_struc *)kmalloc(sizeof(struct process_struc),GFP_KERNEL);
    
    	// setting temp node values for pid and user_cpu_time
    	sscanf( mybuf, "%d", &tmp->pid );
    	tmp->user_cpu_time = 0;
    
    	INIT_LIST_HEAD(&tmp->list);
    	
    	struct mutex list_mutex;
	mutex_init(&list_mutex);
    	
    	mutex_lock(&list_mutex);
	// Critical Section
	list_add(&tmp->list, &registered_process_list);

    	mutex_unlock(&list_mutex);
	
	kfree(mybuf);
	return -1;
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
	

	
	struct process_struc *pos, *n;
	
    	struct mutex list_mutex;
	mutex_init(&list_mutex);
    	
    	mutex_lock(&list_mutex);
    	// Critical Section
	list_for_each_entry(pos, &registered_process_list,list){
   		copy_offset += sprintf( mybuf + copy_offset, " %d : %lu \n", pos->pid, pos->user_cpu_time);
   }
   	copy_to_user(ubuf, mybuf, copy_offset);
   	mutex_unlock(&list_mutex);

   	
   	kfree(mybuf);
	return copy_offset;
}

// timer callback

void cpu_timer_callback(struct timer_list *timer){
	printk(KERN_INFO "Timer Callback Invoked!\n");
	
	queue_work(my_workqueue,my_work);
}

// workqueue callback
static void my_workqueue_function(struct work_struct *work){
	printk(KERN_INFO "wWorkqueue Function Called!\n");
	
	struct process_struc *pos, *n;
	int flag_get_cpu_use;
	
	struct mutex list_mutex;
	mutex_init(&list_mutex);
    	
    	mutex_lock(&list_mutex);
	
	list_for_each_entry_safe(pos,n, &registered_process_list,list){
		flag_get_cpu_use = get_cpu_use(pos->pid, &pos->user_cpu_time);
	   	// printk(KERN_INFO "the value of get_cpu_use = %d",flag_get_cpu_use);
	   	// printk(KERN_INFO "the value of user_cpu_time = %lu", pos->user_cpu_time);
	   	// remove entry if get_cpu_use returns -1 since the process has terminated
	   	if (flag_get_cpu_use == -1){
	   		list_del(&pos->list);
   			kfree(pos);
	   	}
   }
	mutex_unlock(&list_mutex);
	// restart timer to 5 seconds on completion
	mod_timer(&cpu_timer, jiffies + msecs_to_jiffies(5000));
	
}

// set proc_operations to pass callback functions
static struct proc_ops file_ops = {
	.proc_read = myread,
	.proc_write = mywrite,
};

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   // Insert your code here ...
   
   mp1_dir = proc_mkdir("mp1",NULL);
   dir_entry = proc_create("status",0666, mp1_dir, &file_ops);
  
   // setup timer
   timer_setup(&cpu_timer,cpu_timer_callback,0);
   mod_timer(&cpu_timer, jiffies + msecs_to_jiffies(5000));
   
   // create workqueue
   my_workqueue = create_workqueue("my_workqueue");
   my_work = (struct work_struct *)kmalloc(sizeof(struct work_struct),GFP_KERNEL);
   INIT_WORK(my_work,my_workqueue_function);
   
   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
   
   struct process_struc *pos, *n;
   
   proc_remove(dir_entry);
   proc_remove(mp1_dir);
   
   // delete linked list
   
   list_for_each_entry_safe(pos,n, &registered_process_list,list){
   	list_del(&pos->list);
   	kfree(pos);
   }
   
   // delete workqueue
   flush_workqueue(my_workqueue);
   destroy_workqueue(my_workqueue);
   kfree(my_work);
   
   
   // delete timer
   del_timer(&cpu_timer);
   

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
