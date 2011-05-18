NULL =

all: html man

include $(abs_top_srcdir)/build/makefiles/sphinx-build.mk
include $(abs_top_srcdir)/build/makefiles/gettext-files.mk

document_source_files =				\
	$(source_files)				\
	$(mo_files_relative_from_locale_dir)

.PHONY: help clean
.PHONY: man generate-man
.PHONY: html dirhtml pickle json htmlhelp qthelp latex changes linkcheck doctest

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  man       to make man files"
	@echo "  html      to make standalone HTML files"
	@echo "  dirhtml   to make HTML files named index.html in directories"
	@echo "  pickle    to make pickle files"
	@echo "  json      to make JSON files"
	@echo "  htmlhelp  to make HTML files and a HTML help project"
	@echo "  qthelp    to make HTML files and a qthelp project"
	@echo "  latex     to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  rdoc      to make RDoc files"
	@echo "  textile   to make Textile files"
	@echo "  changes   to make an overview of all changed/added/deprecated items"
	@echo "  linkcheck to check all external links for integrity"
	@echo "  doctest   to run all doctests embedded in the documentation (if enabled)"

clean-doctree:
	-rm -rf $(DOCTREES_BASE)

clean-local: clean-doctree
	-rm -rf $(DOCTREES_BASE)
	-rm -rf man
	-rm -rf html
	-rm -rf dirhtml
	-rm -rf pickle
	-rm -rf json
	-rm -rf htmlhelp
	-rm -rf qthelp
	-rm -rf latex
	-rm -rf rdoc
	-rm -rf textile
	-rm -rf changes
	-rm -rf linkcheck
	-rm -rf doctest
	-rm -rf pdf

man: generate-man
generate-man: man-build-stamp
man-build-stamp: $(document_source_files)
	$(MAKE) sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/man		\
	  -b man				\
	  $(ALLSPHINXOPTS)			\
	  man
	@touch $@

html: generate-html
generate-html: html-build-stamp
html-build-stamp: $(document_source_files)
	$(MAKE) sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/html		\
	  -b html				\
	  $(ALLSPHINXOPTS)			\
	  html
	@touch $@

dirhtml: generate-dirhtml
generate-dirhtml: dirhtml-build-stamp
dirhtml-build-stamp: $(document_source_files)
	$(MAKE) sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)				\
	  -Dlanguage=$(LOCALE)				\
	  -d $(DOCTREES_BASE)/dirhtml			\
	  -b dirhtml					\
	  $(ALLSPHINXOPTS)				\
          dirhtml
	@touch $@

pickle: sphinx-ensure-updated pickle/index.fpickle

