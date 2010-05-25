#!/usr/bin/perl

open(BUILD, "/home/tutty-0.58.1/build.h") or die "cannot open build.h!";
$line = <BUILD>;
close(BUILD);

($define, $buildnumber, $build) = split(' ', $line);
$build++;

open(BUILD, ">/home/tutty-0.58.1/build.h");
print BUILD "#define BUILDNUMBER $build\n";
close(BUILD);

system("touch /home/tutty-0.58.1/build.h");
system("touch /home/tutty-0.58.1/version.c");
