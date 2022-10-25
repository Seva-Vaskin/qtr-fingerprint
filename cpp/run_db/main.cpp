#include <filesystem>
#include <functional>
#include <string>
#include <numeric>
#include <future>
#include <unordered_map>
#include <mutex>
#include <iterator>

#include "crow.h"

#include <glog/logging.h>
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include "Utils.h"
#include "io/BufferedReader.h"
#include "BallTreeRAMSearchEngine.h"
#include "BallTreeDriveSearchEngine.h"
#include "BallTreeNoChecksSearchEngine.h"
#include "fingerprint_table_io/FingerprintTableReader.h"
#include "IndigoSubstructureMatcher.h"
#include "IndigoQueryMolecule.h"
#include "smiles_table_io/SmilesTableReader.h"

ABSL_FLAG(std::vector<std::string>, data_dir_paths, {},
          "Path to directories where data are stored");

ABSL_FLAG(std::string, other_data_path, "",
          "Path to directory with other files");

ABSL_FLAG(std::string, db_name, "",
          "Name of folders with data base's files");

ABSL_FLAG(uint64_t, start_search_depth, -1,
          "Depth for start search from. There will be 2^start_search_depth threads.");

ABSL_FLAG(std::string, mode, "",
          "possible modes: interactive, from_file, web");

ABSL_FLAG(std::string, input_file, "",
          "file to load test molecules from");

ABSL_FLAG(std::uint64_t, ans_count, -1,
          "how many answers program will find");

using SmilesTable = std::vector<qtr::smiles_table_value_t>;

void initLogging(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    google::SetLogDestination(google::INFO, "run_db.info");
    FLAGS_alsologtostderr = true;
}

struct Args {

    enum class Mode {
        Interactive,
        FromFile,
        PseudoRest
    };

    std::vector<std::filesystem::path> dataDirPaths;
    std::filesystem::path otherDataPath;
    std::string dbName;
    uint64_t startSearchDepth;
    Mode mode;
    std::filesystem::path inputFile;
    uint64_t ansCount;

    std::vector<std::filesystem::path> dbDataDirsPaths;
    std::filesystem::path dbOtherDataPath;

    std::filesystem::path ballTreePath;
    std::filesystem::path smilesTablePath;

    Args(int argc, char *argv[]) {
        absl::ParseCommandLine(argc, argv);

        std::vector<std::string> dataDirPathsStrings = absl::GetFlag(FLAGS_data_dir_paths);
        std::copy(dataDirPathsStrings.begin(), dataDirPathsStrings.end(), std::back_inserter(dataDirPaths));
        qtr::emptyArgument(dataDirPaths, "Please specify data_dir_paths option");
        for (size_t i = 0; i < dataDirPaths.size(); i++) {
            LOG(INFO) << "dataDirPaths[" << i << "]: " << dataDirPaths[i];
        }

        otherDataPath = absl::GetFlag(FLAGS_other_data_path);
        qtr::emptyArgument(otherDataPath, "Please specify other_data_path option");
        LOG(INFO) << "otherDataPath: " << otherDataPath;

        dbName = absl::GetFlag(FLAGS_db_name);
        qtr::emptyArgument(dbName, "Please specify db_name option");
        LOG(INFO) << "dbName: " << dbName;

        startSearchDepth = absl::GetFlag(FLAGS_start_search_depth);
        if (startSearchDepth == -1) {
            LOG(INFO) << "Please specify start_search_depth option";
            exit(-1);
        }
        LOG(INFO) << "startSearchDepth: " << startSearchDepth;

        std::string modeStr = absl::GetFlag(FLAGS_mode);
        qtr::emptyArgument(modeStr, "Please specify mode option");
        inputFile = absl::GetFlag(FLAGS_input_file);
        LOG(INFO) << "inputFile: " << inputFile;
        if (modeStr == "interactive") {
            mode = Mode::Interactive;
            LOG(INFO) << "mode: interactive";
        } else if (modeStr == "web") {
            mode = Mode::PseudoRest;
            LOG(INFO) << "mode: web server";
        } else if (modeStr == "from_file") {
            mode = Mode::FromFile;
            LOG(INFO) << "mode: fromFile";
            qtr::emptyArgument(inputFile, "Please specify input_file option");
        } else {
            LOG(ERROR) << "Bad mode option value";
            exit(-1);
        }

        ansCount = absl::GetFlag(FLAGS_ans_count);
        LOG(INFO) << "ansCount: " << ansCount;

        for (auto &dir: dataDirPaths) {
            dbDataDirsPaths.emplace_back(dir / dbName);
        }
        for (size_t i = 0; i < dbDataDirsPaths.size(); i++) {
            LOG(INFO) << "dbDataDirPaths[" << i << "]: " << dbDataDirsPaths[i];
        }

        dbOtherDataPath = otherDataPath / dbName;
        LOG(INFO) << "dbOtherDataPath" << dbOtherDataPath;

        ballTreePath = dbOtherDataPath / "tree";
        LOG(INFO) << "ballTreePath: " << ballTreePath;

        smilesTablePath = dbOtherDataPath / "smilesTable";
        LOG(INFO) << "smilesTablePath: " << smilesTablePath;
    }
};

