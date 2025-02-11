#include "Collection.hpp"
#include <functional>

#define CHECK_COLLECTION() \
        auto collection = LUA->GetUserType<mongoc_collection_t>(1, CollectionMetaTableId); \
        if (collection == nullptr) return 0; \

void run_async(lua_State* state, std::function<void()> task) {
    std::thread([state, task]() {
        task();
    }).detach();
}        

LUA_FUNCTION(destroy_collection) {
    CHECK_COLLECTION()

    mongoc_collection_destroy(collection);

    return 0;
}

LUA_FUNCTION(destroy_collection_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    run_async(LUA, [=]() {
        mongoc_collection_destroy(collection);
        LUA->PushReference(callback_ref);
        LUA->PCall(0, 0);
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_command) {
    CHECK_COLLECTION()

    CHECK_BSON(command, opts)

    SETUP_QUERY(error, reply)

    bool success = mongoc_collection_command_with_opts(collection, command, nullptr, opts, &reply, &error);

    CLEANUP_BSON(command, opts)

    CLEANUP_QUERY(error, reply, !success)

    LUA->ReferencePush(BSONToLua(LUA, &reply));

    return 1;
}

LUA_FUNCTION(collection_command_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Table);
    LUA->CheckType(3, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(command, opts)

    run_async(LUA, [=]() {
        bson_error_t error;
        bson_t reply;
        bool success = mongoc_collection_command_with_opts(collection, command, nullptr, opts, &reply, &error);
        CLEANUP_BSON(command, opts)

        LUA->PushReference(callback_ref);
        if (success) {
            LUA->ReferencePush(BSONToLua(LUA, &reply));
            LUA->PCall(1, 0);
        } else {
            LUA->PushNil();
            LUA->PushString(error.message);
            LUA->PCall(2, 0);
        }
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_name) {
    CHECK_COLLECTION()

    LUA->PushString(mongoc_collection_get_name(collection));

    return 1;
}

LUA_FUNCTION(collection_name_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    run_async(LUA, [=]() {
        const char* name = mongoc_collection_get_name(collection);
        LUA->PushReference(callback_ref);
        LUA->PushString(name);
        LUA->PCall(1, 0);
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_count) {
    CHECK_COLLECTION()

    CHECK_BSON(filter, opts)

    SETUP_QUERY(error)

    int64_t count = mongoc_collection_count_documents(collection, filter, opts, nullptr, nullptr, &error);

    CLEANUP_BSON(filter, opts)

    CLEANUP_QUERY(error, count == -1)

    LUA->PushNumber((double)count);

    return 1;
}

LUA_FUNCTION(collection_count_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(filter, opts)

    run_async(LUA, [=]() {
        bson_error_t error;
        int64_t count = mongoc_collection_count_documents(collection, filter, opts, nullptr, nullptr, &error);
        CLEANUP_BSON(filter, opts)

        LUA->PushReference(callback_ref);
        if (count == -1) {
            LUA->PushNil();
            LUA->PushString(error.message);
            LUA->PCall(2, 0);
        } else {
            LUA->PushNumber((double)count);
            LUA->PCall(1, 0);
        }
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_find) {
    CHECK_COLLECTION()

    CHECK_BSON(filter, opts)

    auto cursor = mongoc_collection_find_with_opts(collection, filter, opts, mongoc_read_prefs_new(MONGOC_READ_PRIMARY));

    CLEANUP_BSON(filter, opts)

    LUA->CreateTable();

    int table = LUA->ReferenceCreate();

    const bson_t * bson;
    for (int i = 0; mongoc_cursor_next(cursor, &bson); ++i) {
        LUA->ReferencePush(table);
        LUA->PushNumber(i + 1);
        LUA->ReferencePush(BSONToLua(LUA, bson));
        LUA->SetTable(-3);
    }

    mongoc_cursor_destroy(cursor);
    LUA->ReferencePush(table);

    return 1;
}

LUA_FUNCTION(collection_find_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(filter, opts)

    run_async(LUA, [=]() {
        auto cursor = mongoc_collection_find_with_opts(collection, filter, opts, mongoc_read_prefs_new(MONGOC_READ_PRIMARY));
        CLEANUP_BSON(filter, opts)
        
        LUA->PushReference(callback_ref);
        LUA->CreateTable();
        int table = LUA->ReferenceCreate();

        const bson_t* bson;
        for (int i = 0; mongoc_cursor_next(cursor, &bson); ++i) {
            LUA->ReferencePush(table);
            LUA->PushNumber(i + 1);
            LUA->ReferencePush(BSONToLua(LUA, bson));
            LUA->SetTable(-3);
        }

        mongoc_cursor_destroy(cursor);
        LUA->ReferencePush(table);
        LUA->PCall(1, 0);
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_find_one) {
    CHECK_COLLECTION()

    CHECK_BSON(filter, opts)

    bson_t options;
    bson_init(&options);
    if (opts) bson_copy_to_excluding_noinit(opts, &options, "limit", "singleBatch", (char *)nullptr);

    BSON_APPEND_INT32(&options, "limit", 1);
    BSON_APPEND_BOOL(&options, "singleBatch", true);

    auto cursor = mongoc_collection_find_with_opts(collection, filter, &options, mongoc_read_prefs_new(MONGOC_READ_PRIMARY));

    CLEANUP_BSON(filter, opts)

    LUA->CreateTable();

    const bson_t* bson;
    if (mongoc_cursor_next(cursor, &bson)) {
        LUA->ReferencePush(BSONToLua(LUA, bson));
        mongoc_cursor_destroy(cursor);
        return 1;
    } else {
        mongoc_cursor_destroy(cursor);
        return -1;
    }
}

LUA_FUNCTION(collection_find_one_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(filter, opts)

    run_async(LUA, [=]() {
        bson_t options;
        bson_init(&options);
        if (opts) bson_copy_to_excluding_noinit(opts, &options, "limit", "singleBatch", (char *)nullptr);

        BSON_APPEND_INT32(&options, "limit", 1);
        BSON_APPEND_BOOL(&options, "singleBatch", true);

        auto cursor = mongoc_collection_find_with_opts(collection, filter, &options, mongoc_read_prefs_new(MONGOC_READ_PRIMARY));
        CLEANUP_BSON(filter, opts)

        LUA->PushReference(callback_ref);
        const bson_t* bson;
        if (mongoc_cursor_next(cursor, &bson)) {
            LUA->ReferencePush(BSONToLua(LUA, bson));
            LUA->PCall(1, 0);
        } else {
            LUA->PushNil();
            LUA->PCall(1, 0);
        }
        mongoc_cursor_destroy(cursor);
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_insert) {
    CHECK_COLLECTION()

    CHECK_BSON(document)

    SETUP_QUERY(error)

    bool success = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, document, nullptr, &error);

    CLEANUP_BSON(document)

    CLEANUP_QUERY(error, !success)

    LUA->PushBool(success);

    return 1;
}

LUA_FUNCTION(collection_insert_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Table);
    LUA->CheckType(3, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(document)

    run_async(LUA, [=]() {
        bson_error_t error;
        bool success = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, document, nullptr, &error);
        CLEANUP_BSON(document)

        LUA->PushReference(callback_ref);
        LUA->PushBool(success);
        if (!success) {
            LUA->PushString(error.message);
        }
        LUA->PCall(success ? 1 : 2, 0);
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_remove) {
    CHECK_COLLECTION()

    CHECK_BSON(selector)

    SETUP_QUERY(error)

    bool success = mongoc_collection_remove(collection, MONGOC_REMOVE_NONE, selector, nullptr, &error);

    CLEANUP_BSON(selector)

    CLEANUP_QUERY(error, !success)

    LUA->PushBool(success);

    return 1;
}

LUA_FUNCTION(collection_remove_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(selector)

    run_async(LUA, [=]() {
        bson_error_t error;
        bool success = mongoc_collection_remove(collection, MONGOC_REMOVE_NONE, selector, nullptr, &error);
        CLEANUP_BSON(selector)

        LUA->PushReference(callback_ref);
        LUA->PushBool(success);
        if (!success) {
            LUA->PushString(error.message);
            LUA->PCall(2, 0);
        } else {
            LUA->PCall(1, 0);
        }
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_update) {
    CHECK_COLLECTION()

    LUA->CheckType(2, GarrysMod::Lua::Type::Table);
    LUA->CheckType(3, GarrysMod::Lua::Type::Table);

    CHECK_BSON(selector, update)

    SETUP_QUERY(error)

    bool success = mongoc_collection_update(collection, MONGOC_UPDATE_MULTI_UPDATE, selector, update, nullptr, &error);

    CLEANUP_BSON(selector, update)

    CLEANUP_QUERY(error, !success)

    LUA->PushBool(success);

    return 1;
}

LUA_FUNCTION(collection_update_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(selector, update)

    run_async(LUA, [=]() {
        bson_error_t error;
        bool success = mongoc_collection_update(collection, MONGOC_UPDATE_MULTI_UPDATE, selector, update, nullptr, &error);
        CLEANUP_BSON(selector, update)

        LUA->PushReference(callback_ref);
        LUA->PushBool(success);
        if (!success) {
            LUA->PushString(error.message);
            LUA->PCall(2, 0);
        } else {
            LUA->PCall(1, 0);
        }
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_bulk) {
    CHECK_COLLECTION()

    CHECK_BSON(opts)

    auto bulk = mongoc_collection_create_bulk_operation_with_opts(collection, opts);

    CLEANUP_BSON(opts)

    LUA->PushUserType(bulk, BulkMetaTableId);

    return 1;
}

LUA_FUNCTION(collection_bulk_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(opts)

    run_async(LUA, [=]() {
        auto bulk = mongoc_collection_create_bulk_operation_with_opts(collection, opts);
        CLEANUP_BSON(opts)

        LUA->PushReference(callback_ref);
        LUA->PushUserType(bulk, BulkMetaTableId);
        LUA->PCall(1, 0);
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}

LUA_FUNCTION(collection_aggregate) {
    CHECK_COLLECTION()

    CHECK_BSON(pipeline, opts)

    SETUP_QUERY(error)

    auto cursor = mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, pipeline, opts, nullptr);

    CLEANUP_BSON(pipeline, opts)

    if (!cursor) {
        CLEANUP_QUERY(error, true)
        LUA->PushNil();
        LUA->PushString("Failed to create aggregate cursor");
        return 2;
    }

    LUA->CreateTable();
    int table = LUA->ReferenceCreate();

    const bson_t* bson;
    int index = 1;
    while (mongoc_cursor_next(cursor, &bson)) {
        LUA->ReferencePush(table);
        LUA->PushNumber(index++);
        LUA->ReferencePush(BSONToLua(LUA, bson));
        LUA->SetTable(-3);
    }

    if (mongoc_cursor_error(cursor, &error)) {
        mongoc_cursor_destroy(cursor);
        CLEANUP_QUERY(error, true)
        LUA->PushNil();
        LUA->PushString(error.message);
        return 2;
    }

    mongoc_cursor_destroy(cursor);
    LUA->ReferencePush(table);

    CLEANUP_QUERY(error, false)

    return 1;
}

LUA_FUNCTION(collection_aggregate_async) {
    CHECK_COLLECTION()
    LUA->CheckType(2, GarrysMod::Lua::Type::Function);
    int callback_ref = LUA->ReferenceCreate();

    CHECK_BSON(pipeline, opts)

    run_async(LUA, [=]() {
        bson_error_t error;
        auto cursor = mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, pipeline, opts, nullptr);
        CLEANUP_BSON(pipeline, opts)

        LUA->PushReference(callback_ref);
        if (!cursor) {
            LUA->PushNil();
            LUA->PushString("Failed to create aggregate cursor");
            LUA->PCall(2, 0);
            LUA->ReferenceFree(callback_ref);
            return;
        }

        LUA->CreateTable();
        int table = LUA->ReferenceCreate();
        const bson_t* bson;
        int index = 1;
        while (mongoc_cursor_next(cursor, &bson)) {
            LUA->ReferencePush(table);
            LUA->PushNumber(index++);
            LUA->ReferencePush(BSONToLua(LUA, bson));
            LUA->SetTable(-3);
        }

        if (mongoc_cursor_error(cursor, &error)) {
            mongoc_cursor_destroy(cursor);
            LUA->PushNil();
            LUA->PushString(error.message);
            LUA->PCall(2, 0);
        } else {
            mongoc_cursor_destroy(cursor);
            LUA->ReferencePush(table);
            LUA->PCall(1, 0);
        }
        LUA->ReferenceFree(callback_ref);
    });
    return 0;
}