#!/usr/bin/perl

open(BUILD, "/home/putty/build.h") or die "cannot open build.h!";
$line = <BUILD>;
close(BUILD);

($define, $buildnumber, $build) = split(' ', $line);
$build++;

open(BUILD, ">/home/putty/build.h");
print BUILD "#define BUILDNUMBER $build\n";
close(BUILD);

system("touch /home/putty/build.h");
system("touch /home/putty/version.c");
