#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "redis_command.h"
#include "redis_client.h"
#include "redis_helper.h"
#include "redis_log.h"
#include "redis_reply.h"

redis_command::redis_command(redis_client * client)
: m_client(client)
, m_rcon(NULL)
, m_command("")
, m_slot(0)
, m_request_buf("")
{}

void redis_command::set_client(redis_client * client)
{
    if (client != NULL) {
        m_client = client;
    }
}

void redis_command::hash_slots(std::string key)
{
    if (m_client != NULL) {
        m_slot = m_client->get_key_slot(key);   // ɾ
        m_rcon = m_client->get_redis_context_by_slot(m_slot); // ɾ

        m_client->set_hash_slot(key);
    }
}

std::string redis_command::build_command(const char * format,...)
{
    char cmd[10240] = {0};

    va_list ap;
    va_start(ap,format);
    vsnprintf(cmd, sizeof(cmd), format, ap);
    va_end(ap);

    m_command = cmd;

    return m_command;
}

bool redis_command::is_normal_context()
{
    if (m_rcon == NULL || m_rcon->err) {
        return false;
    }
    return true;
}

void redis_command::confirm_redis_context()
{
    if (m_client != NULL) {
        if (!is_normal_context()) {
            m_rcon = m_client->get_redis_context();
        }
    }
    else {
        m_rcon = NULL;
    }
}

redisReply* redis_command::run_command()
{
    confirm_redis_context();
    if (!is_normal_context()) {
        ERROR("redisContext is abnormal, Error: %s",
            m_rcon ? m_rcon->errstr : "redisContext is NULL");
        return NULL;
    }

    redisReply* reply = (redisReply*)redisCommand(m_rcon, m_command.c_str());

    return reply;
}

std::string redis_command::parse_reply(redisReply * reply)
{
    if (reply == NULL) {
        return "Reply is NULL";
    }

    std::string result("Reply Type: ");
    result += REPLY_TYPE[reply->type];
    switch(reply->type) {
        case REDIS_REPLY_STATUS:
            return result + ", Status: " + reply->str;
        case REDIS_REPLY_NIL:
            return result;
        case REDIS_REPLY_STRING:
            return result + ", String: " + reply->str;
        case REDIS_REPLY_ERROR:
            return result + ", Errstr: " + reply->str;
        case REDIS_REPLY_INTEGER:
            return result + ", Integer: "
                    + TO_STRING(reply->integer);
        case REDIS_REPLY_ARRAY:
            return result + ", Array Elements:"
                    + TO_STRING(reply->elements);
        default:
            return "Unkonw Type";
    }
}

bool redis_command::get_string(std::string & result)
{
    redis_reply* reply = run();
    if (reply == NULL || reply->get_type() != T_REDIS_REPLY_STRING) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return false;
    }
    DEBUG("Execute command success! [%s]", m_command.c_str());

    result = reply->get_string();
    SAFE_DELETE(reply);
    return true;
}

bool redis_command::get_string(std::string * result)
{
    redis_reply* reply = run();
    if (reply == NULL || reply->get_type() != T_REDIS_REPLY_STRING) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return false;
    }
    DEBUG("Execute command success! [%s]", m_command.c_str());

    if (result != NULL)
        *result = reply->get_string();
    SAFE_DELETE(reply);
    return true;
}

int redis_command::get_string_or_nil(std::string& result)
{
    redis_reply* reply = run();
    if (reply == NULL
        || (reply->get_type() != T_REDIS_REPLY_STRING
            && reply->get_type() != T_REDIS_REPLY_NIL)) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return -1;
    }

    if (reply->get_type() == T_REDIS_REPLY_NIL) {
        DEBUG("Execute command success! [%s], Reply is nil!",
            m_command.c_str());
        SAFE_DELETE(reply);
        return 0;
    }

    result = reply->get_string();
    SAFE_DELETE(reply);
    return 1;
}

long long redis_command::get_integer64(bool * success /*= NULL*/)
{
    long long llret = -1;

    redisReply* reply = run_command();

    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_ASSIGN(success, false);
    }
    else if (reply->type == REDIS_REPLY_INTEGER) {
        NORMAL("Execute command success! [%s]", m_command.c_str());
        llret = reply->integer;
        SAFE_ASSIGN(success, true);
    }
    else {
        WARN("Unexpected reply: %s", parse_reply(reply).c_str());
        SAFE_ASSIGN(success, false);
    }

    freeReplyObject(reply);
    return llret;
}

