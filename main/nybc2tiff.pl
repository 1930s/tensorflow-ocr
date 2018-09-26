#!/usr/bin/perl -w
#
# nybc2tiff.pl FILE ...
#
# for each PDF file, build a directory in which the file is first burst and
# then converted to TIFF.
#
# Raphael Finkel © 2014

use strict;

for my $file (@ARGV) {
	die("$file should be a pdf file.\n") unless $file =~ /(\w*)\.pdf/;
	my $prefix = $1;
	mkdir $prefix;
	chdir $prefix;
	system("/usr/bin/pdftk ../$file burst");
	my @pdfs = glob "*.pdf";
	for my $burstFile (@pdfs) {
		$burstFile =~ /(\w*)\.pdf/;
		my $name = $1;
		print "working on $prefix/$name\n";
		my $command = "echo '' |" .
			"/usr/bin/gs -sDEVICE=tiffgray -sOutputFile=$name.tiff " .
			"-r400x400 $burstFile > /dev/null";
		system($command);
		# print "$command\n"; exit;
		unlink $burstFile;
	} # each burstFile
	chdir "..";
} # each file
