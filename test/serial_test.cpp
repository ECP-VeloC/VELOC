#include "include/veloc.hpp"
#include "include/veloc/bitsery.hpp"
#include <cassert>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    double f;
};

//define how object should be serialized/deserialized
template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.value2b(o.e);
    s.value8b(o.f);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <veloc_cfg>" << std::endl;
        return -1;
    }

    const unsigned int id = 0;
    veloc::client_t *ckpt = veloc::get_client(id, argv[1]);

    int k = 7;
    MyStruct data{8941, MyEnum::V2, 0.045};

    ckpt->mem_protect(0, veloc::bitsery::serializer(data), veloc::bitsery::deserializer(data));
    ckpt->mem_protect(1, &k, 1, sizeof(k));

    ckpt->checkpoint("serial.test", 0);

    k = 0;
    data.i = 112233; data.f = 0.01;

    ckpt->restart("serial.test", 0);
    assert(data.i = 8941 && data.e = MyEnum::V2 && data.f = 0.01);
    assert(k == 7);

    return 0;
}
