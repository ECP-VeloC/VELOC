#ifndef __COMMAND_HPP
#define __COMMAND_HPP

#include <iostream>
#include <cstring>
#include <stdexcept>

class command_t {
public:
    static const size_t MAX_SIZE = 128;
    enum command_code { INIT, CHECKPOINT_BEGIN, CHECKPOINT_CHUNK, CHECKPOINT_END, CHECKPOINT_RESTART, CHECKPOINT_TEST };
    
    int unique_id, command, version;
    char name[MAX_SIZE] = "";

    command_t() { }
    command_t(int r, int c, int v, const std::string &s) : unique_id(r), command(c), version(v) {
	if (s.length() > MAX_SIZE)
	    throw std::runtime_error("checkpoint name '" + s + "' is longer than admissible size " + std::to_string(MAX_SIZE));
	std::strcpy(name, s.c_str());
    }
    std::string stem() const {
	return std::string(name) + "-" + std::to_string(unique_id) +
	    "-" + std::to_string(version) + ".dat";
    }
    std::string filename(const std::string &prefix) const {
	return prefix + "/" + stem();
    }
    std::string filename(const std::string &prefix, int new_version) const {
	return prefix + "/" + name +
	    "-" + std::to_string(unique_id) + "-" +
	    std::to_string(new_version) + ".dat";
    }
    std::string metaname(const std::string &prefix) const {
	return prefix + ".meta";
    }
    friend std::ostream &operator<<(std::ostream &output, const command_t &c) {
	output << "(Rank = '" << c.unique_id << "', Command = '" << c.command
	       << "', Version = '" << c.version << "', File = '" << c.stem() << "')";
	return output;
    }
};

#endif // __COMMAND_HPP
