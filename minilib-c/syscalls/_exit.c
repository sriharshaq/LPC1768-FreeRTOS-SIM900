
#include <stddef.h>
#include "errno.h"

void _exit(int exit_code)
{
	/* Nothing to do */
	for (;;);
}