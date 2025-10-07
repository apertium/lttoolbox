#!/usr/bin/env bash
set -e
args=()

while [[ $# -gt 0 ]];
do
	case "$1" in
	--prefix)
		args+=("-DCMAKE_INSTALL_PREFIX=$2")
		shift 2
		;;
	--prefix=*)
		args+=("-DCMAKE_INSTALL_PREFIX=${1#*=}")
		shift
		;;
	--enable-python-bindings)
		args+=("-DENABLE_PYTHON_BINDINGS=ON")
		shift
		;;
	*)
		args+=("$1")
		shift
		;;
	esac
done

set -- "${args[@]}"

D=$(dirname "$0")

echo "- rm -rf CMake caches"
rm -rf install_manifest.txt CMakeCache.txt *.cmake CMakeFiles [sp]*/CMakeFiles [sp]*/*.cmake _CPack_Packages Testing
echo "- cmake " "$@" "$D"
cmake "$@" "$D"
echo "- You may now perform: make -j8"
