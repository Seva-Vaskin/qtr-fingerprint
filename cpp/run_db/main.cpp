#include <string>

#include <glog/logging.h>
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include "Utils.h"
#include "BallTreeRAMSearchEngine.h"
#include "HuffmanCoder.h"
#include "SmilesTable.h"
#include "RunDbUtils.h"
#include "properties_table_io/PropertiesTableReader.h"

using namespace std;
using namespace qtr;

#include "modes/web/WebMode.h"
#include "modes/InteractiveMode.h"
#include "modes/FromFileMode.h"


ABSL_FLAG(vector<string>, data_dir_paths, {},
          "Path to directories where data are stored");

ABSL_FLAG(string, other_data_path, "",
          "Path to directory with other files");

ABSL_FLAG(string, db_name, "",
          "Name of folders with data base's files");

ABSL_FLAG(uint64_t, threads_count, -1,
          "Number of threads to process leafs.");

ABSL_FLAG(string, mode, "",
          "possible modes: interactive, from_file, web");

ABSL_FLAG(string, input_file, "",
          "file to load test molecules from");

ABSL_FLAG(uint64_t, ans_count, -1,
          "how many answers program will find");

struct Args {

    enum class Mode {
        Interactive,
        FromFile,
        Web
    };

    vector<filesystem::path> destDirPaths;
    filesystem::path otherDestDirPath;
    string dbName;
    uint64_t threadsCount;
    Mode mode;
    filesystem::path inputFile;
    uint64_t ansCount;

    vector<filesystem::path> dbDataDirsPaths;
    filesystem::path dbOtherDataPath;

    filesystem::path ballTreePath;
    filesystem::path smilesTablePath;
    filesystem::path huffmanCoderPath;
    filesystem::path idToStringDirPath;
    filesystem::path propertyTableDestinationPath;

    Args(int argc, char *argv[]) {
        absl::ParseCommandLine(argc, argv);

        // get flags
        ansCount = absl::GetFlag(FLAGS_ans_count);
        vector<string> dataDirPathsStrings = absl::GetFlag(FLAGS_data_dir_paths);
        otherDestDirPath = absl::GetFlag(FLAGS_other_data_path);
        dbName = absl::GetFlag(FLAGS_db_name);
        threadsCount = absl::GetFlag(FLAGS_threads_count);
        string modeStr = absl::GetFlag(FLAGS_mode);
        inputFile = absl::GetFlag(FLAGS_input_file);

        // check empty flags
        checkEmptyArgument(inputFile, "Please specify input_file option");
        checkEmptyArgument(destDirPaths, "Please specify data_dir_paths option");
        checkEmptyArgument(otherDestDirPath, "Please specify other_data_path option");
        checkEmptyArgument(dbName, "Please specify db_name option");
        checkEmptyArgument(modeStr, "Please specify mode option");
        if (threadsCount == -1) {
            LOG(INFO) << "Please specify threads_count option";
            exit(-1);
        }

        // init values
        copy(dataDirPathsStrings.begin(), dataDirPathsStrings.end(), back_inserter(destDirPaths));
        for (auto &dir: destDirPaths) {
            dbDataDirsPaths.emplace_back(dir / dbName);
        }
        dbOtherDataPath = otherDestDirPath / dbName;
        ballTreePath = dbOtherDataPath / "tree";
        smilesTablePath = dbOtherDataPath / "smilesTable";
        huffmanCoderPath = dbOtherDataPath / "huffman";
        idToStringDirPath = dbOtherDataPath / "idToString";
        propertyTableDestinationPath = dbOtherDataPath / "propertyTable";
        if (modeStr == "interactive") {
            mode = Mode::Interactive;
        } else if (modeStr == "web") {
            mode = Mode::Web;
        } else if (modeStr == "from_file") {
            mode = Mode::FromFile;
        } else {
            LOG(ERROR) << "Bad mode option value";
            exit(-1);
        }

        // log
        LOG(INFO) << "inputFile: " << inputFile;
        LOG(INFO) << "otherDestDirPath: " << otherDestDirPath;
        LOG(INFO) << "dbName: " << dbName;
        LOG(INFO) << "threadsCount: " << threadsCount;
        LOG(INFO) << "dbOtherDataPath" << dbOtherDataPath;
        LOG(INFO) << "ballTreePath: " << ballTreePath;
        LOG(INFO) << "smilesTablePath: " << smilesTablePath;
        LOG(INFO) << "huffmanCoderPath: " << huffmanCoderPath;
        LOG(INFO) << "idToStringDirPath: " << idToStringDirPath;
        LOG(INFO) << "propertyTableDestinationPath: " << propertyTableDestinationPath;
        for (size_t i = 0; i < dbDataDirsPaths.size(); i++) {
            LOG(INFO) << "dbDataDirPaths[" << i << "]: " << dbDataDirsPaths[i];
        }
        LOG(INFO) << "stopAnswersNumber: " << ansCount;
        for (size_t i = 0; i < destDirPaths.size(); i++) {
            LOG(INFO) << "destDirPaths[" << i << "]: " << destDirPaths[i];
        }
        if (mode == Mode::FromFile) {
            LOG(INFO) << "mode: from file";
        } else if (mode == Mode::Interactive) {
            LOG(INFO) << "mode: interactive";
        } else if (mode == Mode::Web) {
            LOG(INFO) << "mode: web server";
        }
    }
};

