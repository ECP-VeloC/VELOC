#!/usr/bin/python3

import sys, os.path, shutil
import argparse
import wget, bs4, urllib
import re
import tarfile

# CRAY-specific compiler options
# compiler_options = "-DCMAKE_C_COMPILER=cc -DCMAKE_C_FLAGS=-dynamic -DCMAKE_CXX_COMPILER=CC -DCMAKE_CXX_FLAGS=-dynamic"
compiler_options = ""
cmake_build_type="Release"

def install_dep(git_link, dep_vers):
    name = os.path.basename(git_link).split('.')[0]
    print("Installing {0}...".format(name))
    try:
        os.system("git clone {0} {1}".format(git_link, args.temp + '/' + name))
        os.system("cd {0} && git fetch && git checkout {1}".format(args.temp + '/' + name, dep_vers))
        os.system("cd {0} && cmake -DCMAKE_INSTALL_PREFIX={1} -DCMAKE_BUILD_TYPE={2} {3}\
                   && make install".format(args.temp + '/' + name,
                                           args.prefix, cmake_build_type, compiler_options))
    except Exception as err:
        print("Error installing dependency {0}: {1}!".format(git_link, err))
        sys.exit(4)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='VeloC installation script')
    parser.add_argument('prefix',
                        help='installation path prefix (typically a home directory)')
    parser.add_argument('--no-boost', action='store_true',
                        help='use existing Boost version (must be pre-installed)')
    parser.add_argument('--no-deps', action='store_true',
                        help='use existing component libraries (must be pre-installed)')
    parser.add_argument('--debug', action='store_true',
                        help='build debug and keep temp directory')
    parser.add_argument('--temp', default='/tmp/veloc',
                        help='temporary directory used during the install (default: /tmp/veloc)')
    args = parser.parse_args()
    args.prefix = os.path.abspath(args.prefix)
    if not os.path.isdir(args.prefix):
        print("Installation prefix {0} is not a valid directory!".format(args.prefix))
        sys.exit(1)
    try:
        if (os.path.isdir(args.temp)):
            shutil.rmtree(args.temp)
        os.mkdir(args.temp)
    except OSError as err:
        print("Cannot create temporary directory {0}!".format(args.temp))
        sys.exit(2)

    print("Installing VeloC in {0}...".format(args.prefix))

    if (args.debug):
        cmake_build_type="Debug"

    # Boost
    if (not args.no_boost):
        print("Downloading Boost...")
        try:
            soup = bs4.BeautifulSoup(urllib.request.urlopen("https://www.boost.org/users/download"), "html.parser")
            link_list = soup.findAll('a', attrs={'href': re.compile("bz2")})
            if len(link_list) > 0:
                boost_arch = wget.download(link_list[0].get('href'), out=args.temp)
                f = tarfile.open(boost_arch, mode='r:bz2')
                f.extractall(path=args.temp)
                f.close()
                if os.path.isdir(args.prefix + '/include/boost'):
                    shutil.rmtree(args.prefix + '/include/boost')
                shutil.move(args.temp + '/' + os.path.basename(boost_arch).split('.')[0]
                            + '/boost', args.prefix + '/include/boost')
        except Exception as err:
            print("Error installing Boost: {0}!".format(err))
            sys.exit(3)

    # Other depenencies
    if (not args.no_deps):
        install_dep('https://github.com/ECP-VeloC/KVTree.git', 'v1.0.2')
        install_dep('https://github.com/ECP-VeloC/AXL.git', 'v0.3.0')
        install_dep('https://github.com/ECP-VeloC/rankstr.git', 'v0.0.2')
        install_dep('https://github.com/ECP-VeloC/shuffile.git', 'v0.0.3')
        install_dep('https://github.com/ECP-VeloC/redset.git', 'v0.0.4')
        install_dep('https://github.com/ECP-VeloC/er.git', 'v0.0.3')

        # build pdsh
        pdsh_tarball = wget.download('https://github.com/chaos/pdsh/releases/download/pdsh-2.33/pdsh-2.33.tar.gz', out=args.temp)
        f = tarfile.open(pdsh_tarball, mode='r:gz')
        f.extractall(path=args.temp)
        f.close()
        os.system("cd {0} && ./configure --prefix={1} --with-mrsh --with-rsh --with-ssh \
                       && make install".format(args.temp + '/pdsh-2.33', args.prefix))
    
    # VeloC
    ret = os.WEXITSTATUS(os.system("cmake -DCMAKE_INSTALL_PREFIX={0} -DCMAKE_BUILD_TYPE={1} -DWITH_PDSH_PREFIX={2} {3}\
                                   && make install".format(args.prefix, cmake_build_type, args.prefix, compiler_options)))

    # Cleanup
    if (not args.debug):
        try:
            shutil.rmtree(args.temp)
        except OSError as err:
            print("Cannot cleanup temporary directory {0}!".format(args.temp))
            sys.exit(5)

    if ret == 0:
        print("Installation successful!")
    else:
        print("Installation failed!")
        sys.exit(ret)
