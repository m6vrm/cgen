OUT			= build
TARGETS		= $(OUT)/cgen
TESTS		= $(OUT)/libcgen_test

PREFIX		= /usr/local
BINDIR		= $(PREFIX)/bin

SRC			= $(wildcard src/*/*.?pp) $(wildcard include/*/*.?pp)
SRC_TEST	= $(wildcard tests/*/*.?pp)

release: export CMAKE_BUILD_TYPE=Release
release: build

debug: export CMAKE_BUILD_TYPE=Debug
debug: build

build: $(TARGETS)

CMakeLists.txt: .cgen.yml
	-cgen -g

configure: CMakeLists.txt
	cmake \
		-S . \
		-B "$(OUT)" \
		-G "Unix Makefiles" \
		-D CMAKE_EXPORT_COMPILE_COMMANDS=ON

$(TARGETS): configure $(SRC)
	cmake \
		--build "$(OUT)" \
		--target "$(@F)" \
		--parallel

$(TESTS): configure $(SRC) $(SRC_TEST)
	cmake \
		--build "$(OUT)" \
		--target "$(@F)" \
		--parallel

clean:
	$(RM) -r "$(OUT)"

install:
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 755 "$(OUT)/cgen" "$(DESTDIR)$(BINDIR)"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/cgen"

run: $(TARGETS)
	"./$(OUT)/cgen"

test: $(TESTS)
	"./$(OUT)/libcgen_test"

format:
	clang-format -i $(SRC) $(SRC_TEST)

check:
	cppcheck \
		--cppcheck-build-dir="$(OUT)" \
		--language=c++ \
		--std=c++20 \
		--enable=all \
		--check-level=exhaustive \
		--inconclusive \
		--quiet \
		--inline-suppr \
		--suppress=unmatchedSuppression \
		--suppress=missingInclude \
		--suppress=missingIncludeSystem \
		--suppress=unusedStructMember \
		--suppress=unusedFunction \
		--suppress=useStlAlgorithm \
		$(SRC) $(SRC_TEST)

	clang-tidy \
		-p="$(OUT)" \
		--warnings-as-errors=* \
		$(SRC) $(SRC_TEST)

	codespell \
		-L poost \
		src \
		include \
		tests \
		Makefile \
		README.md \
		LICENSE \
		docs \
		.cgen.yml

asan: export CMAKE_BUILD_TYPE=Asan
asan: test

ubsan: export CMAKE_BUILD_TYPE=Ubsan
ubsan: test

.PHONY: release debug
.PHONY: configure build clean
.PHONY: install uninstall
.PHONY: run test
.PHONY: format check
.PHONY: asan ubsan
