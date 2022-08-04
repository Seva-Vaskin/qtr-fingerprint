#include "SplitterTreeUtils.h"

namespace qtr {


    std::pair<uint64_t, uint64_t> splitRawBucketByBit(const std::filesystem::path &fileToSplitPath, uint64_t splitBit,
                                                      const std::filesystem::path &zeroFilePath,
                                                      const std::filesystem::path &onesFilePath) {
        uint64_t leftSize = 0;
        uint64_t rightSize = 0;
        RawBucketWriter zerosWriter(zeroFilePath);
        RawBucketWriter onesWriter(onesFilePath);
        for (const auto &[smiles, fp]: RawBucketReader(fileToSplitPath)) {
            if (fp[splitBit]) {
                onesWriter.write(make_pair(smiles, fp));
                leftSize++;
            } else {
                zerosWriter.write(make_pair(smiles, fp));
                rightSize++;
            }
        }
        remove(fileToSplitPath.c_str()); // Delete current file
        return {leftSize, rightSize};
    }

    uint64_t findBestBitToSplit(const std::filesystem::path &rawBucketPath) {
        uint64_t bucketSize = 0;
        size_t fingerprintSize = fromBytesToBits(IndigoFingerprint::sizeInBytes);
        std::vector<int64_t> columnsSum(fingerprintSize, 0);
        for (const auto &[_, fingerprint]: RawBucketReader(rawBucketPath)) {
            bucketSize++;
            for (size_t i = 0; i < fingerprintSize; ++i)
                columnsSum[i] += fingerprint[i];
        }
        uint64_t bestBit = std::min_element(columnsSum.begin(), columnsSum.end(), [bucketSize](int64_t a, int64_t b) {
            return std::abs(a - (int64_t) bucketSize) < std::abs(b - (int64_t) bucketSize);
        }) - columnsSum.begin();
        return bestBit;
    }
}