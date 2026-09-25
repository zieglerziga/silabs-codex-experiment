.DEFAULT_GOAL := help

.PHONY: help check test sanitize format format-check verify workflow-check actions-audit generate-firmware prepare-sdk prepare-ci-sdk firmware docker-image docker-verify docker-firmware flash rtt clean

RTT_SECONDS ?= 10

help:
	@echo "Targets:"
	@echo "  check  Configure and compile host-side code with strict warnings"
	@echo "  test   Configure, compile, and run host-side tests"
	@echo "  sanitize  Run host tests with address/undefined sanitizers"
	@echo "  format  Apply clang-format to project C sources"
	@echo "  format-check  Verify project C source formatting"
	@echo "  verify  Run all host-side checks used before a commit"
	@echo "  workflow-check  Run pinned actionlint and zizmor containers"
	@echo "  actions-audit  Compare action pins with latest stable releases"
	@echo "  generate-firmware  Regenerate the Silicon Labs CMake project"
	@echo "  prepare-sdk  Materialize selected SDK Git LFS archives"
	@echo "  prepare-ci-sdk  Fetch the exact public SDK revision used by CI"
	@echo "  firmware  Build the EFR32MG21 firmware with the external SDK"
	@echo "  docker-image  Build the pinned open-source build image"
	@echo "  docker-verify  Run host verification in the build image"
	@echo "  docker-firmware  Build firmware with the SDK mounted read-only"
	@echo "  flash  Program the connected BRD4181A and reset it"
	@echo "  rtt  Capture RTT output (RTT_SECONDS=10 by default)"
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

verify:
	@./scripts/verify.sh

workflow-check:
	@./scripts/validate_workflows.sh

actions-audit:
	@./scripts/audit_github_actions.py

generate-firmware:
	@./scripts/generate_firmware.sh

prepare-sdk:
	@test -n "$(SISDK_ROOT)" || (echo "SISDK_ROOT is required" >&2; exit 1)
	@./scripts/prepare_sdk.py fetch "$(SISDK_ROOT)" \
	  firmware/ble_scanner/ble_scanner_cmake/ble_scanner.cmake

prepare-ci-sdk:
	@test -n "$(CI_SISDK_ROOT)" || (echo "CI_SISDK_ROOT is required" >&2; exit 1)
	@./scripts/prepare_ci_sdk.sh "$(CI_SISDK_ROOT)"

firmware:
	@./scripts/build_firmware.sh

docker-image:
	@./scripts/docker.sh image

docker-verify:
	@./scripts/docker.sh verify

docker-firmware:
	@./scripts/docker.sh firmware

flash:
	@./scripts/flash_firmware.sh

rtt:
	@./scripts/capture_rtt.sh "$(RTT_SECONDS)"

clean:
	@cmake -E remove_directory build
	@cmake -E remove_directory firmware/ble_scanner/ble_scanner_cmake/build
