#include "IndigoFilter.h"

#include "glog/logging.h"
#include "IndigoMolecule.h"
#include "indigo.h"

using namespace std;

namespace qtr {
    bool IndigoFilter::operator()(CIDType id) {
        const auto &smiles = getSmiles(id);
        auto startTime = std::chrono::high_resolution_clock::now();
        bool result;
        try {
            auto candidateMol = _indigoSessionPtr->loadMolecule(smiles);
            candidateMol.aromatize();
            auto matcher = _indigoSessionPtr->substructureMatcher(candidateMol);
            result = bool(indigoMatch(matcher.id(), _queryMolecule.id()));
        }
        catch (std::exception &e) {
            LOG(ERROR) << "Indigo error while filtering. "
                          "Query: " << _querySmiles << ", candidate: " << id << " " << smiles << ", error: "
                       << e.what();
            result = false;
        }
        std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - startTime;
        _timer += duration.count();
        return result;
    }

    IndigoFilter::IndigoFilter(std::shared_ptr<const std::string> querySmiles) :
            _querySmiles(std::move(querySmiles)), _indigoSessionPtr(indigo_cpp::IndigoSession::create()),
            _queryMolecule(_indigoSessionPtr->loadQueryMolecule(*_querySmiles)) {
        _queryMolecule.aromatize();
    }

    IndigoFilter::~IndigoFilter() {
        indigoFilteringTimer += _timer;
    }
} // qtr