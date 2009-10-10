#!/usr/bin/perl -w

sub http_list
{
    my $FH;
    my ($baseurl, $url, $level) = @_;
    open $FH, "wget -q $baseurl/$url -O -|" || die "wget error";

    while(<$FH>)
    {
	if(/href="([\w.-]+\/)"/)
        {
	    http_list($baseurl, $url.$1, $level+1);
        }
	elsif(/href="([\w.-]+\.pdf)"/)
	{
	    system("mkdir -p $url");
	    print "$baseurl$url$1\n";
#	    system("curl -z $url$1 $baseurl$url$1 -o $url$1");
	    system("wget -c $baseurl$url$1 -O $url$1");
	}
    }
    close $FH;
}

http_list("http://www.x.org/docs/", "", 0);
