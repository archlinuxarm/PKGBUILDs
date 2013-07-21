#!/usr/bin/perl -w
use strict;

use File::Find;
use POSIX;



my @dirs = qw(arm armv6h armv7h);
my @files = ();
my @unfails = ();
my %packages;



foreach my $arch (@dirs){
    find(\&pushFile, "/var/www/archlinuxarm/html/$arch");
}


foreach (@files){
my $filename = $_;
my @parts = split('/', $_);
    my $package = $parts[-1];
    #break the package name down to it's base part...this is probably missing a few, but it's quick and dirty for now.
    $package =~ s/-\d.*-\d{1,2}-(any|arm|armv6h|armv7h)\.pkg\.tar\.xz//;
        my @temparray = ($parts[5], $package, $filename);
    $packages{(((stat($filename))[9]))} = \@temparray;


}


my $counter = 0;

foreach my $key (sort keys %packages){
    $counter++;
    if ($counter > 10){
        last;
    }
    
    if (${$packages{$key}}[0] eq "arm"){
        print "!unfail 5 ". ${$packages{$key}}[1];
    }
    elsif (${$packages{$key}}[0] eq "armv6h"){
        print "!unfail 6 ". ${$packages{$key}}[1];
    }
    elsif (${$packages{$key}}[0] eq "armv7h"){
        print "!unfail 7 ". ${$packages{$key}}[1];
    }    
}

sub pushFile()
{
    push @files, $File::Find::name if(/\.pkg\.tar\.xz/i);
}
