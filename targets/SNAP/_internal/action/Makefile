subdirs += hw

all: $(subdirs)

# Only build if the subdirectory is existent and if Makefile is there
.PHONY: $(subdirs)
$(subdirs):
	@if [ -d $@ -a -f $@/Makefile ]; then			\
		$(MAKE) -C $@ || exit 1;			\
	else							\
		echo "INFO: No Makefile available in $@ ...";	\
	fi

# Cleanup for all subdirectories.
# Only dive into subdirectory if existent and if Makefile is there.
clean:
	@for dir in $(subdirs); do	\
		if [ -d $$dir -a -f $$dir/Makefile ]; then	\
			$(MAKE) -C $$dir $@ || exit 1;		\
		fi						\
	done
	@find . -depth -name '*~'  -exec rm -rf '{}' \; -print
	@find . -depth -name '.#*' -exec rm -rf '{}' \; -print
