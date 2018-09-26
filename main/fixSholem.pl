#!/usr/bin/perl -w
# 
# fixSholem.pl
#
# Try some standard fixes on OCR results for Sholem Aleykhem
# Raphael Finkel © 2015

use utf8;
use strict;

$/ = undef;
binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
my $text = <STDIN>;
$text =~ s/--/―/g; # replace multiple hyphens with a long hyphen
$text =~ s/[-―][-―]+/―/g; # remove extra/spurious hyphens
$text =~ s/-/־/g; # replace ordinary with Yiddish hyphens
$text =~ s/ם(\p{L})/ס$1/g; # internal final mem is most likely a samekh
$text =~ s/^ *▮[▮ ]*$//mg; # remove lines containing only blotches
$text =~ tr/()/)(/; # fix parentheses
$text =~ s/[',][,'](\p{L})/„$1/g; # adjust quotes
$text =~ s/(\p{L}\p{M}*[\.!?]?)[',][,']/$1“/g; # adjust quotes
print $text;

