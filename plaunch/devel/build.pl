#!/usr/bin/perl

open(BUILD, "/home/plaunch/build.h") or die "cannot open build.h!";
$line = <BUILD>;
close(BUILD);

($define, $buildnumber, $build) = split(' ', $line);
$build++;

open(BUILD, ">/home/plaunch/build.h");
print BUILD "#define BUILDNUMBER $build\n";
close(BUILD);

system("touch /home/plaunch/build.h");
