#!/usr/bin/perl -w
#
# armenian.post.pl
#
# post-processing for Armenian OCR
#
# Raphael Finkel 10/2010

use strict;

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";
use utf8;

my $line;
while ($line = <STDIN>) {
	# $line =~ s/սא/այ/g;
	$line =~ s/▯/ /g;
	print $line;
} # one line
