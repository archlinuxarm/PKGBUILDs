#!/usr/bin/perl -w

use Encode;
use JSON::XS;

$URL=$ARGV[0];

@ALLPAGES=();

my $pageid;
my $pagetitle;
my $from = "";
my $ret;
again:
my $count = 0;
#print STDERR "wget -q \"$URL/api.php?action=query&list=allpages&aplimit=500&format=json&apfilterredir=nonredirects&apfrom=$from\" -O -\n";
$text=`wget -q \"$URL/api.php?action=query&list=allpages&aplimit=500&format=json&apfilterredir=nonredirects&apfrom=$from\" -O -`;
$ret = JSON::XS->new->utf8->decode($text);
$H = $ret->{query}->{allpages};
foreach $i (@$H)
{
    push @ALLPAGES, encode("UTF-8", "$i->{title}");
    printf("%08u %s", $i->{pageid}, encode("UTF-8", "$i->{title}\n"));
    $count++;
}

if($count == 1)
{
    exit 0;
}

@ALLPAGES = sort @ALLPAGES;
$from = $ALLPAGES[-1];
goto again;
