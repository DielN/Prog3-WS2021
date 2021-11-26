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
        "foreign key (column_id) references column (id) ON DELETE CASCADE);";

    result = sqlite3_exec(database, sqlCreateTableColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
    result = sqlite3_exec(database, sqlCreateTableItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    result = sqlite3_exec(database, "PRAGMA foreign_keys = ON;", 0, 0, 0);
    handleSQLError(result, errorMessage);

    // only if dummy data is needed ;)
    //createDummyData();
}

Board BoardRepository::getBoard() {
    vector<Column> columns = getColumns();
    Board board(boardTitle);
    board.setColumns(columns);

    return board;
}

std::vector<Column> BoardRepository::getColumns() {
    // TODO: Error Handling
    vector<Column> columns;
    int result = 0;
    sqlite3_stmt *stmt;

    string sqlGetColumns =
        "SELECT id, name, position FROM column;";

    result = sqlite3_prepare_v2(database, sqlGetColumns.c_str(), -1, &stmt, NULL);
    if (checkForSQLError(result, stmt)) return vector<Column> {};

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        int columnId = sqlite3_column_int(stmt, 0);
        const unsigned char *pColumnName = sqlite3_column_text(stmt, 1);
        string columnName = (pColumnName != NULL ? reinterpret_cast<const char *>(pColumnName) : "NULL");
        int columnPosition = sqlite3_column_int(stmt, 2);

        Column retrievedColumn(columnId, columnName, columnPosition);
        columns.push_back(retrievedColumn);
    }
    if (checkForSQLError(result, stmt)) return vector<Column> {};

    for (auto &column : columns) {
        vector<Item> items = getItems(column.getId());
        column.addItems(items);
    }

    sqlite3_finalize(stmt);

    return columns;
}

std::optional<Column> BoardRepository::getColumn(int id) {
    int result = 0;
    sqlite3_stmt *stmt;

    string sqlGetColumn =
        "SELECT * FROM column "
        "WHERE id = ?";

    // https://stackoverflow.com/a/31168999 & https://www.sqlite.org/c3ref/stmt.html
    result = sqlite3_prepare_v2(database, sqlGetColumn.c_str(), -1, &stmt, NULL);
    if (checkForSQLError(result, stmt)) return nullopt;

    sqlite3_bind_int(stmt, 1, id);

    // "If the SQL statement being executed returns any data, then SQLITE_ROW is returned each time a new row of data is ready"
    if ((result = sqlite3_step(stmt)) != SQLITE_ROW) {
        return nullopt;
    }

    // https://www.sqlite.org/c3ref/column_blob.html
    const unsigned char *pColumnName = sqlite3_column_text(stmt, 1);
    string columnName = (pColumnName != NULL ? reinterpret_cast<const char *>(pColumnName) : "NULL");
    int position = sqlite3_column_int(stmt, 2);

    Column retrievedColumn(id, columnName, position);
    vector<Item> itemsOfColumn = getItems(id);
    retrievedColumn.addItems(itemsOfColumn);

    sqlite3_finalize(stmt);

    return retrievedColumn;
}

std::optional<Column> BoardRepository::postColumn(std::string name, int position) {
    optional<Column> retrievedColumn;
    int result = 0;
    char *errorMessage = nullptr;

    string sqlInsertIntoColumn =
        "INSERT into column (name, position) "
        "VALUES ('" + name + "', " + std::to_string(position) + ");";
    result = sqlite3_exec(database, sqlInsertIntoColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK == result) {
        int id = -1;
        string sqlSelectColumn =
            "SELECT id FROM column "
            "WHERE position = " + std::to_string(position) + ";";
        result = sqlite3_exec(database, sqlSelectColumn.c_str(), getIdCallback, &id, &errorMessage);
        handleSQLError(result, errorMessage);
        retrievedColumn = Column(id, name, position);
    }

    return retrievedColumn;
}

