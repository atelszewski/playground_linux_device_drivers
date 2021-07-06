#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
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

/* debugfs entries. */

/* The complex way. */

static ssize_t dbg_custom_string_value_get(struct file * file,
    char * __user usrbuf, size_t usrlen, loff_t * off);
static ssize_t dbg_custom_string_value_set(struct file * file,
    char const * __user usrbuf, size_t usrlen, loff_t * off);

static struct file_operations const dbg_custom_string_value_fops = {
    .owner = THIS_MODULE,
    .read = dbg_custom_string_value_get,
    .write = dbg_custom_string_value_set
};

static char dbg_custom_string_value[32];
static struct dentry * dbg_parent_dir_dent;
static struct dentry * dbg_custom_string_value_dent;

static ssize_t dbg_custom_string_value_get(struct file * file,
    char * __user usrbuf, size_t usrlen, loff_t * off)
{
    ssize_t len_kern;
    ssize_t len_left;

    len_kern = min((loff_t) sizeof(dbg_custom_string_value) - *off,
        (loff_t) usrlen);

    if (0 >= len_kern)
    {
        return 0;
    }

    len_left = copy_to_user(usrbuf, &dbg_custom_string_value[*off], len_kern);

    if (0 != len_left)
    {
        return -EFAULT;
    }

    *off += len_kern;

    return len_kern;
}

static ssize_t dbg_custom_string_value_set(struct file * file,
    char const * __user usrbuf, size_t usrlen, loff_t * off)
{
    ssize_t len_kern;
    ssize_t len_left;

    len_kern = min((loff_t) sizeof(dbg_custom_string_value) - *off,
        (loff_t) usrlen);

    if (0 >= len_kern)
    {
        return 0;
    }

    len_left = copy_from_user(&dbg_custom_string_value[*off], usrbuf, len_kern);

    *off += len_kern;

    return len_kern;
}

/* The easy way. */

static bool dbg_boolval;
static struct dentry * dbg_boolval_dent;

static int dbg_create(void);
static void dbg_remove(void);

static int dbg_create(void)
{
    int ret = 0;

    dbg_parent_dir_dent = debugfs_create_dir("e_device", NULL);

    if (IS_ERR(dbg_parent_dir_dent))
    {
        pr_err("unable to create debugfs parent\n");

        ret = -PTR_ERR(dbg_parent_dir_dent);
        goto on_error;
    }

    dbg_custom_string_value_dent = debugfs_create_file("custom_value",
        0644, dbg_parent_dir_dent, NULL, &dbg_custom_string_value_fops);

    if (IS_ERR(dbg_custom_string_value_dent))
    {
        pr_err("unable to create debugfs custom string value entry\n");

        ret = -PTR_ERR(dbg_custom_string_value_dent);
        goto on_error;
    }

    dbg_boolval_dent = debugfs_create_bool("boolval",
        0644, dbg_parent_dir_dent, &dbg_boolval);

    if (IS_ERR(dbg_boolval_dent))
    {
        pr_err("unable to create debugfs boolval entry\n");

        ret = -PTR_ERR(dbg_boolval_dent);
        goto on_error;
    }

    return ret;

on_error:
    dbg_remove();
    return ret;
}

static void dbg_remove(void)
{
    /* TODO:
     *
     * Replace one-by-one removal with `debugfs_remove_recursive()`. */

    if (!IS_ERR_OR_NULL(dbg_boolval_dent))
    {
        debugfs_remove(dbg_boolval_dent);
        dbg_boolval_dent = NULL;
    }

    if (!IS_ERR_OR_NULL(dbg_custom_string_value_dent))
    {
        debugfs_remove(dbg_custom_string_value_dent);
        dbg_custom_string_value_dent = NULL;
    }

    if (!IS_ERR_OR_NULL(dbg_parent_dir_dent))
    {
        debugfs_remove(dbg_parent_dir_dent);
        dbg_parent_dir_dent = NULL;
    }
}

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

static struct attribute * sys_attr[] = {
    &dev_attr_exercise_param.attr,
    NULL
};

