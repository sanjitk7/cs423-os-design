#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp2_given.h"
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define RUNNING 0
#define READY 1
#define SLEEPING 2

//enum task_state{SLEEPING,RUNNING,READY};

struct mp2_task_struct {
	struct task_struct *linux_task;
	struct timer_list t_timer;
	struct list_head list;
	unsigned int pid;
	int processing_time;
	unsigned long period_time;
	//enum task_state process_state;
	unsigned int process_state;
	unsigned long ddl;
};

LIST_HEAD(mp2_tasklist_head);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG 1
static DEFINE_SPINLOCK(lock1);

// declaring proc entry file and dir pointers
static struct proc_dir_entry *dir_entry;
static struct proc_dir_entry *mp2_dir;

// initialise mp2_task_struct and cache
static struct mp2_task_struct mp2_task_list;
static struct kmem_cache *mp2_cache;


// for scheduler 
static struct mp2_task_struct *current_task;
static struct task_struct* dispatch_thread;

static struct mp2_task_struct* get_highest_priority_task(void){
printk(KERN_INFO "checking highest priority!\n");
	struct mp2_task_struct *highest_task = NULL;
	struct mp2_task_struct *pos;
	
	printk(KERN_INFO "check 0000!\n");
	// mutex lock
	struct mutex list_mutex;
	mutex_init(&list_mutex);
	mutex_lock_interruptible(&list_mutex);
	printk(KERN_INFO "check 000!\n");
	list_for_each_entry(pos, &mp2_tasklist_head, list) {
        // the task with READY state
        if (pos->process_state == READY) {
        printk(KERN_INFO "checking for highest p task!\n");
            if (highest_task == NULL) {
            	printk(KERN_INFO "check 010!\n");
                highest_task = pos;
            } else {
                // that has the highest priority (lowest period)
                printk(KERN_INFO "check 020!\n");
                if (pos->period_time < highest_task->period_time) {
                printk(KERN_INFO "check 030!\n");
                    highest_task = pos;
                }
            }
        }
      }	
      // mutex unlock
      mutex_unlock(&list_mutex);
      
      return highest_task;
}

int dispatching_thread_function(void *idx){

	struct mp2_task_struct *new_task = NULL;
	struct sched_attr attr;
	
	while(!kthread_should_stop()){
	
		// putting dispatching thread to sleep till it is woken up by YIELD or by TIMER
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		
		// woken up and resumes
		printk(KERN_INFO "Dispatching thread loop!\n");
		new_task = get_highest_priority_task();
		printk(KERN_INFO "get_highest returns");
		
		struct mutex dt_mutex;
		mutex_init(&dt_mutex);
	    	
	    	mutex_lock(&dt_mutex);
		
		if (new_task == NULL ){
			printk(KERN_INFO"get_highest returns nothing!\n");
			// no new task then preempt current task again
			if (current_task != NULL){
				printk(KERN_INFO "current task not null and scheduling same task!\n");
				attr.sched_policy=SCHED_NORMAL;
				attr.sched_priority=0;
				// check failitre 
				if(sched_setattr_nocheck(current_task->linux_task, &attr)!=0){
					printk(KERN_INFO "schedule fail 1");
				}
			}
		} else {
			// new task exists that will be run instead
			printk(KERN_ALERT "New task exists: %u\n", new_task->pid);
			if (current_task != NULL && current_task->period_time > new_task->period_time){
				printk(KERN_ALERT "Current task exists and is lesser priority that new process: %u\n", new_task->pid);
				attr.sched_policy=SCHED_NORMAL;
				attr.sched_priority=0;
                    		current_task->process_state = READY;
				if(sched_setattr_nocheck(current_task->linux_task, &attr)!=0){
					printk(KERN_INFO "schedule fail 2");
				}
            		}
            		printk(KERN_ALERT "check 3\n", new_task->pid);
            		// when there are no running tasks or when there exists a new task with higher priority
            		if (current_task == NULL || current_task->period_time > new_task->period_time){
            		printk(KERN_ALERT "check 4\n", new_task->pid);
            		 // context switch to the chosen task
			    current_task = new_task;
			    wake_up_process(new_task->linux_task);
			    new_task->process_state = RUNNING;
			    printk(KERN_ALERT "check 5 for %d\n", new_task->pid);
			    attr.sched_policy=SCHED_FIFO;
			    attr.sched_priority=99;
			    if(sched_setattr_nocheck(current_task->linux_task, &attr)!=0){
			    	printk(KERN_INFO"schedule fail 3\n");
			    }
			    
			}

			
		}
		        
		printk(KERN_ALERT "Put dispatching thread to sleep\n");
		
		mutex_unlock(&dt_mutex);
	}
	return 0;

}

