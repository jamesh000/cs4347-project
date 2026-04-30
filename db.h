#pragma once
/**
 * db.h — Thin RAII wrapper around the MySQL C API.
 *
 * Requires the MySQL C connector:
 *   Ubuntu/Debian : sudo apt install libmysqlclient-dev
 *   Fedora/RHEL   : sudo dnf install mysql-devel
 *   macOS Homebrew: brew install mysql-client
 */

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>

// A single result row: column-name → value (all strings; NULL → "")
using Row = std::map<std::string, std::string>;

class DB {
public:
    DB()  { mysql_init(&conn_); }
    ~DB() { mysql_close(&conn_); }

    // Non-copyable
    DB(const DB&)            = delete;
    DB& operator=(const DB&) = delete;

    // Connect; returns false on failure (error printed to stderr)
    bool connect(const std::string& host,
                 const std::string& user,
                 const std::string& password,
                 const std::string& database,
                 int port = 3306)
    {
        if (!mysql_real_connect(&conn_,
                                host.c_str(), user.c_str(),
                                password.c_str(), database.c_str(),
                                port, nullptr, 0)) {
            std::cerr << "[mysql] " << mysql_error(&conn_) << "\n";
            return false;
        }
        // Reconnect on dropped connection
        bool reconnect = true;
        mysql_options(&conn_, MYSQL_OPT_RECONNECT, &reconnect);
        return true;
    }

    /**
     * Execute a query and return all rows.
     * Throws std::runtime_error on SQL error.
     */
    std::vector<Row> query(const std::string& sql) {
        if (mysql_query(&conn_, sql.c_str())) {
            throw std::runtime_error(
                std::string("[mysql query error] ") + mysql_error(&conn_)
                + "\n  SQL: " + sql);
        }

        MYSQL_RES* res = mysql_store_result(&conn_);
        std::vector<Row> rows;

        if (!res) {
            // INSERT / UPDATE / DELETE — no result set
            return rows;
        }

        unsigned int numFields = mysql_num_fields(res);
        MYSQL_FIELD* fields    = mysql_fetch_fields(res);

        MYSQL_ROW mysqlRow;
        while ((mysqlRow = mysql_fetch_row(res))) {
            Row row;
            unsigned long* lengths = mysql_fetch_lengths(res);
            for (unsigned int i = 0; i < numFields; ++i) {
                std::string colName = fields[i].name;
                std::string value   = mysqlRow[i]
                    ? std::string(mysqlRow[i], lengths[i]) : "";
                row[colName] = value;
            }
            rows.push_back(row);
        }

        mysql_free_result(res);
        return rows;
    }

    /**
     * Escape a string value for safe embedding in SQL.
     * Always use this before inserting user input into a query string.
     */
    std::string escape(const std::string& s) {
        std::string buf(s.size() * 2 + 1, '\0');
        unsigned long len = mysql_real_escape_string(
            &conn_, &buf[0], s.c_str(), s.size());
        buf.resize(len);
        return buf;
    }

private:
    MYSQL conn_;
};
