#include "BingoSearchEngine.h"

using namespace indigo_cpp;

namespace qtr {

BingoSearchEngine::BingoSearchEngine(const IndigoSessionPtr &indigoSessionPtr) : _indigoSessionPtr(indigoSessionPtr) {}

BingoSearchEngine::~BingoSearchEngine() {
    LOG(INFO) << "Bingo search engine destructor";
    if (currentDatabaseState != NOT_CREATED && currentDatabaseState != CLOSED) {
        bingoCloseDatabase(_db);
        currentDatabaseState = CLOSED;
    }
}

void BingoSearchEngine::build(const std::string &path) {
    
    if (endsWith(path, ".sdf")) {
        
        LOG(INFO) << "Loading database from " << path << " file";
        
        std::string dbName = path.substr(0, path.size() - strlen(".sdf"));
        _db = bingoCreateDatabaseFile(dbName.c_str(), "molecule", "");
        
        size_t moleculesNumber = 0;
        IndigoSDFileIterator iterator = _indigoSessionPtr->iterateSDFile(path);
        
        for(IndigoMoleculeSPtr &molecule : iterator) {       
            molecule->aromatize();
            bingoInsertRecordObj(_db, molecule->id());

            moleculesNumber++;
            if (moleculesNumber % 1000 == 0)
                LOG(INFO) << "Processed " << moleculesNumber << " molecules...";
        }
    }
    else {
        LOG(INFO) << "Loading database from dir=" << path;
        _db = bingoLoadDatabaseFile(path.c_str(), "");
    }
    currentDatabaseState = LOADED;
    LOG(INFO) << "Build search engine with path=" << path << " has finished successfully";
}

std::vector<indigo_cpp::IndigoMolecule>
BingoSearchEngine::findOverMolecules(const indigo_cpp::IndigoQueryMolecule &mol) {
    if (currentDatabaseState != LOADED) {
        LOG(WARNING) << "FindOverMolecules called before build, database is not loaded";
        throw std::runtime_error("Database is not loaded");
    }
    std::vector<IndigoMolecule> result;
    int resultIterator = _indigoSessionPtr->_checkResult(bingoSearchSub(_db, mol.id(), ""));
    int currentId = bingoGetObject(resultIterator);
    while (bingoNext(resultIterator)) {
        const std::string &smiles = indigoCanonicalSmiles(currentId);
        IndigoMolecule curr = _indigoSessionPtr->loadMolecule(smiles);
        result.emplace_back(curr);
    }
    bingoEndSearch(resultIterator);
    return result;
}

} // namespace qtr
