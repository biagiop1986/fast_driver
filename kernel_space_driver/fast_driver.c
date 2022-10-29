#include <linux/device.h> 
#include <linux/errno.h> /* error codes */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/fs.h> /* file manipulation stuff */
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h> /* printk */
#include <linux/kthread.h> /* kernel thread */
#include <linux/mm.h> /* mmap stuff */
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h> /* kmalloc */
#include <linux/sched.h> /* task_struct* current */
#include <linux/types.h> /* size_t */
#include <linux/uaccess.h> /* copy_from/to_user */
#include <linux/wait.h> /* wait queue */

#include "../common/common.h" /* parameters shared with user-space driver */

/* License specification */
MODULE_LICENSE("Dual MIT/GPL");

/* Global variables of the driver */
int DEVICE_MAJOR; /* Let the kernel choose a major number */
dev_t dev;
struct class *cl = NULL;
struct device *p_dev = NULL; 

struct submission_ring_buffer
{
	uint32_t pid;
	uint32_t index;
	struct control controls[NUM_SLOTS];
	struct hlist_node node;
};

struct completion_ring_buffer
{
	uint32_t pid;
	uint32_t index;
	struct output outputs[NUM_SLOTS];
	struct hlist_node node;
};

/* Device structures */
DEFINE_HASHTABLE(submission_ring_tbl, 16);
DEFINE_HASHTABLE(completion_ring_tbl, 16);

/* Kernel thread */
struct task_struct *ring_thread = NULL;
DECLARE_WAIT_QUEUE_HEAD(wq);
atomic_t events_to_process = ATOMIC_INIT(0);

void execute_command(struct control* p_control, struct output* p_output)
{
	p_output->command = p_control->command;
	p_output->outputs[0] = 5u + p_control->command;
}

int ring_thread_body(void* data)
{
	int picked_index;
	unsigned bkt;
	struct submission_ring_buffer *submit;
	struct completion_ring_buffer *complete;
	struct control* p_control;
	struct output* p_output;

	printk(KERN_INFO "%s: kernel thread started\n", DEVICE_NAME);

	while(true)
	{
		wait_event(wq, ((atomic_read(&events_to_process) > 0) || kthread_should_stop()));

		if(kthread_should_stop())
		{
			printk(KERN_INFO "%s: kernel thread exiting\n", DEVICE_NAME);
			do_exit(0);
			break;
		}

		hash_for_each(submission_ring_tbl, bkt, submit, node)
		{
			hash_for_each_possible(completion_ring_tbl, complete, node, submit->pid)
			{
				if(complete->pid == submit->pid)
					break;
			}

			if(!complete)
			{
				printk(KERN_ALERT "%s: found a submission buffer with no completion"
					" buffer associated for pid %d\n", DEVICE_NAME, submit->pid);
			}

			picked_index = submit->index; // this should be atomic
			while(picked_index != complete->index)
			{
				/* Retrieve first non-complete command */
				/* It is positioned at the index pointed by completion buffer index */
				p_control = submit->controls + complete->index;
				p_output = complete->outputs + complete->index;
				printk(KERN_INFO "%s: pid %d gave us work to do - command[%u]\n",
					DEVICE_NAME, submit->pid, p_control->command);

				/* Do work with the retrieved value! */
				/* Process a control entry to produce an output entry */
				execute_command(p_control, p_output);

				/* Increment index to proceed to the next command */
				complete->index = (complete->index + 1) % NUM_SLOTS;
				printk(KERN_INFO "%s: command[%u] from pid %d completed - output[%llu]\n",
					DEVICE_NAME, p_control->command, submit->pid, p_output->outputs[0]);
			}
		}

		atomic_dec(&events_to_process);
		schedule();
	}

	return 0;
}

/* Function declarations */
int fast_driver_init(void);
void fast_driver_exit(void);

int fast_driver_open(struct inode *inode, struct file *filp);
int fast_driver_release(struct inode *inode, struct file *filp);
int fast_driver_mmap(struct file *filp, struct vm_area_struct *vma);
long fast_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/* Structure that declares the usual file operations */
struct file_operations fast_driver_fops = {
  open: fast_driver_open,
  release: fast_driver_release,
  mmap: fast_driver_mmap,
  unlocked_ioctl: fast_driver_ioctl,
};

