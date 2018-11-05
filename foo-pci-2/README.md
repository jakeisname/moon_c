

$ cat tap_eth.sh
<code>
WHOAMI=$(whoami)
DEV=enx00e04c36005f
DEV_TAP=tap0
IP_TAP=$(ip addr show ${DEV_TAP} | awk '/inet / {print $2}' | cut -d/ -f1)

if [ "$IP_TAP" == "20.0.2.4" ]; then
echo tap0 interface is already installed.
exit
fi

echo setup NAT network: tap0 to ethernet:${DEV}
sudo tunctl -d ${DEV_TAP}
sudo tunctl -u ${WHOAMI}
sudo ifconfig ${DEV_TAP} 20.0.2.4/24 up
sudo modprobe ip_tables
sudo modprobe iptable_filter
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -o ${DEV} -j MASQUERADE
</code>

