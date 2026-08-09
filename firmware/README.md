# How to Flash the Master Board

Install esp-idf
--------
The master board is based on a ESP32 module programmed in C++ using the [esp-idf SDK](https://github.com/espressif/esp-idf). The firmware was originally compatible only with ESP-IDF 3.x and has recently been migrated to ESP-IDF 6.x to make it more accessible to new contributors. This guide assumes ESP-IDF v6.0.1.

To install the SDK, first install the dependencies
```bash
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util
```

Then, make sure that your system `python` points to **Python3**. If not, run the following command to do so
```bash
sudo update-alternatives --install /usr/local/bin/python python /usr/bin/python3 10
```
**Note** : If for some reason you have to use **Python2**, you need to install packages at specific versions before running the installation: `pip install python-socketio==1.5.0 Flask-SocketIO==2.9`.

Now, clone and install the esp-idf repo
```bash
mkdir ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
./install.sh
```

For more details, refer to [the offical documentation of esp-idf](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/index.html).

Finally, you need to install the [esptool](https://github.com/espressif/esptool). You can install it via pip:
```bash
pip install esptool
```

Flashing the firmware
--------
The firmware of the ESP32 can be found in this repo https://github.com/open-dynamic-robot-initiative/master-board.

To flash the firmware without admin rights, make sure your local user is part of the `dialout` group. You can check the groups of your current user using the `groups` command and add the user to the dialout group using the command 
```bash
sudo usermod -a -G dialout $USER
``` 
You need to reboot your computer for this change to take effect.

The master board needs to be connected to a host computer via the PROG connector, and to be powered from a DC source from 5V to 60V. The programmer is a simple USB to SERIAL adapter with line RTS and DTR accessible.

To put the ESP32 in a flash mode, a special circuit is needed to lower the G0 pin from the RTS and DTR lines and generate a reset. To avoid using a dedicated hardware, we can use an ESP dev board containing this circuit and the USB to SERIAL adapter, where the orginal ESP module have been removed:

<img alt="Wiring of the esp32" src="../images/master_board_esp32_prog_wire.jpg" width="500px">

<img alt="Wiring esp32 to master-board" src="../images/master_board_esp32_prog_2.jpg" width="500px">


On a freshly assembled board, we first need to burn a configuration fuse because of a conflict with a boot pin. To do so, use the [espefuse.py script](https://github.com/espressif/esptool): (the fuse burning process is irreversible, be sure to only execute the following command). If you successfully installed esptool, you should be able to do it by directly running the following command
```bash
espefuse.py set_flash_voltage 3.3V
```

To flash the board, the esp environment variables must be sourced:
- Either you installed esp-idf in the standard path, which would be the case if you followed the instructions above. You can simply do:
  ```bash
  cd master-board/firmware
  source setup_esp_idf.bash
  ```
- Or you can source the `export.sh` in the esp-idf repo (This requires a version higher than 4.0). Assuming the esp-idf installation is under `~/esp/esp-idf`:
  ```bash
  cd ~/esp/esp-idf
  source export.sh
  ```

Then, from the `master-board/firmware` folder, you can run:

* Flash the board: `idf.py flash`
* Change configurations: `idf.py menuconfig`
* Debug the board: `idf.py monitor`
