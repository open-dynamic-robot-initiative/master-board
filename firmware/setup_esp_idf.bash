# add_to_env is a function that modify a BASH env var
# it will add a user defined path and check if it already exist.
# if so the function does nothing
# if not the function adds the given path to the env var
#   USAGE : add_to_env $arg1 $arg2
#   $arg1 is the path to add
#   $arg2 is the BASH environment to modify 
add_to_env(){
    if [ "$#" -ne 2 ]
    then
    	echo "Illegal number of parameters"
    else
        envVar=$1
        pathToAdd=$2
        if [[ :${!envVar}: == *:$pathToAdd:* ]] ; then
    	    echo "$pathToAdd is already in the $envVar"
	    else
	        export $envVar=$pathToAdd:${!envVar}
	    fi
    fi
}

# Path to esp-idf.
export IDF_PATH=$HOME/esp/esp-idf

# Standard esp variables.
export IDF_TOOLS_EXPORT_CMD=$IDF_PATH/export.sh
export IDF_TOOLS_INSTALL_CMD=$IDF_PATH/install.sh
export IDF_PYTHON_ENV_PATH=$HOME/.espressif/python_env/idf4.2_py3.6_env

# Path to the installed binaries of ESP and XTENSA.
add_to_env PATH $IDF_PATH/components/esptool_py/esptool
add_to_env PATH $IDF_PATH/components/espcoredump
add_to_env PATH $IDF_PATH/components/partition_table
add_to_env PATH $IDF_PATH/components/app_update
add_to_env PATH $IDF_PATH/tools

# Add .espressif tools to the PATH (XTENSA compiler)
for bin_folder in `find $HOME/.espressif -name "bin" -type d`; do add_to_env PATH $bin_folder ; done

# openocd
export OPENOCD_SCRIPTS=`find $HOME/.espressif -path \*openocd-esp32/share/openocd/scripts`