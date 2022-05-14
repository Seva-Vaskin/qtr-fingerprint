#pragma once

#include "indigo.h"
#include "bingo-nosql.h"

#include "IndigoMolecule.h"
#include "IndigoQueryMolecule.h"
#include "IndigoSession.h"
#include "IndigoSDFileIterator.h"
#include "IndigoException.h"
#include "BingoNoSQL.h"
#include "BingoResultIterator.h"

#include "SearchEngineInterface.h"

#include <cstring>

extern qword sessionId;

class BingoSearchEngine : public SearchEngineInterface {
public:
public:
    BingoSearchEngine();

    ~BingoSearchEngine() override;

    void build(const std::string &path) override;

    std::vector<indigo_cpp::IndigoMolecule> findOverMolecules(const indigo_cpp::IndigoQueryMolecule &mol) override;

private:
    int _db = -1;
};