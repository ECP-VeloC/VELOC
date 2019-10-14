import ctypes, ctypes.util, pickle, os

def loadLib(id, cfg):
    veloc_lib = ctypes.util.find_library("veloc-client")
    if veloc_lib == None:
        raise RuntimeError("VELOC-Python: cannot find libveloc-client.so, check LD_LIBRARY_PATH")
    lib = ctypes.CDLL(veloc_lib)
    if lib.VELOC_Init_single(id, cfg.encode()) != 0:
        raise RuntimeError("VELOC-Python: initialization failure; id=%d, cfg=%s" % (id, cfg))
    return lib

class NumPyCkpt:
    def __init__(self, id, cfg):
        self.lib = loadLib(id, cfg)

    def protect(self, id, array):
        self.lib.VELOC_Mem_protect(id, ctypes.c_void_p(array.ctypes.data), ctypes.c_size_t(array.size), ctypes.c_size_t(array.itemsize))

    def unprotect(self, id):
        self.lib.VELOC_Mem_unprotect(id)

    def checkpoint(self, name, version):
        if self.lib.VELOC_Checkpoint(name.encode(), version) != 0:
            raise RuntimeError("VELOC-Python: checkpoint failure; name=%s, version=%d" % (name, version))

    def get_version(self, name, version):
        return self.lib.VELOC_Restart_test(name.encode(), version)

    def restore(self, name, version):
        if self.lib.VELOC_Restart(name.encode(), version) != 0:
            raise RuntimeError("VELOC-Python: restart failure; name=%s, version=%d" % (name, version))

    def __del__(self):
        self.lib.VELOC_Finalize(0)

class PickleCkpt:
    def __init__(self, id, cfg):
        self.protected = None
        self.lib = loadLib(id, cfg)

    def checkpoint(self, name, version):
        if self.protected == None:
            raise RuntimeError("VELOC-Python: checkpoint failure: no data structures protected")
        pickled = pickle.dumps(self.protected)
        self.lib.VELOC_Mem_protect(0, pickled, len(pickled), 1)
        if self.lib.VELOC_Checkpoint(name.encode(), version) != 0:
            raise RuntimeError("VELOC-Python: checkpoint failure; name=%s, version=%d" % (name, version))

    def restart_test(self, name, version):
        return self.lib.VELOC_Restart_test(name.encode(), version)

    def restart(self, name, version):
        if self.lib.VELOC_Restart_begin(name.encode(), version) != 0:
            raise RuntimeError("VELOC-Python: restart begin failure; name=%s, version=%d" % (name, version))
        psize = self.lib.VELOC_Recover_size(0)
        if psize == 0:
            raise RuntimeError("VELOC-Python: invalid checkpoint, no pickled data found")
        pickled = ctypes.create_string_buffer(psize)
        self.lib.VELOC_Mem_protect(0, pickled, psize, 1)
        if self.lib.VELOC_Recover_mem() != 0:
            raise RuntimeError("VELOC-Python: cannot recover pickled memory; size=%d" % psize)
        self.lib.VELOC_Restart_end()
        self.protected = pickle.loads(pickled.raw)

    def __del__(self):
        self.lib.VELOC_Finalize(0)
