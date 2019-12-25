# Tips and tricks

## Building

1. check out repository with submodules
1. run `sudo scripts/debootstrap.sh` 
2. run `scripts/bootstrap-tigerlash.sh`
2. run `scripts/bootstrap-tigerlash.sh` second time
2. run `scripts/bootstrap-tigerlash.sh` third time
2. run `scripts/build-tigerlash.sh`

## Adding convenient debug over NFS

```bash
apt-get install nfs-kernel-server
mkdir -p /export/nfs
mount --bind <desired folder> /export/nfs
```

add the following to `/etc/fstab` if you want automatically bind folders

```
<desired folder>    /export/users   none    bind  0  0
```

To export our directories to a local network `192.168.1.0/24` we add the following two lines to `/etc/exports`
    
```
/export       192.168.1.0/24(rw,fsid=0,insecure,no_subtree_check,sync)
/export/nfs   192.168.1.0/24(rw,nohide,insecure,no_subtree_check,sync)
```

Restart nfs service

```
service nfs-kernel-server restart
```

### Mounting on client

```
mount -t nfs <nfs-server-IP>:/nfs /mnt
```

## SPI flash boot

based on http://linux-sunxi.org/Bootable_SPI_flash
and http://linux-sunxi.org/Bootable_SPI_flash

enable overlay in `/boot/armbianEnv.txt`

e.g

```
overlays=spi-spidev
param_spidev_spi_bus=0
```

reboot the device and see that the following succeed

```
ls /dev/spidev0.0
```

```
apt-get install flashrom
```

check the flash

```
flashrom -p linux_spi:dev=/dev/spidev0.0


sudo flashrom -p linux_spi:dev=/dev/spidev0.0
flashrom v0.9.9-r1954 on Linux 4.14.18-sunxi (armv7l)
flashrom is free software, get the source code at https://flashrom.org

Calibrating delay loop... OK.
Found Macronix flash chip "MX25L12805D" (16384 kB, SPI) on linux_spi.
Found Macronix flash chip "MX25L12835F/MX25L12845E/MX25L12865E" (16384 kB, SPI) on linux_spi.
Multiple flash chip definitions match the detected chip(s): "MX25L12805D", "MX25L12835F/MX25L12845E/MX25L12865E"
Please specify which chip definition to use with the -c <chipname> option.
```

The actual flash on R1 is `MX25L12835F`. Verify it with

```
sudo flashrom -c "MX25L12835F/MX25L12845E/MX25L12865E" -p linux_spi:dev=/dev/spidev0.0
```

now erase the flash

```
sudo flashrom -c "MX25L12835F/MX25L12845E/MX25L12865E" -E -p linux_spi:dev=/dev/spidev0.0
```

get the spi-uboot image

```
wget https://lampcore.ru/wp-content/uploads/2017/02/U-Boot_8Mb.zip
unzip U-Boot_8Mb.zip
rm U-Boot_8Mb.zip
```

padd the file to 16M

```
truncate -s 16M u-boot_8.bin
```

write the flash

```
sudo flashrom -c "MX25L12835F/MX25L12845E/MX25L12865E" -w u-boot_8.bin -p linux_spi:dev=/dev/spidev0.0
```


# Compiling linux tools

```bash
make -C tools/ ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- gpio
```

## Making a sysroot to crosscompile to

```bash
#!/bin/bash

sudo apt install debootstrap qemu-user-static
sudo debootstrap --foreign --arch=armhf stretch sysroot http://deb.debian.org/debian/
sudo cp /usr/bin/qemu-arm-static sysroot/usr/bin/
sudo cp /etc/resolv.conf sysroot/etc/
sudo chroot sysroot/ /debootstrap/debootstrap --second-stage
sudo chroot sysroot/ apt -y install libboost-all-dev libjsoncpp-dev libssl-dev zlib1g-dev
```

## Watching resources

### ulimits

```bash
ulimit -a
```

### file descriptors

```bash
sudo sysctl fs.file-nr
```

or

```bash
cat /proc/sys/fs/file-nr
```

### temperature

```bash
cat /sys/class/thermal/thermal_zone0/temp
```

### pids

```bash
ps aux
```

## Типы I/O, которые у нас есть:

* `Input`
* `Output`

По умолчанию все I/O работают в режиме Rising/Falling interrupt, чтобы обеспечить разумное использование ресурсов ЦП и достаточную производительность.

При инициализации управляющией программы происходит захват всех перечисленных в секции `GPIOManager` системных GPIO. Если GPIO были уже кем-то захвачены, то `GPIOManager` принудительно освобождает GPIO методом unexport.

`GPIO` по умолчанию читается и пишется в **неинвертированном виде**. т.о. запись 1 приведет к выдаче 1 на выход GPIO, а чтение значения 1 будет означать, что на входе 1. Параметр `"invert": true` в определении GPIO инвертирует эту логику.

