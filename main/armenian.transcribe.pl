#!/usr/bin/perl -w
#
# armenian.transcribe.pl
#
# Transcription from Armenian to Roman
#
# Raphael Finkel 10/2010

use strict;

binmode STDIN, ":utf8";
binmode STDOUT, ":utf8";
binmode STDERR, ":utf8";
use utf8;

my $line;
while ($line = <STDIN>) {
	$line =~ s/▯/ /g; # OCR did not recognize
	print $line;

	# Western Armenian
	$line =~ s/ու/u/; 
	$line =~ s/ոյ\b/o/; 
	$line =~ s/\bե/jɛ/; 
	$line =~ s/\bո/vo/; 
	$line =~ s/այ/ɑj/; 
	$line =~ s/յա|իա|եա|եայ/jɑ/; 
	$line =~ s/եյ|էյ/ɛj/; 
	$line =~ s/ույ|ոյ/uj/; 
	$line =~ s/յու|իւ/ju/; 
	$line =~ s/\x{0561}/ɑ/g; # ayb ա
	$line =~ s/\x{0562}/p/g; # pen (ben) բ
	$line =~ s/\x{0563}/kʰ/g; # kim (gim) գ
	$line =~ s/\x{0564}/tʰ/g; # ta (da) դ
	$line =~ s/\x{0565}/ɛ/g; # yech (ech) ե
	$line =~ s/\x{0566}/z/g; # za զ
	$line =~ s/\x{0567}/ɛ/g; # eh է
	$line =~ s/\x{0568}/ə/g; # et ը
	$line =~ s/\x{0569}/tʰ/g; # to, թ
	$line =~ s/\x{056a}/ʒ/g; # zhe ժ
	$line =~ s/\x{056b}/i/g; # ini ի
	$line =~ s/\x{056c}/l/g; # liwn լ
		$line =~ s/\x{053c}/L/g; # liwn Լ
	$line =~ s/\x{056d}/χ/g; # xeh խ
	$line =~ s/\x{056e}/dz/g; # dza (ca) ծ
	$line =~ s/\x{056f}/g/g; # gen (ken) կ
	$line =~ s/\x{0570}/h/g; # ho հ
	$line =~ s/\x{0571}/tsʰ/g; # tsa (ja) ձ
	$line =~ s/\x{0572}/ʁ/g; # ghad ղ
	$line =~ s/\x{0573}/dʒ/g; # (je) cheh ճ
	$line =~ s/\x{0574}/m/g; # men մ
	$line =~ s/\x{0575}/j/g; # hee (yi) յ
	$line =~ s/\x{0576}/n/g; # now ն
	$line =~ s/\x{0577}/ʃ/g; # sha շ
	$line =~ s/\x{0578}/o/g; # vo ո
	$line =~ s/\x{0579}/tʃʰ/g; # cha չ 
	$line =~ s/\x{057a}/b/g; # bey (peh) պ
	$line =~ s/\x{057b}/tʃʰ/g; # che (jheh) ջ
	$line =~ s/\x{057c}/ɾ/g; # ra ռ
	$line =~ s/\x{057d}/s/g; # seh ս
	$line =~ s/\x{057e}/v/g; # vew վ
	$line =~ s/\x{057f}/d/g; # diun (tiwn) տ
	$line =~ s/\x{0580}/ɾ/g; # reh ր 
	$line =~ s/\x{0581}/tsʰ/g; # co, ց
	$line =~ s/\x{0582}/v/g; # yiwn ւ
	$line =~ s/\x{0583}/pʰ/g; # piwr փ
	$line =~ s/\x{0584}/kʰ/g; # keh ք
	$line =~ s/\x{0585}/o/g; # oh օ
	$line =~ s/\x{0586}/f/g; # feh ֆ
	$line =~ s/\x{055a}/'/g; # apostrophe
	$line =~ s/\x{055c}/!/g; # exclamation mark
	$line =~ s/\x{055e}/?/g; # question mark

	print $line;
} # one line
