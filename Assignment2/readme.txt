Kernel module is written to create file named as "dof" which is basically a user space file, being presented as a block device to the system.This block device have 2 primary partitions (/dev/dof1 & /dev/dof2) and no extended partitions. Some commands used to demonstrate this are:

cat /proc/devices
displays block devices currently configured.

ls -l /dev
the output of it for each file returned represents a block special file

lsblk
Shows partitions created in the disk.
