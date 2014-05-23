# Name of your emacs binary
EMACS = emacs

BATCH = $(EMACS) --batch --no-init-file					\
	--eval "(require 'org)"						\
	--eval "(org-babel-do-load-languages 'org-babel-load-languages  \
		'((shell . t)))"					\
	--eval "(setq org-confirm-babel-evaluate nil)"

files_org    = $(shell find . -name "*.org")
files_tangle = $(shell echo $(files_org) | sed 's/README.org/.README.tangle/g')

all: $(files_tangle)

.%.tangle: %.org
	@echo "NOTICE: Tangling $<..."
	@$(BATCH) --visit "$<" --funcall org-babel-tangle > /dev/null 2>&1
	@(find config -type f -print0 | xargs -0 sed -i 's#@SNEMO_SIMULATION_MODULES_DIR@#'`pwd`'#g')
	@touch $@

clean:
	@rm -rf config $(files_tangle)
