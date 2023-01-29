#include "WebMode.h"

#include <utility>

using namespace std;

namespace qtr {


    WebMode::WebMode(const qtr::BallTreeSearchEngine &ballTree, std::shared_ptr<const SmilesTable> smilesTable,
                     qtr::TimeTicker &timeTicker, uint64_t ansCount, uint64_t threadsCount,
                     std::shared_ptr<const IdConverter> idConverter,
                     std::shared_ptr<const std::vector<PropertiesFilter::Properties>> molPropertiesTable)
            : _ballTree(ballTree), _smilesTable(std::move(smilesTable)), _ansCount(ansCount),
              _threadsCount(threadsCount), _idConverter(std::move(idConverter)),
              _molPropertiesTable(std::move(molPropertiesTable)) {}


    crow::json::wvalue
    WebMode::prepareResponse(BallTreeQueryData &queryData, size_t minOffset, size_t maxOffset) {
        cout << minOffset << " " << maxOffset << endl;
        auto [isFinish, result] = queryData.getAnswers(minOffset, maxOffset);
        LOG(INFO) << "found " << result.size() << " results";
        crow::json::wvalue::list molecules;
        molecules.reserve(result.size());
        for (auto res: result) {
            auto [id, libraryId] = _idConverter->fromDbId(res);
            molecules.emplace_back(crow::json::wvalue{{"id",        id},
                                                      {"libraryId", libraryId}});
        }
        return crow::json::wvalue{
                {"molecules",  molecules},
                {"isFinished", isFinish}
        };
    }

    PropertiesFilter::Bounds extractBounds(const crow::json::rvalue &json) {
        PropertiesFilter::Bounds bounds;

        for (size_t i = 0; i < std::size(PropertiesFilter::propertyNames); ++i) {
            std::string minKey = PropertiesFilter::propertyNames[i] + "_MIN";
            std::string maxKey = PropertiesFilter::propertyNames[i] + "_MAX";

            if (json.has(minKey)) {
                bounds.minBounds[i] = (float)json[minKey].d();
            }
            if (json.has(maxKey)) {
                bounds.maxBounds[i] = (float)json[maxKey].d();
            }
        }

        return bounds;
    }


    void WebMode::run() {
        crow::SimpleApp app;
        mutex newTaskMutex;

        vector<string> smiles;
        vector<unique_ptr<BallTreeQueryData>> queries;

        CROW_ROUTE(app, "/query")
                .methods(crow::HTTPMethod::POST)([&queries, &newTaskMutex, this, &smiles](const crow::request &req) {
                    auto json = crow::json::load(req.body);
                    if (!json)
                        return crow::response(400);

                    lock_guard<mutex> lock(newTaskMutex);
                    smiles.emplace_back(json["smiles"].s());
                    string &currSmiles = smiles.back();

                    auto bounds = extractBounds(json);
                    size_t queryId = queries.size();
                    auto [error, queryData] = doSearch(currSmiles, _ballTree, _smilesTable, _ansCount,
                                                       _threadsCount, bounds, _molPropertiesTable);
                    queries.emplace_back(std::move(queryData));

                    return crow::response(to_string(queryId));
                });

        CROW_ROUTE(app, "/query")
                .methods(crow::HTTPMethod::GET)([&queries, this](const crow::request &req) {
                    auto searchId = crow::utility::lexical_cast<uint64_t>(req.url_params.get("searchId"));
                    auto offset = crow::utility::lexical_cast<int>(req.url_params.get("offset"));
                    auto limit = crow::utility::lexical_cast<int>(req.url_params.get("limit"));

                    if (searchId >= queries.size())
                        return crow::json::wvalue();

                    return prepareResponse(*queries[searchId], offset, offset + limit);
                });

        app.port(8080).concurrency(2).run();

    }

} // namespace qtr