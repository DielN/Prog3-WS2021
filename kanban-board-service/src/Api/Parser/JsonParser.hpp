#pragma once

#include "ParserIf.hpp"
#include "rapidjson/document.h"

namespace Prog3 {
namespace Api {
namespace Parser {

class JsonParser : public ParserIf {
  private:
    static inline std::string const EMPTY_JSON = "{}";

    std::string valueToString(rapidjson::Value &value);

    bool isValidColumn(rapidjson::Document const &document);
    bool isValidItem(rapidjson::Document const &document);

    rapidjson::Value convertSingleColumnToValue(Prog3::Core::Model::Column &column, rapidjson::Document::AllocatorType &allocator);
    rapidjson::Value convertColumnsToValue(std::vector<Prog3::Core::Model::Column> &columns, rapidjson::Document::AllocatorType &allocator);

    rapidjson::Value convertItemsToValue(std::vector<Prog3::Core::Model::Item> &items, rapidjson::Document::AllocatorType &allocator);
    rapidjson::Value convertSingleItemToValue(Prog3::Core::Model::Item &item, rapidjson::Document::AllocatorType &allocator);

  public:
    JsonParser(){};
    virtual ~JsonParser(){};

    virtual std::string convertToApiString(Prog3::Core::Model::Board &board);

    virtual std::string convertToApiString(Prog3::Core::Model::Column &column);
    virtual std::string convertToApiString(std::vector<Prog3::Core::Model::Column> &columns);

    virtual std::string convertToApiString(Prog3::Core::Model::Item &item);
    virtual std::string convertToApiString(std::vector<Prog3::Core::Model::Item> &items);

    virtual std::optional<Prog3::Core::Model::Column> convertColumnToModel(int columnId, std::string &request);
    virtual std::optional<Prog3::Core::Model::Item> convertItemToModel(int itemId, std::string &request);

    virtual std::string getEmptyResponseString() {
        return JsonParser::EMPTY_JSON;
    }
};

} // namespace Parser
} // namespace Api
} // namespace Prog3
