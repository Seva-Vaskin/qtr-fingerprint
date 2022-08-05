#include "indigo.h"

#include "IndigoMolecule.h"
#include "IndigoSession.h"
#include "IndigoWriteBuffer.h"
#include "IndigoSDFileIterator.h"

#include <glog/logging.h>

#include <chrono>
#include <iostream>
#include <filesystem>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include "Utils.h"
#include "Fingerprint.h"
#include "SplitterTree.h"
#include "ColumnsChoice.h"

using namespace indigo_cpp;
using namespace qtr;
using namespace std;

using std::filesystem::path;

vector<int> readColumns(const path &pathToColumns) {
    ifstream fin(pathToColumns.string());
    std::vector<int> zeroColumns;
    int number;
    while (fin >> number) {
        zeroColumns.push_back(number);
    }
    assert(is_sorted(zeroColumns.begin(), zeroColumns.end()));
    return zeroColumns;
}

IndigoFingerprint cutZeroColumns(FullIndigoFingerprint fingerprint, const vector<int> &zeroColumns) {
    IndigoFingerprint cutFingerprint;
    int j = 0;
    int currentZeroPos = 0;
    for (int i = 0; i < fromBytesToBits(qtr::FullIndigoFingerprint::sizeInBytes); ++i) {
        if (currentZeroPos < zeroColumns.size() && i == zeroColumns[currentZeroPos]) {
            currentZeroPos++;
            continue;
        }
        assert(i - currentZeroPos == j);
        cutFingerprint[j++] = fingerprint[i];
    }
    assert(j == fromBytesToBits(IndigoFingerprint::sizeInBytes));
    return cutFingerprint;
}

/**
 * Merges many sdf files into one, named "0". Suitable for Splitter Tree.
 * @param sdfDir -- directory with sdf files
 */
void prepareSDFsForSplitterTree(const path &sdfDir, const path &columnsPath, const path& outFilePath) {
    auto indigoSessionPtr = IndigoSession::create();
    ofstream out(outFilePath);
    auto zeroColumns = readColumns(columnsPath);
    uint64_t cntMols = 0;
    uint64_t cntSkipped = 0;
    out.write((char *) (&cntMols), sizeof(cntMols)); // Reserve space for bucket size
    for (auto &sdfFile: findFiles(sdfDir, ".sdf")) {
        for (auto &mol: indigoSessionPtr->iterateSDFile(sdfFile)) {
            try {
                mol->aromatize();
                int fingerprint = indigoFingerprint(mol->id(), "sub");
                FullIndigoFingerprint fp(indigoToString(fingerprint));
                IndigoFingerprint cutFP = cutZeroColumns(fp, zeroColumns);
                cutFP.saveBytes(out);
                out << mol->smiles() << '\n';
                ++cntMols;
            }
            catch (...) {
                cntSkipped++;
            }
        }
    }
    out.seekp(0, ios::beg); // Seek to the beginning of file
    out.write((char *) (&cntMols), sizeof(cntMols));
    if (cntSkipped)
        std::cerr << "Skipped " << cntSkipped << " molecules\n";
}

ABSL_FLAG(std::string, data_dir_path, "", "Path to data dir");

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    absl::ParseCommandLine(argc, argv);

    auto now = std::chrono::high_resolution_clock::now;
    using duration = std::chrono::duration<double>;

    path dataDirPath = absl::GetFlag(FLAGS_data_dir_path);
    path sdfFilesPath = dataDirPath / "sdf";
    path zeroColumnsPath = dataDirPath / "zero_columns";
    path rawBucketsDirPath = dataDirPath / "raw_buckets";
    path splitterTreeFilePath = dataDirPath / "tree";

    filesystem::create_directory(rawBucketsDirPath);

    auto startTime = now();
    // Parse sdf files
    prepareSDFsForSplitterTree(sdfFilesPath, zeroColumnsPath, rawBucketsDirPath / "0");
    auto timePoint1 = std::chrono::high_resolution_clock::now();
    duration parseSdfTime = timePoint1 - startTime;
    std::cout << "SDF files are parsed in time: " << parseSdfTime.count() << "s\n";

    // Build splitter tree
    SplitterTree tree(rawBucketsDirPath);
    tree.build(30, 100, 2);
    ofstream treeFileOut(splitterTreeFilePath);
    tree.dump(treeFileOut);
    auto timePoint2 = now();
    duration buildSplitterTreeTime = timePoint2 - timePoint1;
    std::cout << "Splitter tree is built in time: " << buildSplitterTreeTime.count() << '\n';

    // Choose minimum correlated columns
    ColumnsChooser<qtr::PearsonCorrelationChoiceFunc> columnsChooser(rawBucketsDirPath,
                                                                     qtr::PearsonCorrelationChoiceFunc());
    columnsChooser.handleRawBuckets();
    auto timePoint3 = now();
    duration chooseMinCorrColsTime = timePoint3 - timePoint2;
    std::cout << "Columns are chosen in time: " << chooseMinCorrColsTime.count() << '\n';

    auto endTime = now();
    duration elapsed_seconds = endTime - startTime;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";
    return 0;
}