/* Function implementations */
int fast_driver_init(void)
{
	const char* ring_thread_name = "fast_driver_thread";
	int result;

	/* Register device */
	result = register_chrdev(0, DEVICE_NAME, &fast_driver_fops);
	if (result < 0) 
	{
		printk(KERN_CRIT "%s: cannot obtain major number\n", DEVICE_NAME);
		return result;
	}

	DEVICE_MAJOR = result;
	printk(KERN_INFO "%s: obtained major number %d\n", DEVICE_NAME, DEVICE_MAJOR);

	/* Create device node */
	dev = MKDEV(DEVICE_MAJOR, 0);
	cl = class_create(THIS_MODULE, DEVICE_NAME);
	if(!cl)
	{
		printk(KERN_CRIT "%s: cannot create class\n", DEVICE_NAME);
		result = -EINVAL;
		goto FAIL;
	}

	p_dev = device_create(cl, NULL, dev, NULL, DEVICE_NAME);
	if(!p_dev)
	{
		printk(KERN_CRIT "%s: cannot create device /dev/%s\n", DEVICE_NAME, DEVICE_NAME);
		result = -EINVAL;
		goto FAIL;
	}

	/* Allocate kernel thread to manage accesses from user processes */
	ring_thread = kthread_create(ring_thread_body, NULL, ring_thread_name);
	if (!ring_thread)
	{
		printk(KERN_CRIT "%s: kernel thread creation failed\n", DEVICE_NAME);
		result = -ENOMEM;
		goto FAIL;
	}
	wake_up_process(ring_thread);

	printk(KERN_INFO "%s: module started!\n", DEVICE_NAME);

	return 0;

FAIL:
	fast_driver_exit();
	return result;
}

void fast_driver_exit(void)
{
	printk(KERN_INFO "%s: exiting...\n", DEVICE_NAME);

	/* Stop kernel thread */
	if(ring_thread)
	{
		printk(KERN_INFO "%s: Stopping thread\n", DEVICE_NAME);
		kthread_stop(ring_thread);
		wake_up(&wq);
	}

	/* Destroy device */
	if(p_dev)
		device_destroy(cl, dev);

	/* Destroy device class */
	if(cl)
		class_destroy(cl);

	/* Unregister device */
	unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);

	printk(KERN_INFO "%s: goodbye!\n", DEVICE_NAME);
}

size_t get_mem_size(size_t struct_size)
{
	return (1u + ((struct_size + PAGE_SIZE - 1) >> PAGE_SHIFT)) << PAGE_SHIFT;
}

int fast_driver_open(struct inode *inode, struct file *filp)
{
	struct submission_ring_buffer *submit, *sptr;
	struct completion_ring_buffer *complete, *cptr;

	uint32_t pid = current->pid;
	printk(KERN_INFO "%s: pid %d opening\n", DEVICE_NAME, pid);

	/* Allocate ring buffers for this process */
	submit = (struct submission_ring_buffer*)kzalloc(get_mem_size(sizeof(struct submission_ring_buffer)), GFP_KERNEL);
	complete = (struct completion_ring_buffer*)kzalloc(get_mem_size(sizeof(struct completion_ring_buffer)), GFP_KERNEL);

	if (!submit || !complete)
	{
		printk(KERN_CRIT "%s: not enough memory\n", DEVICE_NAME);
		return -ENOBUFS;
	}
	if (((PAGE_SIZE - 1) & (uint64_t)submit) || 
		((PAGE_SIZE - 1) & (uint64_t)complete))
	{
		printk(KERN_CRIT "%s: ring buffers not aligned to memory page\n", DEVICE_NAME);
		return -ENOBUFS;
	}

	printk(KERN_INFO "%s: allocated ring buffers\n", DEVICE_NAME);

	/* Set allocated pages reserved */
	for(sptr = submit; sptr < submit + get_mem_size(sizeof(struct submission_ring_buffer)); sptr += PAGE_SIZE)
		SetPageReserved(virt_to_page(sptr));
	for(cptr = complete; cptr < complete + get_mem_size(sizeof(struct completion_ring_buffer)); cptr += PAGE_SIZE)
		SetPageReserved(virt_to_page(cptr));

	/* Add ring buffers to hash table to associate them to the current process */
	submit->pid = pid;
	hash_add(submission_ring_tbl, &(submit->node), pid);
	complete->pid = pid;
	hash_add(completion_ring_tbl, &(complete->node), pid);

	return 0;
}

int fast_driver_release(struct inode *inode, struct file *filp)
{
	struct submission_ring_buffer *submit = NULL, *sptr;
	struct completion_ring_buffer *complete = NULL, *cptr;

	uint32_t pid = current->pid;
	printk(KERN_INFO "%s: pid %d closing\n", DEVICE_NAME, pid);

	/* Retrieve ring buffers associated to this process from hash table */
	hash_for_each_possible(submission_ring_tbl, submit, node, pid)
	{
		if(submit->pid == pid)
			break;
	}
	hash_for_each_possible(completion_ring_tbl, complete, node, pid)
	{
		if(complete->pid == pid)
			break;
	}

	/* Clear reservation state from allocated pages and free ring buffers*/
	if(submit)
	{
		printk(KERN_INFO "%s: removing submit ring buffer\n", DEVICE_NAME);
		for(sptr = submit; sptr < submit + get_mem_size(sizeof(struct submission_ring_buffer)); sptr += PAGE_SIZE)
			ClearPageReserved(virt_to_page(sptr));
		kfree(submit);
		hash_del(&(submit->node));
	}
	if(complete)
	{
		printk(KERN_INFO "%s: removing completion ring buffer\n", DEVICE_NAME);
		for(cptr = complete; cptr < complete + get_mem_size(sizeof(struct completion_ring_buffer)); cptr += PAGE_SIZE)
			ClearPageReserved(virt_to_page(cptr));
		kfree(complete);
		hash_del(&(complete->node));
	}

	return 0;
}

int fast_driver_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct submission_ring_buffer *submit = NULL;
	struct completion_ring_buffer *complete = NULL;
	uint64_t size = vma->vm_end - vma->vm_start;
	uint64_t pfn;

	uint32_t pid = current->pid;
	printk(KERN_INFO "%s: pid %d mmapping %s ring buffer\n", DEVICE_NAME, pid, vma->vm_pgoff ? "completion" : "submit");

	if(vma->vm_pgoff == 0)
	{
		/* Retrieve ring buffer associated to this process from hash table */
		hash_for_each_possible(submission_ring_tbl, submit, node, pid)
		{
			if(submit->pid == pid)
				break;
		}

		if(!submit)
		{
			printk(KERN_CRIT "%s: submit ring buffer cannot be retrieved\n", DEVICE_NAME);
			return -EINVAL;
		}

		/* Force write permission on this memory area */
		vma->vm_flags |= VM_WRITE;
		/* Force shared mapping on this memory area */
		vma->vm_flags |= VM_SHARED;

		/* Get submit ring buffer page number */
		pfn = __pa(submit) >> PAGE_SHIFT;
	}
	else
	{
		/* Retrieve ring buffer associated to this process from hash table */
		hash_for_each_possible(completion_ring_tbl, complete, node, pid)
		{
			if(complete->pid == pid)
				break;
		}

		if(!complete)
		{
			printk(KERN_CRIT "%s: completion ring buffer cannot be retrieved\n", DEVICE_NAME);
			return -EINVAL;
		}

		/* Remove write permissions from this memory area */
		vma->vm_flags &= ~VM_WRITE;
		/* Force shared mapping on this memory area */
		vma->vm_flags |= VM_SHARED;

		/* Get submit ring buffer page number */
		pfn = __pa(complete) >> PAGE_SHIFT;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
	{
		printk(KERN_CRIT "%s: mmap failed\n", DEVICE_NAME);
		return -EIO;
	}

	return 0;
}

long fast_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* Signal to kernel thread to unblock */
	printk(KERN_INFO "%s: ioctl call received - signaling kernel thread\n", DEVICE_NAME);
	atomic_inc(&events_to_process);
	wake_up(&wq);

	return 0l;
}

module_init(fast_driver_init);
module_exit(fast_driver_exit);