template<typename SmilesTable>
std::vector<qtr::smiles_table_value_t>
getSmiles(const std::vector<size_t> &smilesIndexes, SmilesTable &smilesTable, uint64_t ansCount) {
    std::vector<qtr::smiles_table_value_t> result;
    for (size_t i = 0; i < smilesIndexes.size() && i < ansCount; i++) {
        result.emplace_back(smilesTable[smilesIndexes[i]]);
    }
    return result;
}

void printSmiles(const std::vector<qtr::smiles_table_value_t> &smiles) {
    size_t answersToPrint = std::min(size_t(10), smiles.size());
    for (size_t i = 0; i < answersToPrint; i++) {
        if (i == 0)
            std::cout << "[";
        std::cout << '\"' << smiles[i].first << '\"';
        std::cout << ", \"" << smiles[i].second << '\"';
        std::cout << (i + 1 == answersToPrint ? "]\n" : ", ");
    }
}

std::pair<bool, SmilesTable>
doSearch(const std::string &querySmiles, const qtr::BallTreeSearchEngine &ballTree,
         const SmilesTable &smilesTable, const Args &args) {
    qtr::IndigoFingerprint fingerprint;
    try {
        fingerprint = qtr::IndigoFingerprintFromSmiles(querySmiles);
    }
    catch (std::exception &exception) {
        std::cout << "skip query:" << exception.what() << std::endl;
        return {true, {}};
    }

    auto filter = [&smilesTable, &querySmiles](size_t ansId) {
        auto [cid, ansSmiles] = smilesTable[ansId];
        auto indigoSessionPtr = indigo_cpp::IndigoSession::create();
        auto queryMol = indigoSessionPtr->loadQueryMolecule(querySmiles);
        queryMol.aromatize();
        try {
            auto candidateMol = indigoSessionPtr->loadMolecule(ansSmiles);
            candidateMol.aromatize();
            auto matcher = indigoSessionPtr->substructureMatcher(candidateMol);
            return bool(indigoMatch(matcher.id(), queryMol.id()));
        }
        catch (std::exception &e) {
            LOG(ERROR) << "Error while filtering answer. "
                          "Query: " << querySmiles << ", candidate: " << ansId << " " << ansSmiles << ", error: "
                       << e.what();
            return false;
        }
    };
    auto candidateIndexes = ballTree.search(fingerprint, args.ansCount, args.startSearchDepth, filter);
    auto answerSmiles = getSmiles(candidateIndexes, smilesTable, args.ansCount);
    LOG(INFO) << "found answers: " << answerSmiles.size();
    printSmiles(answerSmiles);
    return {false, answerSmiles};
}


template<typename SmilesTable>
void runInteractive(const qtr::BallTreeSearchEngine &ballTree, SmilesTable &smilesTable,
                    qtr::TimeTicker &timeTicker, const Args &args) {
    while (true) {
        std::cout << "Enter smiles: ";
        std::string smiles;
        std::cin >> smiles;
        if (smiles.empty())
            break;
        timeTicker.tick();
        const auto result = doSearch(smiles, ballTree, smilesTable, args);
        if (result.first)
            continue;
        std::cout << "Search time: " << timeTicker.tick("Search time") << std::endl;
    }
}

template<typename SmilesTable>
void runFromFile(const qtr::BallTreeSearchEngine &ballTree, SmilesTable &smilesTable,
                 qtr::TimeTicker &timeTicker, const Args &args) {
    std::ifstream input(args.inputFile);
    std::vector<std::string> queries;
    while (input.peek() != EOF) {
        std::string query;
        input >> query;
        std::string otherInfoInLine;
        std::getline(input, otherInfoInLine);
        queries.emplace_back(query);
    }
    std::vector<double> times;
    size_t skipped = 0;
    LOG(INFO) << "Loaded " << queries.size() << " queries";
    for (size_t i = 0; i < queries.size(); i++) {
        LOG(INFO) << "Start search for " << i << ": " << queries[i];
        timeTicker.tick();
        const auto result = doSearch(queries[i], ballTree, smilesTable, args);
        if (result.first) {
            skipped++;
            continue;
        }
        times.emplace_back(timeTicker.tick("search molecule " + std::to_string(i) + ": " + queries[i]));
    }

    double mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    double min = *std::min_element(times.begin(), times.end());
    double max = *std::max_element(times.begin(), times.end());
    std::sort(times.begin(), times.end());
    double median = times[times.size() / 2];
    double p60 = times[times.size() * 6 / 10];
    double p70 = times[times.size() * 7 / 10];
    double p80 = times[times.size() * 8 / 10];
    double p90 = times[times.size() * 9 / 10];
    double p95 = times[times.size() * 95 / 100];
    LOG(INFO) << "skipped queries: " << skipped;
    LOG(INFO) << "  mean: " << mean;
    LOG(INFO) << "   max: " << max;
    LOG(INFO) << "   min: " << min;
    LOG(INFO) << "median: " << median;
    LOG(INFO) << "60%: " << p60 << " | 70%: " << p70 << " | 80%: " << p80 << " | 90%: " << p90 << " | 95%: " << p95;
}

