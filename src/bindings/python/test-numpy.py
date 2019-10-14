import sys, veloc
import numpy as np

if __name__ == "__main__":
    print("VELOC-Python numpy test suite")

    ### Checkpoint
    ckpt = veloc.NumPyCkpt(0, sys.argv[1])
    x = np.array([[1.1, 2.2, 3.3], [4.4, 5.5, 6.6]], dtype=np.float32)
    y = np.array([2.2, 3.3, 4.4], dtype=np.float64)
    ckpt.protect(0, x)
    ckpt.protect(1, y)
    ckpt.checkpoint("python-test", 100)

    z = np.zeros((2, 3), dtype=np.float32)
    t = np.zeros(3, dtype=np.float64)
    latest = ckpt.get_version("python-test", 0)
    ckpt.protect(0, z)
    ckpt.protect(1, t)
    ckpt.restore("python-test", latest)
    print(z, t)
    assert np.array_equal(x, z) and np.array_equal(y, t), "numpy array_equal failed"

    print("Passed successfully!")
