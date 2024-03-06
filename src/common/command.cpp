#include "command.hpp"

//#define __DEBUG
#include "debug.hpp"

command_t::command_t() {
}

command_t::command_t(int r, int c, int v, const std::string &src) : unique_id(r), command(c), version(v) {
    if (src.length() + 1> CKPT_NAME_MAX)
        FATAL("checkpoint name '" + src + "' is longer than admissible size " + std::to_string(CKPT_NAME_MAX));
    std::strcpy(name, src.c_str());
}

void command_t::assign_path(const std::string &src) {
    if (src.length() + 1 > PATH_MAX)
        FATAL("routed path '" + src + "' is longer than admissible size " + std::to_string(PATH_MAX));
    std::strcpy(original, src.c_str());
}

std::string command_t::stem() const {
    return std::string(name) + "-" + std::to_string(unique_id) +
        "-" + std::to_string(version) + ".dat";
}

std::regex command_t::regex(const std::string &cname) {
    return std::regex(cname + "-([0-9]+|ec|agg)-([0-9]+).*");
}

bool command_t::match(const std::string &str, const std::regex &ex, int &id, int &version) {
    std::smatch sm;
    std::regex_match(str, sm, ex);
    if (sm.size() != 3)
        return false;
    if (sm[1] == "ec")
        id = ID_EC;
    else if (sm[1] == "agg")
        id = ID_AGG;
    else
        id = std::stoi(sm[1]);
    version = std::stoi(sm[2]);
    return true;
}

std::string command_t::filename(const std::string &prefix) const {
    return prefix + "/" + stem();
}

std::string command_t::meta_filename(const std::string &prefix) const {
    return filename(prefix) + ".chksum";
}

std::string command_t::agg_filename(const std::string &prefix) const {
    return prefix + "/" + std::string(name) + "-agg-" + std::to_string(version) + ".dat";
}

std::ostream &operator<<(std::ostream &output, const command_t &c) {
    output << "(Rank = '" << c.unique_id << "', Command = '" << c.command
           << "', Version = '" << c.version << "', File = '" << c.stem() << "')";
    return output;
}

template<typename A> void command_t::save(A& ar) {
    ar.write(this);
}

template<typename A> void command_t::load(A& ar) {
    ar.read(this);
}
