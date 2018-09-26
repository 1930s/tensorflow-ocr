#!/usr/bin/perl -w
# 
# print pages in reverse order

use strict;
my $start = 'tmp/sa_tevye/nybc210429_0*';
my $end = '.tif';

print $start . join("$end\n$start", reverse(63 .. 271)) . "$end\n";
