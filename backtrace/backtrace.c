/*

  Based on backtrace.c by Cloud Wu and addr2line.c by Ulrich Lauther.

  Use, modification and distribution are subject to the "New BSD License"
  as listed at <url: http://www.opensource.org/licenses/bsd-license.php>.

  filename: backtrace.c

  compiler: gcc (mingw-win32)

  build command: gcc -02 -static-libgcc -shared -Wall -o backtrace.dll backtrace.c -Wl,-Bstatic -lbfd -liberty -limagehlp -lz -lintl

  how to use: Call LoadLibraryA("backtrace.dll"); at beginning of your program.

 */

#define PACKAGE "wolfcamql-backtrace"
#define PACKAGE_VERSION "1.2"

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
#if defined(__x86__64__)
	DWORD64 module_base;
#else
	DWORD module_base;
#endif
	bfd_vma preferred_base;
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
#if defined(__x86__64__)
	DWORD64 module_base;
#else
	DWORD module_base;
#endif
	bfd_vma preferred_base;

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
	if (buf) {
		ob->buf[0] = '\0';
	}
}

static void
output_print(struct output_buffer *ob, const char * format, ...)
{
	if (ob->sz == ob->ptr)
		return;

	if (ob->buf) {
		ob->buf[ob->ptr] = '\0';
	}

	va_list ap;
	va_start(ap,format);

	if (ob->buf == NULL) {
		vfprintf(stderr, format, ap);
	} else {
		vsnprintf(ob->buf + ob->ptr , ob->sz - ob->ptr , format, ap);
	}

	va_end(ap);

	if (ob->buf) {
		ob->ptr = strlen(ob->buf + ob->ptr) + ob->ptr;
	}
}

static void
lookup_section(bfd *abfd, asection *sec, void *opaque_data)
{
	struct find_info *data = opaque_data;
	bfd_vma counter;
	bfd_vma offset;

	if (data->func)
		return;

	if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
		return;

	bfd_vma vma = bfd_get_section_vma(abfd, sec);

	counter = data->counter - (data->module_base - data->preferred_base);

	if (counter < vma || vma + bfd_get_section_size(sec) <= counter)
		return;

	offset = counter - vma;
	// is '-1' offset a little more accurate?
	if (offset > 0) {
		offset--;
	}

	bfd_find_nearest_line(abfd, sec, data->symbol, offset, &(data->file), &(data->func), &(data->line));
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
	data.module_base = b->module_base;
	data.preferred_base = b->preferred_base;

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
init_bfd_ctx(struct output_buffer *ob, struct bfd_ctx *bc, const char * procname, int *err)
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

	void *symbol_table = NULL;
	unsigned dummy = 0;
	long symcount;

	// bfd_read_minisymbols() frees symbol_table if symbol count is 0 and -1 error return doesn't allocate memory
	symcount = bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy);
	if (symcount < 0) {
		bfd_close(b);
		if (err) {
			*err = BFD_ERR_READ_SYMBOL;
		}
		return 1;
	} else if (symcount == 0) {
		symcount = bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy);
		if (symcount < 0) {
			bfd_close(b);
			if (err) {
				*err = BFD_ERR_READ_SYMBOL;
			}
			return 1;
		} else if (symcount == 0) {
			// shouldn't happen since HAS_SYMS check passed
			output_print(ob, "%s couldn't read symbols even though HAS_SYMS\n", __FUNCTION__);
			bfd_close(b);
			if (err) {
				*err = BFD_ERR_READ_SYMBOL;
			}
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

static bfd_vma get_preferred_base (struct output_buffer *ob, const char *module_name, int *err)
{
	PIMAGE_NT_HEADERS ntHeaders;
	HANDLE hFile;
	HANDLE hMapping;
	LPVOID addrHeader;
	bfd_vma base;

	if (err == NULL) {
		output_print(ob, "%s err == NULL\n", __FUNCTION__);
		return 0;
	}

	hFile = CreateFile(module_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		output_print(ob, "%s CreateFile error %d\n", __FUNCTION__, GetLastError());
		*err = 1;
		return 0;
	}

	hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapping == NULL) {
		output_print(ob, "%s CreateFileMapping error %d\n", __FUNCTION__, GetLastError());
		CloseHandle(hFile);
		*err = 1;
		return 0;
	}

	addrHeader = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (addrHeader == NULL) {
		output_print(ob, "%s MapViewOfFile error %d\n", __FUNCTION__, GetLastError());
		CloseHandle(hMapping);
		CloseHandle(hFile);
		*err = 1;
		return 0;
	}

	ntHeaders = ImageNtHeader(addrHeader);
	if (ntHeaders == NULL) {
		output_print(ob, "%s ImageNtHeader error %d\n", __FUNCTION__, GetLastError);
		UnmapViewOfFile(addrHeader);
		CloseHandle(hMapping);
		CloseHandle(hFile);

		*err = 1;
		return 0;
	}

	base = ntHeaders->OptionalHeader.ImageBase;

	UnmapViewOfFile(addrHeader);
	CloseHandle(hMapping);
	CloseHandle(hFile);

	*err = 0;

	return base;
}

