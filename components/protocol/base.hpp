#pragma once

#include <string>
#include <vector>
#include <variant>

#include <components/document/document.hpp>

enum class DataType {
    UNKNOWN,
    CHAR,
    DATE,
    DATETIME,
    DECIMAL,
    DOUBLE,
    FLOAT,
    INT,
    LONG,
    REAL,
    SMALLINT,
    TEXT,
    TIME,
    VARCHAR,
};

enum class statement_type : char {
    error = 0x00, // unused
    create_database,
    create_collection,
    drop_collection,
    insert_one,
    insert_many,
    delete_one,
    delete_many,
    update_one,
    update_many
};

// Base struct for every SQL statement
struct statement_t {
    statement_t(statement_type type,  const std::string& database, const std::string& collection)
        : type_(type)
        , database_(database)
        , collection_(collection) {}

    statement_t() :type_(statement_type::error){}

    virtual ~statement_t();

    ///    std::vector<Expr*>* hints;

    statement_type type() const {
        return type_;
    }

    statement_type type_;
    std::string database_;
    std::string collection_;
};

enum class transaction_command : char {
    Begin,
    Commit,
    Rollback
};

struct transaction_statement : statement_t {
    transaction_statement(transaction_command command);
    ~transaction_statement() override;
    transaction_command command;
};

enum class  join_t { inner, full, left, right, cross, natural };

struct collection_ref_t {

};

struct join_definition_t {
    join_definition_t();
    virtual ~join_definition_t();

    collection_ref_t* left;
    collection_ref_t* right;
    /// Expr* condition;

    join_t type;
};