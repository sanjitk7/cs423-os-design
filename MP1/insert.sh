echo "inserting kernel module...."
sudo insmod mp1.ko
sudo dmesg
echo "writing to proc/mp1/status"
./userapp
sudo dmesg
echo "reading from proc/mp1/status"
cat /proc/mp1/status