// timer callback that gets invoked at every new period in every process

void wakeup_timer_callback(struct timer_list *timer){
	printk(KERN_INFO "Timer Callback Invoked!\n");
	
    //using from_timer to pass arguments to callback
    struct mp2_task_struct *r_task = from_timer(r_task, timer, t_timer);
    printk(KERN_ALERT "Timer bell rung! \t I belong to %u!\n", r_task->pid);
    spin_lock(&lock1);
    
    r_task->process_state = READY;
    spin_unlock(&lock1);

    printk(KERN_ALERT "Waking up dispatch\n");
    //wakes up the dispatching thread
    wake_up_process(dispatch_thread);
}

static ssize_t mywrite(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos){

	printk(KERN_INFO "Write Callback Called! \n");
	char mode;
	unsigned int pid;
	unsigned long period, proc_time;
	
	char *mybuf = (char *)kmalloc(count,GFP_KERNEL);
	copy_from_user(mybuf,ubuf,count);

	
	if (mybuf[0] == 'R'){
		printk(KERN_INFO "Register Section!\n");
		printk(KERN_INFO "mybuf: %s",mybuf);
		
	
		//splitting variables from input char buffer
		sscanf(strsep(&mybuf,","),"%c",&mode);
    		sscanf(strsep(&mybuf,","),"%u",&pid);
    		sscanf(strsep(&mybuf,","),"%lu",&period);
    		sscanf(strsep(&mybuf,","),"%lu",&proc_time);
    		printk(KERN_INFO "pid: %d,mode: %c,period: %lu, proc_time:%lu\n",pid,mode,period,proc_time);
		
		/*
		if(!admission_control(period,proc_time)){
			kfree(mybuf);
			return count;
		}	
		*/
		// initialising mp2_task_struct
		struct mp2_task_struct *r_task;
		r_task = (struct mp2_task_struct*) kmem_cache_alloc(mp2_cache,GFP_KERNEL);
		r_task->pid = pid;
		r_task->period_time = period;
		r_task->processing_time = proc_time;
		r_task->process_state = SLEEPING;
		//r_task->ddl = jiffies + msecs_to_jiffies(r_task->period_time);
		r_task -> linux_task = find_task_by_pid(pid);
		r_task -> ddl = 0;
		
		timer_setup(&r_task->t_timer, wakeup_timer_callback, 0);
		
		INIT_LIST_HEAD(&r_task->list);
		struct mutex list_mutex;
		mutex_init(&list_mutex);
    	
    		mutex_lock_interruptible(&list_mutex);
		
		list_add_tail(&r_task->list, &mp2_tasklist_head);
		
		mutex_unlock(&list_mutex);
		printk(KERN_INFO "\n");
		
		
	} else if (mybuf[0] == 'Y') {
		printk(KERN_INFO "Yield Section!\n");
		printk(KERN_INFO "mybuf: %s",mybuf);		
		sscanf(strsep(&mybuf,","),"%c",&mode);
    		sscanf(strsep(&mybuf,","),"%u",&pid);
    		printk(KERN_INFO "pid: %d,mode: %c\n",pid,mode);
    		
    		    struct mp2_task_struct *pos;
		    struct mp2_task_struct *yielding_task = NULL;

		    struct sched_attr attr;
		    
		    struct mutex list_mutex;
		    mutex_init(&list_mutex);
		    mutex_lock_interruptible(&list_mutex);
		    list_for_each_entry(pos, &mp2_tasklist_head, list) {
			if (pos->pid == pid) {
			    printk(KERN_ALERT "Found my PID!!\n");
			    yielding_task = pos;
			}    
		    }
		    mutex_unlock(&list_mutex);
		    
		    printk(KERN_INFO "check 33");
		if (yielding_task == NULL){
			printk(KERN_INFO "yield task is NULL\n");
			return -1;
		}
		printk(KERN_INFO "check 44");
		
		mutex_lock_interruptible(&list_mutex);
		// updating deadline 
		if (yielding_task->ddl == 0){
			printk(KERN_INFO "check 111");
			yielding_task->ddl = jiffies + msecs_to_jiffies(yielding_task->period_time);
			printk(KERN_INFO "check 112");
		} else {
			printk(KERN_INFO "check 222");
			yielding_task->ddl += msecs_to_jiffies(yielding_task->period_time);
			if (jiffies > yielding_task->ddl){
				printk(KERN_INFO "check 333");
				printk(KERN_INFO "jiffies > yield task\n");
				return -1;
			}
		}
		printk(KERN_INFO "check 55");
		
		// updating deadline and timer
		mod_timer(&yielding_task->t_timer, yielding_task->ddl);
		
		printk(KERN_INFO "put to sleep yield task\n");
		// put task to sleep on yield
		yielding_task->process_state = SLEEPING;
		
		current_task = NULL;
		
		mutex_unlock(&list_mutex);
		
		printk(KERN_INFO "wakeup dispatch thread\n");
		
		wake_up_process(dispatch_thread);
		
		
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule();
		
		
		
		printk(KERN_INFO "Yield fin\n");
		
		
    		
    		
	} else if (mybuf[0] == 'D'){
		printk(KERN_INFO "Deregister Section!\n");
		printk(KERN_INFO "mybuf: %s",mybuf);		
		sscanf(strsep(&mybuf,","),"%c",&mode);
    		sscanf(strsep(&mybuf,","),"%u",&pid);
    		printk(KERN_INFO "pid: %d,mode: %c\n",pid,mode);
    		
    		struct mp2_task_struct *pos, *n;
    		// iterate though list till correct task via pid is found and delete entry from list
    		struct mutex list_mutex;
		mutex_init(&list_mutex);
		
		
		list_for_each_entry_safe(pos,n,&mp2_tasklist_head,list){
		if(pos->pid == pid){
			del_timer(&pos->t_timer);
			list_del(&pos->list);
			kmem_cache_free(mp2_cache,pos);
		}
		
		}
		mutex_unlock(&list_mutex);
    		
	} else {
		printk(KERN_INFO "INVALID INPUT!\n");
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
	

	
	struct mp2_task_struct *pos, *n;
	
    	struct mutex list_mutex;
	mutex_init(&list_mutex);
    	
    	mutex_lock(&list_mutex);
    	// Critical Section
	list_for_each_entry(pos, &mp2_tasklist_head,list){
   		copy_offset += sprintf( mybuf + copy_offset, "pid: %d,state: %d, period_time: %lu, process_time:%d, ddl: %lu\n", pos->pid,pos->process_state, pos->period_time,pos->processing_time, pos->ddl);
   }
   	copy_to_user(ubuf, mybuf, copy_offset);
   	mutex_unlock(&list_mutex);

   	
   	kfree(mybuf);
	return copy_offset;
}

// set proc_operations to pass callback functions
static struct proc_ops file_ops = {
	.proc_read = myread,
	.proc_write = mywrite,
};


// mp2_init - Called when module is loaded
int __init mp2_init(void)
{
        #ifdef DEBUG
        printk(KERN_ALERT "MP2 MODULE LOADING\n");
        #endif
        // Insert your code here ...
	mp2_dir = proc_mkdir("mp2",NULL);
   	dir_entry = proc_create("status",0666, mp2_dir, &file_ops);
   	mp2_cache = kmem_cache_create("mp2_cache_name", sizeof(mp2_task_list), 0, 0, NULL);
   	
   	 dispatch_thread = kthread_run(dispatching_thread_function, NULL, "dispaching_thread");
  


        printk(KERN_ALERT "MP2 MODULE LOADED\n");
        return 0;
        }

// mp2_exit - Called when module is unloaded
void __exit mp2_exit(void)
{
        #ifdef DEBUG
        printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
        #endif
        // Insert your code here ...
	
	   struct mp2_task_struct *pos, *n;
	   
	   proc_remove(dir_entry);
	   proc_remove(mp2_dir);
	   
	   // delete linked list of tasks
	   struct mutex list_mutex;
	   mutex_init(&list_mutex);
	   mutex_lock(&list_mutex);
	   list_for_each_entry_safe(pos,n, &mp2_tasklist_head,list){
	   	printk(KERN_INFO "0\n");
	   	list_del(&pos->list);
	   	printk(KERN_INFO "00\n");
	   	del_timer(&pos->t_timer);
	   	kmem_cache_free(mp2_cache, pos);
	   	printk(KERN_INFO "000\n");
	   }
	   mutex_unlock(&list_mutex);
	   printk(KERN_INFO "1\n");
	   /*
	   if (dispatch_thread.linux_task != NULL) {
	   	kthread_stop(dispatch_thread);
	   }
	   */
	   // clear cache
	   printk(KERN_INFO "2\n");
	   kmem_cache_destroy(mp2_cache);
	   printk(KERN_INFO "3\n");



        printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);

