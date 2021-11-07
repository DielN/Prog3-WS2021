#include "BoardRepository.hpp"
#include "Core/Exception/NotImplementedException.hpp"
#include <ctime>
#include <filesystem>
#include <string.h>

using namespace Prog3::Repository::SQLite;
using namespace Prog3::Core::Model;
using namespace Prog3::Core::Exception;
using namespace std;

#ifdef RELEASE_SERVICE
string const BoardRepository::databaseFile = "./data/kanban-board.db";
#else
string const BoardRepository::databaseFile = "../data/kanban-board.db";
#endif

BoardRepository::BoardRepository() : database(nullptr) {

    string databaseDirectory = filesystem::path(databaseFile).parent_path().string();

    if (filesystem::is_directory(databaseDirectory) == false) {
        filesystem::create_directory(databaseDirectory);
    }

    int result = sqlite3_open(databaseFile.c_str(), &database);

    if (SQLITE_OK != result) {
        cout << "Cannot open database: " << sqlite3_errmsg(database) << endl;
    }

    initialize();
}

BoardRepository::~BoardRepository() {
    sqlite3_close(database);
}

void BoardRepository::initialize() {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlCreateTableColumn =
        "create table if not exists column("
        "id integer not null primary key autoincrement,"
        "name text not null,"
        "position integer not null UNIQUE);";

    string sqlCreateTableItem =
        "create table if not exists item("
        "id integer not null primary key autoincrement,"
        "title text not null,"
        "date text not null,"
        "position integer not null,"
        "column_id integer not null,"
        "unique (position, column_id),"
        "foreign key (column_id) references column (id));";

    result = sqlite3_exec(database, sqlCreateTableColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
    result = sqlite3_exec(database, sqlCreateTableItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    // only if dummy data is needed ;)
    //createDummyData();
}

Board BoardRepository::getBoard() {
    throw NotImplementedException();
}

std::vector<Column> BoardRepository::getColumns() {
    throw NotImplementedException();
}

std::optional<Column> BoardRepository::getColumn(int id) {
    throw NotImplementedException();
}

std::optional<Column> BoardRepository::postColumn(std::string name, int position) {
    optional<Column> retrievedColumn;
    int result = 0;
    char *errorMessage = nullptr;

    int id2;
    string sqlInsertIntoColumn =
        "insert into column (name, position)"
        "VALUES ('" +
        name + "', " + std::to_string(position) + ");";
    result = sqlite3_exec(database, sqlInsertIntoColumn.c_str(), getIdCallback, &id2, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        int id;
        string sqlSelectColumn =
            "select id from column "
            "where position = " +
            std::to_string(position) + ";";
        result = sqlite3_exec(database, sqlSelectColumn.c_str(), getIdCallback, &id, &errorMessage);
        handleSQLError(result, errorMessage);
        retrievedColumn = Column(id, name, position);
    }

    return retrievedColumn;
}

std::optional<Prog3::Core::Model::Column> BoardRepository::putColumn(int id, std::string name, int position) {
    throw NotImplementedException();
}

void BoardRepository::deleteColumn(int id) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlDeleteColumn =
        "DELETE FROM column "
        "WHERE id = " +
        std::to_string(id) + ";";

    result = sqlite3_exec(database, sqlDeleteColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

std::vector<Item> BoardRepository::getItems(int columnId) {
    throw NotImplementedException();
}

std::optional<Item> BoardRepository::getItem(int columnId, int itemId) {
    throw NotImplementedException();
}

std::optional<Item> BoardRepository::postItem(int columnId, std::string title, int position) {
    optional<Item> retrievedItem;
    time_t t = std::time(nullptr);
    string timestamp = BoardRepository::getTimestamp(t);
    int result = 0;
    char *errorMessage = nullptr;

    // getColumn(columnId) -> prüfen ob column mit gegebener ID exisitiert

    string sqlInsertItem =
        "INSERT INTO item (title, position, column_id, date)"
        "VALUES ('" +
        title + "', " +
        std::to_string(position) + ", " +
        std::to_string(columnId) + ", '" +
        timestamp +
        "');";
    result = sqlite3_exec(database, sqlInsertItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        int id;
        string sqlSelectItem =
            "SELECT id FROM item "
            "WHERE position = " +
            std::to_string(position) + ";";
        result = sqlite3_exec(database, sqlSelectItem.c_str(), getIdCallback, &id, &errorMessage);
        handleSQLError(result, errorMessage);

        retrievedItem = Item(id, title, position, timestamp);
    }

    return retrievedItem;
}

std::optional<Prog3::Core::Model::Item> BoardRepository::putItem(int columnId, int itemId, std::string title, int position) {
    throw NotImplementedException();
}

void BoardRepository::deleteItem(int columnId, int itemId) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlDeleteItem =
        "DELETE FROM item "
        "WHERE id = " +
        std::to_string(itemId) + ";";
    result = sqlite3_exec(database, sqlDeleteItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

void BoardRepository::handleSQLError(int statementResult, char *errorMessage) {

    if (statementResult != SQLITE_OK) {
        cout << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
    }
}

void BoardRepository::createDummyData() {

    cout << "creatingDummyData ..." << endl;

    int result = 0;
    char *errorMessage;
    string sqlInsertDummyColumns =
        "insert into column (name, position)"
        "VALUES"
        "(\"prepare\", 1),"
        "(\"running\", 2),"
        "(\"finished\", 3);";

    result = sqlite3_exec(database, sqlInsertDummyColumns.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    string sqlInserDummyItems =
        "insert into item (title, date, position, column_id)"
        "VALUES"
        "(\"in plan\", date('now'), 1, 1),"
        "(\"some running task\", date('now'), 1, 2),"
        "(\"finished task 1\", date('now'), 1, 3),"
        "(\"finished task 2\", date('now'), 2, 3);";

    result = sqlite3_exec(database, sqlInserDummyItems.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

std::string BoardRepository::getTimestamp(time_t time) {
    // https://en.cppreference.com/w/cpp/chrono/c/strftime
    char offset[6];
    char date[100];

    std::strftime(offset, sizeof(offset), "%z", std::localtime(&time));
    std::string offsetString(offset);
    if (offsetString.empty()) {
        offsetString = "Z";
    } else {
        offsetString.insert(3, ":");
    }

    std::strftime(date, sizeof(date), "%FT%T", std::localtime(&time));
    std::string dateString(date);
    dateString += offsetString;

    return dateString;
}

/*
  I know source code comments are bad, but this one is to guide you through the use of sqlite3_exec() in case you want to use it.
  sqlite3_exec takes a "Callback function" as one of its arguments, and since there are many crazy approaches in the wild internet,
  I want to show you how the signature of this "callback function" may look like in order to work with sqlite3_exec()
*/
int BoardRepository::getIdCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    int *i = static_cast<int *>(data);
    *i = std::stoi(fieldValues[0]);
    return 0;
}
