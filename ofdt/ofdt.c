/* Example of how to obtain config data from Device Tree. */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrzej Telszewski");
MODULE_DESCRIPTION("Device Tree example");
MODULE_VERSION("3.14");

/* Upon module loading, try to get the value of the _stdout-path_ property
 * of the _chosen_ node. Refuse to load the module if the node or
 * property don't exist. */

static int __init ofdt_init(void)
{
    int ret = -EINVAL;
    struct device_node * np = NULL;
    void * stdout_path_prop = NULL;
    char * stdout_path = NULL;
    int stdout_path_len = 0;

    /* NOTE:
     *
     * DT node name is the string before '@'. The part of the string after '@'
     * is not validated nor needed, it's only informational so that the entries
     * in proc/sys have distinct names should there be more nodes of the same
     * name. */

    np = of_find_node_by_name(NULL, "chosen");

    if (NULL == np)
    {
        pr_err("node: \"chosen\" not found\n");

        ret = -ENODEV;
        goto on_error;
    }

    stdout_path_prop =
        (char *) of_get_property(np, "stdout-path", &stdout_path_len);

    if (NULL == stdout_path_prop)
    {
        pr_err("prop: \"stdout-path\" not found\n");

        ret = -ENODEV;
        goto on_error;
    }

    /* Allocate virtually contiguous (and zeroed) memory. */

    stdout_path = (char *) vzalloc(((unsigned long) stdout_path_len) + 1U);

    if (NULL == stdout_path)
    {
        pr_err("out of memory\n");

        ret = -ENOMEM;
        goto on_error;
    }

    memcpy(stdout_path, stdout_path_prop, stdout_path_len);

    pr_info("chosen:stdout-path: %s\n", stdout_path);

    vfree(stdout_path);
    stdout_path = NULL;

    of_node_put(np);
    np = NULL;

    return 0;

on_error:
    if (NULL != stdout_path)
    {
        vfree(stdout_path);
        stdout_path = NULL;
    }

    if (NULL != np)
    {
        of_node_put(np);
        np = NULL;
    }

    return ret;
}

static void __exit ofdt_exit(void)
{
    /* Intentionally empty. */
}

module_init(ofdt_init);
module_exit(ofdt_exit);