void
loadSmilesTable(std::vector<qtr::smiles_table_value_t> &smilesTable, const std::filesystem::path &smilesTablePath) {
    LOG(INFO) << "Start smiles table loading";
    for (const auto &value: qtr::SmilesTableReader(smilesTablePath)) {
        smilesTable.emplace_back(value);
    }
    LOG(INFO) << "Finish smiles table loading";
}

crow::json::wvalue prepareResponse(const std::vector<uint64_t> &ids, size_t minOffset, size_t maxOffset) {
    crow::json::wvalue::list response;
    std::copy(ids.begin() + minOffset, ids.end() + maxOffset, std::back_inserter(response));
    return crow::json::wvalue{response};
}

void runPseudoRest(const qtr::BallTreeSearchEngine &ballTree, const SmilesTable &smilesTable,
                   qtr::TimeTicker &timeTicker, const Args &args) {
    crow::SimpleApp app;
    std::mutex newTaskMutex;
    uint64_t _queryIdTicker = 0;

    std::unordered_map<uint64_t, std::future<std::pair<bool, SmilesTable>>> tasks;
    std::unordered_map<uint64_t, std::vector<uint64_t>> resultTable;
    std::unordered_map<std::string, uint64_t> queryToId;

    CROW_ROUTE(app, "/query").methods(crow::HTTPMethod::POST)([&tasks, &_queryIdTicker, &queryToId,
                                                                      &ballTree, &smilesTable, &args, &newTaskMutex](
            const crow::request &req) {
        std::string smiles = req.url_params.get("smiles");
        std::lock_guard<std::mutex> lock(newTaskMutex);
        if (!queryToId.contains(smiles)) {
            _queryIdTicker += 1;
            tasks[_queryIdTicker] = std::async(std::launch::async, doSearch, smiles,
                                               std::ref(ballTree), std::ref(smilesTable), args);
            queryToId[smiles] = _queryIdTicker;
        }
        return crow::response(std::to_string(queryToId[smiles]));
    });

    CROW_ROUTE(app, "/query").methods(crow::HTTPMethod::GET)([&tasks, &resultTable](const crow::request &req) {
        auto searchId = crow::utility::lexical_cast<uint64_t>(req.url_params.get("searchId"));
        auto offset = crow::utility::lexical_cast<int>(req.url_params.get("offset"));
        auto limit = crow::utility::lexical_cast<int>(req.url_params.get("limit"));

        if (resultTable.contains(searchId))
            return prepareResponse(resultTable[searchId], offset, offset + limit);
        if (!tasks.contains(searchId))
            return crow::json::wvalue();
        const auto &task = tasks[searchId];
        if (task.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            const auto [isSkipped, result] = tasks[searchId].get();
            std::vector<uint64_t> ids;
            std::transform(result.begin(), result.end(), std::back_inserter(ids),
                           [](const qtr::smiles_table_value_t &v) { return v.first; });
            resultTable[searchId] = std::move(ids);
        }
        else {
            return crow::json::wvalue();
        }
        return prepareResponse(resultTable[searchId], offset, offset + limit);
    });

    app.port(8080).multithreaded().run();
}

int main(int argc, char *argv[]) {
    initLogging(argc, argv);
    Args args(argc, argv);

    qtr::TimeTicker timeTicker;
    qtr::BufferedReader ballTreeReader(args.ballTreePath);

    SmilesTable smilesTable;
    auto loadSmilesTableTask = std::async(std::launch::async, loadSmilesTable, std::ref(smilesTable),
                                          std::cref(args.smilesTablePath));
    LOG(INFO) << "Start ball tree loading";
    qtr::BallTreeNoChecksSearchEngine ballTree(ballTreeReader, args.dbDataDirsPaths);
    LOG(INFO) << "Finish ball tree loading";
    loadSmilesTableTask.get();
    timeTicker.tick("DB initialization");
    if (args.mode == Args::Mode::Interactive)
        runInteractive(ballTree, smilesTable, timeTicker, args);
    else if (args.mode == Args::Mode::FromFile)
        runFromFile(ballTree, smilesTable, timeTicker, args);
    else if (args.mode == Args::Mode::PseudoRest)
        runPseudoRest(ballTree, smilesTable, timeTicker, args);

    return 0;
}