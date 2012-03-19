@echo Making TEXT documentation
@halibut --text blurb.but intro.but gs.but using.but config.but pubkey.but errors.but faq.but feedback.but licence.but index.but
@echo Making PDF documentation
@halibut --pdf blurb.but intro.but gs.but using.but config.but pubkey.but errors.but faq.but feedback.but licence.but index.but
@echo Making HTML documentation
@halibut --html blurb.but intro.but gs.but using.but config.but pubkey.but errors.but faq.but feedback.but licence.but index.but
@echo Making WinHelp documentation
@halibut --winhelp blurb.but intro.but gs.but using.but config.but pubkey.but errors.but faq.but feedback.but licence.but index.but