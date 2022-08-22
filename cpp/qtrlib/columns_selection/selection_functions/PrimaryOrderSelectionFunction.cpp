#include "PrimaryOrderSelectionFunction.h"

#include <numeric>

namespace qtr {

    selection_result_t PrimaryOrderSelectionFunction::operator()(const IndigoFingerprintTable &fingerprints) const {
        selection_result_t result(IndigoFingerprint::sizeInBits);
        std::iota(result.begin(), result.end(), 0);
        return result;
    }

} // namespace qtr
