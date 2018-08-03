WHOAMI=$(whoami)
DEV_ETH=enp3s0f1
DEV_WIFI=wlp2s0
DEV_TAP=tap0
BR=virbr0
IP_ETH=$(ip addr show ${DEV_ETH} | awk '/inet / {print $2}' | cut -d/ -f1)
IP_WIFI=$(ip addr show ${DEV_WIFI} | awk '/inet / {print $2}' | cut -d/ -f1)
IP_BR=$(ip addr show ${BR} | awk '/inet / {print $2}' | cut -d/ -f1)

echo add tap0 to ${BR} bridge
echo ------------------------------------------------------
echo IP_ETH=${IP_ETH}
echo IP_WIFI=${IP_WIFI}
echo IP_BR=${IP_BR}

if [ "$IP_BR" == "20.0.2.4" ]; then
echo tap0 interface is already installed.
exit
fi

if [ "$IP_E" == "" ]; then
DEV=$DEV_WIFI
else
DEV=$DEV_ETH
fi

sudo brctl addbr ${BR}
sudo tunctl -d tap0
sudo tunctl -u ${WHOAMI}
sudo ifconfig ${DEV_TAP} 0.0.0.0 up
sudo brctl addif ${BR} tap0
sudo ifconfig ${BR} 20.0.2.4 up
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -o ${DEV} -j MASQUERADE
#sudo ifconfig wlp2s0 promisc
