#pragma once

#include <sqlite3.h>

#include <mutex>
#include <string>

namespace weiqi {

class Database final {
public:
    explicit Database(std::string file_path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void migrate();
    [[nodiscard]] sqlite3* handle() const noexcept { return db_; }
    [[nodiscard]] std::mutex& mutex() noexcept { return mutex_; }

private:
    void execute(const char* sql);
    void add_column_if_missing(const char* table, const char* column, const char* definition);
    sqlite3* db_{};
    std::mutex mutex_;
};

} // namespace weiqi
