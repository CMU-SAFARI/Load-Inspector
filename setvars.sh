REALPATH_PWD=`realpath .`

export INSPECTOR_HOME=${REALPATH_PWD}
export SDE_BUILD_KIT=${REALPATH_PWD}/sde-kit
export PATH=$PATH:$INSPECTOR_HOME
