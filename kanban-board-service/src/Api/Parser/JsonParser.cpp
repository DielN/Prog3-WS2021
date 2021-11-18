#define RAPIDJSON_ASSERT(x)

#include "JsonParser.hpp"
#include "Core/Exception/NotImplementedException.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace Prog3::Api::Parser;
using namespace Prog3::Core::Model;
using namespace Prog3::Core::Exception;
using namespace rapidjson;
using namespace std;

string JsonParser::convertToApiString(Board &board) {
    throw NotImplementedException();
}

string JsonParser::convertToApiString(Column &column) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();

    Value columnAsValue = convertSingleColumnToValue(column, allocator);
    doc.CopyFrom(columnAsValue, allocator);

    return valueToString(doc);
}

string JsonParser::convertToApiString(std::vector<Column> &columns) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();

    Value columnsAsValue = convertColumnsToValue(columns, allocator);

    return valueToString(columnsAsValue);
}

Value JsonParser::convertSingleColumnToValue(Column &column, rapidjson::Document::AllocatorType &allocator) {
    Value columnValue(kObjectType);

    Value name(column.getName().c_str(), allocator);
    vector<Item> itemVector = column.getItems();
    Value items = convertItemsToValue(itemVector, allocator);

    columnValue.AddMember("id", column.getId(), allocator);
    columnValue.AddMember("name", name, allocator);
    columnValue.AddMember("position", column.getPos(), allocator);
    columnValue.AddMember("items", items, allocator);

    return columnValue;
}

Value JsonParser::convertColumnsToValue(std::vector<Column> &columns, rapidjson::Document::AllocatorType &allocator) {
    Value columnsValue(kArrayType);

    for (Column &column : columns) {
        Value singleColumnValue = convertSingleColumnToValue(column, allocator);
        columnsValue.PushBack(singleColumnValue, allocator);
    }

    return columnsValue;
}

string JsonParser::convertToApiString(Item &item) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();

    Value itemAsValue = convertSingleItemToValue(item, allocator);
    doc.CopyFrom(itemAsValue, allocator);

    return valueToString(doc);
}

string JsonParser::convertToApiString(std::vector<Item> &items) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();

    Value itemsValue = convertItemsToValue(items, allocator);

    return valueToString(itemsValue);
}

Value JsonParser::convertSingleItemToValue(Item &item, rapidjson::Document::AllocatorType &allocator) {
    Value itemValue(kObjectType);

    itemValue.AddMember("id", item.getId(), allocator);
    itemValue.AddMember("title", Value(item.getTitle().c_str(), allocator), allocator);
    itemValue.AddMember("position", item.getPos(), allocator);
    itemValue.AddMember("timestamp", Value(item.getTimestamp().c_str(), allocator), allocator);

    return itemValue;
}

Value JsonParser::convertItemsToValue(vector<Item> &items, rapidjson::Document::AllocatorType &allocator) {
    Value itemsValue(kArrayType);

    for (Item &item : items) {
        Value singleItemValue = convertSingleItemToValue(item, allocator);
        itemsValue.PushBack(singleItemValue, allocator);
    }

    return itemsValue;
}

std::optional<Column> JsonParser::convertColumnToModel(int columnId, std::string &request) {
    optional<Column> convertedColumn;
    Document doc;
    doc.Parse(request.c_str());

    if (!isValidColumn(doc)) {
        return convertedColumn;
    }

    string name = doc["name"].GetString();
    auto position = doc["position"].GetInt64();

    convertedColumn = Column(columnId, name, position);

    return convertedColumn;
}

std::optional<Item> JsonParser::convertItemToModel(int itemId, std::string &request) {
    optional<Item> convertedItem;
    Document doc;
    doc.Parse(request.c_str());

    if (!isValidItem(doc)) {
        return convertedItem;
    }

    string itemTitle = doc["title"].GetString();
    auto itemPosition = doc["position"].GetInt64();

    convertedItem = Item(itemId, itemTitle, itemPosition, "");

    return convertedItem;
}

string JsonParser::valueToString(Value &value) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    value.Accept(writer);

    return buffer.GetString();
}

bool JsonParser::isValidColumn(rapidjson::Document const &document) {

    bool isValid = true;

    if (document.HasParseError()) {
        isValid = false;
    }
    if (false == document["name"].IsString()) {
        isValid = false;
    }
    if (false == document["position"].IsInt()) {
        isValid = false;
    }

    return isValid;
}

bool JsonParser::isValidItem(rapidjson::Document const &document) {

    bool isValid = true;

    if (document.HasParseError()) {
        isValid = false;
    }
    if (false == document["title"].IsString()) {
        isValid = false;
    }
    if (false == document["position"].IsInt()) {
        isValid = false;
    }

    return isValid;
}
