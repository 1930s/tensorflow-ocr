#!/usr/bin/perl -w
# 
# raismann.post.pl
#
# convert to YIVO
# Raphael Finkel © 2017

use utf8;
use strict;

$/ = undef;
binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
my $text = lc <STDIN>;
$text =~ s/i\./i/g; # OCR correction
# germanic spelling
$text =~ s/\bsitzen\b/SITSEN/g;
$text =~ s/\bviel\b/FIL/g;
$text =~ s/\bviel\b/FIL/g;
$text =~ s/\bverdriessen\b/FERDRISEN/g;
# german/dutch conversions
# $text =~ s/\bie/YE/g;
$text =~ s/ei/AY/g;
$text =~ s/ee/EY/g;
$text =~ s/ui/OY/g;
$text =~ s/nsch/NTSH/g;
$text =~ s/sch/SH/g;
$text =~ s/ch/KH/g;
$text =~ s/ss/S/g;
$text =~ s/tz/TS/g;
$text =~ s/c(k?)/K/g;
$text =~ s/z/TS/g;
$text =~ s/s(t|p)/SH$1/g;
$text =~ s/jim\b/YIM/g; # narrojim
$text =~ s/\bjo/YO/g; # jogen
$text =~ s/\bj(è|è)/YE/gi; # jè
$text =~ s/j/ZH/g; # mujik
$text =~ s/u/U/g;
$text =~ s/ie/I/g;
$text =~ s/eh/EY/g;
$text =~ s/s([aeiou])/Z$1/ig;
$text =~ s/\b([iU])s\b/$1z/g;
$text =~ s/è/E/g;
$text =~ s/w/V/g;
$text =~ s/aa/A/g;
$text =~ s/mm/M/g;
$text =~ s/rr/R/g;
$text =~ s/tt/T/g;
$text =~ s/ll/L/g;
$text =~ s/ff/F/g;
# punctuation
$text =~ s/'([\s,])/,$1/g;
$text =~ s/,'(?=\w)/,,/g;
$text =~ s/,,(?!\w)/''/g;
$text =~ s/(\w),(\w)/$1'$2/g;
$text =~ s/([^mnv])en/$1n/ig;
print lc $text;
