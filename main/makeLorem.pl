#!/usr/bin/perl -w
#
# makeLorem.pl LENGTH START END
#
# makes random text using the Unicode range from START to END, of total length
# LENGTH.

use strict;

# constants
my $wordLength = 5;
my ($length, $start, $end); # initialized

# variables
my @alphabetic; # characters in the range that are letters

sub init {
	($length, $start, $end) = @ARGV;
	if (!defined($length)) {
		die("Usage: $0 LENGTH [START END]
		LENGTH: number of characters
		START: start of Unicode block (as hex string)
		END: end+1 of Unicode block (as hex string)\n");
	}
	# ($start, $end) = ('0530', '0590') unless defined($start); # Armenian
	# ($start, $end) = ('0590', '05f5') unless defined($start); # Hebrew
	# ($start, $end) = ('0400', '0500') unless defined($start); # Cyrillic
	($start, $end) =   ('1200', '137e') unless defined($start); # Ethiopic
	binmode STDIN, ":utf8";
	binmode STDOUT, ":utf8";
	binmode STDERR, ":utf8";
	for my $index (hex($start) .. hex($end)-1) {
		push @alphabetic, chr($index) if chr($index) =~ /\p{L}/;
	} # each index
	# print join('', @alphabetic) . "\n";
} # init

sub generate {
	my @answer;
	my $size = @alphabetic;
	for my $sequence (1 .. $length) {
		my $index = int(rand($size));
		# print("index $index\n");
		push @answer, $alphabetic[int(rand(@alphabetic))];
		push @answer, ' ' if ($sequence % $wordLength == 0);
	}
	print join('', @answer) . "\n";
} # generate

init();
generate();

