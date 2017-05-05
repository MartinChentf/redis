#include "redis_key.h"
#include "redis_helper.h"

int redis_key::del(const std::vector<std::string>& keys)
{
    std::vector<std::string> argv;
    argv.push_back("DEL");

    for (size_t i = 0; i < keys.size(); i++) {
        argv.push_back(keys[i]);
    }

    build_request(argv);

    if (!keys.empty()) {
        hash_slots(keys[0]);
    }

    return get_integer32();
}

int redis_key::del(const std::string& key)
{
    std::vector<std::string> str;
    str.push_back(key);

    return del(str);
}

int redis_key::dump(const std::string& key, std::string& result)
{
    std::vector<std::string> argv;
    argv.push_back("DUMP");
    argv.push_back(key);

    build_request(argv);
    hash_slots(key);

    return get_string_or_nil(result);
}

int redis_key::exists(const std::vector<std::string>& keys)
{
    std::vector<std::string> argv;
    argv.push_back("EXISTS");

    for (size_t i = 0; i < keys.size(); i++) {
        argv.push_back(keys[i]);
    }

    build_request(argv);
    if (!keys.empty()) {
        hash_slots(keys[0]);
    }

    return get_integer32();
}

int redis_key::exists(const std::string& key)
{
    std::vector<std::string> argv;
    argv.push_back("EXISTS");
    argv.push_back(key);

    build_request(argv);
    hash_slots(key);

    return get_integer32();
}

int redis_key::expire(const std::string& key, int second)
{
    std::vector<std::string> argv;
    argv.push_back("EXPIRE");
    argv.push_back(key);
    argv.push_back(TO_STRING(second));

    build_request(argv);
    hash_slots(key);

    return get_integer32();
}

int redis_key::expireat(const std::string& key, time_t timestamp)
{
    std::vector<std::string> argv;
    argv.push_back("EXPIREAT");
    argv.push_back(key);
    argv.push_back(TO_STRING(timestamp));

    build_request(argv);
    hash_slots(key);

    return get_integer32();
}

int redis_key::keys(const std::string& pattern,std::vector<std::string>& result)
{
    std::vector<std::string> argv;
    argv.push_back("KEYS");
    argv.push_back(pattern);

    build_request(argv);

    return get_array_or_nil(result);
}

bool redis_key::migrate(const std::string& host, uint16_t port,
    const std::string& key, int dest_db, long long timeout,
    bool is_copy /*= false*/, bool is_replace /*= false*/,
    const std::vector<std::string>* keys /*= NULL*/)
{
    std::vector<std::string> argv;
    argv.push_back("MIGRATE");
    argv.push_back(host);
    argv.push_back(TO_STRING(port));    
    argv.push_back(key);    
    argv.push_back(TO_STRING(dest_db));
    argv.push_back(TO_STRING(timeout));

    if (is_copy) {
        argv.push_back("COPY");
    }
    if (is_replace) {
        argv.push_back("REPLACE");
    }
    if (key == "" && keys != NULL) {
        argv.push_back("KEYS");
        for (size_t i = 0; i < keys->size(); i++) {
            argv.push_back((*keys)[i]);
        }
    }

    build_request(argv);

    return check_status();
}

int redis_key::move(const std::string& key, int db)
{
    std::vector<std::string> argv;
    argv.push_back("MOVE");
    argv.push_back(key);    
    argv.push_back(TO_STRING(db));

    build_request(argv);

    return get_integer32();
}

int redis_key::object_refcount(const std::string& key)
{
    std::vector<std::string> argv;
    argv.push_back("OBJECT");
    argv.push_back("REFCOUNT");
    argv.push_back(key);    

    build_request(argv);

    return get_integer32_or_nil();
}

bool redis_key::object_encoding(const std::string& key, std::string& encode)
{
    std::vector<std::string> argv;
    argv.push_back("OBJECT");
    argv.push_back("ENCODING");
    argv.push_back(key);    

    build_request(argv);

    return get_string_or_nil(encode) >= 0;
}

long long redis_key::object_idletime(const std::string& key)
{
    std::vector<std::string> argv;
    argv.push_back("OBJECT");
    argv.push_back("IDLETIME");
    argv.push_back(key);    

    build_request(argv);

    return get_integer64_or_nil();
}

int redis_key::scan(int cursor, std::vector<std::string>& result,
                    const char * pattern /*= NULL*/, int count /*= 10*/)
{
    scan_keys("SCAN", NULL, cursor, pattern, count);
    return get_cursor_array(&result);
}

