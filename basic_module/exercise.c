#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>

/* Note: Module info entries are only refreshed after module is installed
 * in /lib/modules and dempmod-ed. They are not available after sole insmod. */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrzej Telszewski");
MODULE_DESCRIPTION("Cool module because it does nothing");
MODULE_VERSION("1.2.3");

static int exercise_param = 42;
module_param(exercise_param, int, 0644);
MODULE_PARM_DESC(exercise_param, "does completely nothing");

static int __init exercise_init(void)
{
    pr_info("exercise_param=%i\n", exercise_param);

    return 0;
}

static void __exit exercise_exit(void)
{
}

module_init(exercise_init);
module_exit(exercise_exit);
