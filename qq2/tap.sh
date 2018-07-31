WHOAMI=$(whoami)

echo add tap0 to virbr0 bridge
echo ------------------------------------------------------
sudo tunctl -d tap0
sudo tunctl -u ${WHOAMI}
sudo ifconfig tap0 0.0.0.0 up
sudo brctl addif virbr0 tap0


