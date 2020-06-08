#ifndef __COMMAND_HPP
#define __COMMAND_HPP

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <limits.h>

class command_t {
public:
    static const int INIT = 0, CHECKPOINT = 1, RESTART = 2, TEST = 3, STATUS = 4;

    int unique_id, command, version;
    char name[PATH_MAX] = {}, original[PATH_MAX] = {};

    command_t() { }
    command_t(int r, int c, int v, const std::string &s) : unique_id(r), command(c), version(v) {
	assign_path(name, s.c_str());
    }
    void assign_path(char *dest, const std::string &src) {
	if (src.length() > PATH_MAX)
	    throw std::runtime_error("checkpoint name '" + src + "' is longer than admissible size " + std::to_string(PATH_MAX));
	std::strcpy(dest, src.c_str());
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
    friend std::ostream &operator<<(std::ostream &output, const command_t &c) {
	output << "(Rank = '" << c.unique_id << "', Command = '" << c.command
	       << "', Version = '" << c.version << "', File = '" << c.stem() << "')";
	return output;
    }
    template<typename A> void save(A& ar) {
        ar.write(this);
    }
    template<typename A> void load(A& ar) {
        ar.read(this);
    }
};

#endif // __COMMAND_HPP
