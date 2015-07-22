#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>		//for msleep
#include <linux/workqueue.h>		//for workqueues
#include <linux/jiffies.h>		//msecs_to_jiffies

struct node {
	int val;
	struct node *next;
};
struct node *head,*tail,*temp;

// data given to each work
typedef struct my_work{
	struct work_struct work;
	int data;
} my_work_t;

// represents work
my_work_t *add_work, delete_work;


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




/* API to call add_node */
void call_add(void *arg) {
	my_work_t *d = (my_work_t *)arg;
	printk(KERN_INFO "data = %d\n",d->data);
	add_node(d->data);
}

/* API to call delete_node */
void call_delete(void *arg) {
	int ret = delete_node();
	if (ret) {
		printk("List is empty. Unable to delete\n");
	}
}

int __init work_init(void)
{
	add_work = kmalloc(sizeof(*add_work), GFP_KERNEL);
	add_work->data = 10;
	printk("add_work->data = %d\n", add_work->data);	

	/* initialize work structure 'add_work' at runtime
	 * with func 'call_add' to execute
         */
	INIT_DELAYED_WORK ((struct delayed_work *)add_work, call_add);

	/* Add work to events workqueue (default kernel workqueue) and will be executed after timeout */
	schedule_delayed_work((struct delayed_work *)add_work, msecs_to_jiffies(2000));
	
	//cannot delete immediately as data is not added yet		
	msleep(4000);

	INIT_WORK ((struct work_struct *)&delete_work, call_delete);

	/* The work is scheduled immediately and run as soon as worker thread on current processor wakes up */
	schedule_work((struct work_struct *)&delete_work);

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
	printk("waiting for jobs to completing, if any, in the workqueue\n");
	flush_scheduled_work();
}

module_init(work_init);
module_exit(work_exit);

MODULE_LICENSE("GPL");