pickle/index.fpickle: $(document_source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/pickle		\
	  -b pickle				\
	  $(ALLSPHINXOPTS)			\
	  pickle

json: sphinx-ensure-updated json/index.fjson

json/index.fjson: $(document_source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/json		\
	  -b json				\
	  $(ALLSPHINXOPTS)			\
	  json

htmlhelp: sphinx-ensure-updated htmlhelp/index.html

htmlhelp/index.html: $(document_source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/htmlhelp		\
	  -b htmlhelp				\
	  $(ALLSPHINXOPTS)			\
	  htmlhelp

qthelp: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/qthelp		\
	  -b qthelp				\
	  $(ALLSPHINXOPTS)			\
	  qthelp
	@echo
	@echo "Build finished; now you can run 'qcollectiongenerator' with the" \
	      ".qhcp project file in qthelp/*/, like this:"
	@echo "# qcollectiongenerator qthelp/groonga.qhcp"
	@echo "To view the help file:"
	@echo "# assistant -collectionFile qthelp/groonga.qhc"

latex: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/latex		\
	  -b latex				\
	  $(ALLSPHINXOPTS)			\
	  latex
	@echo
	@echo "Build finished; the LaTeX files are in latex/*/."
	@echo "Run \`make all-pdf' or \`make all-ps' in that directory to" \
	      "run these through (pdf)latex."

rdoc: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/rdoc		\
	  -b rdoc				\
	  $(ALLSPHINXOPTS)			\
	  rdoc

textile: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/textile		\
	  -b textile				\
	  $(ALLSPHINXOPTS)			\
	  textile

changes: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/changes		\
	  -b changes				\
	  $(ALLSPHINXOPTS)			\
	  changes

linkcheck: sphinx-ensure-updated linkcheck/output.txt

linkcheck/output.txt: $(document_source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/linkcheck		\
	  -b linkcheck				\
	  $(ALLSPHINXOPTS)			\
	  linkcheck

doctest: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/doctest		\
	  -b doctest				\
	  $(ALLSPHINXOPTS)			\
	  doctest

pdf: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(LOCALE)			\
	  -d $(DOCTREES_BASE)/pdf		\
	  -b pdf				\
	  $(ALLSPHINXOPTS)			\
	  pdf

EXTRA_DIST =					\
	html-build-stamp			\
	man-build-stamp

if ENABLE_DOCUMENT
dist_man1_MANS =				\
	man/groonga.1

nobase_dist_doc_DATA =				\
	$(html_files)

# (cd ../../doc/locale/en; echo "html_files = \\"; find html -type f | sort | sed -e 's,^,\t,' -e 's,$, \\,'; echo "\t\$(NULL)")
html_files = \
	html/.buildinfo \
	html/_sources/characteristic.txt \
	html/_sources/command_version.txt \
	html/_sources/commands.txt \
	html/_sources/commands/cache_limit.txt \
	html/_sources/commands/check.txt \
	html/_sources/commands/clearlock.txt \
	html/_sources/commands/column_create.txt \
	html/_sources/commands/column_list.txt \
	html/_sources/commands/column_remove.txt \
	html/_sources/commands/define_selector.txt \
	html/_sources/commands/defrag.txt \
	html/_sources/commands/delete.txt \
	html/_sources/commands/dump.txt \
	html/_sources/commands/load.txt \
	html/_sources/commands/log_level.txt \
	html/_sources/commands/log_put.txt \
	html/_sources/commands/log_reopen.txt \
	html/_sources/commands/quit.txt \
	html/_sources/commands/select.txt \
	html/_sources/commands/shutdown.txt \
	html/_sources/commands/status.txt \
	html/_sources/commands/suggest.txt \
	html/_sources/commands/table_create.txt \
	html/_sources/commands/table_list.txt \
	html/_sources/commands/table_remove.txt \
	html/_sources/commands/view_add.txt \
	html/_sources/commands_not_implemented/add.txt \
	html/_sources/commands_not_implemented/get.txt \
	html/_sources/commands_not_implemented/set.txt \
	html/_sources/developer.txt \
	html/_sources/developer/com.txt \
	html/_sources/developer/document.txt \
	html/_sources/developer/query.txt \
	html/_sources/developer/test.txt \
	html/_sources/execfile.txt \
	html/_sources/expr.txt \
	html/_sources/functions.txt \
	html/_sources/functions/edit_distance.txt \
	html/_sources/functions/geo_distance.txt \
	html/_sources/functions/geo_in_circle.txt \
	html/_sources/functions/geo_in_rectangle.txt \
	html/_sources/functions/now.txt \
	html/_sources/functions/rand.txt \
	html/_sources/grnslap.txt \
	html/_sources/grntest.txt \
	html/_sources/http.txt \
	html/_sources/index.txt \
	html/_sources/install.txt \
	html/_sources/limitations.txt \
	html/_sources/news.txt \
	html/_sources/process.txt \
	html/_sources/pseudo_column.txt \
	html/_sources/reference.txt \
	html/_sources/spec.txt \
	html/_sources/spec/search.txt \
	html/_sources/troubleshooting.txt \
	html/_sources/troubleshooting/different_results_with_the_same_keyword.txt \
	html/_sources/tutorial.txt \
	html/_sources/tutorial/tutorial01.txt \
	html/_sources/tutorial/tutorial02.txt \
	html/_sources/tutorial/tutorial03.txt \
	html/_sources/tutorial/tutorial04.txt \
	html/_sources/tutorial/tutorial05.txt \
	html/_sources/tutorial/tutorial06.txt \
	html/_sources/tutorial/tutorial07.txt \
	html/_sources/tutorial/tutorial08.txt \
	html/_sources/tutorial/tutorial09.txt \
	html/_sources/tutorial/tutorial10.txt \
	html/_sources/type.txt \
	html/_static/ajax-loader.gif \
	html/_static/basic.css \
	html/_static/comment-bright.png \
	html/_static/comment-close.png \
	html/_static/comment.png \
	html/_static/default.css \
	html/_static/doctools.js \
	html/_static/down-pressed.png \
	html/_static/down.png \
	html/_static/file.png \
	html/_static/jquery.js \
	html/_static/minus.png \
	html/_static/plus.png \
	html/_static/pygments.css \
	html/_static/searchtools.js \
	html/_static/sidebar.js \
	html/_static/underscore.js \
	html/_static/up-pressed.png \
	html/_static/up.png \
	html/_static/websupport.js \
	html/characteristic.html \
	html/command_version.html \
	html/commands.html \
	html/commands/cache_limit.html \
	html/commands/check.html \
	html/commands/clearlock.html \
	html/commands/column_create.html \
	html/commands/column_list.html \
	html/commands/column_remove.html \
	html/commands/define_selector.html \
	html/commands/defrag.html \
	html/commands/delete.html \
	html/commands/dump.html \
	html/commands/load.html \
	html/commands/log_level.html \
	html/commands/log_put.html \
	html/commands/log_reopen.html \
	html/commands/quit.html \
	html/commands/select.html \
	html/commands/shutdown.html \
	html/commands/status.html \
	html/commands/suggest.html \
	html/commands/table_create.html \
	html/commands/table_list.html \
	html/commands/table_remove.html \
	html/commands/view_add.html \
	html/commands_not_implemented/add.html \
	html/commands_not_implemented/get.html \
	html/commands_not_implemented/set.html \
	html/developer.html \
	html/developer/com.html \
	html/developer/document.html \
	html/developer/query.html \
	html/developer/test.html \
	html/execfile.html \
	html/expr.html \
	html/functions.html \
	html/functions/edit_distance.html \
	html/functions/geo_distance.html \
	html/functions/geo_in_circle.html \
	html/functions/geo_in_rectangle.html \
	html/functions/now.html \
	html/functions/rand.html \
	html/genindex.html \
	html/grnslap.html \
	html/grntest.html \
	html/http.html \
	html/index.html \
	html/install.html \
	html/limitations.html \
	html/news.html \
	html/objects.inv \
	html/process.html \
	html/pseudo_column.html \
	html/reference.html \
	html/search.html \
	html/searchindex.js \
	html/spec.html \
	html/spec/search.html \
	html/troubleshooting.html \
	html/troubleshooting/different_results_with_the_same_keyword.html \
	html/tutorial.html \
	html/tutorial/tutorial01.html \
	html/tutorial/tutorial02.html \
	html/tutorial/tutorial03.html \
	html/tutorial/tutorial04.html \
	html/tutorial/tutorial05.html \
	html/tutorial/tutorial06.html \
	html/tutorial/tutorial07.html \
	html/tutorial/tutorial08.html \
	html/tutorial/tutorial09.html \
	html/tutorial/tutorial10.html \
	html/type.html \
	$(NULL)
endif
