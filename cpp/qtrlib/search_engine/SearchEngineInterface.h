#pragma once

#include "IndigoMolecule.h"
#include "IndigoQueryMolecule.h"

#include <string>
#include <vector>

namespace qtr {

class SearchEngineInterface {
public:
    /**
     * Build search engine from SDF file
     *
     * @param path Path to the file with .sdf.gz extension
     */
    virtual void build(const std::string& path) = 0;

    /**
     * Find all "parent" molecules. Given molecule should be sub molecule for parent.
     *
     * If build was not called behavior is undefined
     *
     * @param mol Indigo molecule
     * @return
     */
    virtual std::vector<indigo_cpp::IndigoMolecule> findOverMolecules(const indigo_cpp::IndigoQueryMolecule &mol) = 0;

    virtual ~SearchEngineInterface() = default;
};

typedef std::shared_ptr<SearchEngineInterface> SearchEnginePtr;

} // namespace qtr

