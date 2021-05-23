#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/threads.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrzej Telszewski");
MODULE_DESCRIPTION("Second thread");
MODULE_VERSION("1.2.3");

/* Note:
 *
 * The module fails to load if the `extern` symbols are not available.
 * It's a protection mechanism. */

extern char e_precious_data[16];
extern struct mutex e_precious_data_lock;

static int second_thread_fn(void * data)
{
    while (!kthread_should_stop())
    {
        mutex_lock(&e_precious_data_lock);
        trace_printk("%s: mutex locked: %s\n", KBUILD_MODNAME, e_precious_data);
        strcpy(e_precious_data, "CAFFEE");

        mutex_unlock(&e_precious_data_lock);
        trace_printk("%s: mutex unlocked\n", KBUILD_MODNAME);
        msleep(100U);
    }

    return 0;
}

static struct task_struct * second_thread = NULL;

static int __init second_thread_init(void)
{
    second_thread = kthread_run(second_thread_fn, NULL, "second_thread");

    if (IS_ERR(second_thread))
    {
        pr_err("unable to create kernel thread\n");

        return -PTR_ERR(second_thread);
    }

    return 0;
}

static void __exit second_thread_exit(void)
{
    if (!IS_ERR_OR_NULL(second_thread))
    {
        kthread_stop(second_thread);
    }
}

module_init(second_thread_init);
module_exit(second_thread_exit);
