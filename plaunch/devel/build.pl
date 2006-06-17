#!/usr/bin/perl

system("svnversion > revision");

open(REVISION, "<revision");
$rev = <REVISION>;
close(REVISION);
unlink("revision");
chomp $rev;

open(BUILD, ">/home/devel/plaunch/build.h");
print BUILD "#define BUILDNUMBER $rev\n";
close(BUILD);

system("touch /home/devel/plaunch/build.h");
