#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/reboot.h>
#include <linux/kmsg_dump.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/syscore_ops.h>
#include <linux/uaccess.h>
#include "config.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nate Brune");
MODULE_DESCRIPTION("A module that protects you from having a very bad no good terrible day.");

static uint initialProducts[64];
static uint initialSerials[64];
static uint currentProducts[64];
static uint currentSerials[64];
static uint counter = 0;
static uint bcounter = 0;
static struct usb_device *dev, *childdev = NULL;
static struct usb_bus *bus = NULL;
static int chix = 0;
static bool debugDone = false;
static struct task_struct *thread1;

static int guardian(void *data)
{
	int i;
	int j;

	while (!kthread_should_stop()) {
		if (debugDone) {
		} else {
			i = 0;
			list_for_each_entry(bus, &usb_bus_list, bus_list) {
				dev = bus->root_hub;
				usb_hub_for_each_child(dev, chix, childdev) {
				if (childdev) {
					currentProducts[bcounter]=childdev->descriptor.idProduct;
					currentSerials[bcounter]=childdev->descriptor.iSerialNumber;
					bcounter=bcounter+1;
				}
			}
		}
		for(; i<=counter; i++) {
			if (initialProducts[i]!=currentProducts[i] || initialSerials[i]!=currentSerials[i]) {
				if (debugDone) {
				} else {
					printk("Change detected!\n");
					printk("Removing files... ");
					for (; j < sizeof(removeFiles) / sizeof(removeFiles[0]); j++) {
						char *shred_argv[] = { "/usr/bin/shred", "-f", "-u", "-n", shredIterations,  removeFiles[j], NULL };
						call_usermodehelper(shred_argv[0], shred_argv, NULL, UMH_WAIT_EXEC);
					}
					printk("done.\n");
					printk("Syncing & powering off.\n");
					printk("Good luck in court!\n");
					kernel_power_off();
					debugDone=true;
					}
				}
			}
		bcounter = 0;
		}
	}
	return 0;
}
static int __init hello_init(void)
{
	printk("Silk Guardian Module Loaded\n");
	printk("Listing Currently Trusted USB Devices\n");
	printk("-------------------------------------\n");
	list_for_each_entry(bus, &usb_bus_list, bus_list) {
		dev = bus->root_hub;
		//usb_hub_for_each_child macro not supported in 3.2.0, so trying with 3.7.6.
		usb_hub_for_each_child(dev, chix, childdev) {
		if (childdev) {
			initialProducts[counter] = childdev->descriptor.idProduct;
			initialSerials[counter] = childdev->descriptor.iSerialNumber;

			printk("Vendor Id:%x, Product Id:%x\n",
			       childdev->descriptor.idVendor,
			       childdev->descriptor.idProduct);
			counter = counter + 1;
			}
		}
	}
	thread1 = kthread_create(guardian,NULL,"thread1");
	if ((thread1)) {
		wake_up_process(thread1);
	}
	return 0;

}

static void __exit hello_cleanup(void)
{
	kthread_stop(thread1);
	printk("Guardian stopped successfully!\n");
}

module_init(hello_init);
module_exit(hello_cleanup);
