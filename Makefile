NAME := ft_ping

# Compiler and linker
CC := clang
LD := $(CC)
AR := ar

# Paths
SRCDIR := src
INCDIR := include
LIBDIR := lib

OBJDIR := obj
BINDIR := .

# Library dependencies
LIBS := $(addprefix $(LIBDIR)/, libnet_utils/libnet_utils.a libft/libft.a)

LIBDIRS := $(dir $(LIBS))
LIBINCS := $(addsuffix $(INCDIR), $(LIBDIRS))
LIBARS := $(notdir $(LIBS))

# Sources
INCS := $(LIBINCS) $(INCDIR)
SRCS := $(addprefix $(SRCDIR)/,\
	main.c\
	opt_parse.c\
	ping_params.c\
	ping_stats.c\
	ping.c\
)

OBJS := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Flags
DBGFLAGS := -g3 -fsanitize=address
CFLAGS := -Wall -Wextra -Werror
DFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d
LDFLAGS := $(LIBDIRS:%=-L%)
LDLIBS := $(LIBARS:lib%.a=-l%) -lm
ARFLAGS := -rcus

# Compiling commands
COMPILE.c = $(CC) $(DFLAGS) $(CFLAGS) $(INCS:%=-I%) -c
COMPILE.o = $(LD) $(LDFLAGS)
ARCHIVE.o = $(AR) $(ARFLAGS)

all: $(BINDIR)/$(NAME)

# Directories
$(OBJDIR) $(BINDIR):
	@echo "MK $@"
	mkdir -p "$@"

# Libraries
$(LIBS): %.a: FORCE
	$(MAKE) -C $(dir $@) NAME=$(@F)

# Objects
$(OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	$(COMPILE.c) $< -o $@

# Dependencies
$(DEPS): $(OBJDIR)/%.d:
include $(wildcard $(DEPS))

# Binaries
$(BINDIR)/$(NAME): $(OBJS) $(LIBS) | $(BINDIR)
	@echo "LD $@"
	$(COMPILE.o) $^ -o $@ $(LDLIBS)
# Use	@echo "AR $@"
#		$(ARCHIVE.o) $@ $^
# instead when building a static library
clean:
	$(foreach dir, $(LIBDIRS),\
		echo "MK $(addprefix -C , $(dir)) $@" && $(MAKE) -C $(dir) $@ ; ):

	@rm -r "$(OBJDIR)" 2>/dev/null && echo "RM $(OBJDIR)" || :

fclean: clean
	$(foreach dir, $(LIBDIRS),\
		echo "MK $(addprefix -C, $(dir)) $@" && $(MAKE) -C $(dir) $@ ; ):

	@rm "$(BINDIR)/$(NAME)" 2>/dev/null && echo "RM $(BINDIR)/$(NAME)" || :
	@rmdir "$(BINDIR)" 2>/dev/null && echo "RM $(BINDIR)" || :

re: fclean all

FORCE: ;

.PHONY: all clean fclean re

# Assign a value to VERBOSE to enable verbose output
$(VERBOSE).SILENT:
