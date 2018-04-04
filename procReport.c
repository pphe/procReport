/* 
 * Peter Phe
 * TCSS 422 - Operating Systems
 * Winter 2017
 * Assignment 2 - Process Reporter - Linux Kernel Module
 * Filename: procReport.c
 * Description: The kernel module generates a report describing the list of 
 * 				processes upon being loaded into kernel space. 
 */

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#define AUTHOR		"Peter Phe <peterphe@uw.edu>"
#define DESC		"Simple Process Reporter"
#define PROCNAME	"proc_report"
#define BUF_SIZE	255

static struct process_data data;

// container for /proc output data
struct process_data
{
	int unrunnable;
	int runnable;
	int stopped;
	int total;
	char **count_output;
	char **process_output;
};

// generates count data (must be called before generate_proc_data!)
static void generate_state_data(void)
{
	struct task_struct *task;
	char buffer[BUF_SIZE];

	data.unrunnable = data.runnable = data.stopped = data.total = 0;
	for_each_process(task) // macro ~ sched.h
	{
		if (task->state == -1) // unrunnable
			data.unrunnable++;
		else if (task->state == 0) // runnable
			data.runnable++;
		else if (task->state > 0) // stopped
			data.stopped++;

		data.total++;
	}

	// populate struct data count_output field
	data.count_output = kmalloc(sizeof(char *)*3+1, GFP_KERNEL);
	sprintf(buffer, "Unrunnable:%d\n", data.unrunnable);
	data.count_output[0] = kmalloc(strlen(buffer)+1, GFP_KERNEL);
	strcpy(data.count_output[0], buffer);
	sprintf(buffer, "Runnable:%d\n", data.runnable);
	data.count_output[1] = kmalloc(strlen(buffer)+1, GFP_KERNEL);
	strcpy(data.count_output[1], buffer);
	sprintf(buffer, "Stopped:%d\n", data.stopped);
	data.count_output[2] = kmalloc(strlen(buffer)+1, GFP_KERNEL);
	strcpy(data.count_output[2], buffer);
}

// generates process data
static void generate_proc_data(void)
{
	struct task_struct *task;
	struct list_head *childList;
	char buffer[BUF_SIZE];
	int pos; // track process_output index
	pos = 0;

	data.process_output = kmalloc(sizeof(char *)*data.total+1, GFP_KERNEL);
	for_each_process(task)
	{
		sprintf(buffer, "Process ID=%d Name=%s ", task->pid, task->comm);

		// check for children
		childList = &task->children;
		if (!list_empty(childList)) // macro ~ list.h
		{
			char child_buffer[BUF_SIZE];
			struct list_head *list_cursor;
			struct task_struct *childTask;
			int numChildren = 0;

			// count number of children
			list_for_each(list_cursor, childList) // macro ~ list.h
				numChildren++;

			// grab first child task (list_first_entry() macro ~ list.h)
			childTask = list_first_entry(childList, struct task_struct, sibling);

			// append child_buffer to process buffer
			sprintf(child_buffer,
					"number_of_children=%d first_child_pid=%d first_child_name=%s\n",
					numChildren, childTask->pid, childTask->comm);
			strcat(buffer, child_buffer);
		}
		else
			strcat(buffer, "*No Children\n");

		// copy buffer data
		data.process_output[pos] = kmalloc(strlen(buffer)+1, GFP_KERNEL);
		strcpy(data.process_output[pos], buffer);
		pos++;
	}
}

// accepts list of strings to output to /proc file
static void output_report(struct seq_file *m, char **data)
{
	int i;
	for (i = 0; data[i] != NULL; i++)
		seq_printf(m, "%s", data[i]);
}

// frees kmalloc'd list of strings
static void free_data(char **data)
{
	int i;
	for (i = 0; data[i] != NULL; i++)
		kfree(data[i]);
	kfree(data);
}

// implements *show() from seq_file.h interface to generate report 
static int show_report(struct seq_file *m, void *v)
{
	seq_printf(m, "PROCESS REPORTER:\n");
	output_report(m, data.count_output);
	output_report(m, data.process_output);
	return 0;
}

// open interface for seq_file operations
static int proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_report, NULL);
}

// initialize proc interface settings and specify callback
static const struct file_operations proc_fops =
{
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

// initialize module
static int proc_init(void)
{
	printk(KERN_INFO "procReport: Loading module.\n");

	// initialize procfs
	if (!proc_create(PROCNAME, 0, NULL, &proc_fops))
		return -ENOMEM;

	// generate state count, then process data
	generate_state_data();
	generate_proc_data();
	return 0;
}

// cleanup module
static void proc_cleanup(void)
{
	printk(KERN_INFO "procReport: Unloading module.\n");
	remove_proc_entry(PROCNAME, NULL);
	free_data(data.count_output);
	free_data(data.process_output);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);
module_init(proc_init);
module_exit(proc_cleanup);