shared_ptr<BallTreeSearchEngine> loadBallTree(const Args &args) {
    BufferedReader ballTreeReader(args.ballTreePath);
    LOG(INFO) << "Start ball tree loading";
    auto res = make_shared<BallTreeRAMSearchEngine>(ballTreeReader, args.dbDataDirsPaths);
    LOG(INFO) << "Finish ball tree loading";
    return res;
}

shared_ptr<IdConverter> loadIdConverter(const std::filesystem::path &idToStringDirPath) {
    return std::make_shared<IdConverter>(idToStringDirPath);
}

shared_ptr<vector<PropertiesFilter::Properties>> loadPropertiesTable(const std::filesystem::path &propertiesTablePath) {
    auto res = make_shared<vector<PropertiesFilter::Properties>>();
    auto reader = PropertiesTableReader(propertiesTablePath);
    for (const auto &[id, properties]: reader) {
        assert(id == res->size());
        res->emplace_back(properties);
    }
    return res;
}


int main(int argc, char *argv[]) {
    initLogging(argv, google::INFO, "run_db.info", true);
    Args args(argc, argv);

    TimeTicker timeTicker;

    HuffmanCoder huffmanCoder = HuffmanCoder::load(args.huffmanCoderPath);
    auto loadBallTreeTask = async(launch::async, loadBallTree, cref(args));
    auto loadSmilesTableTask = async(launch::async, loadSmilesTable, cref(args.smilesTablePath), cref(huffmanCoder));
    auto loadIdConverterTask = async(launch::async, loadIdConverter, cref(args.idToStringDirPath));
    auto loadPropertyTableTask = async(launch::async, loadPropertiesTable, cref(args.propertyTableDestinationPath));

    auto ballTreePtr = loadBallTreeTask.get();
    auto smilesTablePtr = loadSmilesTableTask.get();
    auto idConverterPtr = loadIdConverterTask.get();
    auto propertiesTablePtr = loadPropertyTableTask.get();
    timeTicker.tick("DB initialization");

    shared_ptr<RunMode> mode = nullptr;
    if (args.mode == Args::Mode::Interactive)
        mode = make_shared<InteractiveMode>(*ballTreePtr, smilesTablePtr, timeTicker, args.ansCount, args.threadsCount,
                                            propertiesTablePtr);
    else if (args.mode == Args::Mode::FromFile)
        mode = make_shared<FromFileMode>(*ballTreePtr, smilesTablePtr, timeTicker, args.inputFile, args.ansCount,
                                         args.threadsCount, propertiesTablePtr);
    else if (args.mode == Args::Mode::Web)
        mode = make_shared<WebMode>(*ballTreePtr, smilesTablePtr, timeTicker, args.ansCount, args.threadsCount,
                                    idConverterPtr, propertiesTablePtr);

    mode->run();
    return 0;
}