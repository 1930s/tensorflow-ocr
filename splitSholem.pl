#!/usr/bin/perl -w
#
# splitSholem.pl
#
# split the output files and put them in the Subversion directory
#
# Raphael Finkel © 2015

use strict;

# constants
my $sourceFile = 'tmp/sholem-aleykhem-ale/output.2016.12.05';
my $destDir = 'SholemAleykhemUpdate.2016-12-07';

open DATA, $sourceFile or die("Cannot read $sourceFile\n");
binmode DATA, ":utf8";
$/ = undef;
my $text = <DATA>;
close DATA;

mkdir $destDir unless -d $destDir;
for my $text (split /(?=tmp\/)/, $text) {
	$text =~ s/tmp\/nybc(\d+).*\/nybc.*_(\d+)\n//;
	my ($book, $page) = ($1, $2);
	$text =~ s///g;
	print "Book $book page $page\n";
	mkdir "$destDir/$book" unless -d "$destDir/$book";
	open PAGE, ">$destDir/$book/$page"
		or die("Cannot write to $destDir/$book/$page\n");
	binmode PAGE, ":utf8";
	print PAGE $text;
	close PAGE;
} # each page
