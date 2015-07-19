#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>		//for msleep
#include <linux/workqueue.h>		//for workqueues
#include <linux/jiffies.h>		//msecs_to_jiffies

MODULE_DESCRIPTION("Creating your own ST workqueue");
MODULE_AUTHOR("Kashish_Bhatia");

struct node {
	int val;
	struct node *next;
};
struct node *head,*tail,*temp;

// represents custom workqueue
struct workqueue_struct *my_wq;

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
	msleep(10000);
	add_node(added_work->data);
}

/*
 * Callback func to delete node from llist
 */
void call_delete(struct work_struct *arg) {
	int ret;
	msleep(20000);
	ret = delete_node();
	if (ret) {
		printk("List is empty. Unable to delete\n");
	}
}

int __init work_init(void)
{
	//create workqueue with 1 worker thread for entire system
	my_wq = create_singlethread_workqueue("test_workqueue");
	if (!my_wq) {
		printk("Failed to create workqueue\n");
		goto out;
	}

	//allocate and init data	
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

	/* Add work to given workqueue
 	 * and will be executed after specified timeout
 	 *
 	 * @args :
 	 * struct workqueue_struct *wq
 	 * struct delayed_work *work
 	 * time in jiffies
 	 */
	queue_delayed_work(my_wq, &(add_work->work), msecs_to_jiffies(2000));
	
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
 	 * struct workqueue_struct *wq
 	 * struct work_struct *work
 	 */
	queue_work(my_wq, &(delete_work.work));
out:
	return 0;
}



void work_exit(void)
{
	if (delayed_work_pending(&(add_work->work))) {
		printk("add_work is pending\n");
		/*
 		 * cancel delayed work.
 		 * It will only cancel the work if it is not
 		 * removed out by worker thread from workqueue.
 		 * Means it is yet to be removed and serviced
 		 */
		cancel_delayed_work_sync(&(add_work->work));
	}

	if (work_pending(&(delete_work.work))) {
		printk("delete_work is pending\n");
		//cancel work
		cancel_work_sync(&(delete_work.work));
	}

	/* this function will flush workqueue
 	 * but before flushing block the control
 	 * untill all works in the queue are 
 	 * completed
 	 *
 	 * we can also use flush_work() API 
 	 * to flush the specific work from
 	 * the queue
 	 * destroy_workqueue() will destroy given wq
 	 *
 	 */
	if (my_wq) {
		printk("flush wq\n");
		flush_workqueue(my_wq);
		//free allocated memory
		kfree(add_work);
		printk("destroy wq\n");
		destroy_workqueue(my_wq);
	}
	printk("Exiting from the module\n");
}

module_init(work_init);
module_exit(work_exit);

MODULE_LICENSE("GPL");
