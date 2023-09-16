#include "include/veloc.hpp"
#include "common/ckpt_util.hpp"

#include <fstream>
#include <memory>
#include <getopt.h>

#define __DEBUG
#include "common/debug.hpp"

void exit_with_usage() {
    std::cerr << "Usage: veloc-inspect --header | --extract <id> [--size <size>] --ckpt <ckpt_name>" << std::endl;
    std::cerr << "Note: shortcuts (-h, -e, -s, -c) are also allowed" << std::endl;
    std::cerr << "Extract will dump binary output to stdout. You probably want to redirect stdout to a file." << std::endl;

    exit(-1);
}

int main(int argc, char **argv) {
    int ret;
    bool header = false;
    int id = -1;
    size_t size = 0;
    std::map<int, size_t> region_info;
    std::string ckpt_name;

    // initialize both long and short form arguments
    static struct option long_ops[] = {
        {"header", no_argument, 0, 'h'},
        {"extract", required_argument, 0, 'e'},
        {"size", required_argument, 0, 's'},
        {"ckpt", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    while ((ret = getopt_long(argc, argv, "he:s:c:", long_ops, NULL)) != -1)
        switch (ret) {
        case 'h':
            header = true;
            break;
        case 'e':
            if (sscanf(optarg, "%d", &id) != 1)
                exit_with_usage();
            break;
        case 's':
            if (sscanf(optarg, "%lu", &size) != 1)
                exit_with_usage();
            break;
        case 'c':
            ckpt_name = optarg;
        }

    if (ckpt_name == "" || (!header && id < 0))
        exit_with_usage();

    size_t header_size = read_header(ckpt_name, region_info);
    if (header_size == 0)
        return -1;

    if (header) {
        std::cout << "Header for " << ckpt_name << ":" << std::endl;
        size_t total = 0;
        for (const auto &e : region_info) {
            std::cout << "id = " << e.first << ", size = " << e.second << std::endl;
            total += e.second;
        }
        std::cout << "Total checkpoint size = " << total << std::endl;
        return 0;
    }

    try {
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(ckpt_name, std::ifstream::in | std::ifstream::binary);
        size_t seek_offset = header_size;
        bool found = false;
        for (const auto &e: region_info) {
            if (e.first == id) {
                found = true;
                break;
            }
            seek_offset += e.second;
        }
        if (!found)
            throw std::ifstream::failure("cannot find region " + std::to_string(id) + " in the header");
        if (region_info[id] < size)
            throw std::ifstream::failure("region " + std::to_string(id) + " is of size " + std::to_string(region_info[id]) +
                                         ", which is smaller than requested size " + std::to_string(size));
	f.seekg(seek_offset);
        if (size == 0)
            size = region_info[id];
        std::vector<char> region(size);
        f.read(&region[0], size);
        std::cout.write(&region[0], size);
    } catch (std::fstream::failure &e) {
        ERROR("cannot read from checkpoint file " << ckpt_name << ", reason: " << e.what());
        return -1;
    }

    return 0;
}
