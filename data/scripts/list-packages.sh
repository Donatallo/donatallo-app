#!/bin/sh

case $(uname -s) in
FreeBSD)
	exec pkg query "%n"
	;;
Linux)
	if which dpkg-query >/dev/null 2>&1; then
		# Ubuntu/Debian
		exec dpkg-query --show --showformat='${Package}\n' '*'
	fi
	;;
esac