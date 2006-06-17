#!/usr/bin/perl

system("/usr/apps/Subversion/bin/svn commit -m \"Automated commit before build.\"");
system("/usr/apps/Subversion/bin/svn update");
system("/usr/apps/Subversion/bin/svnversion > revision");

open(REVISION, "<revision");
$rev = <REVISION>;
close(REVISION);
unlink("revision");
chomp $rev;

open(BUILD, ">/home/devel/plaunch/build.h");
print BUILD "#define REVISION $rev\n";
close(BUILD);

system("touch /home/devel/plaunch/build.h");
