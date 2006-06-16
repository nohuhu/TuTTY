#!/usr/bin/perl

open(BUILD, "/home/devel/tutty/build.h") or die "cannot open build.h!";
$line = <BUILD>;
close(BUILD);

($define, $buildnumber, $build) = split(' ', $line);
$build++;

open(BUILD, ">/home/devel/tutty/build.h");
print BUILD "#define BUILDNUMBER $build\n";
close(BUILD);

system("touch /home/devel/tutty/build.h");
system("touch /home/devel/tutty/version.c");
