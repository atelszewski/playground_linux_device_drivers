#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrzej Telszewski");

static int __init exercise_init(void)
{
    return 0;
}

static void __exit exercise_exit(void)
{
}

module_init(exercise_init);
module_exit(exercise_exit);
