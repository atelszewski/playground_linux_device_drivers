#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/threads.h>
#include <linux/types.h>

/* Allow to disable sysfs entries until the error with module
 * unloading is fixed. */

#define SYSFS_GROUP_ENABLE 0

/* NOTE:
 *
 * Module info entries are only refreshed after module is installed
 * in /lib/modules and dempmod-ed. They are not available after sole insmod. */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrzej Telszewski");
MODULE_DESCRIPTION("Cool module because it does nothing");
MODULE_VERSION("1.2.3");

static int exercise_param = 42;
module_param(exercise_param, int, 0644);
MODULE_PARM_DESC(exercise_param, "does completely nothing");

/* sysfs entries. */

#if SYSFS_GROUP_ENABLE

static ssize_t sys_show(struct device * dev,
    struct device_attribute * attr, char * buf)
{
    /* TODO:
     *
     * What's the max size of buf?
     * Better to use `snprintf()`? */

    return sprintf(buf, "exercise_param = %i\n", exercise_param);
}

static ssize_t sys_store(struct device * dev,
    struct device_attribute * attr, char const * buf, size_t count)
{
    int ret;

    ret = sscanf(buf, "%i", &exercise_param);

    if (1 != ret)
    {
        return -EINVAL;
    }

    return count;
}

static DEVICE_ATTR(exercise_param, 0644, sys_show, sys_store);

static struct attribute * sys_attr[] =
{
    &dev_attr_exercise_param.attr,
    NULL
};

static struct attribute_group const sys_group =
{
    .name = "e_group",
    .attrs = sys_attr
};

#endif

/* NOTE:
 *
 * The module fails to `unload` if the symbol exported with
 * `EXPORT_SYMBOL_GPL()` is in use by another module.
 * It's a protection mechanism. */

static char e_precious_data[16];
static DEFINE_MUTEX(e_precious_data_lock);
EXPORT_SYMBOL_GPL(e_precious_data);
EXPORT_SYMBOL_GPL(e_precious_data_lock);

/* NOTE:
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
        /* NOTE:
         *
         * - For tracing (ftrace) to work, _debugfs_ must be mounted.
         * - Tracing settings are located in _/sys/kernel/debug/tracing_. */

        /* TODO:
         *
         * What is the following message all about?
         *
         *     ** trace_printk() being used. Allocating extra memory.  **
         *     **                                                      **
         *     ** This means that this is a DEBUG kernel and it is     **
         *     ** unsafe for production use. */

        mutex_lock(&e_precious_data_lock);
        trace_printk("%s: mutex locked: %s\n", KBUILD_MODNAME, e_precious_data);
        strcpy(e_precious_data, "DEADBEEF");
        msleep(1000U);

        mutex_unlock(&e_precious_data_lock);
        trace_printk("%s: mutex unlocked\n", KBUILD_MODNAME);
        msleep(100U);
    }

    return exercise_param;
}

static struct task_struct * e_thread = NULL;

/* TODO:
 *
 * It's not possible to unload the module, because of the following error:
 *
 *     $ rmmod exercise
 *     rmmod: ERROR: Module exercise is in use by: e_group
 *     $ lsmod | head
 *     Module                  Size  Used by
 *     exercise               16384  0 e_group
 *
 * `e_group` is the name of the group created with `sysfs_create_group()`. */

static int __init exercise_init(void)
{
    int ret = 0;

#if SYSFS_GROUP_ENABLE
    int sys_ret;

    sys_ret = sysfs_create_group(THIS_MODULE->holders_dir, &sys_group);

    if (0 != sys_ret)
    {
        pr_err("error creating sysfs group: %i\n", sys_ret);

        ret = sys_ret;
        goto on_error;
    }
#endif

    pr_info("exercise_param=%i\n", exercise_param);

    e_thread = kthread_create(e_thread_fn, NULL, "e_thread");

    if (IS_ERR(e_thread))
    {
        pr_err("unable to create kernel thread\n");

        ret = -PTR_ERR(e_thread);
        goto on_error;
    }

    wake_up_process(e_thread);

    return ret;

on_error:
    if (!IS_ERR(e_thread))
    {
        /* TODO:
         *
         * Is it needed to somehow clean-up after `kthread_create`?
         * Try using _kmemleak_ to find out:
         *
         *     $ echo scan > /sys/kernel/debug/kmemleak */

        kthread_stop(e_thread);
        e_thread = NULL;
    }

#if SYSFS_GROUP_ENABLE
    if (0 == sys_ret)
    {
        sysfs_remove_group(THIS_MODULE->holders_dir, &sys_group);
        sys_ret = -EINVAL;
    }
#endif

    return ret;
}

static void __exit exercise_exit(void)
{
    kthread_stop(e_thread);

#if SYSFS_GROUP_ENABLE
    sysfs_remove_group(THIS_MODULE->holders_dir, &sys_group);
#endif
}

module_init(exercise_init);
module_exit(exercise_exit);
