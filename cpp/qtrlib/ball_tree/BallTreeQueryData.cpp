#include "BallTreeQueryData.h"

#include <algorithm>

using namespace std;

namespace qtr {
    bool BallTreeQueryData::isFinished() const {
        return _startedTasksCount == _finishedTasksCount;
    }

    void BallTreeQueryData::stopProcess() {
        _shouldStopProcess = true;
    }

    void BallTreeQueryData::addAnswers(const vector <CIDType> &answers) {
        lock_guard<mutex> lock(_resultLock);
        copy(answers.begin(), answers.end(), back_inserter(_result));
        if (checkFoundEnoughAnswers())
            stopProcess();
    }

    BallTreeQueryData::BallTreeQueryData(size_t stopAnswersCount, const IndigoFingerprint &query,
                                         unique_ptr <AnswerFilter> &&filter) : _stopAnswersNumber(stopAnswersCount),
                                                                               _queryFingerprint(query),
                                                                               _filter(std::move(filter)),
                                                                               _startedTasksCount(0),
                                                                               _finishedTasksCount(0),
                                                                               _shouldStopProcess(false),
                                                                               _tasks() {}

    const IndigoFingerprint &BallTreeQueryData::getQueryFingerprint() const {
        return _queryFingerprint;
    }

    pair<bool, vector<CIDType>> BallTreeQueryData::getAnswers(size_t beginPos, size_t endPos) {
        lock_guard<mutex> lock(_resultLock);
        beginPos = min(beginPos, _result.size());
        endPos = min(endPos, _result.size());
        vector<CIDType> result(_result.begin() + beginPos, _result.begin() + endPos);
        return {isFinished(), result};
    }

    bool BallTreeQueryData::checkFoundEnoughAnswers() const {
        return getCurrentAnswersCount() >= _stopAnswersNumber;
    }

    size_t BallTreeQueryData::getCurrentAnswersCount() const {
        return _result.size();
    }

    void
    BallTreeQueryData::filterAndAddAnswers(const vector<CIDType> &answers, AnswerFilter &filterObject) {
        vector<CIDType> filteredAnswers;
        for (auto &ans: answers) {
            if (filterObject(ans)) {
                filteredAnswers.emplace_back(ans);
            }
        }
        if (filteredAnswers.empty())
            return;
        addAnswers(filteredAnswers);
    }

    void BallTreeQueryData::waitAllTasks() {
        for (auto &task: _tasks)
            task.wait();
    }

    void BallTreeQueryData::addTask(future<void> &&task) {
        _tasks.emplace_back(std::move(task));
    }

    void BallTreeQueryData::tagFinishTask() {
        _finishedTasksCount++;
    }

    void BallTreeQueryData::tagStartTask() {
        _startedTasksCount++;
    }

    std::unique_ptr<AnswerFilter> BallTreeQueryData::getFilterObject() const {
        return _filter->copy();
    }

    bool BallTreeQueryData::checkShouldStop() const {
        return _shouldStopProcess;
    }
} // namespace qtr