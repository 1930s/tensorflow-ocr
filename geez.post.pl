#!/usr/bin/perl -w
#
# geez.post.pl
#
# post-processing for Geez OCR
#
# Raphael Finkel 6/2014

use strict;

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";
use utf8;

my $line;
while ($line = <STDIN>) {
	chomp $line;
	$line =~ s/\s//g; # no internal spaces
	$line =~ s/፡/ ፡ /g; # extra space around :
	$line =~ s/።/ ። /g;
	print "$line\n";
} # one line
