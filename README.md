# silk-guardian
Silk Guardian is an anti-forensic LKM kill-switch that waits for a change on your usb ports then deletes precious files and turns off your computer.

 Inspired by [usbkill](https://github.com/hephaest0s/usbkill). 
 I remade this project as a Linux kernel driver for fun and to learn. Many people have contributed since, and I thank them.

To run:

```shell
make
sudo insmod silk.ko
```

You will need to have the `linux-headers` package installed. If you haven't:

```shell
sudo apt-get install linux-headers
```
### Why?

There are 3 reasons (maybe more?) to use this tool:

- In case the police or other thugs come busting in. The police commonly uses a « [mouse jiggler](http://www.amazon.com/Cru-dataport-Jiggler-Automatic-keyboard-Activity/dp/B00MTZY7Y4/ref=pd_bxgy_pc_text_y/190-3944818-7671348) » to keep the screensaver and sleep mode from activating.
- You don't want someone retrieve documents from your computer via USB or install malware or backdoors.
- You want to improve the security of your (Full Disk Encrypted) home or corporate server (e.g. Your Raspberry).

> **[!] Important**: Make sure to use (partial) disk encryption ! Otherwise intruders will be able to access your harddrive.

> **Tip**: Additionally, you may use a cord to attach a USB key to your wrist. Then insert the key into your computer and insert the kernel module. If they steal your computer, the USB will be removed and the computer shuts down immediately.

### Feature List

- Shutdown the computer when there is USB activity
- Secure deletion of incriminating files before shutdown
- No dependencies
- Difficult to detect

### To Do
- Ability to whitelist USB devices ![](http://www.gia.edu/img/sprites/icon-green-check.png)
- Remove files before shutdown ![](http://www.gia.edu/img/sprites/icon-green-check.png)
- Remove userspace dependancy upon shutdown ![](http://www.gia.edu/img/sprites/icon-green-check.png)

More like... to-done. Way to go community you did it!

### Change Log
2.0 - Updated to use notifier interface.

1.5 - Updated to use shred and remove files on shutdown

1.0 - Initial release.

### Contact

[natebrune@gmail.com](mailto:natebrune@gmail.com)  
https://keybase.io/natebrune
