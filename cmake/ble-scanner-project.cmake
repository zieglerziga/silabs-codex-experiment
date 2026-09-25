# Apply strict diagnostics only to hand-written firmware sources. Silicon Labs
# SDK and generated sources retain their vendor-provided warning policy.
set_property(
    SOURCE
        "../app.c"
        "../src/scan_tracker.c"
    APPEND
    PROPERTY COMPILE_OPTIONS
        -Werror
)
