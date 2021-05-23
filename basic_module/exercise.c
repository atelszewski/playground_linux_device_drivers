#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/threads.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/err.h>

/* Note: Module info entries are only refreshed after module is installed
 * in /lib/modules and dempmod-ed. They are not available after sole insmod. */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrzej Telszewski");
MODULE_DESCRIPTION("Cool module because it does nothing");
MODULE_VERSION("1.2.3");

static int exercise_param = 42;
module_param(exercise_param, int, 0644);
MODULE_PARM_DESC(exercise_param, "does completely nothing");

/* Note:
 * The module fails to `unload` if the symbol exported with
 * `EXPORT_SYMBOL_GPL()` is in use by another module.
 * It's a protection mechanism. */

static char e_precious_data[16];
static DEFINE_MUTEX(e_precious_data_lock);
EXPORT_SYMBOL_GPL(e_precious_data);
EXPORT_SYMBOL_GPL(e_precious_data_lock);

/* Note:
 *
 * kthreads cannot:
 * - use copy_{to|from}_user,
 * - busy wait,
 *
 * kthreads can/should:
 * - use mutexes,
 * - use blocking functions,
 * - be event driven (implement state machines). */

static int e_thread_fn(void * data)
{
    while (!kthread_should_stop())
    {
        mutex_lock(&e_precious_data_lock);
        pr_info("%s: mutex locked: %s\n", KBUILD_MODNAME, e_precious_data);
        strcpy(e_precious_data, "DEADBEEF");
        msleep(1000U);

        mutex_unlock(&e_precious_data_lock);
        pr_info("%s: mutex unlocked\n", KBUILD_MODNAME);
        msleep(100U);
    }

    return exercise_param;
}

static struct task_struct * e_thread = NULL;

static int __init exercise_init(void)
{
    pr_info("exercise_param=%i\n", exercise_param);

    e_thread = kthread_create(e_thread_fn, NULL, "e_thread");

    if (IS_ERR(e_thread))
    {
        pr_err("unable to create kernel thread\n");

        return -PTR_ERR(e_thread);
    }

    wake_up_process(e_thread);

    return 0;
}

static void __exit exercise_exit(void)
{
    int e_thread_ret;

    if (!IS_ERR_OR_NULL(e_thread))
    {
        e_thread_ret = kthread_stop(e_thread);

        pr_info("e_thread returned %i\n", e_thread_ret);
    }
}

module_init(exercise_init);
module_exit(exercise_exit);
