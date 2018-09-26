#!/usr/bin/perl -w
#
# fixutf8: clean up codes used in forwards http://yiddish.forward.com
# and in OCR to create ligatures, but no precomposed, letters
#
# Raphael Finkel 6/2007

use strict;
use utf8;

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";

$/ = ""; # read in chunks
while (my $data = <STDIN>) {
	$data =~ s/וו/װ/g;
	$data =~ s/(?<!\bמק)וי/ױ/g; # exception: mekuyem.  Need also שינוי
	$data =~ s/\bיי(?=\P{M})/ייִ/g;
	$data =~ s/יִ/יִ/g;
	$data =~ s/יי/ײ/g;
	$data =~ s/ײַ/ײַ/g;
	$data =~ s/יַי/ײַ/g; # ocr of italic might put pasekh only on first
	$data =~ s/ײִ/ייִ/g;
	$data =~ s/ױִ/ויִ/g;
	$data =~ s/יַי/ײַ/g;
	$data =~ s/וױי(?=\P{M})/װײ/g;
	$data =~ s/­//g; # soft hyphenation mark
	$data =~ s/שׂ/שׂ/g; # combining, not precomposed
	$data =~ s/בּ/בּ/g; # combining, not precomposed
	$data =~ s/כּ/כּ/g; # combining, not precomposed
	$data =~ s/וּ/וּ/g; # combining, not precomposed
	$data =~ s/אָ/אָ/g; # combining, not precomposed
	$data =~ s/אַ/אַ/g; # combining, not precomposed
	$data =~ s/תּ/תּ/g; # combining, not precomposed
	$data =~ s/פֿ/פֿ/g; # combining, not precomposed
	$data =~ s/פּ/פּ/g; # combining, not precomposed
	$data =~ s/פ(?=\p{L})/פֿ/g; 
	$data =~ s/ {1,2}([:\?\.;!])/$1/g; # remove extraneous space before punctuation
	$data =~ s/(\p{L}\p{M}*),(\p{L}\p{M}*)/$1'$2/g; # not comma, rather apostrophe
	$data =~ s/'( |$)/,$1/mg; # not apostrophe, rather comma
	print $data;
} # each chunk

