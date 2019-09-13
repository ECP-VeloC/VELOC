import sys, veloc

if __name__ == "__main__":
    print("VELOC-Python test suite")

    ### Checkpoint
    ckpt = veloc.Ckpt(0, sys.argv[1])
    data_1 = "abc"
    data_2 = [1, 2, 3]
    ckpt.protected = (data_1, data_2)
    ckpt.checkpoint("python-test", 100)

    ### Restart
    ckpt.protected = None
    latest_version = ckpt.restart_test("python-test", 0)
    assert latest_version == 100, "latest version %d != 100" % latest_version
    ckpt.restart("python-test", latest_version)
    (data_3, data_4) = ckpt.protected
    assert data_3 == data_1 and data_4 == data_2, "recovery failed"

    print("Passed successfully!")