bool redis_command::get_integer64(long long & result)
{
    bool success;
    result = get_integer64(&success);
    return success;
}

bool redis_command::get_integer64(long long * result)
{
    bool success;
    SAFE_ASSIGN_FUNC(result, get_integer64(&success));
    return success;
}

int redis_command::get_integer32(bool * success /*= NULL*/)
{
    return get_integer64(success);
}

long long redis_command::get_integer64_or_nil(bool * success /*= NULL*/)
{
    long long llret = -1;

    redisReply* reply = run_command();

    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_ASSIGN(success, false);
    }
    else if (reply->type == REDIS_REPLY_INTEGER) {
        NORMAL("Execute command success! [%s]", m_command.c_str());
        llret = reply->integer;
        SAFE_ASSIGN(success, true);
    }
    else if (reply->type == REDIS_REPLY_NIL) {
        NORMAL("Execute command success! [%s], Reply is nil!",
            m_command.c_str());
        llret = 0;
        SAFE_ASSIGN(success, true);
    }
    else {
        WARN("Unexpected reply: %s", parse_reply(reply).c_str());
        SAFE_ASSIGN(success, false);
    }

    freeReplyObject(reply);
    return llret;
}

int redis_command::get_integer32_or_nil(bool * success /*= NULL*/)
{
    return get_integer64_or_nil(success);
}

bool redis_command::get_array(std::vector<std::string*>& result)
{
    redis_reply* reply = run();
    if (reply == NULL || reply->get_type() != T_REDIS_REPLY_ARRAY) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return false;
    }
    DEBUG("Execute command success! [%s]", m_command.c_str());

    for (size_t i = 0; i < reply->get_size(); i ++) {
        const redis_reply* elem = reply->get_element(i);
        if (elem->get_type() == T_REDIS_REPLY_STRING) {
            result.push_back(new std::string(elem->get_string()));
        }
        else {
            result.push_back(NULL);
        }
    }
    SAFE_DELETE(reply);
    return true;
}

bool redis_command::get_array(std::vector<std::string>& result)
{
    redis_reply* reply = run();
    if (reply == NULL || reply->get_type() != T_REDIS_REPLY_ARRAY) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return false;
    }
    DEBUG("Execute command success! [%s]", m_command.c_str());

    for (size_t i = 0; i < reply->get_size(); i ++) {
        const redis_reply* elem = reply->get_element(i);
        if (elem->get_type() == T_REDIS_REPLY_STRING) {
            result.push_back(elem->get_string());
        }
        else {
            result.push_back("");
        }
    }
    SAFE_DELETE(reply);
    return true;
}

int redis_command::get_array_or_nil(std::vector<std::string>& result)
{
    redis_reply* reply = run();
    if (reply == NULL
        || (reply->get_type() != T_REDIS_REPLY_ARRAY
            && reply->get_type() != T_REDIS_REPLY_NIL)) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return -1;
    }
    
    if (reply->get_type() == REDIS_REPLY_NIL) {
        DEBUG("Execute command success! [%s], Reply is nil!",
            m_command.c_str());
        SAFE_DELETE(reply);
        return 0;
    }
    DEBUG("Execute command success! [%s]", m_command.c_str());

    for (size_t i = 0; i < reply->get_size(); i ++) {
        const redis_reply* elem = reply->get_element(i);
        if (elem->get_type() == REDIS_REPLY_STRING) {
            result.push_back(elem->get_string());
        }
        else {
            result.push_back("");
        }
    }
    SAFE_DELETE(reply);
    return 1;
}

int redis_command::get_cursor_array(std::vector<std::string>* result)
{
    int cursor = -1;
    redisReply* reply = run_command();

    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
    }
    else if (reply->type == REDIS_REPLY_ARRAY) {
        if (reply->elements != 2) {
            ERROR("Execute command fail! [%s], array elements != 2",
                m_command.c_str());
        }
        NORMAL("Execute command success! [%s]", m_command.c_str());

        if (reply->element[0]->type == REDIS_REPLY_STRING) {
            cursor = atoi(reply->element[0]->str);
        }
        if (result) {
            redisReply* array = reply->element[1];
            for (size_t i = 0; i < array->elements; i ++) {
                redisReply* elem = array->element[i];
                if (elem->type == REDIS_REPLY_STRING) {
                    result->push_back(elem->str);
                }
                else {
                    result->push_back("");
                }
            }
        }
    }
    else {
        WARN("Unexpected reply: %s", parse_reply(reply).c_str());
    }

    return cursor;
}