std::optional<Prog3::Core::Model::Column> BoardRepository::putColumn(int id, std::string name, int position) {
    optional<Column> updatedColumn;
    int result = 0;
    char *errorMessage = nullptr;

    string sqlPutColumn =
        "UPDATE column "
        "SET name = '" + name + "', position = " + std::to_string(position) + " "
        "WHERE id = " + std::to_string(id) + ";";
    result = sqlite3_exec(database, sqlPutColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    int modifiedRows = sqlite3_changes(database);
    if (SQLITE_OK == result && modifiedRows > 0) {
        Column column(id, name, position);
        vector<Item> items = getItems(id);
        column.addItems(items);
        updatedColumn = column;
    }

    return updatedColumn;
}

void BoardRepository::deleteColumn(int id) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlDeleteColumn =
        "DELETE FROM column "
        "WHERE id = " + std::to_string(id) + ";";
    result = sqlite3_exec(database, sqlDeleteColumn.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

std::vector<Item> BoardRepository::getItems(int columnId) {
    vector<Item> items;
    int result = 0;
    sqlite3_stmt *stmt;

    string sqlGetItems =
        "SELECT * FROM item "
        "WHERE column_id = ?";
    result = sqlite3_prepare_v2(database, sqlGetItems.c_str(), -1, &stmt, NULL);
    if (checkForSQLError(result, stmt)) return vector<Item> {};
    sqlite3_bind_int(stmt, 1, columnId);

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        int itemId = sqlite3_column_int(stmt, 0);
        const unsigned char *pItemTitle = sqlite3_column_text(stmt, 1);
        string itemTitle = (pItemTitle != NULL ? reinterpret_cast<const char *>(pItemTitle) : "NULL");
        const unsigned char *pItemDate = sqlite3_column_text(stmt, 2);
        string itemDate = (pItemDate != NULL ? reinterpret_cast<const char *>(pItemDate) : "NULL");
        int itemPosition = sqlite3_column_int(stmt, 3);

        Item retrievedItem(itemId, itemTitle, itemPosition, itemDate);
        items.push_back(retrievedItem);
    }
    if (checkForSQLError(result, stmt)) return vector<Item> {};

    sqlite3_finalize(stmt);

    return items;
}

std::optional<Item> BoardRepository::getItem(int columnId, int itemId) {
    vector<Item> items;
    int result = 0;
    char *errorMessage = nullptr;

    string sqlGetItem =
        "SELECT id, title, date, position "
        "FROM item "
        "WHERE column_id = " + std::to_string(columnId) + " AND id = " + std::to_string(itemId) + ";";

    result = sqlite3_exec(database, sqlGetItem.c_str(), getItemsCallback, &items, &errorMessage);
    handleSQLError(result, errorMessage);

    if (SQLITE_OK != result || items.empty()) {
        return nullopt;
    } else {
        return items[0];
    }
}

std::optional<Item> BoardRepository::postItem(int columnId, std::string title, int position) {
    optional<Item> retrievedItem;
    string timestamp = BoardRepository::getTimestamp();
    int result = 0;
    char *errorMessage = nullptr;

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
            "WHERE "
            "position = " + std::to_string(position) + " AND "
            "column_id = " + std::to_string(columnId) +
            ";";
        result = sqlite3_exec(database, sqlSelectItem.c_str(), getIdCallback, &id, &errorMessage);
        handleSQLError(result, errorMessage);

        retrievedItem = Item(id, title, position, timestamp);
    }

    return retrievedItem;
}

