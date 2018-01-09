#include "base_test.hpp"

#include <ftw.h>

#include <metal/metal.h>

int rm(const char *path, const struct stat *s, int flag, struct FTW *f)
{
    int status;
    int (*rm_func)( const char * );

    switch( flag ) {
    default:     rm_func = unlink; break;
    case FTW_DP: rm_func = rmdir;
    }
    if (status = rm_func( path ), status != 0) {
      // perror( path );
    }
    return status;
}

void BaseTest::SetUp() {
    // Recursively remove
    if (nftw("test_files", rm, FOPEN_MAX, FTW_DEPTH)) {
        // perror("test_files");
    }

    mkdir("test_files", S_IRWXU);
    mkdir("test_files/metadata_store", S_IRWXU);

}

void BaseTest::TearDown() {
    mtl_deinitialize();
}

void BaseTest::test_initialize_env() {
    mdb_env_create(&env);
    mdb_env_set_maxdbs(env, 4); // inodes, extents, heap, meta
    int res = mdb_env_open(env, "test_files/metadata_store", 0, 0644);
}

MDB_txn *BaseTest::test_create_txn() {
    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);
    return txn;
}

void BaseTest::test_commit_txn(MDB_txn *txn) {
    mdb_txn_commit(txn);
}

void MetalTest::SetUp() {
    BaseTest::SetUp();

    mtl_initialize("test_files/metadata_store");
}
