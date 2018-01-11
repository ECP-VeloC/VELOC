#ifndef __COMMAND_HPP
#define __COMMAND_HPP

#include <iostream>

#include <boost/filesystem.hpp>

typedef std::function<void (int)> completion_t;

class command_t {
public:
    static const size_t MAX_SIZE = 128;
    static const int INIT = 0, CHECKPOINT = 1, RESTART = 2, TEST = 3;
    
    int unique_id, command, version;
    char ckpt_name[MAX_SIZE];

    command_t() { }
    command_t(int r, int c, int v, const std::string &s) : unique_id(r), command(c), version(v) {
	if (s.length() > MAX_SIZE)
	    throw std::runtime_error("checkpoint name '" + s + "' is longer than admissible size " + std::to_string(MAX_SIZE));
	std::strcpy(ckpt_name, s.c_str());
    }
    friend std::ostream &operator<<(std::ostream &output, const command_t &c) {
	output << "(Rank = '" << c.unique_id << "', Command = '" << c.command
	       << "', Version = '" << c.version << "', File = '" << c.ckpt_name << "')";
	return output;
    }
};

#endif // __COMMAND_HPP
