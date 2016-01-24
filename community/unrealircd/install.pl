#!/usr/bin/perl -w
use Env qw(pkgdir);
my $last = pop @ARGV;
my $cmd = "/usr/bin/install ".(join ' ',@ARGV)." $pkgdir/$last";
system($cmd);
