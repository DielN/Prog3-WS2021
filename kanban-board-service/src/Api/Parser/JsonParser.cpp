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

    Value name(column.getName().c_str(), allocator);
    Value items(kArrayType);
    vector<Item> itemVector = column.getItems(); // items ist leer
    for (Item item : itemVector) {
        Value itemValue(kObjectType);
        itemValue.AddMember("id", item.getId(), allocator);
        itemValue.AddMember("title", Value(item.getTitle().c_str(), allocator), allocator);
        itemValue.AddMember("position", item.getPos(), allocator);
        itemValue.AddMember("timestamp", Value(item.getTimestamp().c_str(), allocator), allocator);

        items.PushBack(itemValue, allocator);
    }
    doc.AddMember("id", column.getId(), allocator);
    doc.AddMember("name", name, allocator);
    doc.AddMember("position", column.getPos(), allocator);
    doc.AddMember("items", items, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

string JsonParser::convertToApiString(std::vector<Column> &columns) {
    throw NotImplementedException();
}

string JsonParser::convertToApiString(Item &item) {
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();

    Value title(item.getTitle().c_str(), allocator);
    Value timestamp(item.getTimestamp().c_str(), allocator);

    doc.AddMember("id", item.getId(), allocator);
    doc.AddMember("title", title, allocator);
    doc.AddMember("position", item.getPos(), allocator);
    doc.AddMember("timestamp", timestamp, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

string JsonParser::convertToApiString(std::vector<Item> &items) {
    throw NotImplementedException();
}

std::optional<Column> JsonParser::convertColumnToModel(int columnId, std::string &request) {
    optional<Column> convertedColumn;
    Document doc;
    doc.Parse(request.c_str());

    string name = doc["name"].GetString();
    auto position = doc["position"].GetInt64();

    convertedColumn = Column(columnId, name, position);

    return convertedColumn;
}

std::optional<Item> JsonParser::convertItemToModel(int itemId, std::string &request) {
    optional<Item> convertedItem;
    Document doc;
    doc.Parse(request.c_str());

    string itemTitle = doc["title"].GetString();
    auto itemPosition = doc["position"].GetInt64();

    convertedItem = Item(itemId, itemTitle, itemPosition, "");

    return convertedItem;
}
