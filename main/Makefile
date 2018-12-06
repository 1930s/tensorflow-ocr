GOAL = ocr
CFLAGS = -Wall -g 
# LIBS = -ltiff `pkg-config --libs gtk+-2.0 fribidi` 
LIBS = -lefence -lm # for debugging
LIBS = -lm
# LIBS += -ltiff `pkg-config --libs gtk+-2.0` fribidi/libfribidi.`uname -m`.so
BIDI = ../libfribidi.so.0
PANGOX = ../libpangoxft-1.0.so.0
LIBS += -ltiff `pkg-config --libs gtk+-2.0` $(BIDI) $(PANGOX)
# LIBS += /tmp/dmalloc-5.5.2/libdmalloc.a
OBJS = readPicture.o main.o segment.o display.o gtkDisplay.o categorize.o kd.o \
	training.o template.o 
INCLUDES = `pkg-config --cflags gtk+-2.0`
OLDINCLUDES = -I /usr/include/gtk-2.0 -I /usr/include/glib-2.0 \
	-I /usr/lib/x86_64-linux-gnu/glib-2.0/include \
	-I /usr/lib/x86_64-linux-gnu/gtk-2.0/include \
	-I /usr/include/cairo \
	-I /usr/include/pango-1.0 \
	-I /usr/include/gdk-pixbuf-2.0 \
	-I /usr/include/atk-1.0
.SUFFIXES: .pdf .ps .tiff .tif .uyid
OCR = ./ocr
	# changed this from Prof Finkel's root directory so it's not hardcoded
	# so it is machine-inspecific

default: $(GOAL)

.c.o:
	gcc $(CFLAGS) $(INCLUDES) -c $*.c 

$(GOAL): $(OBJS) tags
	gcc $(OBJS) $(LIBS) -o $(GOAL)

$(OBJS): ocr.h

sample: $(GOAL)
	$(GOAL) sample courierData

sample-i: $(GOAL)
	$(GOAL) -f fontData/sample-27.data samples/sample-english.tiff

tags: *.c *.h
	ctags *.c *.h

clean:
	rm -f $(GOAL) $(OBJS)