////////////////////////////////////////////////////////////////////////////////
redis_reply* redis_command::run()
{
    return m_client->run(m_request_buf);
}

std::string redis_command::parse_reply(const redis_reply* reply)
{
    if (reply == NULL) {
        return "Reply is NULL";
    }

    std::string result("Reply Type: ");
    result += REPLY_TYPE[reply->get_type()];
    switch(reply->get_type()) {
        case REDIS_REPLY_STATUS:
            return result + ", Status: " + reply->get_status();
        case REDIS_REPLY_NIL:
            return result;
        case REDIS_REPLY_STRING:
            return result + ", String: " + reply->get_string();
        case REDIS_REPLY_ERROR:
            return result + ", Errstr: " + reply->get_error();
        case REDIS_REPLY_INTEGER:
            return result + ", Integer: "
                    + TO_STRING(reply->get_integer());
        case REDIS_REPLY_ARRAY:
            return result + ", Array Elements:"
                    + TO_STRING(reply->get_size());
        default:
            return "Unkonw Type";
    }
}

void redis_command::build_request(const std::vector<std::string>& argv)
{
    m_request_buf.clear();

    m_request_buf += "*";
    m_request_buf += TO_STRING(argv.size());
    m_request_buf += "\r\n";

    for (size_t i = 0; i < argv.size(); i++) {
        m_request_buf += "$";
        m_request_buf += TO_STRING(argv[i].size());
        m_request_buf += "\r\n";
        m_request_buf += argv[i];
        m_request_buf += "\r\n";
    }

    generate_cmdstr(argv);
}

void redis_command::generate_cmdstr(const std::vector<std::string>& argv)
{
    m_command.clear();

    for (size_t i = 0; i < argv.size(); i++) {
        m_command.push_back('"');
        m_command += argv[i];
        m_command.push_back('"');
        m_command.push_back(' ');
    }
}

bool redis_command::check_status(const char * expection /*= "OK"*/)
{
    const redis_reply* reply = run();
    if (reply == NULL || reply->get_type() != T_REDIS_REPLY_STATUS) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return false;
    }

    const std::string status = reply->get_status();
    if (status.empty()) {
        WARN("Execute command fail! , status is empty");
        SAFE_DELETE(reply);
        return false;
    }
    else if (expection == NULL || strcasecmp(status.c_str(), expection) == 0) {
        NORMAL("Execute command success! [%s]", m_command.c_str());
        SAFE_DELETE(reply);
        return true;
    }
    else {
        WARN("Execute command fail! , status:[%s], expection:[%s]",
             status.c_str(), expection);
        SAFE_DELETE(reply);
        return false;
    }
}

int redis_command::check_status_or_nil(const char * expection /*= "OK"*/)
{
    redis_reply* reply = run();
    if (reply == NULL
        || (reply->get_type() != T_REDIS_REPLY_STATUS
            && reply->get_type() != T_REDIS_REPLY_NIL)) {
        ERROR("Execute command fail! [%s], %s",
            m_command.c_str(), parse_reply(reply).c_str());
        SAFE_DELETE(reply);
        return -1;
    }

    if (reply->get_type() == T_REDIS_REPLY_NIL) {
        NORMAL("Execute command success! [%s], Reply is nil!",
            m_command.c_str());
        SAFE_DELETE(reply);
        return 0;
    }

    const std::string status = reply->get_status();
    if (status.empty()) {
        WARN("Execute command fail! , status is empty");
        SAFE_DELETE(reply);
        return false;
    }
    else if (expection == NULL || strcasecmp(status.c_str(), expection) == 0) {
        NORMAL("Execute command success! [%s]", m_command.c_str());
        SAFE_DELETE(reply);
        return 1;
    }
    else {
        WARN("Execute command fail! , status:[%s], expection:[%s]",
             status.c_str(), expection);
        SAFE_DELETE(reply);
        return -1;
    }
}

