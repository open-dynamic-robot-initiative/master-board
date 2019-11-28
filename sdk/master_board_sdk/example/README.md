# To run the example with C++ (example.cpp)

* Tune the number of controlled driver boards in `sdk/master_board_sdk/example/example.cpp` and `sdk/master_board_sdk/include/defines.h` (N_SLAVES_CONTROLED variable)
* Open a terminal in `sdk/master_board_sdk`
* Run `make`. A `build` and `bin` folder should be generated in `sdk/master_board_sdk` if everything goes well. 
* Run `cd bin`
* Check the name of your interface by running `ifconfig` 
* Run `sudo ./example "name_of_your_interface"` ("enp1s0" for instance)

# To run the example with Python bindings (example.py)

* Edit `sdk/master_board_sdk/CMakeLists.txt` and turn on Python bindings generation `OPTION (BUILD_PYTHON_INTERFACE "Build the python binding" ON)`
* Tune the number of controlled driver boards in `sdk/master_board_sdk/example/example.cpp`, `sdk/master_board_sdk/example/example.py` and `sdk/master_board_sdk/include/defines.h` (N_SLAVES_CONTROLED variable)
* Create a `build` folder in `sdk/master_board_sdk/`
* Open a terminal in `sdk/master_board_sdk/build`
* Run `cmake ..`
* Run `make`. A `bindings` folder should be generated in `sdk/master_board_sdk/build` if everything goes well.
* Open a terminal in `sdk/master_board_sdk/example`
* Check the name of your interface by running `ifconfig` 
* Run `sudo python2.7 example.py -i "name_of_your_interface"` ("enp1s0" for instance)

* Symbolic links are in the example folder to make links with the .so files generated in the `build` folder. I think a cleaner way to do that would be to do something in the CMakeList.txt to link those files with the .py file.