.DEFAULT_GOAL := help

.PHONY: help check test sanitize format format-check clean

help:
	@echo "Targets:"
	@echo "  check  Configure and compile host-side code with strict warnings"
	@echo "  test   Configure, compile, and run host-side tests"
	@echo "  sanitize  Run host tests with address/undefined sanitizers"
	@echo "  format  Apply clang-format to project C sources"
	@echo "  format-check  Verify project C source formatting"
	@echo "  clean  Remove repository build output"

check:
	@./scripts/host_checks.sh check

test:
	@./scripts/host_checks.sh test

sanitize:
	@./scripts/host_checks.sh sanitize

format:
	@./scripts/format.sh write

format-check:
	@./scripts/format.sh check

clean:
	@cmake -E remove_directory build
