#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nate Brune");
MODULE_DESCRIPTION("A module that protects you from having a very bad no good terrible day.");

uint initialDevices[64];
uint currentDevices[64];


static struct task_struct *thread1;

int guardian(){
	while(true)
	{
		list_for_each_entry(bus, &usb_bus_list, bus_list)
		{
		   dev = bus->root_hub;
		   usb_hub_for_each_child(dev, chix, childdev)
		   {
		        if(childdev)
		        {
		        	currentDevices[counter]=childdev->descriptor.idProduct;
		        	bcounter=bcounter+1;

		        }
		   }
		
		}
		int i = 0;
		for(; i<=counter; i++)
		{
			if(initialDevices[i]==currentDevices[i])
			{
				printk("Change Detected!\n");
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
	printk("\n-------------------------------------s\n");
	int chix = 0;
	uint counter = 0;
	uint bcounter = 0;
	struct usb_device *dev, *childdev = NULL;
	struct usb_bus *bus = NULL;

	list_for_each_entry(bus, &usb_bus_list, bus_list)
	{
	   dev = bus->root_hub;
	   //usb_hub_for_each_child macro not supported in 3.2.0, so trying with 3.7.6.
	   usb_hub_for_each_child(dev, chix, childdev)
	   {
	        if(childdev)
	        {
	        	initialDevices[counter]=childdev->descriptor.idProduct;
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
	    printk(KERN_INFO "Stopping guardian.\n");
	    int ret;
 		ret = kthread_stop(thread1);
 		if(!ret)
  			printk(KERN_INFO "Guardian stopped successfully!");
}

module_init(hello_init);
module_exit(hello_cleanup);
