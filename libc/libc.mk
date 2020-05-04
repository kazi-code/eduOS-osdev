DIR := libc
BDIR := $(DIR)
CSRC := $(shell find $(DIR) -type f -name '*.c')
ASRC := $(DIR)/setjmp.S
COBJ := $(patsubst %,$(BUILD)/%.o,$(CSRC))
AOBJ := $(patsubst %,$(BUILD)/%.o,$(ASRC))
OBJ := $(COBJ) $(AOBJ)
OUT := $(SYSLIB)/libc.a

LIBC := $(OUT)

$(OUT): CFLAGS := $(UCFLAGS) -nostdlib -ffreestanding
$(OUT): INCLUDE := $(UINCLUDE)
$(COBJ): $(BUILD)/$(BDIR)/%.c.o: $(DIR)/%.c
	$(MKDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<
$(AOBJ): $(BUILD)/$(BDIR)/%.S.o: $(DIR)/%.S
	$(MKDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

$(OUT): $(OBJ)
	$(MKDIR)
	$(AR) rcs $@ $^