std::optional<Prog3::Core::Model::Item> BoardRepository::putItem(int columnId, int itemId, std::string title, int position) {
    optional<Item> updatedItem;
    int result = 0;
    char *errorMessage = nullptr;

    string sqlPutItem =
        "UPDATE item "
        "SET "
        "title = '" + title + "', "
        "position = " + std::to_string(position) + " "
        "WHERE id = " + std::to_string(itemId) + " "
        "AND column_id = " + std::to_string(columnId) + ";";
    result = sqlite3_exec(database, sqlPutItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);

    optional<Item> optModifiedItem = getItem(columnId, itemId);

    int modifiedRows = sqlite3_changes(database);
    if (SQLITE_OK == result && modifiedRows > 0 && optModifiedItem.has_value()) {
        Item modifiedItem = optModifiedItem.value();
        string timestamp = modifiedItem.getTimestamp();
        updatedItem = Item(itemId, title, position, timestamp);
    }

    return updatedItem;
}

void BoardRepository::deleteItem(int columnId, int itemId) {
    int result = 0;
    char *errorMessage = nullptr;

    string sqlDeleteItem =
        "DELETE FROM item "
        "WHERE id = " + std::to_string(itemId) + " AND column_id = " + std::to_string(columnId) + ";";
    result = sqlite3_exec(database, sqlDeleteItem.c_str(), NULL, 0, &errorMessage);
    handleSQLError(result, errorMessage);
}

void BoardRepository::handleSQLError(int statementResult, char *errorMessage) {

    if (statementResult != SQLITE_OK) {
        cout << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
    }
}

bool BoardRepository::checkForSQLError(int statementResult, sqlite3_stmt *stmt) {
    bool hasErrorOccured = statementResult != SQLITE_OK && statementResult != SQLITE_DONE && statementResult != SQLITE_ROW;
    if (hasErrorOccured) {
        hasErrorOccured = true;
        cout << "SQL error: " << sqlite3_errmsg(database) << endl;
        sqlite3_finalize(stmt);
    }

    return hasErrorOccured;
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

std::string BoardRepository::getTimestamp() {
    // https://en.cppreference.com/w/cpp/chrono/c/strftime
    char offset[6];
    char date[100];
    time_t t = std::time(nullptr);

    std::strftime(offset, sizeof(offset), "%z", std::localtime(&t));
    std::string offsetString(offset);
    if (offsetString.empty()) {
        offsetString = "Z";
    } else {
        offsetString.insert(3, ":");
    }

    std::strftime(date, sizeof(date), "%FT%T", std::localtime(&t));
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

optional<Item> BoardRepository::dbRowToItem(int numberOfColumns, char **fieldValues, char **columnNames) {
    const int EXPECTED_COUNT_OF_ITEMS = 4;
    int retrievedValues = 0;

    optional<Item> retrievedItem;
    int itemId = -1;
    string itemTitle;
    string itemDate;
    int itemPosition;

    for (int i = 0; i < numberOfColumns; i++) {
        if (fieldValues[i] == NULL) {
            break;
        }
        string currentColumn(columnNames[i]);
        string valueOfColumn(fieldValues[i]);
        if (currentColumn == "id") {
            itemId = std::stoi(valueOfColumn);
            retrievedValues++;
        } else if (currentColumn == "title") {
            itemTitle = valueOfColumn;
            retrievedValues++;
        } else if (currentColumn == "date") {
            itemDate = valueOfColumn;
            retrievedValues++;
        } else if (currentColumn == "position") {
            itemPosition = std::stoi(valueOfColumn);
            retrievedValues++;
        }
    }
    if (retrievedValues == 4) {
        retrievedItem = Item(itemId, itemTitle, itemPosition, itemDate);
    } else if (retrievedValues > 0) {
        cerr << "Only retrieved " << retrievedValues << " values, but expected " << EXPECTED_COUNT_OF_ITEMS << endl;
    }
    return retrievedItem;
}

int BoardRepository::getItemsCallback(void *data, int numberOfColumns, char **fieldValues, char **columnNames) {
    vector<Item> *items = static_cast<vector<Item> *>(data);
    optional<Item> retrievedItem = dbRowToItem(numberOfColumns, fieldValues, columnNames);
    if (retrievedItem.has_value()) {
        items->push_back(retrievedItem.value());
    }
    return 0;
}
