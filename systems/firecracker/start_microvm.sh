#!/bin/bash

#set -e
set -x

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <vm-rootdir> [ssh-cmd]"
    exit 1
fi

VMROOT=$1
if [ ! -d $VMROOT ]; then
    echo "Error: the vm directory '$VMROOT' doesn't exist"
    exit 1
fi

ssh_cmd=
if [ "$#" -ge 2 ]; then
    ssh_cmd="$2"
fi


# Get the last character of VMROOT
last_char="${VMROOT: -1}"

# Check if the last character is a digit
VM_ID=
if [[ "$last_char" =~ [0-9] ]]; then
    VM_ID=$((last_char + 1))
else
    echo "Error: The last character of VMROOT is not a digit"
    exit 1
fi

HOST_IP="172.16.100.1"

# Use unique TAP device per VM
TAP_DEV="tap$VM_ID"
TAP_IP="172.16.0.$((($VM_ID * 4) + 1))"
VM_IP="172.16.0.$((($VM_ID * 4) + 2))"
MASK_SHORT="/30"

# Generate a MAC address where the last octet matches the VM's IP
MAC_SUFFIX=$(printf "%02x" $((($VM_ID * 4) + 2)))
FC_MAC="06:00:AC:10:00:${MAC_SUFFIX}"

# Setup network interface (rest of your networking setup)
sudo ip link del "$TAP_DEV" 2> /dev/null || true
sudo ip tuntap add dev "$TAP_DEV" mode tap
sudo ip addr add "${TAP_IP}${MASK_SHORT}" dev "$TAP_DEV"
sudo ip link set dev "$TAP_DEV" up

# Enable ip forwarding
sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"
sudo iptables -P FORWARD ACCEPT

# This tries to determine the name of the host network interface to forward
# VM's outbound network traffic through. If outbound traffic doesn't work,
# double check this returns the correct interface!
HOST_IFACE=$(ip -j route list default |jq -r '.[0].dev')

# Set up microVM internet access
sudo iptables -t nat -D POSTROUTING -o "$HOST_IFACE" -j MASQUERADE || true
sudo iptables -t nat -A POSTROUTING -o "$HOST_IFACE" -j MASQUERADE

API_SOCKET="/tmp/firecracker_$VMROOT.socket"
LOGFILE="./$VMROOT/firecracker.log"

# Create log file
touch $LOGFILE

# Set log file
sudo curl -X PUT --unix-socket "${API_SOCKET}" \
    --data "{
        \"log_path\": \"${LOGFILE}\",
        \"level\": \"Debug\",
        \"show_level\": true,
        \"show_log_origin\": true
    }" \
    "http://localhost/logger"

KERNEL="./$(ls $VMROOT/vmlinux* | tail -1)"
KERNEL_BOOT_ARGS="console=ttyS0 reboot=k panic=1 pci=off"

echo "Booting kernel $KERNEL"

ARCH=$(uname -m)

if [ ${ARCH} = "aarch64" ]; then
    KERNEL_BOOT_ARGS="keep_bootcon ${KERNEL_BOOT_ARGS}"
fi

# Set boot source
sudo curl -X PUT --unix-socket "${API_SOCKET}" \
    --data "{
        \"kernel_image_path\": \"${KERNEL}\",
        \"boot_args\": \"${KERNEL_BOOT_ARGS}\"
    }" \
    "http://localhost/boot-source"

ROOTFS="./$VMROOT/ubuntu-24.04.ext4"

# Set rootfs
sudo curl -X PUT --unix-socket "${API_SOCKET}" \
    --data "{
        \"drive_id\": \"rootfs\",
        \"path_on_host\": \"${ROOTFS}\",
        \"is_root_device\": true,
        \"is_read_only\": false
    }" \
    "http://localhost/drives/rootfs"

DATAFS="./$VMROOT/data.ext4"
# Set datafs (to sync user-custom files)
sudo curl -X PUT --unix-socket "${API_SOCKET}" \
    --data "{
        \"drive_id\": \"data\",
        \"path_on_host\": \"${DATAFS}\",
        \"is_root_device\": false,
        \"is_read_only\": false
    }" \
    "http://localhost/drives/data"

# Set network interface
sudo curl -X PUT --unix-socket "${API_SOCKET}" \
    --data "{
        \"iface_id\": \"net1\",
        \"guest_mac\": \"$FC_MAC\",
        \"host_dev_name\": \"$TAP_DEV\"
    }" \
    "http://localhost/network-interfaces/net1"

# API requests are handled asynchronously, it is important the configuration is
# set, before `InstanceStart`.
sleep 0.015s

# Start microVM
sudo curl -X PUT --unix-socket "${API_SOCKET}" \
    --data "{
        \"action_type\": \"InstanceStart\"
    }" \
    "http://localhost/actions"

# API requests are handled asynchronously, it is important the microVM has been
# started before we attempt to SSH into it.
sleep 2s

# Setup internet access in the guest
ssh -i ./$VMROOT/ubuntu-24.04.id_rsa root@$VM_IP  "ip route add default via $TAP_IP dev eth0"

# hard-code alias to the tap ip (to share the same host ip across VMs)
ssh -i ./$VMROOT/ubuntu-24.04.id_rsa root@$VM_IP  "ip addr add $HOST_IP dev eth0"

# Setup DNS resolution in the guest
ssh -i ./$VMROOT/ubuntu-24.04.id_rsa root@$VM_IP  "echo 'nameserver 8.8.8.8' > /etc/resolv.conf"

# Setup /mnt/data
ssh -i ./$VMROOT/ubuntu-24.04.id_rsa root@$VM_IP  "mount /dev/vdb /mnt/data"

if [ -n "$ssh_cmd" ]; then
    echo "going to execute cmd on vm: $ssh_cmd"
    # SSH into the microVM with the user-supplied command
    ssh -i ./$VMROOT/ubuntu-24.04.id_rsa root@$VM_IP  $ssh_cmd
else
    # SSH into the microVM
    ssh -i ./$VMROOT/ubuntu-24.04.id_rsa root@$VM_IP
fi

echo "microVM $VMROOT execution complete."


# Use `root` for both the login and password.
# Run `reboot` to exit.
