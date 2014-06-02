#ifndef RAPIDAPP_CTRL_CMD_KEYWORD_H_
#define RAPIDAPP_CTRL_CMD_KEYWORD_H_

#include <tr1/unordered_map>
#include <functional>
#include <string>

namespace rapidapp {

const char CTRL_CMD_DELIMETER = ' ';

class ICommandEventListener {
    public:
        ICommandEventListener(){};
        virtual ~ICommandEventListener(){};

    public:
        virtual void OnCommand(const char* cmd) = 0;
};

inline int BKDRHash(const char* str)
{
    if (NULL == str)
    {
        return -1;
    }

    unsigned int seed = 13131313;                /* 31 131 1313 13131 131313 etc.. */
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

struct Equal :
public std::unary_function<std::string, std::size_t> {
    std::size_t operator()(const std::string& s) const {
        return BKDRHash(s.c_str());
    }
};

struct Compare :
public std::binary_function<std::string, std::string, bool> {
    bool operator()(const std::string& s, const std::string& d) const {
        return (s == d);
    }
};

// TODO 命令使用描述
typedef std::tr1::unordered_map<std::string, ICommandEventListener*, Equal, Compare> CmdDictionary;

class AppControlDispatcher {
    public:
        AppControlDispatcher(){};
        ~AppControlDispatcher(){};

    public:
        inline void AddSupportedCommand(std::string cmd, ICommandEventListener* listener) {
            commad_dictionary_[cmd] = listener;
        }

        inline int Dispatch(const char* command) {
            std::string cmd(command);
            size_t pos = cmd.find(CTRL_CMD_DELIMETER);
            if (pos > 0)
            {
                cmd = std::string(command, pos);
            }

            ICommandEventListener* listener = GetEventListener(cmd);
            if (listener != NULL) {
                listener->OnCommand(command);
                return 0;
            } else {
                return -1;
            }
        }

    private:
        inline ICommandEventListener* GetEventListener(std::string cmd) {
            const CmdDictionary::iterator it = commad_dictionary_.find(cmd);
            if (it != commad_dictionary_.end()) {
                return it->second;
            } else {
                return NULL;
            }
        }

    private:
        CmdDictionary commad_dictionary_;
};

}

#endif
