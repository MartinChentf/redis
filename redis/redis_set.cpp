#include "redis_set.h"
#include "redis_helper.h"

long long redis_set::sadd(const std::string& key,
                          const std::vector<std::string>& member)
{
    std::string member_list = redis_helper::join(member);
    build_command("SADD %s %s", key.c_str(), member_list.c_str());
    hash_slots(key);

    return get_integer64();
}

long long redis_set::scard(const std::string& key)
{
    build_command("SCARD %s", key.c_str());
    hash_slots(key);

    return get_integer64();
}

bool redis_set::sdiff(const std::vector<std::string>& keys,
                      std::vector<std::string>& result)
{
    return set_operation("SDIFF", keys, result);
}

bool redis_set::sinter(const std::vector<std::string>& keys,
                       std::vector<std::string>& result)
{
    return set_operation("SINTER", keys, result);
}

bool redis_set::sunion(const std::vector<std::string>& keys,
                       std::vector<std::string>& result)
{
    return set_operation("SUNION", keys, result);
}

bool redis_set::set_operation(const char* op,
                              const std::vector<std::string>& keys,
                              std::vector<std::string>& result)
{
    std::string key_list = redis_helper::join(keys);
    build_command("%s %s", op, key_list.c_str());

    if (keys.size()) {
        hash_slots(keys[0]);
    }

    return get_array(&result);
}

long long redis_set::sdiffstore(const std::string& dest,
                                const std::vector<std::string>& keys)
{
    return set_operation_with_store("SDIFFSTORE", dest, keys);
}

long long redis_set::sinterstore(const std::string& dest,
                                 const std::vector<std::string>& keys)
{
    return set_operation_with_store("SINTERSTORE", dest, keys);
}

long long redis_set::sunionstore(const std::string& dest,
                                 const std::vector<std::string>& keys)
{
    return set_operation_with_store("SUNIONSTORE", dest, keys);
}

long long 
redis_set::set_operation_with_store(const char* op,
                                    const std::string& dest,
                                    const std::vector<std::string>& keys)
{
    std::string key_list = redis_helper::join(keys);
    build_command("%s %s %s", op, dest.c_str(), key_list.c_str());

    hash_slots(dest);

    return get_integer64();
}
