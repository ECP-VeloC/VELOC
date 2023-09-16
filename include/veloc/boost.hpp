#ifndef __BOOST_ARCHIVE_HPP
#define __BOOST_ARCHIVE_HPP

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace veloc::boost {
using namespace ::boost;
template <typename T> std::function<void (std::ostream &)> serializer(T &data) {
    return [&data](std::ostream &out) {
	archive::binary_oarchive ar(out);
	ar << data;
    };
}

template <typename T> std::function<bool (std::istream &)> deserializer(T &data) {
    return [&data](std::istream &in) {
        try {
            archive::binary_iarchive ar(in);
            ar >> data;
        } catch (std::exception &e) {
            return false;
        }
        return true;
    };
}
}

#endif //__BOOST_ARCHIVE_HPP
