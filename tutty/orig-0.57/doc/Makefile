CHAPTERS := $(SITE) blurb intro gs using config pscp psftp plink pubkey
CHAPTERS += pageant errors faq feedback licence
CHAPTERS += index

INPUTS = $(patsubst %,%.but,$(CHAPTERS))

# This is temporary. Hack it locally or something.
HALIBUT = $(HOME)/src/halibut/build/halibut

all: man index.html

index.html: $(INPUTS)
	$(HALIBUT) --text --html --winhelp $(INPUTS)

MKMAN = $(HALIBUT) --man=$@ mancfg.but $<
MANPAGES = putty.1 puttygen.1 plink.1 pscp.1 psftp.1 puttytel.1 pterm.1
man: $(MANPAGES)

putty.1: man-putt.but mancfg.but; $(MKMAN)
puttygen.1: man-pg.but mancfg.but; $(MKMAN)
plink.1: man-pl.but mancfg.but; $(MKMAN)
pscp.1: man-pscp.but mancfg.but; $(MKMAN)
psftp.1: man-psft.but mancfg.but; $(MKMAN)
puttytel.1: man-ptel.but mancfg.but; $(MKMAN)
pterm.1: man-pter.but mancfg.but; $(MKMAN)

clean:
	rm -f *.html *.txt *.hlp *.cnt *.1
