#include <ll-config-file.hpp>
#include <common.hpp>

namespace pp { 

std::string
LLConfigFile::parseLine(std::string& line)
{
    int pos = line.find(" ");
    auto key = line;
    if (pos != std::string::npos) {
	key = line.substr(0, pos);
	line = line.substr(pos + 1);
    }
    else {
	line = std::string("");
    }

    return key;
}


LLConfigFile::LLConfigFile(const char* file_name)
    : m_fileName(std::string(file_name))
{
    handlers.clear();
}

void
LLConfigFile::addHandler(const std::string& key, cfg_handler_t handler)
{
    if (handlers.find(key) != handlers.end()) {
	LOG_ERROR("redundant key : %s\n", key.c_str());
    }
    else {
	handlers[key] = handler;
    }
}

void
LLConfigFile::parse(const std::string& head)
{
    std::string realHead = head + " {";
    std::string line;

    m_file = fopen(m_fileName.c_str(), "r");
    if (!m_file) {
	LOG_ERROR("can not open file : %s\n", m_fileName.c_str());
    };
    
    // read file until reach the end or hit the head
    bool notEnd = true;
    while ((notEnd = readLine(line)) && line != realHead);
    if (!notEnd) {
	fclose(m_file);
	return;
    }

    while (readLine(line) && line != "}") {
	// ignore comments
	if (line[0] == '#') continue;

	auto key = LLConfigFile::parseLine(line); // line becomes the rest
	auto it = handlers.find(key);
	if (it != handlers.end()) {
	    //LOG_INFO("parse: %s %s\n", key.c_str(), line.c_str());
	    it->second(line);
	}
    }
    fclose(m_file);
}

bool
LLConfigFile::readLine(std::string& res)
{
    char line[1024];
    int pos;
    res = std::string("");
    if (fgets(line, 1024, m_file)) {
	res = line;
	if ((pos = res.find_first_of("\n\r")) != std::string::npos) {
	    res = res.substr(0, pos);
	}
	return true;
    }

    return false;
}

} // namespace pp
