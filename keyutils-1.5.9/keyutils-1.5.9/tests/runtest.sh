#!/bin/sh

TESTS=$*

PARENTTEST=${TEST}

if [ `id -u` != 0 ]
then
    echo "#### Some tests require root privileges." >&2
    echo "#### It is recommended that this be run as root." >&2
fi

for i in ${TESTS}; do
	export TEST=$i
        pushd $i >/dev/null
	sh ./runtest.sh || exit 1
	popd >/dev/null
done

if [ `id -u` != 0 ]
then
    echo "#### Some tests required root privileges." >&2
    echo "#### They have been tested for the appropriate failure." >&2
    echo "#### It is recommended that this be run as root." >&2
fi
