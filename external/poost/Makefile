OUT			= build
TESTS		= $(OUT)/poost_test
SRC			= $(wildcard src/*/*.?pp) $(wildcard include/*/*.?pp)
SRC_TESTS	= $(wildcard tests/*.?pp)

release: export CMAKE_BUILD_TYPE=Release
release: build

debug: export CMAKE_BUILD_TYPE=Debug
debug: build

build: $(TESTS)

CMakeLists.txt: .cgen.yml
	-cgen -g

configure: CMakeLists.txt
	cmake \
		-S . \
		-B "$(OUT)" \
		-G "Unix Makefiles" \
		-D CMAKE_EXPORT_COMPILE_COMMANDS=ON

$(TESTS): configure $(SRC) $(SRC_TESTS)
	cmake \
		--build "$(OUT)" \
		--target "$(@F)" \
		--parallel

clean:
	$(RM) -r "$(OUT)"

test: $(TESTS)
	"./$(OUT)/poost_test"

format:
	clang-format -i $(SRC) $(SRC_TESTS)

check:
	cppcheck \
		--cppcheck-build-dir="$(OUT)" \
		--language=c++ \
		--std=c++23 \
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
		$(SRC) $(SRC_TESTS)

	clang-tidy \
		-p="$(OUT)" \
		--warnings-as-errors=* \
		$(SRC) $(SRC_TESTS)

	codespell \
		-L poost \
		src \
		include \
		tests \
		Makefile \
		README.md \
		LICENSE \
		.cgen.yml

asan: export CMAKE_BUILD_TYPE=Asan
asan: test

ubsan: export CMAKE_BUILD_TYPE=Ubsan
ubsan: test

.PHONY: release debug
.PHONY: configure build clean
.PHONY: test
.PHONY: format check
.PHONY: asan ubsan
