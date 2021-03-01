#ifndef __CEREAL_HPP
#define __CEREAL_HPP

#include <cereal/archives/binary.hpp>

namespace veloc::cereal {
using namespace ::cereal;
template <typename T> std::function<void (std::ostream &)> serializer(T &data) {
    return [&data](std::ostream &out) {
        BinaryOutputArchive ar(out);
        ar(data);
    };
}

template <typename T> std::function<bool (std::istream &)> deserializer(T &data) {
    return [&data](std::istream &in) {
        try {
            BinaryInputArchive ar(in);
            ar(data);
        } catch (std::exception &e) {
            return false;
        }
        return true;
    };
}
}

#endif //__CEREAL_HPP
