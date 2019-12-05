/*
 	Copyright (c) 2010 ,
 		Cloud Wu . All rights reserved.

 		http://www.codingnow.com

 	Use, modification and distribution are subject to the "New BSD License"
 	as listed at <url: http://www.opensource.org/licenses/bsd-license.php >.

   filename: backtrace.c

   compiler: gcc 3.4.5 (mingw-win32)

   build command: gcc -O2 -shared -Wall -o backtrace.dll backtrace.c -lbfd -liberty -limagehlp

   how to use: Call LoadLibraryA("backtrace.dll"); at beginning of your program .

  */

#define PACKAGE "your-program-name"
#define PACKAGE_VERSION "0.1"

#include <windows.h>
#include <excpt.h>
#include <imagehlp.h>
#include <bfd.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_MAX (16*1024)

#define BFD_ERR_OK          (0)
#define BFD_ERR_OPEN_FAIL   (1)
#define BFD_ERR_BAD_FORMAT  (2)
#define BFD_ERR_NO_SYMBOLS  (3)
#define BFD_ERR_READ_SYMBOL (4)

static const char *const bfd_errors[] = {
	"",
	"(Failed to open bfd)",
	"(Bad format)",
	"(No symbols)",
	"(Failed to read symbols)",
};

struct bfd_ctx {
	bfd * handle;
	asymbol ** symbol;
};

struct bfd_set {
	char * name;
	struct bfd_ctx * bc;
	struct bfd_set *next;
};

struct find_info {
	asymbol **symbol;
	bfd_vma counter;
	const char *file;
	const char *func;
	unsigned line;
};

struct output_buffer {
	char * buf;
	size_t sz;
	size_t ptr;
};

static void
output_init(struct output_buffer *ob, char * buf, size_t sz)
{
	ob->buf = buf;
	ob->sz = sz;
	ob->ptr = 0;
	ob->buf[0] = '\0';
}

static void
output_print(struct output_buffer *ob, const char * format, ...)
{
	if (ob->sz == ob->ptr)
		return;
	ob->buf[ob->ptr] = '\0';
	va_list ap;
	va_start(ap,format);
	vsnprintf(ob->buf + ob->ptr , ob->sz - ob->ptr , format, ap);
	va_end(ap);

	ob->ptr = strlen(ob->buf + ob->ptr) + ob->ptr;
}

static void
lookup_section(bfd *abfd, asection *sec, void *opaque_data)
{
	struct find_info *data = opaque_data;

	if (data->func)
		return;

	if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
		return;

	bfd_vma vma = bfd_get_section_vma(abfd, sec);
	if (data->counter < vma || vma + bfd_get_section_size(sec) <= data->counter)
		return;

	bfd_find_nearest_line(abfd, sec, data->symbol, data->counter - vma, &(data->file), &(data->func), &(data->line));
}

static void
find(struct bfd_ctx * b, DWORD offset, const char **file, const char **func, unsigned *line)
{
	struct find_info data;
	data.func = NULL;
	data.symbol = b->symbol;
	data.counter = offset;
	data.file = NULL;
	data.func = NULL;
	data.line = 0;

	bfd_map_over_sections(b->handle, &lookup_section, &data);
	if (file) {
		*file = data.file;
	}
	if (func) {
		*func = data.func;
	}
	if (line) {
		*line = data.line;
	}
}

static int
init_bfd_ctx(struct bfd_ctx *bc, const char * procname, int *err)
{
	bc->handle = NULL;
	bc->symbol = NULL;

	bfd *b = bfd_openr(procname, 0);
	if (!b) {
		if(err) { *err = BFD_ERR_OPEN_FAIL; }
		return 1;
	}

	if(!bfd_check_format(b, bfd_object)) {
		bfd_close(b);
		if(err) { *err = BFD_ERR_BAD_FORMAT; }
		return 1;
	}

	if(!(bfd_get_file_flags(b) & HAS_SYMS)) {
		bfd_close(b);
		if(err) { *err = BFD_ERR_NO_SYMBOLS; }
		return 1;
	}

	void *symbol_table;

	unsigned dummy = 0;
	if (bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy) == 0) {
		if (bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy) < 0) {
			free(symbol_table);
			bfd_close(b);
			if(err) { *err = BFD_ERR_READ_SYMBOL; }
			return 1;
		}
	}

	bc->handle = b;
	bc->symbol = symbol_table;

	if(err) { *err = BFD_ERR_OK; }
	return 0;
}

static void
close_bfd_ctx(struct bfd_ctx *bc)
{
	if (bc) {
		if (bc->symbol) {
			free(bc->symbol);
		}
		if (bc->handle) {
			bfd_close(bc->handle);
		}
	}
}

static struct bfd_ctx *
get_bc(struct bfd_set *set , const char *procname, int *err)
{
	while(set->name) {
		if (strcmp(set->name , procname) == 0) {
			return set->bc;
		}
		set = set->next;
	}
	struct bfd_ctx bc;
	if (init_bfd_ctx(&bc, procname, err)) {
		return NULL;
	}
	set->next = calloc(1, sizeof(*set));
	set->bc = malloc(sizeof(struct bfd_ctx));
	memcpy(set->bc, &bc, sizeof(bc));
	set->name = strdup(procname);

	return set->bc;
}

