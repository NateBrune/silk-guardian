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

uint initialProducts[64];
uint initialSerials[64];
uint currentProducts[64];
uint currentSerials[64];
uint counter = 0;
uint bcounter = 0;
struct usb_device *dev, *childdev = NULL;
struct usb_bus *bus = NULL;
int chix = 0;
bool stop = false;


static struct task_struct *thread1;

int guardian(void){
	while(true)
	{
		int i = 0;
		if(stop)
		{
			break;
		}

		list_for_each_entry(bus, &usb_bus_list, bus_list)
		{
		   dev = bus->root_hub;
		   usb_hub_for_each_child(dev, chix, childdev)
		   {
		        if(childdev)
		        {
		        	currentProducts[bcounter]=childdev->descriptor.idProduct;
		        	currentSerials[bcounter]=childdev->descriptor.iSerialNumber;
		        	bcounter=bcounter+1;

		        }
		   }

		}
		for(; i<=counter; i++)
		{
			if(initialProducts[i]!=currentProducts[i] || initialSerials[i]!=currentSerials[i])
			{
				printk("Change detected!\n");
				printk("Removing files... ");
				unsigned int i = 0;
				/*for(; i < sizeof(removeFiles) / sizeof(removeFiles[0]); i++){
					char *shutdown_argv[] = { "/usr/bin/shred", "-u", "-f", "-n", shredIterations,  removeFiles[i], NULL };
					call_usermodehelper(shutdown_argv[0], shutdown_argv, NULL, UMH_NO_WAIT);
				}
				*/
				printk("done.");
				printk("Syncing & powering off.\n Good luck in court!\n");
				//kernel_power_off();
				return 0;
				break;
			}
		}
		bcounter = 0;

	}
	return 0;
}
static int __init hello_init(void)
{


	printk("Silk Guardian Module Loaded");
	printk("\nListing Currently Trusted USB Devices");
	printk("\n-------------------------------------\n");
	list_for_each_entry(bus, &usb_bus_list, bus_list)
	{
	   dev = bus->root_hub;
	   //usb_hub_for_each_child macro not supported in 3.2.0, so trying with 3.7.6.
	   usb_hub_for_each_child(dev, chix, childdev)
	   {
	        if(childdev)
	        {
	        	initialProducts[counter]=childdev->descriptor.idProduct;
	        	initialSerials[counter]=childdev->descriptor.iSerialNumber;

	        	printk("Vendor Id:%x, Product Id:%x\n", childdev->descriptor.idVendor, childdev->descriptor.idProduct);
	        	counter=counter+1;
	        }
	   }
	}
	thread1 = kthread_create(guardian,NULL,"thread1");
    if((thread1)){
    	wake_up_process(thread1);
    }

	return 0;

}

static void __exit hello_cleanup(void)
{
		int ret = kthread_stop(thread1);
		stop=true;
	    printk("Stopping guardian.\n");
 		if(ret)
  			printk("Guardian stopped successfully!");
  		return ret;

}

module_init(hello_init);
module_exit(hello_cleanup);
