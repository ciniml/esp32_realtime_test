.PHONY: all clean

PANDOC ?= pandoc
PANDOC_CROSSREF ?= pandoc-crossref

INPUT_MD := doc.md

all: output.docx output.html output.json output.txt

clean:
	-@rm output.*

output.docx: doc.md
	$(PANDOC) -f markdown -s --filter $(PANDOC_CROSSREF) $^ -o $@ --standalone

output.html: doc.md
	$(PANDOC) -f markdown -s --filter $(PANDOC_CROSSREF) $^ -o $@ --standalone

output.txt: doc.md
	$(PANDOC) -f markdown --filter $(PANDOC_CROSSREF) -t ./cqarticle.lua $^ -o $@

output.json: doc.md
	$(PANDOC) -f markdown --filter $(PANDOC_CROSSREF) -t json $^ -o $@
