#pragma once

#include "SearchData.h"

namespace qtr {

    class DriveSearchData : public SearchData {
    public:

        DriveSearchData(std::shared_ptr<const BallTreeSearchEngine> ballTree,
                        std::shared_ptr<const IdConverter> idConverter,
                        TimeTicker &timeTicker, size_t ansCount,
                        size_t threadCount, double timeLimit);

        [[nodiscard]] DerivedClasses getClass() const override;

    };

} // qtr
