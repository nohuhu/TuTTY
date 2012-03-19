@echo Making CHM help file
@halibut --html blurb.but intro.but gs.but using.but config.but pubkey.but errors.but faq.but feedback.but licence.but index.but chm.but
@hhc tutty.hhp
@del *.html *.hh?
