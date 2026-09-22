#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>

static struct list_head *prev_module;

static int __init hide_init(void)
{
    printk(KERN_INFO "hide_module: loaded\n");

    // Save previous entry and unlink module
    // TODO:verify locking before list_del()

    return 0;
}

static void __exit hide_exit(void)
{
    // TODO:restore module to the list
    printk(KERN_INFO "hide_module: unloaded\n");
}

module_init(hide_init);
module_exit(hide_exit);

MODULE_LICENSE("GPL");