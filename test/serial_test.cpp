#include "include/veloc.hpp"
#include "include/veloc/bitsery.hpp"
#include "include/veloc/cereal.hpp"
#include <map>
#include <cereal/types/map.hpp>

#undef NDEBUG
#include <cassert>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    double f;
};

// Bitsey
template <typename S> void serialize(S &s, MyStruct &o) {
    s.value4b(o.i);
    s.value2b(o.e);
    s.value8b(o.f);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <veloc_cfg>" << std::endl;
        return -1;
    }

    // for MPI applications, pass the MPI communicator to use the rank as id automatically
    const unsigned int id = 0;
    veloc::client_t *ckpt = veloc::get_client(id, argv[1]);

    int k = 7;
    MyStruct data{8941, MyEnum::V2, 0.045};
    std::map<int, int> map;
    map[1] = 2;

    // mixing of raw (pointer, size) and serialized data structures in the same checkpoint is allowed
    // even the use of different serialization libraries is possible (e.g. bitsey and cereal)
    ckpt->mem_protect(0, &k, 1, sizeof(k));
    ckpt->mem_protect(1, veloc::bitsery::serializer(data), veloc::bitsery::deserializer(data));
    ckpt->mem_protect(2, veloc::cereal::serializer(map), veloc::cereal::deserializer(map));

    ckpt->checkpoint("serial.test", 0);

    // change the data structures after taking the checkpoint
    k = 0;
    data.i = 112233; data.e = MyEnum::V3; data.f = 0.01;
    map.clear();
    map[2] = 3;

    // verify the data structures were correctly restored to their original values on restart
    ckpt->restart("serial.test", 0);
    assert(k == 7);
    assert(data.i == 8941 && data.e == MyEnum::V2 && data.f == 0.045);
    assert(map.size() == 1 && map[1] == 2);
    std::cout << "All tests passed!" << std::endl;

    return 0;
}
