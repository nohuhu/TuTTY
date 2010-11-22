#!/usr/bin/perl

open(ENTR, "/Users/nohuhu/Documents/Development/tutty/trunk/.svn/entries") or die "cannot open SVN entries!";
<ENTR>; <ENTR>; <ENTR>;
$rev = <ENTR>;
close(ENTR);

chomp $rev;
$rev++;

open(BUILD, ">/Users/nohuhu/Documents/Development/tutty/trunk/build.h");
print BUILD "#define SVN_REV $rev\n";
close(BUILD);

$now = time;
utime $now, $now, "/Users/nohuhu/Documents/Development/tutty/trunk/build.h";
utime $now, $now, "/Users/nohuhu/Documents/Development/tutty/trunk/version.c";
