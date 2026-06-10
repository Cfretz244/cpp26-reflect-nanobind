// Native ground-truth oracle for the SQLiteCpp run. Drives the SAME scenario the Python
// differential drives -- against an in-memory (":memory:") database for determinism -- and
// prints ONE JSON object on stdout. test_bindings.py loads it as expected.json and asserts
// equality on the bound module's behavior.
//
// Scenario:
//   1. open :memory: with OPEN_READWRITE|OPEN_CREATE
//   2. CREATE a table; tableExists before/after
//   3. INSERT three rows via a bound prepared statement (int / int64 / double / text)
//   4. SELECT them back: typed column values, names, declared types, count, last-rowid, changes
//   5. a transaction that is rolled back leaves no trace; a committed one persists
//   6. a UNIQUE-constraint INSERT throws SQLite::Exception
#include "SQLiteCpp/SQLiteCpp.h"

#include <cstdio>
#include <string>
#include <sstream>

static std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else o += c;
    }
    return o;
}

int main() {
    std::ostringstream js;
    js << "{";

    // Open with the SAME explicit 4-arg ctor the Python side must use (default-argument
    // VALUES are a C++26 reflection gap, so every arg is passed explicitly). The bound
    // const char* ctor overload does not do the std::string overload's empty->nullptr vfs
    // conversion, so the vfs is named explicitly ("unix", the platform default) on both sides.
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE, 0, "unix");

    js << "\"filename\":\"" << esc(db.getFilename()) << "\",";
    js << "\"exists_before\":" << (db.tableExists("widgets") ? "true" : "false") << ",";

    db.exec("CREATE TABLE widgets (id INTEGER PRIMARY KEY, name TEXT UNIQUE, qty INTEGER, weight REAL)");
    js << "\"exists_after\":" << (db.tableExists("widgets") ? "true" : "false") << ",";

    // Insert three rows via a bound prepared statement.
    {
        SQLite::Statement ins(db, "INSERT INTO widgets (id, name, qty, weight) VALUES (?, ?, ?, ?)");
        struct Row { int id; const char* name; int64_t qty; double weight; };
        Row rows[] = {
            {1, "alpha", 10, 1.5},
            {2, "beta",  20, 2.25},
            {3, "gamma", 30, 3.125},
        };
        int total_changes = 0;
        for (const auto& r : rows) {
            ins.reset();
            ins.bind(1, r.id);
            ins.bind(2, std::string(r.name));
            ins.bind(3, r.qty);
            ins.bind(4, r.weight);
            total_changes += ins.exec();
        }
        js << "\"insert_total_changes\":" << total_changes << ",";
    }
    js << "\"last_rowid\":" << db.getLastInsertRowid() << ",";
    js << "\"total_changes\":" << db.getTotalChanges() << ",";

    // Select them back, comparing typed column values + names + declared types.
    {
        SQLite::Statement q(db, "SELECT id, name, qty, weight FROM widgets ORDER BY id");
        js << "\"col_count\":" << q.getColumnCount() << ",";
        js << "\"col_name_0\":\"" << esc(q.getColumnName(0)) << "\",";
        js << "\"col_name_1\":\"" << esc(q.getColumnName(1)) << "\",";
        js << "\"col_idx_weight\":" << q.getColumnIndex("weight") << ",";

        js << "\"rows\":[";
        bool first = true;
        std::string decl_type_id, decl_type_name;
        int type_code_name = -1, type_code_qty = -1;
        while (q.executeStep()) {
            if (!first) js << ",";
            first = false;
            SQLite::Column cId = q.getColumn(0);
            SQLite::Column cName = q.getColumn(1);
            SQLite::Column cQty = q.getColumn(2);
            SQLite::Column cWeight = q.getColumn(3);
            decl_type_id = q.getColumnDeclaredType(0);
            decl_type_name = q.getColumnDeclaredType(1);
            type_code_name = cName.getType();
            type_code_qty = cQty.getType();
            js << "{\"id\":" << cId.getInt()
               << ",\"name\":\"" << esc(cName.getString()) << "\""
               << ",\"name_getText\":\"" << esc(cName.getText()) << "\""
               << ",\"colname\":\"" << esc(cName.getName()) << "\""
               << ",\"qty\":" << cQty.getInt64()
               << ",\"weight\":" << cWeight.getDouble()
               << ",\"id_isInteger\":" << (cId.isInteger() ? "true" : "false")
               << ",\"name_isText\":" << (cName.isText() ? "true" : "false")
               << ",\"weight_isFloat\":" << (cWeight.isFloat() ? "true" : "false")
               << "}";
        }
        js << "],";
        js << "\"decl_type_id\":\"" << esc(decl_type_id) << "\",";
        js << "\"decl_type_name\":\"" << esc(decl_type_name) << "\",";
        js << "\"type_code_name\":" << type_code_name << ",";
        js << "\"type_code_qty\":" << type_code_qty << ",";
    }

    // Transaction: rolled back -> not visible; committed -> visible.
    {
        SQLite::Transaction tx(db);
        db.exec("INSERT INTO widgets (id, name, qty, weight) VALUES (99, 'ghost', 0, 0.0)");
        tx.rollback();
    }
    {
        SQLite::Statement c(db, "SELECT COUNT(*) FROM widgets WHERE id = 99");
        c.executeStep();
        js << "\"ghost_after_rollback\":" << c.getColumn(0).getInt() << ",";
    }
    {
        SQLite::Transaction tx(db);
        db.exec("INSERT INTO widgets (id, name, qty, weight) VALUES (100, 'committed', 0, 0.0)");
        tx.commit();
    }
    {
        SQLite::Statement c(db, "SELECT COUNT(*) FROM widgets WHERE id = 100");
        c.executeStep();
        js << "\"committed_after_commit\":" << c.getColumn(0).getInt() << ",";
    }

    // Constraint-violation error path: duplicate UNIQUE name throws SQLite::Exception.
    int constraint_errcode = 0;
    bool threw = false;
    std::string err_msg;
    try {
        db.exec("INSERT INTO widgets (id, name, qty, weight) VALUES (4, 'alpha', 0, 0.0)");
    } catch (const SQLite::Exception& e) {
        threw = true;
        constraint_errcode = e.getErrorCode();
        err_msg = e.what();
    }
    js << "\"constraint_threw\":" << (threw ? "true" : "false") << ",";
    js << "\"constraint_errcode\":" << constraint_errcode << ",";
    js << "\"constraint_msg_has_unique\":"
       << (err_msg.find("UNIQUE") != std::string::npos ? "true" : "false") << ",";

    // Final row count (committed=4 base rows + 1 committed = 4 originals... recompute live).
    {
        SQLite::Statement c(db, "SELECT COUNT(*) FROM widgets");
        c.executeStep();
        js << "\"final_count\":" << c.getColumn(0).getInt();
    }

    js << "}";
    std::string out = js.str();
    std::printf("%s\n", out.c_str());
    return 0;
}
