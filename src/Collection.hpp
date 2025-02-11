#ifndef GMSV_MONGODB_COLLECTION_HPP
#define GMSV_MONGODB_COLLECTION_HPP

#include "MongoDB.hpp"

int destroy_collection(lua_State* state);

int destroy_collection_async(lua_State* state);

int collection_name(lua_State* state);

int collection_name_async(lua_State* state);

int collection_command(lua_State* state);

int collection_command_async(lua_State* state);

int collection_count(lua_State* state);

int collection_count_async(lua_State* state);

int collection_find(lua_State* state);

int collection_find_async(lua_State* state);

int collection_find_one(lua_State* state);

int collection_find_one_async(lua_State* state);

int collection_insert(lua_State* state);

int collection_insert_async(lua_State* state);

int collection_remove(lua_State* state);

int collection_remove_async(lua_State* state);

int collection_update(lua_State* state);

int collection_update_async(lua_State* state);

int collection_bulk(lua_State* state);

int collection_bulk_async(lua_State* state);

int collection_aggregate(lua_State* state);

int collection_aggregate_async(lua_State* state);

extern int CollectionMetaTableId;

#endif //GMSV_MONGODB_COLLECTION_HPP
