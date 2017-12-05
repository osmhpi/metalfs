#include <sqlite3.h>

#include "metal.h"

sqlite3* db;

int mtl_initialize(char* metadata_store) {
    sqlite3_open(metadata_store, &db);

    return 0;
}

int mtl_deinitialize() {
    sqlite3_close(db);
    return 0;
}

int mtl_open(char* filename) {
    return 0;
}
