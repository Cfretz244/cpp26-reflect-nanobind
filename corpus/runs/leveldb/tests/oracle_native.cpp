// Native C++ ground-truth oracle for the leveldb binding (Layer-1 differential). Drives the
// SAME open-put-get-delete-get / WriteBatch / re-open scenario the Python test drives through
// the binding, against a fresh on-disk database, and emits the observable results as one JSON
// object. Shared compiler + shared leveldb => any divergence is the binding layer's.
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
    const std::string dbpath = (argc > 1) ? argv[1] : "/tmp/leveldb_oracle_db";

    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) { if (c == '\\' || c == '"') e += '\\'; e += c; }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };

    // Start from a clean slate.
    leveldb::Options opts;
    leveldb::DestroyDB(dbpath, opts);

    // --- open with create_if_missing ---
    opts.create_if_missing = true;
    leveldb::DB* db = nullptr;
    leveldb::Status s = leveldb::DB::Open(opts, dbpath, &db);
    add_b("open_ok", s.ok());
    add_s("open_str", s.ToString());

    leveldb::WriteOptions wopts;
    leveldb::ReadOptions ropts;

    // --- put alpha=1, beta=2 ---
    add_b("put_alpha_ok", db->Put(wopts, "alpha", "one").ok());
    add_b("put_beta_ok", db->Put(wopts, "beta", "two").ok());

    // --- get alpha ---
    std::string v;
    leveldb::Status gs = db->Get(ropts, "alpha", &v);
    add_b("get_alpha_ok", gs.ok());
    add_s("get_alpha_val", v);

    // --- get missing key ---
    std::string vm;
    leveldb::Status ms = db->Get(ropts, "missing", &vm);
    add_b("get_missing_ok", ms.ok());
    add_b("get_missing_notfound", ms.IsNotFound());
    add_s("get_missing_str", ms.ToString());

    // --- delete alpha, get alpha -> NotFound ---
    add_b("del_alpha_ok", db->Delete(wopts, "alpha").ok());
    std::string vd;
    leveldb::Status ds = db->Get(ropts, "alpha", &vd);
    add_b("get_deleted_notfound", ds.IsNotFound());

    // --- WriteBatch atomicity: batch puts gamma=3, deletes beta ---
    leveldb::WriteBatch batch;
    batch.Put("gamma", "three");
    batch.Delete("beta");
    add_i("batch_approx_size", static_cast<std::int64_t>(batch.ApproximateSize()));
    add_b("write_batch_ok", db->Write(wopts, &batch).ok());

    std::string vg;
    add_b("get_gamma_ok", db->Get(ropts, "gamma", &vg).ok());
    add_s("get_gamma_val", vg);
    std::string vb;
    add_b("get_beta_after_batch_notfound", db->Get(ropts, "beta", &vb).IsNotFound());

    delete db;

    // --- re-open persistence: gamma must survive ---
    leveldb::Options ropts2;
    leveldb::DB* db2 = nullptr;
    leveldb::Status rs = leveldb::DB::Open(ropts2, dbpath, &db2);
    add_b("reopen_ok", rs.ok());
    std::string vp;
    add_b("reopen_get_gamma_ok", db2->Get(leveldb::ReadOptions(), "gamma", &vp).ok());
    add_s("reopen_get_gamma_val", vp);
    delete db2;

    // --- Slice surface ---
    leveldb::Slice sl("hello");
    add_i("slice_size", static_cast<std::int64_t>(sl.size()));
    add_s("slice_str", sl.ToString());
    add_b("slice_starts_with", sl.starts_with(leveldb::Slice("he")));
    add_i("slice_compare", sl.compare(leveldb::Slice("hello")));
    add_b("slice_eq", leveldb::Slice("hello") == leveldb::Slice("hello"));

    // --- enum ground truth ---
    add_i("comp_none", static_cast<std::int64_t>(leveldb::kNoCompression));
    add_i("comp_snappy", static_cast<std::int64_t>(leveldb::kSnappyCompression));

    leveldb::DestroyDB(dbpath, opts);

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
