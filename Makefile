COMPILER := gcc

INCLUDE_FLAGS := -I src/
DEFINE_FLAGS  := -D _DEBUG \
							   -D LOG_STATUSES
								# -D LOG_FORCE_TRACE
LIBS          := -lm -lc

ARTIFACT_PATH := build
BINARY_PATH   := bin
LOG_PATH      := .log

PROGRAM_NAME  := $(BINARY_PATH)/rpgc

define to_object
	$(patsubst %.c, $(ARTIFACT_PATH)/%.o, $(notdir $(1)))
endef

SOURCES := $(shell find src/ -type f -name '*.c')
OBJECTS := $(call to_object,$(SOURCES))

C_FLAGS := -ggdb3 -O0 -Wall -Wextra                                       \
				   -Waggressive-loop-optimizations                                \
				   -Wmissing-declarations -Wcast-align -Wcast-qual                \
				   -Wchar-subscripts                                              \
				   -Wconversion  -Wempty-body                                     \
				   -Wfloat-equal -Wformat-nonliteral -Wformat-security            \
				   -Wformat-signedness -Wformat=2 -Winline -Wlogical-op           \
				   -Wopenmp-simd                                                  \
				   -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls         \
				   -Wshadow -Wsign-conversion                                     \
				   -Wstrict-overflow=2 -Wsuggest-attribute=noreturn               \
				   -Wsuggest-final-methods -Wsuggest-final-types                  \
				   -Wswitch-default -Wsync-nand                                   \
				   -Wundef -Wunreachable-code -Wunused -Wuseless-cast             \
				   -Wvariadic-macros                                              \
				   -Wno-missing-field-initializers -Wno-narrowing                 \
				   -Wno-varargs -Wstack-protector                                 \
				   -fcheck-new -fstack-protector                                  \
				   -fstrict-overflow                        \
				   -fno-omit-frame-pointer -Wlarger-than=64000                    \
				   -Wstack-usage=8192 -pie -fPIE -Werror=vla                      \
				   -fsanitize=address,alignment,bool,bounds,enum,$\
				   float-cast-overflow,float-divide-by-zero,$\
				   integer-divide-by-zero,leak,nonnull-attribute,$\
				   null,object-size,return,returns-nonnull-attribute,$\
				   shift,signed-integer-overflow,undefined,$\
				   unreachable,vla-bound,vptr

build: ensure_directories_exist $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJECTS)
	@echo -e "•Linking the project together"
	@$(COMPILER) $(INCLUDE_FLAGS) $(C_FLAGS) $^ -o $@ $(LIBS)

define declare_recipe
$(call to_object,$(1)): $(1)
endef

$(foreach src,$(SOURCES),$(eval $(strip $(call declare_recipe,$(src)))))

%.o:
	@echo -e "•Compiling" $<
	@$(COMPILER) -c $(DEFINE_FLAGS) $(INCLUDE_FLAGS) $(LIBS) $(C_FLAGS) $< -o $@

.PHONY: ensure_directories_exist clean run build clean_logs

run: build
	./$(PROGRAM_NAME)

ensure_directories_exist:
	mkdir -p $(BINARY_PATH) $(ARTIFACT_PATH) $(LOG_PATH)

clean:
	rm -f $(PROGRAM_NAME)
	rm -f -r $(ARTIFACT_PATH)
	mkdir -p $(ARTIFACT_PATH)

clean_logs:
	rm -f -r $(LOG_PATH)
	mkdir -p $(LOG_PATH)