static struct attribute_group const sys_group = {
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

/* NOTE:
 *
 * Platform device is one that cannot be auto-discovered, nor it can be
 * hot-plugged. In most cases, it's embedded on the board or within the SOC.
 * Additional resources:
 *
 * - [Kernel's device model](https://lwn.net/Articles/645810/)
 * - [Linux Platform Device Driver](https://tinyurl.com/a3whctpc) */

/* Platform device specific data. */

struct platform_data;

static void e_driver_power_on(struct platform_data * pdata);
static void e_driver_power_off(struct platform_data * pdata);
static void e_driver_reset(struct platform_data * pdata);

struct platform_data {
    void (*power_on)(struct platform_data * pdata);
    void (*power_off)(struct platform_data * pdata);
    void (*reset)(struct platform_data * pdata);
};

static struct platform_data e_platform_data = {
    .power_on = e_driver_power_on,
    .power_off = e_driver_power_off,
    .reset = e_driver_reset
};

/* Platform device. */

/* NOTE:
 *
 *  `.name` is used by the kernel for matching the driver and calling its
 *  `probe()` function when the driver is installed. This function must be
 *  called before registering the platform driver. */

static struct platform_device e_device = {
    .name = "e_device",
    .id = PLATFORM_DEVID_NONE,
    .dev.platform_data = &e_platform_data
};

/* NOTE:
 *
 * Platform driver implements the `probe()` function that gets called when
 * the driver is inserted. It makes use of platform specific data to make
 * the driver platform agnostic, by using per-platform configuration.
 * `.driver.name` should match the corresponding device's `.name`. */

/* Platform driver. */

static int e_driver_probe(struct platform_device * pdev);
static int e_driver_remove(struct platform_device * pdev);

static struct platform_driver e_driver = {
    .probe = e_driver_probe,
    .remove = e_driver_remove,
    .driver = {
        .name = "e_device",
        .owner = THIS_MODULE,
        .pm = NULL
    }
};

static int e_driver_probe(struct platform_device * pdev)
{
    struct platform_data * pdata;

    trace_printk("e_driver: probe()\n");

    pdata = dev_get_platdata(&pdev->dev);

    if (NULL != pdata->power_on)
    {
        pdata->power_on(pdata);
    }

    udelay(5000U);

    if (NULL != pdata->reset)
    {
        pdata->reset(pdata);
    }

    return 0;
}

static int e_driver_remove(struct platform_device * pdev)
{
    struct platform_data * pdata;

    pdata = dev_get_platdata(&pdev->dev);

    trace_printk("e_driver: remove()\n");

    if (NULL != pdata->power_off)
    {
        pdata->power_off(pdata);
    }

    return 0;
}

static void e_driver_power_on(struct platform_data * pdata)
{
    trace_printk("e_driver: power_on()\n");
}

static void e_driver_power_off(struct platform_data * pdata)
{
    trace_printk("e_driver: power_off()\n");
}

static void e_driver_reset(struct platform_data * pdata)
{
    trace_printk("e_driver: reset()\n");
}

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
    int dbg_ret;
    int pdev_ret;
    int pdrv_ret;

    dbg_ret = dbg_create();

    if (0 != dbg_ret)
    {
        ret = dbg_ret;
        goto on_error;
    }

#if SYSFS_GROUP_ENABLE
    int sys_ret;

    /* NOTE:
     *
     * The actual _sysfs_ entries (device attributes) will be located in
     * `/sys/module/MODULE_NAME/holders/GROUP_NAME` directory. */

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

    pdev_ret = platform_device_register(&e_device);

    if (0 != pdev_ret)
    {
        pr_err("unable to register platform device: %i\n", pdev_ret);

        ret = pdev_ret;
        goto on_error;
    }

    /* TODO:
     *
     * - Write down difference between `platform_driver_register()` and
     *   `platform_driver_probe()`.
     * - Learn about `module_platform_driver()`.
     * - Learn about using DT for device/driver configuration. */

    pdrv_ret = platform_driver_probe(&e_driver, NULL);

    if (0 != pdrv_ret)
    {
        pr_err("error probing the device: %i\n", pdrv_ret);

        ret = pdrv_ret;
        goto on_error;
    }

    return ret;

on_error:
    if (0 == pdrv_ret)
    {
        platform_driver_unregister(&e_driver);
        pdrv_ret = -EINVAL;
    }

    if (0 == pdev_ret)
    {
        platform_device_unregister(&e_device);
        pdev_ret = -EINVAL;
    }

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

    if (0 == dbg_ret)
    {
        dbg_remove();
        dbg_ret = -EINVAL;
    }

    return ret;
}

static void __exit exercise_exit(void)
{
    platform_driver_unregister(&e_driver);

    /* TODO:
     *
     *     Device 'e_device' does not have a release() function, it is broken
     *     and must be fixed. See Documentation/core-api/kobject.rst.
     *
     * Problem [explanation and solution](https://tinyurl.com/ya3svc35). */

    platform_device_unregister(&e_device);
    kthread_stop(e_thread);

#if SYSFS_GROUP_ENABLE
    sysfs_remove_group(THIS_MODULE->holders_dir, &sys_group);
#endif

    dbg_remove();
}

module_init(exercise_init);
module_exit(exercise_exit);
