#ifndef RAPIDJSON_UTIL_H_
#define RAPIDJSON_UTIL_H_

#include "rapidjson/document.h"
#include <string>

namespace json_util {

class JsonReader;
class JsonWriter;
// 单进程单线程模式，使用单件实例
class JsonWriterSingleton {
    public:
        static inline JsonWriter& GetInstance();
};

class JsonReaderSingleton {
    public:
        static inline JsonReader& GetInstance();
};


class JsonReader {
    public:
        JsonReader();
        ~JsonReader();

    public:
        int Parse(const char* text);

        int GetValueString(const char* key, std::string& value) const;
        int GetValueInt(const char* key, int* value) const;
        int GetValueIntAt(const char* key, int* value, size_t pos) const;

        int GetCount(const char* key, int* count);

        int Enter(const char* key);
        int EnterAt(const char* key, size_t pos);
        void EnterRoot();

    public:
        inline const char* GetLastError();

    private:
        rapidjson::Document docu_;
        rapidjson::Value* curr_value_;
        rapidjson::Value* prev_value_;
};

class JsonWriter {
    public:
        JsonWriter(std::string* text);
        JsonWriter();
        ~JsonWriter();

    public:
        int StructBegin(const char* name);
        int StructEnd();

        int ArrayBegin(const char* name);
        int ArrayEnd();

    public:
        int PushBack(const char* key, const char* value);
        int PushBack(const char* key, int value);

    private:
        std::string EscapeString(const char* name);

    private:
        std::string* ptext_;
        bool first_element_;
};

// inline
/*inline const char* JsonReader::GetLastError()*/
/*{*/
/*if (docu_.HasParseError())*/
/*{*/
/*return docu_.GetParseError();*/
/*}*/
/*else*/
/*{*/
/*return NULL;*/
/*}*/
/*}*/

JsonReader& JsonReaderSingleton::GetInstance()
{
    static JsonReader gs_json_reader;
    return gs_json_reader;
}

JsonWriter& JsonWriterSingleton::GetInstance()
{
    static JsonWriter gs_json_writer;
    return gs_json_writer;
}


}

#endif