IMPORTANT = ocr/*.c ocr/*.h ocr/demo ocr/fontData/yiddishData \
	ocr/README ocr/Makefile ocr/COPYRIGHT ocr/fixutf8.pl

DOWNLOAD = *.c *.h demo fontData README Makefile COPYRIGHT fridibi fixutf8.pl

tar:
	cd ..; tar hczf ocr.tar.gz $(IMPORTANT)

upload:
	cd ..; rsync -a $(IMPORTANT) bud:projects/ocr

download:
	rsync -a `echo "bud:projects/ocr/{$(DOWNLOAD)}" | tr ' ' ,` .

# a sample to show how to do it
interactive: ocr
	ocr -f fontData/yiddishData demo/nybc200960_0100

# a sample to show how to do it
batch: ocr
	./ocr -f fontData/yiddishData -t demo/nybc200960_0100 | perl fixutf8.pl

burstPDF:
	pdftk $(PDF) burst

# a sample to show how to do it.
TIFFS = $(subst .pdf,.tiff,$(wildcard tmp/raismann/*.pdf))
tiffs: $(TIFFS)

# .pdf.tiff:
#	 convert -density 200 -threshold 75% -compress LZW $*.pdf $*.tiff

.pdf.ps:
	ps2pdf $*.pdf $*.ps

.ps.tiff:
	echo '' | gs -sDEVICE=tiffgray -sOutputFile=$*.tiff -r400x400 $*.ps > /dev/null

# the following works very nicely
.pdf.tiff:
	echo '' | \
		gs -sDEVICE=tiffgray -sOutputFile=$*.tiff -sCompression=lzw \
			-r400x400 $*.pdf \
		> /dev/null
	tiffsplit $*.tiff page_
	@echo Please move page_* to the directory of $*

# rotate clockwise
rotate:
	for i in $(DIR)/*.tif* ; do \
		echo $$i; \
		tiffcrop -R 90 $$i tmp.tiff; \
		mv tmp.tiff $$i; \
	done

# specific settings
SDIR = nybc208867_orig_tif
SDIR = nybc208871_tif
SDIR = other

sutskever-b: $(OCR)
	$(OCR) -w 5 -h 5 -g 0.50 -p 2.0 -x -i -d 100 \
		-m 1.5 -A -f fontData/sutskever-yi-27.data \
		-H 250 -W 200 \
		-t tmp/sutskever/$(SDIR)/ny*.tif \
		| ~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/sutskever/$(SDIR)/output.uyid

sutskever-i: $(OCR)
	$(OCR) -w 5 -h 5 -g 0.50 -p 2.0 -x -i -d 100 \
		-H 250 -W 200 \
		-m 1.5 -A -f fontData/sutskever-yi-27.data \
		tmp/sutskever/$(SDIR)/ny*.tif

nister-b: $(OCR)
	$(OCR) -w 6 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/nister-yi-27.data \
			-t tmp/nister-mashber/*.tif | \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/nister-mashber/output.uyid

nister-i: $(OCR)
	$(OCR) -w 8 -h 8 -g 0.50 -W 120 -H 100 -p 1.1 \
			-x -f fontData/nister-yi-27.data \
			tmp/nister-mashber/*.tif 

asch-b: $(OCR)
	$(OCR) -w 9 -h 9 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/asch-yi-27.data \
			-t tmp/asch-motke/*.tif | \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/asch-motke/output.uyid

asch-i: $(OCR)
	$(OCR) -w 7 -h 8 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/asch-yi-27.data \
			tmp/asch-motke/*.tif 

alg-b: $(OCR)
	$(OCR) -w 7 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/nister-yi-27.data \
			-t tmp/algemeyne-1/nybc205701_0032	 | \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/algemeyne-1/output.uyid

alg-i: $(OCR)
	$(OCR) -w 7 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/algemenye-yi-27.data \
			tmp/algemeyne-1/*.tif

# motl peysi dem khazns

motl-b: $(OCR)
	$(OCR) -w 5 -h h -g 0.50 -W 120 -H 100 -p 1.1 \
			-t -f fontData/motl-yi-27.data \
			tmp/motl_peysi/*003?.tiff \
		> \
		tmp/motl_peysi/output-unfiltered.uyid


# groyse verterbukh 

ver1-i: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 3 \
			-f fontData/groyse-verterbukh-yi-27.data \
			tmp/groyse-verterbukh-1/*003[4-9].tif
			# tmp/groyse-verterbukh-1/*004[7-9].tif


ver1-b: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 3 \
			-t -f fontData/groyse-verterbukh-yi-27.data \
			tmp/groyse-verterbukh-1/*003[4-9].tif \
			tmp/groyse-verterbukh-1/*00[4-9]?.tif \
			tmp/groyse-verterbukh-1/*0[1-5]??.tif \
		| \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/groyse-verterbukh-1/output.uyid.new

ver2-b: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 3 \
			-t -f fontData/groyse-verterbukh-yi-27.data \
			tmp/groyse-verterbukh-2/*003[4-9].tif \
			tmp/groyse-verterbukh-2/*00[4-9]?.tif \
			tmp/groyse-verterbukh-2/*0[1-5]??.tif \
		| \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/groyse-verterbukh-2/output.uyid.new

ver3-b: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 3 \
			-t -f fontData/groyse-verterbukh-yi-27.data \
			tmp/groyse-verterbukh-3/*002[6-9].tif \
			tmp/groyse-verterbukh-3/*00[3-9]?.tif \
			tmp/groyse-verterbukh-3/*0[1-5]??.tif \
		| \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/groyse-verterbukh-3/output.uyid.new

ver4-b: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 3 \
			-t -f fontData/groyse-verterbukh-yi-27.data \
			tmp/groyse-verterbukh-4/*00[3-9]?.tif \
			tmp/groyse-verterbukh-4/*0[1-5]??.tif \
		| \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/groyse-verterbukh-4/output.uyid.new

# leksikon, volume 1

lex-i: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 2 \
			-f fontData/leksikon-yi-27.data \
			tmp/leksikon-1/*0017.tif

lex-b: $(OCR)
	$(OCR) -w 6 -h 5 -g 0.60 -W 120 -H 100 -p 1.1 -c 2 \
			-t -f fontData/leksikon-yi-27.data \
			tmp/leksikon-1/*001[6-9].tif \
			tmp/leksikon-1/*00[2-9]?.tif \
			tmp/leksikon-1/*0[1-4]??.tif \
		| \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/leksikon-1/output.uyid

# Sholem Asch, East river
ash-b: $(OCR)
	$(OCR) -w 6 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/asch-yi-27.data \
			-t \
			tmp/asch-east-river/*009.tif \
			tmp/asch-east-river/*0[1-9]?.tif \
			tmp/asch-east-river/*[1-4]??.tif | \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixYIVO.pl | ~raphael/links/y/fixspell.pl > \
		tmp/asch-east-river/output.uyid

ash-i: $(OCR)
	$(OCR) -w 6 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/asch-yi-27.data \
			tmp/asch-east-river/*009.tif \
			tmp/asch-east-river/*0[1-9]?.tif \
			tmp/asch-east-river/*[1-4]??.tif 

# Sholem Aleykhem Tevye der Milkhiker
# the pages are 271 down to 63.

tev-i: $(OCR)
	$(OCR) -w 7 -h 3 -g 0.60 -W 120 -H 200 -p 1.1 \
			-f fontData/sa-tevye-27.data \
			`pages.pl`
tev-b: $(OCR)
	$(OCR) -w 7 -h 3 -g 0.60 -W 120 -H 200 -p 1.1 \
			-t -f fontData/sa-tevye-27.data \
			`pages.pl` | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixYIVO.pl | \
			~raphael/links/y/fixspell.pl > tmp/sa_tevye/output.uyid

# Armenian paradigms
#

arm-i: $(OCR)
	$(OCR) -w 4 -h 4 -g 0.50 -m 0.70 \
			-f fontData/armenian.data \
			tmp/armenian-paradigms/p3[12].tif

arm-b: $(OCR)
	$(OCR) -w 4 -h 4 -g 0.50 -m 0.70 \
			-t -f fontData/armenian.data \
			tmp/armenian-paradigms/*tif | \
			armenian.transcribe.pl > \
			tmp/armenian-paradigms/output.txt

# Singer: Perl

perl-i: $(OCR)
	$(OCR) -w 4 -h 3 -g 0.50 -m 0.70 \
			-f fontData/singer-perl.data \
			tmp/perlSing-15.tiff

# Gordin: Mirele Efros

mirele-i: $(OCR)
	$(OCR) -w 4 -h 3 -g 0.60 -m 0.90 \
			-f fontData/gordon-mirele.data \
			tmp/mirele-efros/nybc207540_0021.tif

mirele-b: $(OCR)
	$(OCR) -w 4 -h 3 -g 0.60 -m 0.90 -t \
			-f fontData/gordon-mirele.data \
			tmp/mirele-efros/*.tif | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixYIVO.pl | \
			~raphael/links/y/fixspell.pl > tmp/mirele-efros/output.uyid

# Chawa Rosenfarb: Botshani
this: bot-i
bot-i: $(OCR)
	$(OCR) -w 5 -h 5 -g 0.85 -m 0.85 -W 200 \
			-f fontData/rosenfarb-botshaniData \
			tmp/rozenfarb_botshani/*001[3-9].tif 

bot-b: $(OCR)
	$(OCR) -w 7 -h 5 -g 0.85 -m 0.85 -W200 -t \
			-f fontData/rosenfarb-botshaniData \
			tmp/rozenfarb_botshani/*.tif  | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixYIVO.pl | \
			~raphael/links/y/fixspell.pl > tmp/rozenfarb_botshani/output.uyid

# Eliahu Fridman: zikhronos
fri-i: $(OCR)
	$(OCR) -w 5 -h 5 -g 0.85 -m 0.85 -W 200 \
			-f fontData/fridmanData \
			tmp/fridman.zikhronos/*002[3-9].tiff

fri-b: $(OCR)
	$(OCR) -w 5 -h 5 -g 0.85 -m 0.85 -W 200 -t \
			-f fontData/fridmanData \
			tmp/fridman.zikhronos/*.tiff | \
			~raphael/links/y/fixutf8.pl > tmp/fridman.zikhronos/output.uyid

# Dovid Kats: oyb nit nokh kliger
kats-i: $(OCR)
	$(OCR) -w 5 -h 3 -g 0.85 -m 0.85 -H 200 -W 200 \
			-f fontData/katsData \
			tmp/kats.kliger/pg_??.tiff

kats-b: $(OCR)
	$(OCR) -w 5 -h 3 -g 0.85 -m 0.85 -H 200 -W 200 -t \
			-f fontData/katsData \
			tmp/kats.kliger/pg_??.tiff | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixYIVO.pl | \
			~raphael/links/y/fixspell.pl > tmp/kats.kliger/output.uyid

# siddur
sid-i: $(OCR)
	$(OCR) -w 1 -h 1 -g 0.10 -m 0.85 -H 200 -W 200 \
			-f fontData/siddurData \
			tmp/siddur/pg_????.tiff

# Yehoash 
yeh-i: $(OCR)
	$(OCR) -w 1 -h 1 -g 0.01 -m 0.2 -H 139 -W 100 -i -S \
			-f fontData/yehoashData \
			tmp/yehoash-breyshis/pg_001[5-9].tiff \
			tmp/yehoash-breyshis/pg_00[2-9]?.tiff

yeh-b: $(OCR)
	$(OCR) -w 1 -h 1 -g 0.01 -m 0.2 -H 139 -W 100 -i -S \
			-f fontData/yehoashData -t \
			tmp/yehoash-breyshis/pg_*.tiff > tmp/yehoash-breyshis/output.uyid


# Lebns-fragn
leb-i: $(OCR)
	$(OCR) -c 2 -w 1 -h 1 -g 0.20 -m 0.90 -H 139 -W 100 -p 0.8 -s 0.5 -i \
			-f fontData/lebnsData \
			tmp/lebnsfragn/page*.tif

debug: $(OCR)
	MALLOC_CHECK_=2; export MALLOC_CHECK_; \
	$(OCR) -c 2 -w 6 -h 5 -g 0.40 -m 0.90 -H 139 -W 300 -p 0.8 -s 0.5 -i -t \
			-f fontData/shvelData \
			tmp/shvel/335*tif 

# Oyfn shvel
shv-i: $(OCR)
	$(OCR) -c 2 -w 6 -h 5 -g 0.50 -m 0.90 -H 139 -W 300 -p 0.8 -s 0.5 -i \
			-f fontData/shvelData \
			tmp/shvel/25*tif

# Oyfn shvel
shv-b: $(OCR)
	$(OCR) -c 2 -w 6 -h 5 -g 0.40 -m 0.90 -H 139 -W 300 -p 0.8 -s 0.5 -i -t \
			-f fontData/shvelData \
			tmp/shvel/*.tif | \
			~raphael/links/y/fixutf8.pl | uniq > tmp/shvel/output.uyid

# grins af shavues
gri-i: $(OCR)
	$(OCR) -w 10 -h 10 -g 0.40 -W 120 -H 200 -p 1.1 -i \
			-f fontData/mayses.data \
			tmp/mayses-yiddishe-kinder-2/nybc200080_0126.tif
gri-b: $(OCR)
	$(OCR) -w 10 -h 10 -g 0.40 -W 120 -H 200 -p 1.1 -i -t \
			-f fontData/mayses.data \
			tmp/mayses-yiddishe-kinder-2/nybc200080_012[6789].tif \
			tmp/mayses-yiddishe-kinder-2/nybc200080_013[0123456].tif  \
			> tmp/mayses-yiddishe-kinder-2/output.uyid
			# ~raphael/links/y/fixutf8.pl | uniq > tmp/mayses-yiddishe-kinder-2/output.uyid

gar-i: $(OCR)
	$(OCR) -w 5 -h 5 -g 0.40 -m 0.85 -H 250 -W 200 \
			-c 2 \
			-f fontData/garwolin.data \
			tmp/yizkor.garwolin/????.tiff 

gar-b: $(OCR)
	$(OCR) -w 2 -h 2 -g 0.40 -m 0.85 -H 200 -W 200 -t \
		-f fontData/garwolin.data \
		tmp/yizkor.garwolin/pg80.tiff | \
		~raphael/y/fixutf8.pl | ~raphael/y/fixYIVO.pl | \
		~raphael/y/fixspell.pl > tmp/yizkor.garwolin/output.uyid


testx: $(GOAL) $(OCR)
	$(OCR) -S -w 5 -h 1 -g 0.40 -m 0.90 -H 139 -W 100 -p 0.8 -s 0.5 -i \
			-x -f /tmp/testFontData \
			/tmp/test.tiff

montoyo: $(OCR)
	$(OCR) -w 10 -h 10 -g 0.90 -W 120 -H 200 -p 1.1 -i -t \
			-f fontData/yiddishData \
			tmp/montoyo/s?.tiff

tsar-i: $(OCR)
	$(OCR) -w 6 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/tsar-yi-27.data \
			tmp/tsar-bale-khayem/*.tiff

tsar-b: $(OCR)
	$(OCR) -w 6 -h 6 -g 0.50 -m 1.2 -W 120 -H 100 -p 1.1 -t \
			-f fontData/tsar-yi-27.data \
			tmp/tsar-bale-khayem/*.tiff | \
		~raphael/y/fixutf8.pl | ~raphael/y/fixYIVO.pl | \
		~raphael/y/fixspell.pl > tmp/tsar-bale-khayem/output.uyid

keit-i: $(OCR)
	$(OCR) -w 6 -h 8 -g 0.50 -W 120 -H 100 -p 1.1 \
			-f fontData/keitelman.data \
			tmp/keitelman/pg_0011.tiff

creole-i: ocr
	MALLOC_CHECK_=3; export MALLOC_CHECK_; \
	$(OCR) -w 6 -h 5 -g 0.50 -W 320 -H 100 -p 1.1 -c 2 \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/creole-dict.data \
			tmp/creole-dict/pg_*.tiff

creole-b: ocr
	MALLOC_CHECK_=3; export MALLOC_CHECK_; \
	$(OCR) -w 6 -h 5 -g 0.50 -W 320 -H 100 -p 1.1 -c 2 -t \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/creole-dict.data \
			tmp/creole-dict/pg_*.tiff

c-ec-b: ocr
	@ MALLOC_CHECK_=3; export MALLOC_CHECK_; \
	$(OCR) -w 6 -h 5 -g 0.50 -W 320 -H 100 -p 1.1 -c 2 -t \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/creole-dict.data \
			tmp/creole-dict-2/pg_017[89].tiff \
			tmp/creole-dict-2/pg_01[89]*.tiff \
			tmp/creole-dict-2/pg_02[01234]*.tiff \
			tmp/creole-dict-2/pg_025[0123456].tiff \

creole-fc-b: ocr
	@ MALLOC_CHECK_=3; export MALLOC_CHECK_; \
	$(OCR) -w 6 -h 5 -g 0.50 -W 320 -H 100 -c 2 -t \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/creole-dict.data \
			tmp/creole-dict-2/pg_025[7-9].tiff \
			tmp/creole-dict-2/pg_02[6-9]*.tiff \
			tmp/creole-dict-2/pg_03*.tiff | \
			tmp/creole-dict/outputToHTML.pl > tmp/creole-dict/output-fc.html

creole-2-i: ocr
	MALLOC_CHECK_=3; export MALLOC_CHECK_; \
	$(OCR) -w 6 -h 5 -g 0.50 -W 320 -H 100 -c 2 \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/creole-dict.data \
			tmp/creole-dict-2/pg_017[89].tiff \
			tmp/creole-dict-2/pg_01[89]*.tiff

geez-old: ocr
	$(OCR) -w 15 -h 5 -g 0.50 -W 250 -H 100 -c 3 \
			-i -s 0.3 -p 1.5 -m 1.0 \
			-f fontData/geez-old.data \
			-t \
			tmp/geez/104r.tiff

geez-i: ocr
	$(OCR) -w 15 -h 5 -g 0.50 -W 120 -H 100 -c 3 \
			-i -s 0.3 -p 1.5 -m 1.0 \
			-x -f fontData/geez-new.data \
			-C .50 -X \
			tmp/geez/104r.tiff

try: ocr
	$(OCR) -w 15 -h 5 -g 0.50 -W 120 -H 100 \
			-i -s 0.3 -p 1.5 -m 1.0 \
			-x -f fontData/geez-new.data \
			-C .50 \
			/tmp/small.tiff

geez-b: ocr
	$(OCR) -w 15 -h 5 -g 0.50 -W 120 -H 100 -c 3 \
			-i -s 0.3 -p 1.5 -m 1.0 \
			-x -f fontData/geez-new.data \
			-t -C .50 \
			tmp/geez/104r.tiff

BL: ocr
	$(OCR) -w 5 -h 5 -g 0.50 -W 320 -H 100 \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/geez.data \
			tmp/geez/BLOr.818.Sample.tiff

EMML: ocr
	$(OCR) -w 5 -h 5 -g 0.50 -W 320 -H 100 \
			-s 0.3 -p 0.8 -m 1.0 \
			-f fontData/geez.data \
			tmp/geez/EMML.3.Sample.3.tiff

neidus-b: ocr
	$(OCR) -w 6 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-x -i -f fontData/neydus.data \
			-t tmp/neydus/pg_018[4-7].tiff | \
		~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
		tmp/neydus/output.uyid

neidus-i: ocr
	$(OCR) -w 6 -h 6 -g 0.50 -W 120 -H 100 -p 1.1 \
			-x -i -f fontData/neydus.data \
			tmp/neydus/pg_018[4-7].tiff 
	
# sholem-aleykhem
SHOLEMPDF = -w 2 -h 2 -x -g 0.40 -W 120 -H 200 -p 3.0 -i -d 60 -A -f \
	fontData/sholem-aleykhem-ale-1952.data 
SHOLEMTIFF = -L -3.6 -w 3 -h 3 -x -g 0.40 -m 1.2 -W 180 -H 300 -p 3.0 -i -d 100 -f \
	fontData/sholem-aleykhem-ale-1952.data 

DIRS = $(wildcard tmp/nybc*_orig_tif)
OUTS = $(DIRS:=/output.uyid)

now: $(OUTS)

$(OUTS): $(OCR)
	echo $(@D)
	$(OCR) $(SHOLEMTIFF) -t $(@D)/*.tiff | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl > \
			$(@D)/output.uyid

sholem-i: ocr
	$(OCR) $(SHOLEMTIFF) tmp/nybc202595_orig_tif/nybc202595_orig_012?.tiff
		

test1: ocr
	$(OCR) $(SHOLEMTIFF) -t \
			tmp/nybc202588_orig_tif/nybc202588_orig_0151.tiff | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl | fixSholem.pl 

.PHONY: sholem-all sholem-b
sholem-all: 
sholem-b: ocr
	$(OCR) $(SHOLEMTIFF) -t \
			tmp/nybc2025??_orig_tif/*.tiff | \
			~raphael/links/y/fixutf8.pl | ~raphael/links/y/fixspell.pl | fixSholem.pl >> \
			tmp/sholem-aleykhem-ale/output.`date +%Y.%m.%d`

LADINO = -w 3 -h 3 -x -g 0.40 -W 50 -H 50 -p 1.5 -i -d 30 -A -f \
	fontData/ladino.data

ladino-i: ocr
	$(OCR) $(LADINO) tmp/ladino/*.tiff

ladino-b: ocr
	$(OCR) $(LADINO) -t tmp/ladino/*.tiff | fixSholem.pl > \
		tmp/ladino/output.txt

LAZEROV = -w 2 -h 2 -x -g 0.40 -W 120 -H 200 -p 3.0 -i -d 150 -A -f \
	fontData/lazerov.data 

lazerov-i: ocr
	$(OCR) $(LAZEROV) tmp/nybc206561_orig_tif/*.tif

lazerov-b: ocr
	$(OCR) -t $(LAZEROV) tmp/nybc206561_orig_tif/*.tif |  \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		tmp/nybc206561_orig_tif/output.uyid

OPATOSHU = -w 10 -h 10 -x -g 0.40 -m 1.2 -W 120 -H 200 -p 3.0 -i -d 150 -A -f \
	fontData/opatoshu.data

opatoshu-i: ocr
	$(OCR) $(OPATOSHU) tmp/nybc205884_tif/*.tif

opatoshu-b: ocr
	$(OCR) $(OPATOSHU) -t tmp/nybc205884_tif/*.tif | \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		tmp/nybc205884_tif/output.uyid

YUDLMARK = -w 10 -h 3 -c 2 -x -g 0.40 -m 1.2 -W 120 -H 200 -p 3.0 -i -d 15 -A -f \
	fontData/yudlmark.data

yudlmark-i: ocr
	$(OCR) $(YUDLMARK) tmp/yudlMark_Grammatik/*ab?.tif

yudlmark-b: ocr
	$(OCR) $(YUDLMARK) tmp/yudlMark_Grammatik/*.tif | \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		tmp/nybc205884_tif/output.uyid

test: ocr
	$(OCR) $(SHOLEM) -t tmp/nybc202588/pg_0026.tiff

cedar: ocr
	$(OCR) -f fontData/cedarbaum.data -Xx -d 1 -w 2 -h 2 tmp/cedarbaum/*.tiff

# toldos anshey shem: Yehoshua Raker
TOLDOS = -x -d 40 -w 3 -h 5 -A -p 2.0 -m 1.7 -i

toldos-i: ocr
	$(OCR) -f fontData/toldos $(TOLDOS) \
		tmp/toldesansheyshem/xaa[e-z].tif 

toldos-b: ocr
	$(OCR) -f fontData/toldos $(TOLDOS) -t \
		tmp/toldesansheyshem/nybc*tif | \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		tmp/toldesansheyshem/output.uyid

TURIN = -H 600 -W 600 -d 10 -w 5 -h 5 -x -i -A
turin-i:
	$(OCR) -f fontData/turin.data $(TURIN) tmp/turin/e.tiff

RAISMAN = -H 600 -W 600 -d 500 -w 20 -h 20 -x -i -p 2.5 -s 0.4 
raismann-i:
	$(OCR) -f fontData/raismann.data $(RAISMAN) \
	tmp/raismann/raismann-nit-in-golus-grijs-kleur-*tif

raismann-b:
	$(OCR) -f fontData/raismann.data $(RAISMAN) -t \
		tmp/raismann/raismann-nit-in-golus-grijs-kleur-*tif \
		> tmp/raismann/output.yid

BIRSTEIN = -H 600 -W 600 -d 18 -w 5 -h 3 -x -i -p 2.1 -s 0.4 -c 2 -A
birstein-i:
	$(OCR) -f fontData/birstein.data $(BIRSTEIN) \
	tmp/birstein/page_ad*.tif

PAGES = $(subst tif,uyid,$(wildcard tmp/birstein/page_*.tif))

birstein-all: $(PAGES)

.tif.uyid:
	$(OCR) -f fontData/birstein.data $(BIRSTEIN) -m 1.20 -t \
		$< | \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		$*.uyid

birstein-b: 
	$(OCR) -f fontData/birstein.data $(BIRSTEIN) -m 1.20 -t \
		tmp/birstein/page_*.tif | \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		tmp/birstein/output.uyid

KIPNIS = -H 600 -W 600 -d 18 -w 3 -h 2 -x -i -p 2.1 -s 0.4 -A -m 1.1
kipnis-i:
	$(OCR) -f fontData/kipnis.data $(KIPNIS) \
	tmp/kipnis/page_aba.tiff

kipnis-b:
	$(OCR) -f fontData/kipnis.data $(KIPNIS) -t \
	tmp/kipnis/page_*.tiff | \
		~raphael/links/y/fixutf8.pl | fixSholem.pl | ~raphael/links/y/fixspell.pl > \
		tmp/kipnis/output.uyid

BASHEVIS = -H 600 -W 600 -d 30 -w 3 -h 5 -x -i -p 1.4 -s 0.4 -A -m 1.1
bashevis-i:
	$(OCR) -f fontData/bashevis.data $(BASHEVIS) \
	tmp/bashevis-sotn-in-goray/page*.tif

bashevis-b:
	$(OCR) -f fontData/bashevis.data $(BASHEVIS) -t \
	tmp/bashevis-sotn-in-goray/page*.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \tmp.out

KAFKA = -H 600 -W 600 -d 10 -w 3 -h 2 -x -i -p 1.8 -s 0.4 -A -m 1.1
kafka-i:
	$(OCR) -f fontData/kafka.data $(KAFKA) \
	testtifs/kafka/*.tif

kafka-i2:
	$(OCR) -f fontData/kafka.data $(KAFKA) -P tensorOutput.txt \
	testtifs/kafka/*.tif

quickkafka-i:
	$(OCR) -f fontData/kafka.data $(KAFKA) -I tensorTmp/tensorOutput.txt \
	testtifs/kafka/xaaa.tif

kafka-b:
	$(OCR) -f fontData/kafka.data $(KAFKA) -t \
	testtifs/kafka/*.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \tmp.out

#A full TensorFlow implementation on a multipage document, start to finish
kafka-t:
	$(OCR) -f fontData/kafka.data $(KAFKA) -T -t \
	testtifs/kafka/*.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \k.out
	python tensorPredictions.py fontData/kafka.data k.out >  \k2.out
	$(OCR) -f fontData/kafka.data $(KAFKA) -P k2.out -t \
	testtifs/kafka/*.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl

#Just the Testing Data. Contingent upon completion of training in kafka-t stored in k2.out
kafka-t-test:
	$(OCR) -f fontData/kafka.data $(KAFKA) -P k2.out -t \
	testtifs/kafka/*.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl

#A quick print of the first page of Metamorphosis in Batch Mode
quickkafka-b:
	$(OCR) -f fontData/kafka.data $(KAFKA) -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl 

#This can be used as a first step to TensorFlow with Batch Mode. It runs the training data, and can subsequently process test data
quickkafka-t:
	$(OCR) -f fontData/kafka.data $(KAFKA) -T -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \tmp.out
	python tensorPredictions.py fontData/kafka.data tmp.out

#This is the full script needed to run TensorFlow with Batch Mode output, start to finish
quickkafka-t2:
	$(OCR) -f fontData/kafka.data $(KAFKA) -T -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \tmp.out
	python tensorPredictions.py fontData/kafka.data tmp.out > \tmp2.out
	$(OCR) -f fontData/kafka.data $(KAFKA) -P tmp2.out -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl

#Testing basic functionality of tensorPredictions.py
bashevis-t:
	python t.py fontData/bashevis.data bashevis.out

#Testing the basic functionality of tensorPredictions.py
tensor-test:
	python tensorPredictions.py fontData/bashevis.data bashevis.out

#Just getting the .out file needed for Training Data for Kafka
quickkafka-test:
	$(OCR) -f fontData/kafka.data $(KAFKA) -T -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \tmp.out


#Tester helper functions
meow:
	python t5.py fontData/kafka.data kafka1.out

meow2:
	$(OCR) -f fontData/kafka.data $(KAFKA) -P tensorTmp/tensorOutput.txt -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl


#Three step process to running on any .data file and .tif file
step1:
	$(OCR) -f fontData/kafka.data $(KAFKA) -T -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl > \x.out

step2:
	python t5.py fontData/kafka.data x.out

step3:
	$(OCR) -f fontData/kafka.data $(KAFKA) -P tensorTmp/tensorOutput.txt -t \
	testtifs/kafka/xaaa.tif | \
		../fixutf8.pl | ./fixSholem.pl | ../fixspell.pl

step3-i:
	$(OCR) -f fontData/kafka.data $(KAFKA) -I tensorTmp/tensorOutput.txt \
	testtifs/kafka/xaaa.tif