static void
release_set(struct bfd_set *set)
{
	while(set) {
		struct bfd_set * temp = set->next;
		free(set->name);
		close_bfd_ctx(set->bc);
		free(set);
		set = temp;
	}
}

static void
_backtrace(struct output_buffer *ob, struct bfd_set *set, int depth , LPCONTEXT context)
{
	char procname[MAX_PATH];
	GetModuleFileNameA(NULL, procname, sizeof procname);

	struct bfd_ctx *bc = NULL;
	int err = BFD_ERR_OK;

#if defined(__x86_64__)
	STACKFRAME64 frame;
#else
	STACKFRAME frame;
#endif

	memset(&frame,0,sizeof(frame));

#if defined(__x86_64__)
	frame.AddrPC.Offset = context->Rip;
#else
	frame.AddrPC.Offset = context->Eip;
#endif

	frame.AddrPC.Mode = AddrModeFlat;

#if defined(__x86_64__)
	frame.AddrStack.Offset = context->Rsp;
#else
	frame.AddrStack.Offset = context->Esp;
#endif

	frame.AddrStack.Mode = AddrModeFlat;

#if defined(__x86_64__)
	frame.AddrFrame.Offset = context->Rbp;
#else
	frame.AddrFrame.Offset = context->Ebp;
#endif

	frame.AddrFrame.Mode = AddrModeFlat;

	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	char symbol_buffer[sizeof(IMAGEHLP_SYMBOL) + 255];
	char module_name_raw[MAX_PATH];

	while(StackWalk(
#if defined(__x86_64__)
		IMAGE_FILE_MACHINE_AMD64,
#else
		IMAGE_FILE_MACHINE_I386,
#endif
		process,
		thread,
		&frame,
		context,
		0,

#if defined(__x86_64__)
		SymFunctionTableAccess64,
		SymGetModuleBase64,
#else
		SymFunctionTableAccess,
		SymGetModuleBase,
#endif
		0)) {

		--depth;
		if (depth < 0)
			break;

		IMAGEHLP_SYMBOL *symbol = (IMAGEHLP_SYMBOL *)symbol_buffer;
		symbol->SizeOfStruct = (sizeof *symbol) + 255;
		symbol->MaxNameLength = 254;

#if defined(__x86_64__)
		DWORD64 module_base = SymGetModuleBase64(process, frame.AddrPC.Offset);
#else
		DWORD module_base = SymGetModuleBase(process, frame.AddrPC.Offset);
#endif

		const char * module_name = "[unknown module]";
		if (module_base &&
			GetModuleFileNameA((HINSTANCE)module_base, module_name_raw, MAX_PATH)) {
			module_name = module_name_raw;
			bc = get_bc(set, module_name, &err);
		}

		const char * file = NULL;
		const char * func = NULL;
		unsigned line = 0;

		if (bc) {
			find(bc,frame.AddrPC.Offset,&file,&func,&line);
		}

		if (file == NULL) {
#if defined(__x86_64__)
			DWORD64 dummy = 0;
#else
			DWORD dummy = 0;
#endif

			if (
#if defined(__x86_64__)
				SymGetSymFromAddr64(process, frame.AddrPC.Offset, &dummy, symbol)
#else
				SymGetSymFromAddr(process, frame.AddrPC.Offset, &dummy, symbol)
#endif
				) {
				file = symbol->Name;
			}
			else {
				file = "[unknown file]";
			}
		}
		if (func == NULL) {
			output_print(ob,
#if defined(__x86_64__)
						 "0x%016x : %s : %s %s \n",
#else
						 "0x%08x : %s : %s %s \n",
#endif
				frame.AddrPC.Offset,
				module_name,
				file,
				bfd_errors[err]);
		}
		else {
			output_print(ob,
#if defined(__x86_64__)
						 "0x%016x : %s : %s (%d) : in function (%s) \n",
#else
						 "0x%08x : %s : %s (%d) : in function (%s) \n",
#endif
				frame.AddrPC.Offset,
				module_name,
				file,
				line,
				func);
		}
	}
}

static char * g_output = NULL;
static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

static LONG WINAPI
exception_filter(LPEXCEPTION_POINTERS info)
{
	struct output_buffer ob;
	output_init(&ob, g_output, BUFFER_MAX);

	if (!SymInitialize(GetCurrentProcess(), 0, TRUE)) {
		output_print(&ob,"Failed to init symbol context\n");
	}
	else {
		bfd_init();
		struct bfd_set *set = calloc(1,sizeof(*set));
		_backtrace(&ob , set , 128 , info->ContextRecord);
		release_set(set);

		SymCleanup(GetCurrentProcess());
	}

	fputs(g_output , stderr);

	return EXCEPTION_CONTINUE_SEARCH;
}

static void
backtrace_register(void)
{
	if (g_output == NULL) {
		g_output = malloc(BUFFER_MAX);
		g_prev = SetUnhandledExceptionFilter(exception_filter);
	}
}

static void
backtrace_unregister(void)
{
	if (g_output) {
		free(g_output);
		SetUnhandledExceptionFilter(g_prev);
		g_prev = NULL;
		g_output = NULL;
	}
}

int
__printf__(const char * format, ...) {
	int value;
	va_list arg;
	va_start(arg, format);
	value = vprintf ( format, arg );
	va_end(arg);
	return value;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		backtrace_register();
		break;
	case DLL_PROCESS_DETACH:
		backtrace_unregister();
		break;
	}
	return TRUE;
}
