#ifndef __COMMAND_HPP
#define __COMMAND_HPP

#include <limits.h>
#include <iostream>
#include <string.h>
#include <regex>

class command_t {
public:
    static const int INIT = 0, CHECKPOINT = 1, RESTART = 2, TEST = 3, STATUS = 4;
    static const int ID_EC = -1, ID_AGG = -2;
    static const size_t CKPT_NAME_MAX = 128;

    int unique_id, command, version;
    size_t offset = 0;
    char name[CKPT_NAME_MAX] = {}, original[PATH_MAX] = {};

    static std::regex regex(const std::string &cname);
    static bool match(const std::string &str, const std::regex &ex, int &id, int &version);

    command_t();
    command_t(int r, int c, int v, const std::string &src);
    void assign_path(const std::string &src);
    std::string stem() const;
    std::string filename(const std::string &prefix) const;
    std::string agg_filename(const std::string &prefix) const;
    friend std::ostream &operator<<(std::ostream &output, const command_t &c);
    template<typename A> void save(A& ar);
    template<typename A> void load(A& ar);
};

#endif // __COMMAND_HPP
