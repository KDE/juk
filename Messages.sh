#! /bin/sh
$EXTRACTRC *.rc >> rc.cpp || exit 11
$XGETTEXT *.rc *.cpp *.h -o $podir/juk.pot
