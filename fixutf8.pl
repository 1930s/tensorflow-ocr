#!/usr/bin/perl -w
#
# fixutf8: clean up utf-8 Yiddish text: turn precombined characters into base
# plus mark, and combine separate characters into single glyphs.
# used in forwards http://yiddish.forward.com
#
# Copyright © Raphael Finkel 2007-2009 raphael@cs.uky.edu  
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

use strict;
use utf8;

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";

$/ = undef; # read all at once
my $data = <STDIN>;
$data =~ s/וו/װ/g;
$data =~ s/וי/ױ/g;
$data =~ s/יי/ײ/g;
$data =~ s/יִיִ/ײֵ/g;
$data =~ s/ײִ/ייִ/g;
$data =~ s/ײִ/ייִ/g;
$data =~ s/ױִ/ויִ/g;
$data =~ s/­//g; # soft hyphenation mark
$data =~ s/שׂ/שׂ/g; # combining, not precomposed
$data =~ s/כּ/כּ/g; # combining, not precomposed
$data =~ s/וּ/וּ/g; # combining, not precomposed
$data =~ s/אָ/אָ/g; # combining, not precomposed
$data =~ s/אַ/אַ/g; # combining, not precomposed
$data =~ s/תּ/תּ/g; # combining, not precomposed
$data =~ s/פֿ/פֿ/g; # combining, not precomposed
$data =~ s/פּ/פּ/g; # combining, not precomposed
$data =~ s/פ(?![ּֿ])/פֿ/g; # add rofe
$data =~ s/ ([\?:,\.!])/$1/g; # some OCR generates extra space

print $data;

