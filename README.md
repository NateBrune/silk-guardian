# silk-guardian
Silk Guardian is an anti-forensic LKM kill-switch that waits for a change on your usb ports and then wipes your ram and turns off your computer.
 
 Very Very much inspired by [usbkill](https://github.com/hephaest0s/usbkill)
 I remade this project as a LKM because I feel that it could be implemented better than it already is.
 
 > The project is still under development but it does work and is effective.

To run:

```shell
make
sudo insmod usbkill.ko
```

### Why?

There are 3 reasons (maybe more?) to use this tool:

- In case the police or other thugs come busting in (or steal your laptop from you when you are at a public library as happened to Ross). The police commonly uses a « [mouse jiggler](http://www.amazon.com/Cru-dataport-Jiggler-Automatic-keyboard-Activity/dp/B00MTZY7Y4/ref=pd_bxgy_pc_text_y/190-3944818-7671348) » to keep the screensaver and sleep mode from activating.
- You don't want someone retrieve documents from your computer via USB or install malware or backdoors.
- You want to improve the security of your (Full Disk Encrypted) home or corporate server (e.g. Your Raspberry).

> **[!] Important**: Make sure to use (partial) disk encryption ! Otherwise they will get in anyway.

> **Tip**: Additionally, you may use a cord to attach a USB key to your wrist. Then insert the key into your computer and insert the kernel module. If they steal your computer, the USB will be removed and the computer shuts down immediately.

### Feature List

- Shutdown the computer when there is USB activity
- Secure deletion of incriminating files before shutdown
- No dependencies
- Difficult to detect

### To Do
- Ability to whitelist USB devices ![](http://upload.wikimedia.org/wikipedia/commons/7/7e/Red_x.png) 
- Remove files before shutdown ![](http://www.gia.edu/img/sprites/icon-green-check.png)
- Remove userspace dependancy upon shutdown ![](http://www.gia.edu/img/sprites/icon-green-check.png)

### Contact

[natebrune@gmail.com](mailto:natebrune@gmail.com)  
https://keybase.io/natebrune
