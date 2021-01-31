#ifndef __BITSERY_HPP
#define __BITSERY_HPP

#include <bitsery/bitsery.h>
#include <bitsery/adapter/stream.h>

namespace veloc::bitsery {
using namespace ::bitsery;
template <typename T> std::function<void (std::ostream &)> serializer(T &data) {
    return [&data](std::ostream &out) {
        Serializer<OutputBufferedStreamAdapter> ser{out};
        ser.object(data);
        ser.adapter().flush();
    };
}

template <typename T> std::function<bool (std::istream &)> deserializer(T &data) {
    return [&data](std::istream &in) {
        auto state = quickDeserialization<InputStreamAdapter>(in, data);
        return state.first == ReaderError::NoError;
    };
}
}

#endif //__BITSERY_HPP