static struct bfd_ctx *
get_bc (struct output_buffer *ob,
		struct bfd_set *set ,
		const char *procname,
#if defined(__x86_64__)
		DWORD64 module_base,
#else
		DWORD module_base,
#endif
		int *err)
{
	int pberr = 0;

	while(set->name) {
		if (strcmp(set->name , procname) == 0) {
			return set->bc;
		}
		set = set->next;
	}

	struct bfd_ctx bc;
	if (init_bfd_ctx(ob, &bc, procname, err)) {
		return NULL;
	}

	set->name = strdup(procname);
	if (set->name == NULL) {
		output_print(ob, "%s strdup() failed, error %d\n", __FUNCTION__, GetLastError());
		close_bfd_ctx(&bc);
		return NULL;
	}

	set->next = calloc(1, sizeof(*set));
	if (set->next == NULL) {
		output_print(ob, "%s calloc() for set->next failed, error %d\n", __FUNCTION__, GetLastError());
		free(set->name);
		close_bfd_ctx(&bc);
		return NULL;
	}

	set->bc = malloc(sizeof(struct bfd_ctx));
	if (set->bc == NULL) {
		output_print(ob, "%s malloc() for set->bc failed, error %d\n", __FUNCTION__, GetLastError());
		free(set->name);
		free(set->next);
		set->next = NULL;
		close_bfd_ctx(&bc);
		return NULL;
	}

	memcpy(set->bc, &bc, sizeof(bc));

	set->bc->module_base = module_base;
	set->bc->preferred_base = get_preferred_base(ob, procname, &pberr);

	return set->bc;
}

static void
release_set(struct bfd_set *set)
{
	while(set) {
		struct bfd_set * temp = set->next;

		if (set->name) {
			free(set->name);
		}
		close_bfd_ctx(set->bc);

		free(set);
		set = temp;
	}
}

static void
_backtrace(struct output_buffer *ob, struct bfd_set *set, int depth , LPCONTEXT context)
{
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

		if (depth >= 0) {
			--depth;
			if (depth < 0)
				break;
		}

		IMAGEHLP_SYMBOL *symbol = (IMAGEHLP_SYMBOL *)symbol_buffer;
		symbol->SizeOfStruct = (sizeof *symbol) + 255;
		symbol->MaxNameLength = 254;

#if defined(__x86_64__)
		DWORD64 module_base = SymGetModuleBase64(process, frame.AddrPC.Offset);
#else
		DWORD module_base = SymGetModuleBase(process, frame.AddrPC.Offset);
#endif
		if (!module_base) {
			output_print(ob, "%s SymGetModuleBase failed for address %p error %d\n", __FUNCTION__, frame.AddrPC.Offset, GetLastError());
		}

		const char * module_name = "[unknown module]";
		DWORD file_name_size;

		if (module_base) {
			file_name_size = GetModuleFileNameA((HINSTANCE)module_base, module_name_raw, MAX_PATH);
		}

		if (module_base &&
			(file_name_size > 0  &&  file_name_size < MAX_PATH)
			) {
			module_name = module_name_raw;
			bc = get_bc(ob, set, module_name, module_base, &err);
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

#if 0  // disabled, if g_output is NULL we just print to stderr
	g_output = malloc(BUFFER_MAX);
	if (g_output == NULL) {
		fprintf(stderr, "ERROR %s failed to allocate memory for buffer\n", __FUNCTION__);
	}
#endif

	output_init(&ob, g_output, BUFFER_MAX);

	if (!SymInitialize(GetCurrentProcess(), 0, TRUE)) {
		output_print(&ob,"Failed to init symbol context\n");
	}
	else {
		bfd_init();
		struct bfd_set *set = calloc(1,sizeof(*set));
		if (set == NULL) {
			output_print(&ob, "%s calloc() for set failed\n", __FUNCTION__);
		} else {
			_backtrace(&ob , set , -1 , info->ContextRecord);
			release_set(set);
		}

		if (!SymCleanup(GetCurrentProcess())) {
			output_print(&ob, "%s SymCleanup error %d\n", __FUNCTION__, GetLastError());
		}
	}

	if (g_output) {
		fputs(g_output , stderr);
		free(g_output);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

static void
backtrace_register(void)
{
	g_prev = SetUnhandledExceptionFilter(exception_filter);
}

static void
backtrace_unregister(void)
{
	SetUnhandledExceptionFilter(g_prev);
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
