INCLUDES ?= `pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon`
CXXFLAGS += -std=c++2b -Wall -Wextra -shared -fPIC $(INCLUDES)
WARNING_FLAGS = ``
ifdef WARNINGS
	WARNING_FLAGS = -Wabi -Wabi-tag -Wabi-tag -Walloc-zero -Walloca -Wanalyzer-symbol-too-complex -Wanalyzer-too-complex -Warith-conversion -Wcast-align -Wcast-align=strict -Wcast-qual -Wcomma-subscript -Wconditionally-supported -Wconversion -Wctad-maybe-unsupported -Wctor-dtor-privacy -Wdate-time -Wdeprecated-copy-dtor -Wdeprecated-enum-enum-conversion -Wdeprecated-enum-float-conversion -Wdeprecated-literal-operator -Wdeprecated-variadic-comma-omission -Wdisabled-optimization -Wdouble-promotion -Wduplicated-branches -Wduplicated-cond -Weffc++ -Wenum-conversion -Wflex-array-member-not-at-end -Wfloat-conversion -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat-y2k -Winline -Winvalid-imported-macros -Winvalid-pch -Winvalid-utf8 -Wlogical-op -Wmismatched-tags -Wmissing-braces -Wmissing-declarations -Wmissing-include-dirs -Wmultichar -Wmultiple-inheritance -Wnamespaces -Wnoexcept -Wnon-virtual-dtor -Wnrvo -Wnull-dereference -Wold-style-cast -Wopenacc-parallelism -Woverlength-strings -Wpacked -Wpadded -Wpedantic -Wredundant-decls -Wredundant-tags -Wregister -Wshadow -Wshadow=compatible-local -Wshadow=local -Wsign-conversion -Wsign-promo -Wstack-protector -Wstrict-flex-arrays -Wstrict-null-sentinel -Wsuggest-attribute=cold -Wsuggest-attribute=const -Wsuggest-attribute=format -Wsuggest-attribute=malloc -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure -Wsuggest-attribute=returns_nonnull -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsynth -Wtemplate-id-cdtor -Wtemplates -Wtrivial-auto-var-init -Wundef -Wunused-macros -Wuseless-cast -Wvariadic-macros -Wvector-operation-performance -Wvirtual-inheritance -Wvolatile -Wzero-as-null-pointer-constant
endif

all:
	$(CXX) -O3 -flto=auto $(CXXFLAGS) $(WARNING_FLAGS) main.cpp -o hyprlut.so
	strip hyprlut.so

debug:
	$(CXX) -Og -g $(CXXFLAGS) $(WARNING_FLAGS) main.cpp -o hyprlut.so

unload load:
	hyprctl plugin $@ "$(CURDIR)/hyprlut.so"

clean:
	rm -f hyprlut.so

.PHONY: all debug load unload clean
