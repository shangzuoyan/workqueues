#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>		//for msleep
#include <linux/workqueue.h>		//for workqueues
#include <linux/jiffies.h>		//msecs_to_jiffies

MODULE_DESCRIPTION("Using Kernel Shared workqueue");
MODULE_AUTHOR("Kashish_Bhatia");

struct node {
	int val;
	struct node *next;
};
struct node *head,*tail,*temp;

typedef struct my_add_work{
	struct delayed_work work;
	int data;
} my_add_work_t;

typedef struct my_delete_work {
	struct work_struct work;
} my_delete_work_t;

// represents work
my_add_work_t *add_work;
my_delete_work_t delete_work;


/* add a node to tail */
void add_node(int myval) {

        if(head == NULL) {
                head = kmalloc(sizeof(*head) , GFP_KERNEL);
                if(head == NULL) {
			return;
                }

                head->val = myval;
                head->next = NULL;
                tail = head;
                printk(KERN_INFO "added first node %d to list\n",myval);
        }
        else {
                temp = kmalloc(sizeof(*head) , GFP_KERNEL);
                if(temp == NULL) {
			return;
                }

                temp->val = myval;
                temp->next = NULL;
		tail->next = temp;
		tail = tail->next;
		printk(KERN_INFO "added %d to list\n",myval);	
	}
}


/* free node from head */
int delete_node(void) {

        if(head == NULL) {
                printk(KERN_ALERT "List is empty \n");
                return -1;
        }
        else {
                temp = head;
                head = head->next;
                printk(KERN_ALERT " Freeing node with val = %d \n",temp->val);
                kfree(temp);
        }

        return 0;
}

/*
 * Callback func to insert value in llist
 */
void call_add(struct work_struct *arg) {

	/*
 	 * to_delayed_work() converts work_struct to delayed_work 
 	 * Check its code. It uses container_of for conversion
 	 */
	struct delayed_work *dwork = to_delayed_work(arg);	//IMPORTANT

	//extract the parent structure of delayed work
	my_add_work_t *added_work = container_of(dwork, my_add_work_t, work);
	printk(KERN_INFO "data = %d\n", added_work->data);
	add_node(added_work->data);
}

/*
 * Callback func to delete node from llist
 */
void call_delete(struct work_struct *arg) {
	int ret = delete_node();
	if (ret) {
		printk("List is empty. Unable to delete\n");
	}
}

int __init work_init(void)
{
	//Allocate and init data	
	add_work = kmalloc(sizeof(*add_work), GFP_KERNEL);
	add_work->data = 10;

	/* initialize work structure 'add_work' at runtime
	 * with func 'call_add' to execute
	 * 
	 * @args :
	 * struct delayed_work *work
	 * void (*fn)(struct work_struct *)
         */
	INIT_DELAYED_WORK(&(add_work->work), call_add);

	/* Add work to events workqueue(default kernel workqueue)
 	 * and will be executed after specified timeout
 	 *
 	 * @args :
 	 * struct delayed_work *work
 	 * time in jiffies
 	 */
	schedule_delayed_work(&(add_work->work), msecs_to_jiffies(2000));
	
	//cannot delete immediately as data is not added yet		
	msleep(4000);

	/* initialize work structure 'delete_work' at runtime
	 * with func 'call_delete' to execute
	 * 
	 * @args :
	 * struct work_struct *work
	 * void (*fn)(struct work_struct *)
         */
	INIT_WORK(&(delete_work.work), call_delete);

	/* The work is scheduled immediately and run as soon
 	 * as worker thread on current processor wakes up
 	 *
 	 * @args :
 	 * struct work_struct *work
 	 */
	schedule_work((struct work_struct *)&(delete_work.work));

	return 0;
}



void work_exit(void) {

	/* this function will flush workqueue
 	 * but before flushing block the control
 	 * untill all works in the queue are 
 	 * completed
 	 *
 	 * we can also use flush_work() API 
 	 * to flush the specific work from
 	 * the queue
 	 */
	//flush_scheduled_work();
	/*
 	 * Lesson : we cannot use destroy_workqueue or flush_scheduled_work
 	 * during cleanup, as we are inserting the jobs in system workqueue and
 	 * not in our own workqueue
 	 * When I used flush_scheduled_work() during cleanup, I got a kernel panic.
	 * Also flush_scheduled_work() is now deprecated in kernel.
	 * Read :
	 * http://www.unix.com/man-page/centos/9/flush_scheduled_work/
	 * https://www.redhat.com/archives/dm-devel/2010-December/msg00146.html
	 *
 	 * Stack trace is attached in ISSUES file
 	 */
	kfree(add_work);
	printk("Exiting from the module\n");
}

module_init(work_init);
module_exit(work_exit);

MODULE_LICENSE("GPL");
