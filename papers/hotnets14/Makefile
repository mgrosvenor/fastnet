NAME=paper
BIB_NAME=local

TARGET=$(NAME).pdf

outlines_sty_URL='http://www.cl.cam.ac.uk/~ms705/tmp/outlines.sty'
usenix_sty_URL='http://www.usenix.org/events/samples/usenix.sty'
#sigplanconf_cls_URL='http://drupal.sigplan.org/sites/default/files/sigplanconf.cls'
#sig-alternate-10pt_cls_URL='http://conferences.sigcomm.org/sigcomm/2012/sig-alternate-10pt.cls'

LATEX=pdflatex
PUB_HTML=slogin:/home/ms705/public_html/pub/drafts


.SUFFIXES:
.SUFFIXES: .stamp .tex .pdf
.PHONY:

top: all

recompile: $(TARGET)

all: trigger $(TARGET)

clean:
	rm -f $(NAME).aux $(NAME).bbl $(NAME).blg $(NAME).log $(NAME).pdf $(NAME).tmp $(NAME).trig $(NAME).dvi $(NAME).ps

trigger $(NAME).trig:
	touch $(NAME).trig

# This fetches the shared tex from Google docs
$(NAME).tex: $(NAME).trig outlines.sty

# fetch the required latex packages
%.sty:
	wget -O$@ $($(subst .,_,$@)_URL)
%.cls:
	wget -O$@ $($(subst .,_,$@)_URL)

# bibtex approximation (may cause an extra latex run)
$(NAME).bbl: $(BIB_NAME).bib
	$(LATEX) $(NAME)
	bibtex $(NAME)

# This forces and extra latex and bibtex every time
$(NAME).bbl: $(NAME).tex

# Iterate on latex until cross references don't change
%.pdf: %.tex %.bbl
	while ($(LATEX) $(NAME); egrep -q "(Rerun to get cross)" $(NAME).log) do echo "### rerun"; done
#	dvips -t letter $(NAME).dvi
#	ps2pdf $(NAME).ps
	@echo "bibtex warnings:"
	fgrep -i warning $(NAME).blg
	@echo "latex warnings:"
	fgrep -i warning $(NAME).log

# Useful but non-standard hooks
post: $(NAME).pdf
	scp $(NAME).pdf $(PUB_HTML)/2014-sigcomm-draft.pdf

spell: $(NAME).tex
	cat $(NAME).tex | aspell list | sort -u | aspell -a
