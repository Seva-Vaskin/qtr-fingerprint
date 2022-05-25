# qtr-fingerprint

## Build
1. git clone https://github.com/quantori/qtr-fingerprint.git
2. cd ./qtr-fingerprint/cpp
3. git submodule update --init --recursive
4. Open qtr-fingerprint in VS Code:
   - configure project
   - build project

## Run app
1. cd ./qtr-fingerprint/cpp
2. Copy data from [google drive](https://drive.google.com/drive/u/3/folders/1VrQjWSKFtYdBeHLVDyrCrQFrxso3j45w) to current dir
3. ./dist/lib/linux-x86_64/app

## Run tests
1. cd ./qtr-fingerprint/cpp
2. ./dist/lib/linux-x86_64/tests

## Log configuration

In order to configure logger, you should add environment variables, 
most common with their default values are listed below:
1. `GLOG_log_dir=`
2. `GLOG_alsologtostderr=false`
3. `GLOG_logtostdout=false`
4. `GLOG_minloglevel=0`, order of levels are: `INFO, WARNING, ERROR, FATAL`
