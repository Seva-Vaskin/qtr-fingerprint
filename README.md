# qtr-fingerprint
## Build
1. git clone https://github.com/quantori/qtr-fingerprint.git
2. cd qtr-fingerprint
3. git submodule update --init --recursive
4. Open qtr-fingerprint in VS Code:
   - configure project
   - build project
## Run
1. cd qtr-fingerprint/dist/lib/windows-x86_64
2. app.exe

## App Data
App data can be found on [google drive](https://drive.google.com/drive/u/3/folders/1VrQjWSKFtYdBeHLVDyrCrQFrxso3j45w)

## Program arguments

You can get --help information about your executable just by running 

`./program --helpfull`

You will see the list of available arguments and their descriptions.

And then you can specify each option as following:

`./app --path_to_query=query.mol --database_path=data/119697.sdf`

## Log configuration

In order to configure logger, you should add environment variables, 
most common with their default values are listed below:
1. `GLOG_log_dir=`
2. `GLOG_alsologtostderr=false`
3. `GLOG_logtostdout=false`
4. `GLOG_minloglevel=0`, order of levels are: `INFO, WARNING, ERROR, FATAL`
