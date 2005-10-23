#!/usr/bin/perl

open(BUILD, "/home/tutty/build.h") or die "cannot open build.h!";
$line = <BUILD>;
close(BUILD);

($define, $buildnumber, $build) = split(' ', $line);
$build++;

open(BUILD, ">/home/tutty/build.h");
print BUILD "#define BUILDNUMBER $build\n";
close(BUILD);

system("touch /home/tutty/build.h");
system("touch /home/tutty/version.c");