Для выходов актуален параметр `"value": 1` - активирует выход. По умолчанию = 0.

```
{
	"GPIO" :{
		"size": 0,
		"items": [
			{
				"Name": "",
				"ID": "400",
				"Type": "Input",
				"invert": false
			},
			{
				"Name": "",
				"ID": "401",
				"Type": "Output",
				"Value": 1,
				"invert": false
			}
		]

	}
}
```

Для теста на десктопе можно воспользоваться (`gpio-mockup`)[https://bbs.nextthing.co/t/gpio-mockup-a-virtual-gpio-driver/17038]

```
sudo insmod /lib/modules/$(uname -r)/kernel/drivers/gpio/gpio-mockup.ko gpio_mockup_ranges=1,1020
```

Добавление юзера в группу gpio

You can do this using udev rules, which can define actions to execute when the kernel instantiates new devices. Current versions of the Raspbian distribution for Raspberry Pi devices contain the following in `/etc/udev/rules.d/99-com.rules`:

```
SUBSYSTEM=="gpio*", PROGRAM="/bin/sh -c 'chown -R root:gpio /sys/class/gpio && chmod -R 770 /sys/class/gpio; chown -R root:gpio /sys/devices/virtual/gpio && chmod -R 770 /sys/devices/virtual/gpio'"
```

or better use more generic rule
```
SUBSYSTEM=="gpio*", PROGRAM="/bin/sh -c 'find -L /sys/class/gpio/ -maxdepth 2 -exec chown root:gpio {} \; -exec chmod 770 {} \; || true'"
```

This ensures that entries under `/sys/class/gpio` are always available to members of the gpio group:

```bash
sudo addgroup gpio
sudo usermod -aG gpio $(id -un)
```


```bash
# ls -lL /sys/class/gpio/
total 0
-rwxrwx--- 1 root gpio 4096 May  6 23:36 export
drwxrwx--- 2 root gpio    0 Jan  1  1970 gpiochip0
-rwxrwx--- 1 root gpio 4096 May  6 23:37 unexport
# echo 11 > /sys/class/gpio/export 
# ls -lL /sys/class/gpio/
total 0
-rwxrwx--- 1 root gpio 4096 May  6 23:37 export
drwxrwx--- 2 root gpio    0 May  6 23:37 gpio11
drwxrwx--- 2 root gpio    0 Jan  1  1970 gpiochip0
-rwxrwx--- 1 root gpio 4096 May  6 23:37 unexport
```

Permissions are correct for individual pins as well:

```bash
# ls -Ll /sys/class/gpio/gpio11/
total 0
-rwxrwx--- 1 root gpio 4096 May  6 23:37 active_low
drwxr-xr-x 3 root root    0 May  6 23:36 device
-rwxrwx--- 1 root gpio 4096 May  6 23:37 direction
-rwxrwx--- 1 root gpio 4096 May  6 23:37 edge
drwxrwx--- 2 root gpio    0 May  6 23:37 subsystem
-rwxrwx--- 1 root gpio 4096 May  6 23:37 uevent
-rwxrwx--- 1 root gpio 4096 May  6 23:37 value
```

## Building gpio-mockup module for 16.04.4 (not succeeded)

```bash
apt install linux-image-extra-4.15.0-15-generic install linux-headers-4.15.0-15-generic linux-source-4.15.0 libncursesw5-dev libelf-dev
sudo reboot
cd /usr/src/linux-source-4.15.0/
sudo tar -xjf linux-source-4.15.0.tar.bz2
cd linux-source-4.15.0/
cp /boot/config-4.15.0-15-generic .config
sudo cp ../../linux-headers-4.15.0-15-generic/Module.symvers .
make prepare # setup FT1000 as module
make modules_prepare
make SUBDIRS=scripts/mod
sudo cp drivers/gpio/gpio-mockup.ko /lib/modules/4.15.0-15-generic/kernel/drivers/gpio/
```

For fixing `invalid module format` and `gpio_mockup: version magic '4.15.15 SMP mod_unload ' should be '4.15.0-15-generic SMP mod_unload '` see dmesg and edit

Makefile's `SUBLEVEL` variable

yet, loading was not successfull due to

```
[ 4545.825984] gpio_mockup: loading out-of-tree module taints kernel.
[ 4545.833798] gpio_mockup: module verification failed: signature and/or required key missing - tainting kernel
[ 4545.839158] gpio_mockup: Unknown symbol irq_sim_irqnum (err 0)
[ 4545.839188] gpio_mockup: Unknown symbol devm_irq_sim_init (err 0)
[ 4545.839504] gpio_mockup: Unknown symbol irq_sim_fire (err 0)
```

useful links
1. https://linux.die.net/lkmpg/x380.html
2. https://askubuntu.com/questions/168279/how-do-i-build-a-single-in-tree-kernel-module?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

## Building kernel with gpio-mockup enabled

## Quest Sequencer

```
{
	"QuestSequencer": {

    }
}
```
