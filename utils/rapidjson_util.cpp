#include "rapidjson_util.h"

namespace json_util {

const char JSON_CLASS_BRACE_begin   = '{';
const char JSON_CLASS_BRACE_end     = '}';
const char JSON_ARRAY_BRACKET_begin = '[';
const char JSON_ARRAY_BRACKET_end   = ']';
const char JSON_TAG_NAME_QUOTE      = '"';
const char JSON_SEPARATOR_COLON     = ':';
const char JSON_SEPARATOR_COMMA     = ',';

const char JSON_ESCAPE              = '\\';

JsonReader::JsonReader() : docu_(), curr_value_(NULL), prev_value_(NULL)
{}

JsonReader::~JsonReader()
{}

int JsonReader::Parse(const char* text)
{
    docu_.Parse<0>(text);
    if (docu_.HasParseError())
    {
        return -1;
    }

    curr_value_ = &docu_;
    prev_value_ = NULL;

    return 0;
}

int JsonReader::GetValueString(const char* key, std::string& value) const
{
    if (NULL == key)
    {
        return -1;
    }

    if (curr_value_->HasMember(key) && (*curr_value_)[key].IsString())
    {
        value = (*curr_value_)[key].GetString();
        return 0;
    }
    else
    {
        return -1;
    }
}

int JsonReader::GetValueInt(const char* key, int* value) const
{
    if (NULL == key || NULL == value)
    {
        return -1;
    }

    if (curr_value_->HasMember(key) && (*curr_value_)[key].IsInt())
    {
        *value = (*curr_value_)[key].GetInt();
        return 0;
    }
    else
    {
        return -1;
    }
}

int JsonReader::GetValueIntAt(const char* key, int* value, size_t pos) const
{
    if (NULL == key || NULL == value || 0 == pos)
    {
        return -1;
    }

    if (curr_value_->HasMember(key) && (*curr_value_)[key].IsArray())
    {
        *value = (*curr_value_)[key][pos].GetInt();
        return 0;
    }

    return -1;
}

int JsonReader::GetCount(const char* key, int* count)
{
    if (NULL == key || NULL == count)
    {
        return -1;
    }

    if (curr_value_->HasMember(key)) 
    {
        if ((*curr_value_)[key].IsArray())
        {
            *count = (*curr_value_)[key].Size(); 
        }
        else
        {
            *count = 1;
        }
    }

    return 0;
}

int JsonReader::Enter(const char* key)
{
    if (NULL == key)
    {
        return -1;
    }

    if (curr_value_->HasMember(key) && (*curr_value_)[key].IsObject()) 
    {
        prev_value_ = curr_value_;
        curr_value_ = &(*curr_value_)[key]; 
        return 0;
    }

    return -1;
}

int JsonReader::EnterAt(const char* key, size_t pos)
{
    if (NULL == key)
    {
        return -1;
    }

    if (curr_value_->HasMember(key) && (*curr_value_)[key].IsArray() &&
        pos < (*curr_value_)[key].Size() && (*curr_value_)[key][pos].IsObject()) 
    {
        prev_value_ = curr_value_;
        curr_value_ = &(*curr_value_)[key][pos];
        return 0;
    }

    return -1;
}

void JsonReader::EnterRoot()
{
    prev_value_ = curr_value_;
    curr_value_ = &docu_;
}

JsonWriter::JsonWriter(std::string* ptext) : ptext_(ptext), first_element_(true)
{}

JsonWriter::JsonWriter()
{}

JsonWriter::~JsonWriter()
{}

std::string JsonWriter::EscapeString(const char* name)
{
    std::string str;
    if (name != NULL)
    {
        int len = strlen(name);
        for (int i=0; i<len; ++i)
        {
            if (JSON_TAG_NAME_QUOTE == name[i])
            {
                str.push_back(JSON_ESCAPE);
            }

            str.push_back(name[i]);
        }
    }

    return str;
}

int JsonWriter::StructBegin(const char* name)
{
    if (NULL == ptext_)
    {
        return -1;
    }

    if (!first_element_)
    {
        (*ptext_).push_back(JSON_SEPARATOR_COMMA);
    }

    if (name != NULL)
    {
        // tag
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);
        (*ptext_).append(name);
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);

        // ':'
        (*ptext_).push_back(JSON_SEPARATOR_COLON);
    }

    // '{'
    (*ptext_).push_back(JSON_CLASS_BRACE_begin);

    first_element_ = true;
    return 0;
}

int JsonWriter::StructEnd()
{
    if (NULL == ptext_)
    {
        return -1;
    }

    // '}'
    (*ptext_).push_back(JSON_CLASS_BRACE_end);

    return 0;
}

int JsonWriter::ArrayBegin(const char* name)
{
    if (NULL == ptext_)
    {
        return -1;
    }

    if (!first_element_)
    {
        (*ptext_).push_back(JSON_SEPARATOR_COMMA);
    }

    if (name != NULL)
    {
        // tag
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);
        (*ptext_).append(name);
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);

        // ':'
        (*ptext_).push_back(JSON_SEPARATOR_COLON);
    }

    // '['
    (*ptext_).push_back(JSON_ARRAY_BRACKET_begin);

    first_element_ = true;
    return 0;
}

int JsonWriter::ArrayEnd()
{
    if (NULL == ptext_)
    {
        return -1;
    }

    // ']'
    (*ptext_).push_back(JSON_ARRAY_BRACKET_end);

    return 0;
}

int JsonWriter::PushBack(const char* key, const char* value)
{
    if (NULL == ptext_ || NULL == value)
    {
        return -1;
    }

    if (!first_element_)
    {
        (*ptext_).push_back(JSON_SEPARATOR_COMMA);
    }

    if (key != NULL)
    {
        // key
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);
        (*ptext_).append(EscapeString(key));
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);

        // ':'
        (*ptext_).push_back(JSON_SEPARATOR_COLON);
    }

    // value
    (*ptext_).push_back(JSON_TAG_NAME_QUOTE);
    (*ptext_).append(EscapeString(value));
    (*ptext_).push_back(JSON_TAG_NAME_QUOTE);

    first_element_ = false;
    return 0;
}

int JsonWriter::PushBack(const char* key, int value)
{
    if (NULL == ptext_)
    {
        return -1;
    }

    if (!first_element_)
    {
        (*ptext_).push_back(JSON_SEPARATOR_COMMA);
    }

    if (key != NULL)
    {
        // key
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);
        (*ptext_).append(EscapeString(key));
        (*ptext_).push_back(JSON_TAG_NAME_QUOTE);

        // ':'
        (*ptext_).push_back(JSON_SEPARATOR_COLON);
    }

    // value
    char int_str[16];
    snprintf(int_str, sizeof(int_str) - 1, "%d", value);
    // 整型数值不需要转义处理
    (*ptext_).append(int_str);

    first_element_ = false;
    return 0;
}

}
