#ifndef LL_CONFIG_FILE_H
#define LL_CONFIG_FILE_H

#include <functional>
#include <map>
#include <string>

namespace pp { 

class LLConfigFile
{
public:
    LLConfigFile(const char* file_name);
    static std::string parseLine(std::string& line);

public:
    typedef std::function<void(std::string)> cfg_handler_t;
    void addHandler(const std::string& key, cfg_handler_t handler);
    void parse(const std::string& head);

private:
    bool readLine(std::string& line);
    std::map<std::string, cfg_handler_t> handlers;
    std::string m_fileName;
    FILE* m_file;
};
    
} // namespace pp

#endif
