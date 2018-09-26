#!/usr/bin/perl -w
# 
# use standard spelling rules to try to convert words to correct form

use strict;
use utf8;

# constants
my $spellFile = '/homes/raphael/HTML/yiddish/wordlist.utf8.txt';
my $nonsense = 'X'; # never appears in a normal word

# global variables
my (
	@corrections, # in the form "orig replacement'
	%okWords, # each ok word just maps to 1
	%corrected, # cache for attempted corrections, successful or not
);

sub init {
	@corrections = (
		'ײ ײַ',
		'ת תּ',
		'כ כּ',
		'ב בֿ',
		'יע יִע',
		'עי עיִ',
		'א(?=\P{M}) אַ',
		'א(?=\P{M}) אָ',
		'ש(?=\P{M}) שׂ',
	);
	binmode STDIN, ":utf8";
	binmode STDOUT, ":utf8";
	binmode STDERR, ":utf8";
	open OKS, $spellFile or die("Cannot read $spellFile\n");
	binmode OKS, ":utf8";
	while (my $line = <OKS>) {
		chomp $line;
		$okWords{$line} = 1;
		# print STDERR "put [$line] in list\n";
	} # one line
	close OKS;
} # init

sub testIt {
	@corrections = ('x y', 'a b');
	%okWords = ('ybab' => 1);
	for my $badWord ('ybab', 'xaaa') {
		print STDERR "trying to fix $badWord, get " . fixit($badWord, 0) . "\n";
	} # each test
} # testit

sub fixit {
	my ($word, $where) = @_;
	return $word if $where >= length($word);
	# print STDERR (" " x $where) . "fixit [$word] at $where\n";
	my $orig = $word;
	my $answer;
	return $word if (defined $okWords{$word});
	for my $correction (@corrections) { # try a correction
		my ($target, $replace) = split(/\s+/, $correction);
		# print STDERR "considering $target => $replace at $where\n";
		pos($word) = $where;
		if ($word =~ s/\G$target/$nonsense/) {
			$word =~ /$nonsense/gc; # must be gc, must not be substitution
			my $position = pos($word);
			$word =~ s/$nonsense/$replace/;
			# print STDERR (" " x $where) . "changed [$orig] to [$word]\n";
			# print STDERR (" " x $where) . "trying $word at $position\n";
			return $word if defined $okWords{$word};
			$answer = fixit($word, $where+1);
				# further fixes, including this one
			return($answer) unless $answer eq $word;
			# print STDERR (" " x $where) . "no joy\n";
			$word = $orig; # so we can try again
		} # try a substitution
	} # each possible correction
	return(fixit($orig, $where+1)); # further fixes, but not this one
} # fixit

sub doit {
	my $remainder = ''; # first part of a hyphenated word at end of line
	while (my $line = <STDIN>) {
		# print STDERR "working on $line";
		if ($remainder ne '') {
			$line =~ s/^(\s*)/$1$remainder/;
			$remainder = '';
		}
		if ($line =~ s/(\w+)־$//) {
			$remainder = $1;
		}
		for my $part (split(/([\p{P}\s]+)/, $line)) {
			my $better = $part; # unless we get a better idea
			if ($part =~ /\p{L}/ and !defined($okWords{$part})) {
				$better = $corrected{$part};
				if (!defined($better)) {
					$better = fixit($part, 0);
					$corrected{$part} = $better;
				}
			}
			print $better;
		} # each part
	} # one line
} # doit

init();
# testIt();
doit();
