#pragma once

#include <vector>
#include <filesystem>
#include <future>
#include <atomic>
#include <type_traits>


#include "BallTree.h"
#include "Fingerprint.h"
#include "BallTreeTypes.h"
#include "BallTreeQueryData.h"
#include "answer_filtering/AnswerFilter.h"
#include "answer_filtering/AlwaysTrueFilter.h"

namespace qtr {

    class BallTreeSearchEngine : public BallTree {
    public:
        template<typename BinaryReader>
        BallTreeSearchEngine(BinaryReader &nodesReader, std::vector<std::filesystem::path> dataDirectories);

    protected:
        void initLeafDataPaths();

        [[nodiscard]] const std::filesystem::path &getLeafFile(size_t nodeId) const;

        template<typename BinaryReader>
        void loadNodes(BinaryReader &reader);

        [[nodiscard]] std::vector<size_t> getLeafIds() const;

        void findLeafs(const IndigoFingerprint &fingerprint, size_t currentNode, std::vector<CIDType> &leafs) const;

        virtual std::vector<CIDType> searchInLeaf(size_t leafId, const IndigoFingerprint &query) const = 0;

        void processLeafGroup(BallTreeQueryData &queryData, std::vector<uint64_t> leafs, size_t group,
                              size_t totalGroups) const;


    public:
        void search(BallTreeQueryData &queryData, size_t threads) const;

    protected:
        std::vector<std::filesystem::path> _leafDataPaths;
    };

    template<typename BinaryReader>
    BallTreeSearchEngine::BallTreeSearchEngine(BinaryReader &nodesReader,
                                               std::vector<std::filesystem::path> dataDirectories)
            : BallTree(dataDirectories) {
        loadNodes(nodesReader);
        assert(__builtin_popcountll(_nodes.size() + 1) == 1);
        _depth = log2Floor(_nodes.size());
        assert((1ull << (_depth + 1)) == _nodes.size() + 1);
        initLeafDataPaths();
    }

    template<typename BinaryReader>
    void BallTreeSearchEngine::loadNodes(BinaryReader &reader) {
        uint64_t treeSize;
        reader.read((char *) &treeSize, sizeof treeSize);
        _nodes.reserve(treeSize);
        for (size_t i = 0; i < treeSize; i++) {
            _nodes.emplace_back();
            _nodes.back().centroid.load(reader);
        }
    }

} // qtr

