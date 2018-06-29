#!/usr/bin/python3

import sys, os.path, shutil
import argparse
import wget, bs4, urllib
import re
import tarfile

# CRAY-specific compiler options
# compiler_options = "-DCMAKE_C_COMPILER=cc -DCMAKE_C_FLAGS=-dynamic -DCMAKE_CXX_COMPILER=CC -DCMAKE_CXX_FLAGS='-dynamic -std=c++14'"
compiler_options = ""
cmake_build_type="Release"

def install_dep(git_link):
    name = os.path.basename(git_link).split('.')[0]
    print("Installing {0}...".format(name))
    try:
        os.system("git clone {0} {1}".format(git_link, args.temp + '/' + name))
        os.system("cd {0} && cmake -DCMAKE_PREFIX_PATH={1}\
        -DCMAKE_INSTALL_PREFIX={1} -DCMAKE_BUILD_TYPE={2} {3} && make install".format(args.temp + '/' + name,
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
            for link in soup.findAll('a', attrs={'href': re.compile("bz2")}):
                boost_arch = wget.download(link.get('href'), out=args.temp)
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
        install_dep('https://github.com/ECP-VeloC/KVTree.git')
        install_dep('https://github.com/ECP-VeloC/AXL.git')
        install_dep('https://github.com/ECP-VeloC/rankstr.git')
        install_dep('https://github.com/ECP-VeloC/shuffile.git')
        install_dep('https://github.com/ECP-VeloC/redset.git')
        install_dep('https://github.com/ECP-VeloC/er.git')

    # VeloC
    os.system("cmake -DCMAKE_PREFIX_PATH={0}\
    -DCMAKE_INSTALL_PREFIX={0} -DCMAKE_BUILD_TYPE={1} {2} && make install".format(args.prefix, cmake_build_type, compiler_options))

    # Cleanup
    if (not args.debug):
        try:
            shutil.rmtree(args.temp)
        except OSError as err:
            print("Cannot cleanup temporary directory {0}!".format(args.temp))
            sys.exit(5)

    print("Installation successful!")
