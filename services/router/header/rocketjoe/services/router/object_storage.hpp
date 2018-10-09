#pragma once

#include <unordered_map>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/value_context.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/cursor.hpp>
#include <mongocxx/bulk_write.hpp>

class object_storage final {
    object_storage() = delete;

    object_storage &operator=(object_storage &&) = default;

    object_storage(object_storage &&) = default;

    object_storage &operator=(const object_storage &) = default;

    object_storage(const object_storage &) = default;

    ~object_storage() = default;

    object_storage(const std::string& uri_mongo){
        uri = mongocxx::uri{uri_mongo};
        client = mongocxx::client{uri};
    }

    mongocxx::database& find_and_create_database(const std::string&name) {

        auto db = database_storage.find(name);

        if( db == database_storage.end()){

            auto result  = database_storage.emplace(name,client.database(name));
            return result.first->second;
        } else {
            return db->second;
        }

    }

    mongocxx::collection find_and_create_collection(const std::string&db_name,const std::string&collection) {

        auto& db = find_and_create_database(db_name);

        if( !db.has_collection(collection) ){
            return  db.create_collection(collection);
        } else {
            return  db.collection(collection);
        }

    }

private:
    mongocxx::instance mongo_inst;
    std::unordered_map<std::string,mongocxx::database> database_storage;

    mongocxx::uri uri;
    mongocxx::client client;
    mongocxx::options::bulk_write bulk_opts;
};