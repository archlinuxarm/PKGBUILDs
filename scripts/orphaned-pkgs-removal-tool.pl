#!/usr/bin/perl -w
use strict;

use File::Find;
use POSIX;




my @dirs = qw(arm armv6h armv7h);
my @files = ();
my @unfails = ();
my (%arm, %armv6h, %armv7h);


print "This will output a listing of files that have been abandoned by" .
" plugbuild for some reason or another. \nIt can be pasted " .
"into a bash script/xargs, and then outputs unfails for plugbuild just to make sure\n\n\n";

foreach my $arch (@dirs){
    find(\&pushFile, "/var/www/archlinuxarm/html/$arch");
}

foreach (@files){
    my @parts = split('/', $_);
    my $package = $parts[-1];
    #break the package name down to it's base part...this is probably missing a few, but it's quick and dirty for now.
    $package =~ s/-\d.*-\d{1,2}-(any|arm|armv6h|armv7h)\.pkg\.tar\.xz//;
    if ($package =~ m/.*(dkms|dpi|blank).*/) {
        print "\#Skipping " . $_ . "\n";
        next;
    }
    
    if ($parts[5] eq "arm"){
        if (exists $arm{$package}) {
            push(@unfails, '!unfail 5 ' . $package);
            #stat[9] is the epoch of the last modified date
            if ((stat ($_))[9] < (stat ($arm{$package}))[9]) {
                print "rm \"" . $_ . "\"\n";
                print "\#rm \"" . $arm{$package} . "\"\n";
                
            }
            else {
                print "rm \"" . $arm{$package} . "\"\n";
                print "#rm \"" . $_ . "\"\n";
                $arm{$package} = $_;
            }
            
        }
        else {
            $arm{$package} = $_;
        }
        
    }
    elsif ($parts[5] eq "armv6h"){
        if (exists $armv6h{$package}) {
            push(@unfails, '!unfail 6 ' . $package);
            if ((stat ($_))[9] < (stat ($armv6h{$package}))[9]) {
                print "rm \"" . $_ . "\"\n";
                print "\#rm \"" . $armv6h{$package} . "\"\n";
                
            }
            else {
                print "rm \"" . $armv6h{$package} . "\"\n";
                print "#rm \"" . $_ . "\"\n";
                $armv6h{$package} = $_;
            }
            
        }
        else {
            $armv6h{$package} = $_;
        }
        
    }
    elsif ($parts[5] eq "armv7h"){
        if (exists $armv7h{$package}) {
            push(@unfails, '!unfail 7 ' . $package);
            if ((stat ($_))[9] < (stat ($armv7h{$package}))[9]) {
                print "rm \"" . $_ . "\"\n";
                print "\#rm \"" . $armv7h{$package} . "\"\n";
                
            }
            else {
                print "rm \"" . $armv7h{$package} . "\"\n";
                print "#rm \"" . $_ . "\"\n";
                $armv7h{$package} = $_;
            }
            
        }
        else {
            $armv7h{$package} = $_;
        }
        
    }
    
    
    
}

print "\n\nPaste this into the plugbuild window to make sure that any packages deleted by accident/wrong version get rebuilt.\n\n";
foreach (@unfails){
    print $_ . "\n";
}



sub pushFile()
{
    push @files, $File::Find::name if(/\.pkg\.tar\.xz/i);
}
