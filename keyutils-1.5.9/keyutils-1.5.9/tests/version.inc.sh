# Version comparison shell functions
#
###############################################################################
#
# Copyright (C) 2005, 2013 Red Hat, Inc. All Rights Reserved.
# Written by David Howells (dhowells@redhat.com)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
###############################################################################

###############################################################################
#
# compare version numbers to see if the first is less (older) than the second
#
###############################################################################
function version_less_than ()
{
    a=$1
    b=$2

    if [ "$a" = "$b" ]
    then
	return 1
    fi

    # grab the leaders
    a_version=${a%%-*} a_release=${a#*-}
    b_version=${b%%-*} b_release=${b#*-}

    if [ "$a_version" = "$b_version" ]
    then
	case "$a_release" in
	    rc[0-9]*)
		case "$b_release" in
		    rc[0-9]*)
			__version_less_than_dot "${a_release#rc}" "${b_release#rc}"
			return $?
			;;
		    *)
			return 0;
			;;
		esac
		;;
	esac

	case "$b_release" in
	    rc[0-9]*)
		return 1;
		;;
	esac

	if [ "$a_version" = "$a" -o "$b_version" = "$b" ]
	then
	    if [ "$a_version" = "$b_version" ]
	    then
		[ "$a_version" = "$a" ]
	    else
		__version_less_than_dot "$a_version" "$b_version"
	    fi
	fi
    else
	__version_less_than_dot "$a_version" "$b_version"
    fi
}

function __version_less_than_dot ()
{
    a=$1
    b=$2

    if [ "$a" = "$b" ]
    then
	return 1
    fi

    # grab the leaders
    x=${a%%.*}
    y=${b%%.*}

    if [ "$x" = "$a" -o "$y" = "$b" ]
    then
	if [ "$x" = "$y" ]
	then
	    [ "$x" = "$a" ]
	else
	    expr "$x" \< "$y" >/dev/null
	fi
    elif [ "$x" = "$y" ]
    then
	__version_less_than_dot "${a#*.}" "${b#*.}"
    else
	expr "$x" \< "$y" >/dev/null
    fi
}
