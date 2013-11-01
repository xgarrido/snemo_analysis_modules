# Name of your emacs binary
EMACS = emacs

BATCH = $(EMACS) --batch --no-init-file					\
	--eval "(require 'org)"						\
	--eval "(org-babel-do-load-languages 'org-babel-load-languages  \
		'((sh . t)))"						\
	--eval "(setq org-confirm-babel-evaluate nil)"

files_org = $(wildcard *.org)
files_tangle = .$(files_org:.org=.tangle)

all: $(files_tangle)

.%.tangle: %.org
	@echo "NOTICE: Tangling $<..."
	@mkdir -p config/pipeline
	@$(BATCH) --visit "$<" --funcall org-babel-tangle
	@(cd config/pipeline && sed -i -e 's#@SNEMO_CONTROL_PLOT_DIR@#'`pwd`'#g' *.*)
	@touch $@

clean:
	@rm -rf config .*.tangle
