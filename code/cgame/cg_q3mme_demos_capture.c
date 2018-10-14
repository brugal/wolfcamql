#include "cg_local.h"
#include "cg_syscalls.h"
#include "cg_q3mme_demos_capture.h"

void demoSaveLine (fileHandle_t fileHandle, const char *fmt, ...)
{
	va_list args;
	char buf[1024];
	int len;

	va_start(args, fmt);
	len = Q_vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	trap_FS_Write(buf, len, fileHandle);
}
