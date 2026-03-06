#include <zephyr/linker/iterable_sections.h>

// Place the DAP context in a separate section to ensure it is not placed in flash memory,
// which is typically read-only. This allows the DAP context to be stored in RAM,
// where it can be modified at runtime.
ITERABLE_SECTION_RAM(dap_context, Z_LINK_ITERABLE_SUBALIGN)