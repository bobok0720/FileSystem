SUBDIRS := disk fs

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

test:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d test || exit 1; done

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean || exit 1; done

run:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d run || exit 1; done

.PHONY: all test clean run $(SUBDIRS)
