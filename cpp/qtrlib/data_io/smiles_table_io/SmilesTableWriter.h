#pragma once

#include <string>

#include "glog/logging.h"

#include "SmilesTableIOConsts.h"
#include "basic_io/BasicDataWriter.h"

namespace qtr {

    class SmilesTableWriter
            : public BasicDataWriter<smiles_table_value_t, SmilesTableWriter, std::ofstream> {
    private:
        uint64_t _writtenSmiles;

    public:
        explicit SmilesTableWriter(const std::filesystem::path &fileName) : BaseWriter(fileName), _writtenSmiles(0) {
            _binaryWriter->write((char *) &_writtenSmiles, sizeof _writtenSmiles); // reserve space for table size
            LOG(INFO) << "Create SMILES table writer to " << fileName << " (" << _binaryWriter << ")";
        }

        ~SmilesTableWriter() override {
            _binaryWriter->seekp(0, std::ios::beg);
            _binaryWriter->write((char *) &_writtenSmiles, sizeof _writtenSmiles); // write bucket size
            LOG(INFO) << "Delete SMILES table writer with " << _writtenSmiles << " molecules (" << _binaryWriter << ")";
        }

        SmilesTableWriter &operator<<(const WriteValue &value) override {
            _writtenSmiles++;
            auto &[id, smiles] = value;
            _binaryWriter->write((char *) &id, sizeof id);
            _binaryWriter->write(smiles.c_str(), smiles.size());
            _binaryWriter->write("\n", 1);
            return *this;
        }

        using BaseWriter::operator<<;
    };

} // qtr