#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/workqueue.h>
//#include"/usr/include/stdio.h"

/* ======================  DATA STRUCTURES OF LINKED LIST =======================*/
struct node {
	int inode_num;
	struct node *next;
};

struct node *head,*tail,*temp;

//==============================================



/* ======================  DATA STRUCTURES OF WORKQUEUES ===========================*/

struct workqueue_struct *my_wq;

typedef struct {
	struct work_struct my_work;
	int data;
}my_work_t;

my_work_t *add_work , *delete_work;
//============================================================


/* add a node to tail */
void add_node(int myinode_num) {

        if(head == NULL) {
                head = kmalloc(sizeof(struct node *) , GFP_KERNEL);
                if(head == NULL) {
                        kfree(head);
                        goto out;
                }

                head->inode_num = myinode_num;
                head->next = NULL;
                tail = head;
                printk(KERN_INFO " IF : added %d to list\n",myinode_num);
        }
        else {
                temp = kmalloc(sizeof(struct node *) , GFP_KERNEL);
                if(temp == NULL) {
                        kfree(temp);
                        goto out;
                }

                temp->inode_num = myinode_num;
                temp->next = NULL;
		tail->next = temp;
		tail = tail->next;
		printk(KERN_INFO " ELSE: added %d to list\n",myinode_num);	
	}

out:
;
}


/* free node from head */
int delete_node(void) {

        if(head == NULL) {
                printk(KERN_ALERT " no node \n");
                return -1;
        }
        else {
                temp = head;
                head = head->next;
                printk(KERN_ALERT " Freeing num = %d \n",temp->inode_num);
                kfree(temp);

        }

        return 0;
}




/* API to call add_node */
void call_add(struct work_struct *add_work) {
	my_work_t *work = (my_work_t *)add_work;
	printk(KERN_INFO "data = %d\n",work->data);
	add_node(work->data);
}

/* API to call delete_node */
void call_delete(struct work_struct *work) {
	delete_node();
}

int work_init(void) {

	int cnt=0;
	/* create */
	my_wq = create_workqueue("my_queue");

/* NOTE : same func(work) cannot be added in work queue
 * so this while loop will not work bcoz it is trying to 
 * add work "call_add" 5 times 
 * Hence removing this while
 */

			    add_work = (my_work_t *)kmalloc(sizeof(my_work_t),GFP_KERNEL);
			    add_work->data=cnt;
				/* initialize work structure 
 				 * also type cast add_work to work_struct*/
			    INIT_WORK( (struct work_struct *)add_work,call_add);
				/* add work to queue */
			    queue_work(my_wq,(struct work_struct *)add_work);


			    delete_work = (my_work_t *)kmalloc(sizeof(my_work_t),GFP_KERNEL);
			    INIT_WORK( (struct work_struct *)delete_work,call_delete);
			    queue_work(my_wq,(struct work_struct *)delete_work);

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
	flush_workqueue(my_wq);
	destroy_workqueue(my_wq);
}

module_init(work_init);
module_exit(work_exit);

MODULE_LICENSE("GPL");
