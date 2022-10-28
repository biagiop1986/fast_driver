obj-m := fast_driver.o

NO_NEWLINE_PRINTK := $(shell grep -n "printk" fast_driver.c | grep -v "include" | grep -v "define" | grep -v "\\n\"" | cut -f1 -d:)

all:
ifneq ($(NO_NEWLINE_PRINTK),)
	$(error "printk directive with no newline at line $(NO_NEWLINE_PRINTK)")
endif
	make -C $(LINUX_SOURCES) M=$(PWD) modules
clean:
	make -C $(LINUX_SOURCES) M=$(PWD) clean
