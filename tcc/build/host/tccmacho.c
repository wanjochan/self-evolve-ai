/*
 * Mach-O file handling for TCC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tcc.h"

/* In order to make life easy for us we are generating Mach-O files which
   don't make use of some modern features, but which aren't entirely classic
   either in that they do use some modern features.  We're also only
   generating 64bit Mach-O files, and only native endian at that.

   In particular we're generating executables that don't make use of
   DYLD_INFO for dynamic linking info, as that requires us building a
   trie of exported names.  We're simply using classic symbol tables which
   are still supported by modern dyld.

   But we do use TCC_LC_MAIN, which is a "modern" feature in order to not have
   to setup our own crt code.  We're not using lazy linking, so even function
   calls are resolved at startup.  */

#if !defined TCC_TARGET_X86_64 && !defined TCC_TARGET_ARM64
#error Platform not supported
#endif

// Include TCC headers first
#include "tcc.h"

// Define our own minimal set of Mach-O structures with unique names
// to avoid conflicts with system headers

typedef uint32_t tcc_uint32_t;
typedef uint64_t tcc_uint64_t;
typedef int32_t  tcc_int32_t;
typedef uint8_t  tcc_uint8_t;
typedef uint16_t tcc_uint16_t;

// Mach-O header magic numbers
#define TCC_MH_MAGIC    0xfeedface  /* the mach magic number */
#define TCC_MH_CIGAM    0xcefaedfe  /* NXSwapInt(MH_MAGIC) */
#define TCC_MH_MAGIC_64 0xfeedfacf  /* the 64-bit mach magic number */
#define TCC_MH_CIGAM_64 0xcffaedfe  /* NXSwapInt(MH_MAGIC_64) */

// File types
#define TCC_MH_OBJECT   0x1     /* relocatable object file */
#define TCC_MH_EXECUTE  0x2     /* demand paged executable file */
#define TCC_MH_FVMLIB   0x3     /* fixed VM shared library file */
#define TCC_MH_CORE     0x4     /* core file */
#define TCC_MH_PRELOAD  0x5     /* preloaded executable file */
#define TCC_MH_DYLIB    0x6     /* dynamically bound shared library */
#define TCC_MH_DYLINKER 0x7     /* dynamic link editor */
#define TCC_MH_BUNDLE   0x8     /* dynamically bound bundle file */
#define TCC_MH_DYLIB_STUB   0x9     /* shared library stub for static linking only, no section contents */
#define TCC_MH_DSYM     0xa     /* companion file with only debug sections */

// Flags
#define TCC_MH_NOUNDEFS 0x1     /* the object file has no undefined references */
#define TCC_MH_DYLDLINK 0x4     /* the object file is input for the dynamic linker and can't be staticly link edited again */
#define TCC_MH_PIE      0x200000 /* When this bit is set, the OS will load the main executable at a random address */

// CPU types and subtypes
#define TCC_CPU_TYPE_X86        7
#define TCC_CPU_TYPE_ARM        12
#define TCC_CPU_ARCH_ABI64      0x01000000
#define TCC_CPU_TYPE_X86_64     (TCC_CPU_TYPE_X86 | TCC_CPU_ARCH_ABI64)
#define TCC_CPU_TYPE_ARM64     (TCC_CPU_TYPE_ARM | TCC_CPU_ARCH_ABI64)
#define TCC_CPU_SUBTYPE_I386_ALL   3
#define TCC_TCC_CPU_SUBTYPE_ARM64_ALL  0
#define TCC_CPU_SUBTYPE_LIB64      0x80000000

// Load command types
#define TCC_TCC_LC_SEGMENT_64       0x19    /* 64-bit segment of this file to be mapped */
#define TCC_TCC_LC_SYMTAB           0x2     /* link-edit stab symbol table info */
#define TCC_TCC_LC_DYSYMTAB         0xb     /* dynamic link-edit symbol table info */
#define TCC_TCC_LC_LOAD_DYLIB       0xc     /* load a dynamically linked shared library */
#define TCC_TCC_LC_ID_DYLIB         0xd     /* dynamically linked shared lib ident */
#define TCC_TCC_LC_LOAD_DYLINKER    0xe     /* load a dynamic linker */
#define TCC_LC_DYLD_INFO        0x22    /* compressed dyld information */
#define TCC_TCC_LC_DYLD_INFO_ONLY   (0x22|0x80000000) /* compressed dyld information only */
#define TCC_TCC_LC_MAIN             0x28|0x80000000   /* replacement for LC_UNIXTHREAD */
#define TCC_TCC_LC_RPATH            0x8000001c   /* runpath additions */

// Symbol n_type field bits
#define TCC_N_STAB   0xe0  /* if any of these bits set, a symbolic debugging entry */
#define TCC_N_PEXT   0x10  /* private external symbol bit */
#define TCC_N_TYPE   0x0e  /* mask for the type bits */
#define TCC_TCC_N_EXT    0x01  /* external symbol bit, set for external symbols */

// Symbol types
#define TCC_TCC_N_UNDF   0x0     /* undefined, n_sect == NO_SECT */
#define TCC_TCC_N_ABS    0x2     /* absolute, n_sect == NO_SECT */
#define TCC_TCC_N_SECT   0xe     /* defined in section number n_sect */

// Symbol descriptions
#define TCC_REFERENCED_DYNAMICALLY   0x10
#define TCC_N_DESC_DISCARDED         0x0020

// Segment flags
#define TCC_SG_PROTECTED_VERSION_1   0x8

// Section attributes
#define TCC_TCC_S_ATTR_PURE_INSTRUCTIONS 0x80000000
#define TCC_TCC_S_ATTR_SOME_INSTRUCTIONS 0x00000400
#define TCC_S_ATTR_EXT_RELOC         0x00000200
#define TCC_S_ATTR_LOC_RELOC         0x00000100

// Segment and section names
#define TCC_TCC_SEG_PAGEZERO     "__PAGEZERO"
#define TCC_TCC_SEG_TEXT         "__TEXT"
#define TCC_TCC_SEG_DATA         "__DATA"
#define TCC_SEG_OBJC         "__OBJC"
#define TCC_SEG_IMPORT      "__IMPORT"
#define TCC_TCC_SEG_LINKEDIT    "__LINKEDIT"

#define TCC_TCC_SECT_TEXT       "__text"
#define TCC_SECT_FVMLIB_INIT0 "__fvmlib_init0"
#define TCC_SECT_FVMLIB_INIT1 "__fvmlib_init1"
#define TCC_TCC_SECT_DATA       "__data"
#define TCC_TCC_SECT_BSS        "__bss"
#define TCC_SECT_COMMON     "__common"
#define TCC_SECT_OBJC_CAT_CLASS_METHODS "__cat_cls_meth"
#define TCC_SECT_OBJC_CAT_INST_METHODS "__cat_inst_meth"
#define TCC_SECT_OBJC_MESSAGE_REFS "__message_refs"
#define TCC_SECT_OBJC_CLASS "__class"
#define TCC_SECT_OBJC_META_CLASS "__meta_class"
#define TCC_SECT_OBJC_CLASS_NAMES "__cls_name"
#define TCC_SECT_OBJC_METH_VAR_NAMES "__meth_var_names"
#define TCC_SECT_OBJC_METH_VAR_TYPES "__meth_var_types"
#define TCC_SECT_OBJC_MODULES "__symbols"
#define TCC_SECT_OBJC_CLASS_REFS "__cls_refs"
#define TCC_SECT_OBJC_CLASS_VARS "__instance_vars"
#define TCC_SECT_OBJC_INSTANCE_VARS "__instance_vars"
#define TCC_SECT_OBJC_MODULE_INFO "__module_info"
#define TCC_SECT_OBJC_SYMBOLS "__symbols"
#define TCC_SECT_OBJC "__obc"

// Minimal Mach-O structures with 'tcc_' prefix to avoid conflicts
struct tcc_mach_header_64 {
    tcc_uint32_t magic;
    tcc_uint32_t cputype;
    tcc_uint32_t cpusubtype;
    tcc_uint32_t filetype;
    tcc_uint32_t ncmds;
    tcc_uint32_t sizeofcmds;
    tcc_uint32_t flags;
    tcc_uint32_t reserved;
};

struct tcc_load_command {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
};

struct tcc_lc_str {
    tcc_uint32_t offset;
};

struct tcc_segment_command_64 {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
    char segname[16];
    tcc_uint64_t vmaddr;
    tcc_uint64_t vmsize;
    tcc_uint64_t fileoff;
    tcc_uint64_t filesize;
    tcc_uint32_t maxprot;
    tcc_uint32_t initprot;
    tcc_uint32_t nsects;
    tcc_uint32_t flags;
};

struct tcc_section_64 {
    char sectname[16];
    char segname[16];
    tcc_uint64_t addr;
    tcc_uint64_t size;
    tcc_uint32_t offset;
    tcc_uint32_t align;
    tcc_uint32_t reloff;
    tcc_uint32_t nreloc;
    tcc_uint32_t flags;
    tcc_uint32_t reserved1;
    tcc_uint32_t reserved2;
    tcc_uint32_t reserved3;
};

struct tcc_dylib {
    struct tcc_lc_str name;
    tcc_uint32_t timestamp;
    tcc_uint32_t current_version;
    tcc_uint32_t compatibility_version;
};

struct tcc_dylib_command {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
    struct tcc_dylib dylib;
};

struct tcc_symtab_command {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
    tcc_uint32_t symoff;
    tcc_uint32_t nsyms;
    tcc_uint32_t stroff;
    tcc_uint32_t strsize;
};

struct tcc_dysymtab_command {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
    tcc_uint32_t ilocalsym;
    tcc_uint32_t nlocalsym;
    tcc_uint32_t iextdefsym;
    tcc_uint32_t nextdefsym;
    tcc_uint32_t iundefsym;
    tcc_uint32_t nundefsym;
    tcc_uint32_t tocoff;
    tcc_uint32_t ntoc;
    tcc_uint32_t modtaboff;
    tcc_uint32_t nmodtab;
    tcc_uint32_t extrefsymoff;
    tcc_uint32_t nextrefsyms;
    tcc_uint32_t indirectsymoff;
    tcc_uint32_t nindirectsyms;
    tcc_uint32_t extreloff;
    tcc_uint32_t nextrel;
    tcc_uint32_t locreloff;
    tcc_uint32_t nlocrel;
};

struct tcc_dylinker_command {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
    struct tcc_lc_str name;
};

struct tcc_rpath_command {
    tcc_uint32_t cmd;
    tcc_uint32_t cmdsize;
    struct tcc_lc_str path;
};

struct tcc_nlist_64 {
    union {
        tcc_uint32_t n_strx;
    } n_un;
    tcc_uint8_t n_type;
    tcc_uint8_t n_sect;
    tcc_uint16_t n_desc;
    tcc_uint64_t n_value;
};

// Include standard headers after our definitions
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

// Map our prefixed types to the original names for compatibility
#define mach_header_64 tcc_mach_header_64
#define load_command tcc_load_command
#define segment_command_64 tcc_segment_command_64
#define section_64 tcc_section_64
#define dylib_command tcc_dylib_command
#define symtab_command tcc_symtab_command
#define dysymtab_command tcc_dysymtab_command
#define dylinker_command tcc_dylinker_command
#define rpath_command tcc_rpath_command
#define lc_str tcc_lc_str
#define nlist_64 tcc_nlist_64

// Map our prefixed constants to the original names
#define MH_EXECUTE TCC_MH_EXECUTE
#define MH_DYLDLINK TCC_MH_DYLDLINK
#define MH_DYLIB TCC_MH_DYLIB
#define MH_PIE TCC_MH_PIE
#define CPU_TYPE_X86 TCC_CPU_TYPE_X86
#define CPU_TYPE_X86_64 TCC_CPU_TYPE_X86_64
#define CPU_TYPE_ARM TCC_CPU_TYPE_ARM
#define CPU_TYPE_ARM64 TCC_CPU_TYPE_ARM64
#define TCC_CPU_SUBTYPE_X86_ALL TCC_CPU_SUBTYPE_I386_ALL
#define TCC_CPU_SUBTYPE_ARM64_ALL TCC_TCC_CPU_SUBTYPE_ARM64_ALL
#define CPU_ARCH_ABI64 TCC_CPU_ARCH_ABI64
#define TCC_LC_SEGMENT_64 TCC_TCC_LC_SEGMENT_64
#define TCC_LC_SYMTAB TCC_TCC_LC_SYMTAB
#define TCC_LC_DYSYMTAB TCC_TCC_LC_DYSYMTAB
#define TCC_LC_LOAD_DYLIB TCC_TCC_LC_LOAD_DYLIB
#define TCC_LC_ID_DYLIB TCC_TCC_LC_ID_DYLIB
#define TCC_LC_LOAD_DYLINKER TCC_TCC_LC_LOAD_DYLINKER
#define TCC_LC_RPATH TCC_TCC_LC_RPATH
#define TCC_N_UNDF TCC_TCC_N_UNDF
#define TCC_N_ABS TCC_TCC_N_ABS
#define TCC_N_SECT TCC_TCC_N_SECT
#define TCC_N_EXT TCC_TCC_N_EXT

// Map segment and section names
#define TCC_SEG_PAGEZERO TCC_TCC_SEG_PAGEZERO
#define TCC_SEG_TEXT TCC_TCC_SEG_TEXT
#define TCC_SEG_DATA TCC_TCC_SEG_DATA
#define TCC_SEG_LINKEDIT TCC_TCC_SEG_LINKEDIT
#define TCC_SECT_TEXT TCC_TCC_SECT_TEXT
#define TCC_SECT_DATA TCC_TCC_SECT_DATA
#define TCC_SECT_BSS TCC_TCC_SECT_BSS

// Map section attributes
#define TCC_S_ATTR_PURE_INSTRUCTIONS TCC_TCC_S_ATTR_PURE_INSTRUCTIONS
#define TCC_S_ATTR_SOME_INSTRUCTIONS TCC_TCC_S_ATTR_SOME_INSTRUCTIONS

// Helper macros for structure access
#define DYLIB_CMD_GET(ptr) ((struct tcc_dylib_command *)(ptr))
#define DYLINKER_CMD_GET(ptr) ((struct tcc_dylinker_command *)(ptr))
#define RPATH_CMD_GET(ptr) ((struct tcc_rpath_command *)(ptr))
#define SEG_CMD_64_GET(ptr) ((struct tcc_segment_command_64 *)(ptr))
#define SECT_64_GET(ptr) ((struct tcc_section_64 *)(ptr))
#define SYMTAB_CMD_GET(ptr) ((struct tcc_symtab_command *)(ptr))
#define DYSYMTAB_CMD_GET(ptr) ((struct tcc_dysymtab_command *)(ptr))

// Helper function to get string from lc_str
static const char *get_lc_str(const void *base, uint32_t offset) {
    return (const char *)base + offset;
}

/* XXX: this file uses tcc_error() to the effect of exit(1) */
#undef _tcc_error

// Helper function to align values to a power of 2 boundary
static inline tcc_uint64_t align(tcc_uint64_t value, tcc_uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// Helper function to add padding to align the file offset
static int add_padding(FILE *f, tcc_uint64_t *offset, tcc_uint64_t alignment) {
    tcc_uint64_t new_offset = align(*offset, alignment);
    tcc_uint64_t pad = new_offset - *offset;
    if (pad > 0) {
        static const unsigned char zeros[8] = {0};
        if (fwrite(zeros, 1, pad, f) != pad) {
            return -1;
        };
        *offset = new_offset;
    }
    return 0;
}

// Helper function to write a load command
static int write_load_command(FILE *f, uint32_t cmd, uint32_t cmdsize, const void *data, size_t datalen) {
    struct {
        uint32_t cmd;
        uint32_t cmdsize;
    } lc = {cmd, cmdsize};
    
    if (fwrite(&lc, sizeof(lc), 1, f) != 1 ||
        (datalen > 0 && fwrite(data, datalen, 1, f) != 1)) {
        return -1;
    }
    return 0;
}

const struct {
    int seg_initial;
    uint32_t flags;
    const char *name;
} skinfo[sk_last] = {
    /*[sk_unknown] =*/        { 0 },
    /*[sk_discard] =*/        { 0 },
    /*[sk_text] =*/           { 1, S_REGULAR | TCC_S_ATTR_PURE_INSTRUCTIONS
                                   | TCC_S_ATTR_SOME_INSTRUCTIONS, "__text" },
    /*[sk_stubs] =*/          { 1, S_REGULAR | TCC_S_ATTR_PURE_INSTRUCTIONS | S_SYMBOL_STUBS
                                   | TCC_S_ATTR_SOME_INSTRUCTIONS , "__stubs" },
    /*[sk_stub_helper] =*/    { 1, S_REGULAR | TCC_S_ATTR_PURE_INSTRUCTIONS
                                   | TCC_S_ATTR_SOME_INSTRUCTIONS , "__stub_helper" },
    /*[sk_ro_data] =*/        { 2, S_REGULAR, "__rodata" },
    /*[sk_uw_info] =*/        { 0 },
    /*[sk_nl_ptr] =*/         { 2, S_NON_LAZY_SYMBOL_POINTERS, "__got" },
    /*[sk_debug_info] =*/     { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_info" },
    /*[sk_debug_abbrev] =*/   { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_abbrev" },
    /*[sk_debug_line] =*/     { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_line" },
    /*[sk_debug_aranges] =*/  { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_aranges" },
    /*[sk_debug_str] =*/      { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_str" },
    /*[sk_debug_line_str] =*/ { 3, S_REGULAR | S_ATTR_DEBUG, "__debug_line_str" },
    /*[sk_stab] =*/           { 4, S_REGULAR, "__stab" },
    /*[sk_stab_str] =*/       { 4, S_REGULAR, "__stab_str" },
    /*[sk_la_ptr] =*/         { 4, S_LAZY_SYMBOL_POINTERS, "__la_symbol_ptr" },
    /*[sk_init] =*/           { 4, S_MOD_INIT_FUNC_POINTERS, "__mod_init_func" },
    /*[sk_fini] =*/           { 4, S_MOD_TERM_FUNC_POINTERS, "__mod_term_func" },
    /*[sk_rw_data] =*/        { 4, S_REGULAR, "__data" },
    /*[sk_bss] =*/            { 4, TCC_S_ZEROFILL, "__bss" },
    /*[sk_linkedit] =*/       { 5, S_REGULAR, NULL }
};

#define	START	((uint64_t)1 << 32)

const struct {
    int used;
    const char *name;
    uint64_t vmaddr;
    uint64_t vmsize;
    int maxprot; // 替代 vm_prot_t
    int initprot; // 替代 vm_prot_t
    uint32_t flags;
} all_segment[] = {
    { 1, "__PAGEZERO",       0, START, 0, 0,            0 },
    { 0, "__TEXT",       START,     0, 5, 5,            0 },
    { 0, "__DATA_CONST",    -1,     0, 3, 3, 0x10 }, // SG_READ_ONLY = 0x10
    { 0, "__DWARF",         -1,     0, 7, 3,            0 },
    { 0, "__DATA",          -1,     0, 3, 3,            0 },
    { 1, "__LINKEDIT",      -1,     0, 1, 1,            0 }
};

#define	N_SEGMENT	(sizeof(all_segment)/sizeof(all_segment[0]))

#ifdef CONFIG_NEW_MACHO
/* New Mach-O chained fixups format */
static void calc_fixup_size(TCCState *s1, struct tcc_macho *mo)
{
    int i, size;

    size = (sizeof(struct tcc_dyld_chained_fixups_header) + 7) & -8;
    size += (sizeof(struct tcc_dyld_chained_starts_in_image) + (mo->nseg - 1) * sizeof(uint32_t) + 7) & -8;
    for (i = (s1->output_type == TCC_OUTPUT_EXE); i < mo->nseg - 1; i++) {
	int page_count = (get_segment(mo, i)->vmsize + 16384 - 1) / 16384; // TCC_SEG_PAGE_SIZE = 16384
	size += (sizeof(struct tcc_dyld_chained_starts_in_segment) + (page_count - 1) * sizeof(uint16_t) + 7) & -8;
    }
    size += mo->n_bind * sizeof (struct tcc_dyld_chained_import) + 1;
    for (i = 0; i < mo->n_bind_rebase; i++) {
	if (mo->bind_rebase[i].bind) {
	    int sym_index = ELFW(R_SYM)(mo->bind_rebase[i].rel.r_info);
	    ElfW(Sym) *sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	    const char *name = (char *) symtab_section->link->data + sym->st_name;
	    size += strlen(name) + 1;
	}
    }
    size = (size + 7) & -8;
    section_ptr_add(mo->chained_fixups, size);
}

#else

static void set_segment_and_offset(TCCState *s1, struct tcc_macho *mo, addr_t addr,
				   uint8_t *ptr, int opcode,
				   Section *sec, addr_t offset)
{
    int i;
    struct tcc_segment_command_64 *seg = NULL;

    for (i = (s1->output_type == TCC_OUTPUT_EXE); i < mo->nseg - 1; i++) {
	seg = get_segment(mo, i);
	if (addr >= seg->vmaddr && addr < (seg->vmaddr + seg->vmsize))
	    break;
    };
    *ptr = opcode | i;
    write_uleb128(sec, offset - seg->vmaddr);
};

static void bind_rebase(TCCState *s1, struct tcc_macho *mo)
{
    int i;
    uint8_t *ptr;
    ElfW(Sym) *sym;
    const char *name;

    for (i = 0; i < mo->n_lazy_bind; i++) {
	int sym_index = ELFW(R_SYM)(mo->s_lazy_bind[i].rel.r_info);

	sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	name = (char *) symtab_section->link->data + sym->st_name;
	write32le(mo->stub_helper->data +
		  mo->s_lazy_bind[i].bind_offset,
		  mo->lazy_binding->data_offset);
	ptr = section_ptr_add(mo->lazy_binding, 1);
	set_segment_and_offset(s1, mo, mo->la_symbol_ptr->sh_addr, ptr,
			       BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB,
			       mo->lazy_binding,
			       mo->s_lazy_bind[i].la_symbol_offset +
			       mo->la_symbol_ptr->sh_addr);
	ptr = section_ptr_add(mo->lazy_binding, 5 + strlen(name));
	*ptr++ = BIND_OPCODE_SET_DYLIB_SPECIAL_IMM |
		 (BIND_SPECIAL_DYLIB_FLAT_LOOKUP & 0xf);
	*ptr++ = BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM | 0;
	strcpy((char *)ptr, name);
	ptr += strlen(name) + 1;
	*ptr++ = BIND_OPCODE_DO_BIND;
	*ptr = BIND_OPCODE_DONE;
    };
    for (i = 0; i < mo->n_rebase; i++) {
	Section *s = s1->sections[mo->s_rebase[i].section];

	ptr = section_ptr_add(mo->rebase, 2);
	*ptr++ = REBASE_OPCODE_SET_TYPE_IMM | REBASE_TYPE_POINTER;
	set_segment_and_offset(s1, mo, s->sh_addr, ptr,
			       REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB,
			       mo->rebase,
			       mo->s_rebase[i].rel.r_offset +
			       s->sh_addr);
	ptr = section_ptr_add(mo->rebase, 1);
	*ptr = REBASE_OPCODE_DO_REBASE_IMM_TIMES | 1;
    };
    for (i = 0; i < mo->n_bind; i++) {
	int sym_index = ELFW(R_SYM)(mo->bind[i].rel.r_info);
	Section *s = s1->sections[mo->bind[i].section];
	Section *binding;

	sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	name = (char *) symtab_section->link->data + sym->st_name;
	binding = ELFW(ST_BIND)(sym->st_info) == STB_WEAK
	    ? mo->weak_binding : mo->binding;
        ptr = section_ptr_add(binding, 4 + (binding == mo->binding) +
			     	       strlen(name));
	if (binding == mo->binding)
            *ptr++ = BIND_OPCODE_SET_DYLIB_SPECIAL_IMM |
	             (BIND_SPECIAL_DYLIB_FLAT_LOOKUP & 0xf);
        *ptr++ = BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM |
		 (binding == mo->weak_binding
		  ? BIND_SYMBOL_FLAGS_WEAK_IMPORT : 0);
        strcpy((char *)ptr, name);
        ptr += strlen(name) + 1;
        *ptr++ = BIND_OPCODE_SET_TYPE_IMM | BIND_TYPE_POINTER;
	set_segment_and_offset(s1, mo, s->sh_addr, ptr,
			       BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB,
			       binding,
			       mo->bind[i].rel.r_offset + s->sh_addr);
        ptr = section_ptr_add(binding, 1);
        *ptr++ = BIND_OPCODE_DO_BIND;
    };
    if (mo->rebase->data_offset) {
        ptr = section_ptr_add(mo->rebase, 1);
        *ptr = REBASE_OPCODE_DONE;
    };
    if (mo->binding->data_offset) {
        ptr = section_ptr_add(mo->binding, 1);
        *ptr = BIND_OPCODE_DONE;
    };
    if (mo->weak_binding->data_offset) {
        ptr = section_ptr_add(mo->weak_binding, 1);
        *ptr = BIND_OPCODE_DONE;
    };
    tcc_free(mo->s_lazy_bind);
    tcc_free(mo->s_rebase);
    tcc_free(mo->bind);
};
#endif

struct trie_info {
    const char *name;
    int flag;
    addr_t addr;
    int str_size;
    int term_size;
};

struct trie_node {
    int start;
    int end;
    int index_start;
    int index_end;
    int n_child;
    struct trie_node *child;
};

struct trie_seq {
    int n_child;
    struct trie_node *node;
    int offset;
    int nest_offset;
};

static void create_trie(struct trie_node *node,
                        int from, int to, int index_start,
                        int n_trie, struct trie_info *trie)
{
    int i;
    int start, end, index_end;
    char cur;
    struct trie_node *child;

    for (i = from; i < to; i = end) {
        cur = trie[i].name[index_start];
        start = i++;
        for (; i < to; i++)
            if (cur != trie[i].name[index_start])
                break;
        end = i;
        if (start == end - 1 ||
            (trie[start].name[index_start] &&
             trie[start].name[index_start + 1] == 0))
            index_end = trie[start].str_size - 1;
        else {
            index_end = index_start + 1;
            for (;;) {
                cur = trie[start].name[index_end];
                for (i = start + 1; i < end; i++)
                    if (cur != trie[i].name[index_end])
                        break;
                if (trie[start].name[index_end] &&
                    trie[start].name[index_end + 1] == 0) {
                    end = start + 1;
                    index_end = trie[start].str_size - 1;
                    break;
                }
                if (i != end)
                    break;
                index_end++;
            }
        }
        node->child = tcc_realloc(node->child,
                                  (node->n_child + 1) *
                                  sizeof(struct trie_node));
        child = &node->child[node->n_child];
        child->start = start;
        child->end = end;
        child->index_start = index_start;
        child->index_end = index_end;
        child->n_child = 0;
        child->child = NULL;
        node->n_child++;
        if (start != end - 1)
            create_trie(child, start, end, index_end, n_trie, trie);
    }
}

static int create_seq(int *offset, int *n_seq, struct trie_seq **seq,
                        struct trie_node *node,
                        int n_trie, struct trie_info *trie)
{
    int i, nest_offset, last_seq = *n_seq, retval = *offset;
    struct trie_seq *p_seq;
    struct trie_node *p_nest;

    for (i = 0; i < node->n_child; i++) {
        p_nest = &node->child[i];
        *seq = tcc_realloc(*seq, (*n_seq + 1) * sizeof(struct trie_seq));
        p_seq = &(*seq)[(*n_seq)++];
        p_seq->n_child = i == 0 ? node->n_child : -1;
        p_seq->node = p_nest;
        p_seq->offset = *offset;
        p_seq->nest_offset = 0;
        *offset += (i == 0 ? 1 + 1 : 0) +
                   p_nest->index_end - p_nest->index_start + 1 + 3;
    }
    for (i = 0; i < node->n_child; i++) {
        nest_offset =
            create_seq(offset, n_seq, seq, &node->child[i], n_trie, trie);
        p_seq = &(*seq)[last_seq + i];
        p_seq->nest_offset = nest_offset;
    }
    return retval;
}

static void node_free(struct trie_node *node)
{
    int i;

    for (i = 0; i < node->n_child; i++)
	node_free(&node->child[i]);
    tcc_free(node->child);
}

static int triecmp(const void *_a, const void *_b, void *arg)
{
    struct trie_info *a = (struct trie_info *) _a;
    struct trie_info *b = (struct trie_info *) _b;
    int len_a = strlen(a->name);
    int len_b = strlen(b->name);

    /* strange sorting needed. Name 'xx' should be after 'xx1' */
    if (!strncmp(a->name, b->name, len_a < len_b ? len_a : len_b))
	return len_a < len_b ? 1 : (len_a > len_b ? -1 : 0);
    return strcmp(a->name, b->name);
};

static void export_trie(TCCState *s1, struct tcc_macho *mo)
{
    int i, size, offset = 0, save_offset;
    uint8_t *ptr;
    int sym_index;
    int sym_end = symtab_section->data_offset / sizeof(ElfW(Sym));
    int n_trie = 0, n_seq = 0;
    struct trie_info *trie = NULL, *p_trie;
    struct trie_node node, *p_node;
    struct trie_seq *seq = NULL;
    addr_t vm_addr = get_segment(mo, s1->output_type == TCC_OUTPUT_EXE)->vmaddr;

    for (sym_index = 1; sym_index < sym_end; ++sym_index) {
	ElfW(Sym) *sym = (ElfW(Sym) *)symtab_section->data + sym_index;
	const char *name = (char*)symtab_section->link->data + sym->st_name;

	if (sym->st_shndx != SHN_UNDEF && sym->st_shndx < SHN_LORESERVE &&
            (ELFW(ST_BIND)(sym->st_info) == STB_GLOBAL ||
	     ELFW(ST_BIND)(sym->st_info) == STB_WEAK)) {
	    int flag = TCC_EXPORT_SYMBOL_FLAGS_KIND_REGULAR;
	    addr_t addr =
		sym->st_value + s1->sections[sym->st_shndx]->sh_addr - vm_addr;

	    if (ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
		flag |= TCC_EXPORT_SYMBOL_FLAGS_WEAK_DEFINITION;
	    dprintf ("%s %d %llx\n", name, flag, (long long)addr + vm_addr);
	    trie = tcc_realloc(trie, (n_trie + 1) * sizeof(struct trie_info));
	    trie[n_trie].name = name;
	    trie[n_trie].flag = flag;
	    trie[n_trie].addr = addr;
	    trie[n_trie].str_size = strlen(name) + 1;
	    trie[n_trie].term_size = uleb128_size(flag) + uleb128_size(addr);
	    n_trie++;
	};
    };
    if (n_trie) {
        tcc_qsort(trie, n_trie, sizeof(struct trie_info), triecmp, NULL);
	memset(&node, 0, sizeof(node));
        create_trie(&node, 0, n_trie, 0, n_trie, trie);
	create_seq(&offset, &n_seq, &seq, &node, n_trie, trie);
        save_offset = offset;
        for (i = 0; i < n_seq; i++) {
            p_node = seq[i].node;
            if (p_node->n_child == 0) {
                p_trie = &trie[p_node->start];
                seq[i].nest_offset = offset;
                offset += 1 + p_trie->term_size + 1;
            };
        };
        for (i = 0; i < n_seq; i++) {
            p_node = seq[i].node;
            p_trie = &trie[p_node->start];
            if (seq[i].n_child >= 0) {
                section_ptr_add(mo->exports,
				seq[i].offset - mo->exports->data_offset);
                ptr = section_ptr_add(mo->exports, 2);
                *ptr++ = 0;
                *ptr = seq[i].n_child;
            };
            size = p_node->index_end - p_node->index_start;
            ptr = section_ptr_add(mo->exports, size + 1);
            memcpy(ptr, &p_trie->name[p_node->index_start], size);
            ptr[size] = 0;
            write_uleb128(mo->exports, seq[i].nest_offset);
        };
        section_ptr_add(mo->exports, save_offset - mo->exports->data_offset);
        for (i = 0; i < n_seq; i++) {
            p_node = seq[i].node;
            if (p_node->n_child == 0) {
                p_trie = &trie[p_node->start];
                write_uleb128(mo->exports, p_trie->term_size);
                write_uleb128(mo->exports, p_trie->flag);
                write_uleb128(mo->exports, p_trie->addr);
                ptr = section_ptr_add(mo->exports, 1);
                *ptr = 0;
            }
        }
        section_ptr_add(mo->exports, -mo->exports->data_offset & 7);
	node_free(&node);
        tcc_free(seq);
    }
    tcc_free(trie);
}

static void collect_sections(TCCState *s1, struct tcc_macho *mo, const char *filename)
{
    int i, sk, numsec;
    int used_segment[N_SEGMENT];
    uint64_t curaddr, fileofs;
    Section *s;
    struct tcc_segment_command_64 *seg;
    struct tcc_dylib_command *dylib;
#ifdef CONFIG_NEW_MACHO
    struct tcc_linkedit_data_command *chained_fixups_lc;
    struct tcc_linkedit_data_command *export_trie_lc;
#endif
    struct tcc_build_version_command *dyldbv;
    struct tcc_source_version_command *dyldsv;
    struct tcc_rpath_command *rpath;
    struct tcc_dylinker_command *dyldlc;
    struct tcc_symtab_command *symlc;
    struct tcc_dysymtab_command *dysymlc;
    char *str;

    for (i = 0; i < N_SEGMENT; i++)
	used_segment[i] = all_segment[i].used;

    memset (mo->sk_to_sect, 0, sizeof(mo->sk_to_sect));
    for (i = s1->nb_sections; i-- > 1;) {
        int type, flags;
        s = s1->sections[i];
        type = s->sh_type;
        flags = s->sh_flags;
        sk = sk_unknown;
	/* debug sections have sometimes no SHF_ALLOC */
        if ((flags & SHF_ALLOC) || !strncmp(s->name, ".debug_", 7)) {
            switch (type) {
            default:           sk = sk_unknown; break;
            case SHT_INIT_ARRAY: sk = sk_init; break;
            case SHT_FINI_ARRAY: sk = sk_fini; break;
            case SHT_NOBITS:   sk = sk_bss; break;
            case SHT_SYMTAB:   sk = sk_discard; break;
            case SHT_STRTAB:
		if (s == stabstr_section)
		  sk = sk_stab_str;
		else
		  sk = sk_discard;
		break;
            case SHT_RELX:     sk = sk_discard; break;
            case SHT_LINKEDIT: sk = sk_linkedit; break;
            case SHT_PROGBITS:
                if (s == mo->stubs)
                  sk = sk_stubs;
#ifndef CONFIG_NEW_MACHO
                else if (s == mo->stub_helper)
                  sk = sk_stub_helper;
                else if (s == mo->la_symbol_ptr)
                  sk = sk_la_ptr;
#endif
                else if (s == rodata_section)
                  sk = sk_ro_data;
                else if (s == s1->got)
                  sk = sk_nl_ptr;
                else if (s == stab_section)
                  sk = sk_stab;
                else if (s == dwarf_info_section)
                  sk = sk_debug_info;
                else if (s == dwarf_abbrev_section)
                  sk = sk_debug_abbrev;
                else if (s == dwarf_line_section)
                  sk = sk_debug_line;
                else if (s == dwarf_aranges_section)
                  sk = sk_debug_aranges;
                else if (s == dwarf_str_section)
                  sk = sk_debug_str;
                else if (s == dwarf_line_str_section)
                  sk = sk_debug_line_str;
                else if (flags & SHF_EXECINSTR)
                  sk = sk_text;
                else if (flags & SHF_WRITE)
                  sk = sk_rw_data;
                else
                  sk = sk_ro_data;
                break;
            };
        }; else
          sk = sk_discard;
        s->prev = mo->sk_to_sect[sk].s;
        mo->sk_to_sect[sk].s = s;
	used_segment[skinfo[sk].seg_initial] = 1;
    };

    if (s1->output_type != TCC_OUTPUT_EXE)
	used_segment[0] = 0;

    for (i = 0; i < N_SEGMENT; i++)
	if (used_segment[i]) {
	    seg = add_segment(mo, all_segment[i].name);
	    if (i == 1 && s1->output_type != TCC_OUTPUT_EXE)
	        seg->vmaddr = 0;
	    else
	        seg->vmaddr = all_segment[i].vmaddr;
	    seg->vmsize = all_segment[i].vmsize;
	    seg->maxprot = all_segment[i].maxprot;
	    seg->initprot = all_segment[i].initprot;
	    seg->flags = all_segment[i].flags;
            for (sk = sk_unknown; sk < sk_last; sk++)
		if (skinfo[sk].seg_initial == i)
	            mo->segment[sk] = mo->nseg - 1;
	};

    if (s1->output_type != TCC_OUTPUT_EXE) {
        const char *name = filename;  // Use filename as default
        struct tcc_dylib_command *dylib = (struct tcc_dylib_command *)add_lc(mo, TCC_LC_ID_DYLIB, (sizeof(struct tcc_dylib_command) + strlen(name) + 1 + 7) &-8);
        dylib->dylib.name.offset = sizeof(*dylib);
        dylib->dylib.timestamp = 2;  // Some default timestamp
        dylib->dylib.current_version = 0x10000;  // Default version 1.0.0
        dylib->dylib.compatibility_version = 0x10000;  // Default version 1.0.0
        str = (char*)dylib + dylib->dylib.name.offset;
        strcpy(str, name);
    };

#ifdef CONFIG_NEW_MACHO
// 删除无效语法
    struct tcc_linkedit_data_command *chained_fixups_lc = add_lc(mo, LC_TCC_DYLD_CHAINED_FIXUPS,
			       sizeof(struct tcc_linkedit_data_command));
    struct tcc_linkedit_data_command *export_trie_lc = add_lc(mo, TCC_LC_DYLD_EXPORTS_TRIE,
			    sizeof(struct tcc_linkedit_data_command));
#else
    mo->dyldinfo = add_lc(mo, TCC_LC_DYLD_INFO_ONLY, sizeof(*mo->dyldinfo));
#endif

    struct tcc_symtab_command *symlc = add_lc(mo, TCC_LC_SYMTAB, sizeof(*symlc));
    struct tcc_dysymtab_command *dysymlc = add_lc(mo, TCC_LC_DYSYMTAB, sizeof(*dysymlc));

    if (s1->output_type == TCC_OUTPUT_EXE) {
        i = (sizeof(*dyldlc) + strlen("/usr/lib/dyld") + 1 + 7) &-8;
        struct tcc_dylinker_command *dyldlc = add_lc(mo, TCC_LC_LOAD_DYLINKER, i);
dyldlc->name.offset = sizeof(*dyldlc);
        str = (char*)dyldlc + dyldlc->name.offset;
        strcpy(str, "/usr/lib/dyld");
    };

    struct tcc_build_version_command *dyldbv = add_lc(mo, TCC_LC_BUILD_VERSION, sizeof(*dyldbv));
    dyldbv->platform = TCC_PLATFORM_MACOS;
    dyldbv->minos = (10 << 16) + (6 << 8);
    dyldbv->sdk = (10 << 16) + (6 << 8);
    dyldbv->ntools = 0;

    struct tcc_source_version_command *dyldsv = add_lc(mo, TCC_LC_SOURCE_VERSION, sizeof(*dyldsv));
    dyldsv->version = 0;

    if (s1->output_type == TCC_OUTPUT_EXE) {
        mo->ep = add_lc(mo, TCC_LC_MAIN, sizeof(*mo->ep));
        mo->ep->entryoff = 4096;
    };

    for(i = 0; i < s1->nb_loaded_dlls; i++) {
        DLLReference *dllref = s1->loaded_dlls[i];
        if (dllref->level == 0)
          add_dylib(mo, dllref->name);
    };

    if (s1->rpath) {
	char *path = s1->rpath, *end;
	do {
	    end = strchr(path, ':');
	    if (!end)
		end = strchr(path, 0);
            i = (sizeof(*rpath) + (end - path) + 1 + 7) &-8;
            struct tcc_rpath_command *rpath = add_lc(mo, TCC_LC_RPATH, i);
rpath->path.offset = sizeof(*rpath);
            str = (char*)rpath + rpath->path.offset;
            memcpy(str, path, end - path);
	    str[end - path] = 0;
	    path = end + 1;
	}; while (*end);
    };

    fileofs = 4096;  /* leave space for mach-o headers */
    curaddr = get_segment(mo, s1->output_type == TCC_OUTPUT_EXE)->vmaddr;
    curaddr += 4096;
    seg = NULL;
    numsec = 0;
    mo->elfsectomacho = tcc_mallocz(sizeof(*mo->elfsectomacho) * s1->nb_sections);
    for (sk = sk_unknown; sk < sk_last; sk++) {
        struct tcc_section_64 *sec = NULL;
        if (seg) {
            seg->vmsize = curaddr - seg->vmaddr;
            seg->filesize = fileofs - seg->fileoff;
        };
#ifdef CONFIG_NEW_MACHO
	if (sk == sk_linkedit) {
	    calc_fixup_size(s1, mo);
	    export_trie(s1, mo);
	};
#else
	if (sk == sk_linkedit) {
	    bind_rebase(s1, mo);
	    export_trie(s1, mo);
	};
#endif
        if (skinfo[sk].seg_initial &&
	    (s1->output_type != TCC_OUTPUT_EXE || mo->segment[sk]) &&
	    mo->sk_to_sect[sk].s) {
            uint64_t al = 0;
            int si;
            seg = get_segment(mo, mo->segment[sk]);
            if (skinfo[sk].name) {
                si = add_section(mo, &seg, skinfo[sk].name);
                numsec++;
                mo->lc[mo->seg2lc[mo->segment[sk]]] = (struct tcc_load_command*)seg;
                mo->sk_to_sect[sk].machosect = si;
                sec = get_section(seg, si);
                sec->flags = skinfo[sk].flags;
		if (sk == sk_stubs)
#ifdef TCC_TARGET_X86_64
	    	    sec->reserved2 = 6;
#elif defined TCC_TARGET_ARM64
	    	    sec->reserved2 = 12;
#endif
		if (sk == sk_nl_ptr)
	    	    sec->reserved1 = mo->nr_plt;
#ifndef CONFIG_NEW_MACHO
		if (sk == sk_la_ptr)
	    	    sec->reserved1 = mo->nr_plt + mo->n_got;
#endif
            };
            if (seg->vmaddr == -1) {
                curaddr = (curaddr + TCC_SEG_PAGE_SIZE - 1) & -TCC_SEG_PAGE_SIZE;
                seg->vmaddr = curaddr;
                fileofs = (fileofs + TCC_SEG_PAGE_SIZE - 1) & -TCC_SEG_PAGE_SIZE;
                seg->fileoff = fileofs;
            };

            for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
                int a = exact_log2p1(s->sh_addralign);
                if (a && al < (a - 1))
                  al = a - 1;
                s->sh_size = s->data_offset;
            };
            if (sec)
              sec->align = al;
            al = 1ULL << al;
            if (al > 4096)
              tcc_warning("alignment > 4096"), sec->align = 12, al = 4096;
            curaddr = (curaddr + al - 1) & -al;
            fileofs = (fileofs + al - 1) & -al;
            if (sec) {
                sec->addr = curaddr;
                sec->offset = fileofs;
            };
            for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
                al = s->sh_addralign;
                curaddr = (curaddr + al - 1) & -al;
                dprintf("%s: curaddr now 0x%lx\n", s->name, (long)curaddr);
                s->sh_addr = curaddr;
                curaddr += s->sh_size;
                if (s->sh_type != SHT_NOBITS) {
                    fileofs = (fileofs + al - 1) & -al;
                    s->sh_offset = fileofs;
                    fileofs += s->sh_size;
                    dprintf("%s: fileofs now %ld\n", s->name, (long)fileofs);
                };
                if (sec)
                  mo->elfsectomacho[s->sh_num] = numsec;
            };
            if (sec)
              sec->size = curaddr - sec->addr;
        };
        if (DEBUG_MACHO)
          for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
              int type = s->sh_type;
              int flags = s->sh_flags;
              printf("%d section %-16s %-10s %09lx %04x %02d %s,%s,%s\n",
                     sk,
                     s->name,
                     type == SHT_PROGBITS ? "progbits" :
                     type == SHT_NOBITS ? "nobits" :
                     type == SHT_SYMTAB ? "symtab" :
                     type == SHT_STRTAB ? "strtab" :
                     type == SHT_INIT_ARRAY ? "init" :
                     type == SHT_FINI_ARRAY ? "fini" :
                     type == SHT_RELX ? "rel" : "???",
                     (long)s->sh_addr,
                     (unsigned)s->data_offset,
                     s->sh_addralign,
                     flags & SHF_ALLOC ? "alloc" : "",
                     flags & SHF_WRITE ? "write" : "",
                     flags & SHF_EXECINSTR ? "exec" : ""
                    );
          };
    };
    if (seg) {
        seg->vmsize = curaddr - seg->vmaddr;
        seg->filesize = fileofs - seg->fileoff;
    };

    /* Fill symtab info */
    symlc->symoff = mo->symtab->sh_offset;
    symlc->nsyms = mo->symtab->data_offset / sizeof(struct tcc_nlist_64);
    symlc->stroff = mo->strtab->sh_offset;
    symlc->strsize = mo->strtab->data_offset;

    dysymlc->iundefsym = mo->iundef == -1 ? symlc->nsyms : mo->iundef;
    dysymlc->iextdefsym = mo->iextdef == -1 ? dysymlc->iundefsym : mo->iextdef;
    dysymlc->ilocalsym = mo->ilocal == -1 ? dysymlc->iextdefsym : mo->ilocal;
    dysymlc->nlocalsym = dysymlc->iextdefsym - dysymlc->ilocalsym;
    dysymlc->nextdefsym = dysymlc->iundefsym - dysymlc->iextdefsym;
    dysymlc->nundefsym = symlc->nsyms - dysymlc->iundefsym;
    dysymlc->indirectsymoff = mo->indirsyms->sh_offset;
    dysymlc->nindirectsyms = mo->indirsyms->data_offset / sizeof(uint32_t);

#ifdef CONFIG_NEW_MACHO
    if (mo->chained_fixups->data_offset) {
        chained_fixups_lc->dataoff = mo->chained_fixups->sh_offset;
        chained_fixups_lc->datasize = mo->chained_fixups->data_offset;
    }
    if (mo->exports->data_offset) {
        export_trie_lc->dataoff = mo->exports->sh_offset;
        export_trie_lc->datasize = mo->exports->data_offset;
    }
#else
    if (mo->rebase->data_offset) {
        mo->dyldinfo->rebase_off = mo->rebase->sh_offset;
        mo->dyldinfo->rebase_size = mo->rebase->data_offset;
    }
    if (mo->binding->data_offset) {
        mo->dyldinfo->bind_off = mo->binding->sh_offset;
        mo->dyldinfo->bind_size = mo->binding->data_offset;
    }
    if (mo->weak_binding->data_offset) {
        mo->dyldinfo->weak_bind_off = mo->weak_binding->sh_offset;
        mo->dyldinfo->weak_bind_size = mo->weak_binding->data_offset;
    }
    if (mo->lazy_binding->data_offset) {
        mo->dyldinfo->lazy_bind_off = mo->lazy_binding->sh_offset;
        mo->dyldinfo->lazy_bind_size = mo->lazy_binding->data_offset;
    }
    if (mo->exports->data_offset) {
        mo->dyldinfo->export_off = mo->exports->sh_offset;
        mo->dyldinfo->export_size = mo->exports->data_offset;
    }
#endif
}

static void macho_write(TCCState *s1, struct tcc_macho *mo, FILE *fp)
{
    int i, sk;
    uint64_t fileofs = 0;
    Section *s;
    mo->mh.magic = TCC_MH_MAGIC_64;
#ifdef TCC_TARGET_X86_64
    mo->mh.cputype = TCC_CPU_TYPE_X86_64;
    mo->mh.cpusubtype = TCC_CPU_SUBTYPE_LIB64 | TCC_TCC_CPU_SUBTYPE_X86_ALL;
#elif defined TCC_TARGET_ARM64
    mo->mh.cputype = TCC_CPU_TYPE_ARM64;
    mo->mh.cpusubtype = TCC_TCC_CPU_SUBTYPE_ARM64_ALL;
#endif
    if (s1->output_type == TCC_OUTPUT_EXE) {
        mo->mh.filetype = TCC_MH_EXECUTE;
        mo->mh.flags = TCC_MH_DYLDLINK | TCC_MH_PIE;
    }
    else {
        mo->mh.filetype = TCC_MH_DYLIB;
        mo->mh.flags = TCC_MH_DYLDLINK;
    }
    mo->mh.ncmds = mo->nlc;
    mo->mh.sizeofcmds = 0;
    /* These fields are already set above */
    /* mo->mh.ncmds = mo->nlc;
    mo->mh.sizeofcmds = 0; */
    for (i = 0; i < mo->nlc; i++)
      mo->mh.sizeofcmds += mo->lc[i]->cmdsize;

    fwrite(&mo->mh, 1, sizeof(mo->mh), fp);
    fileofs += sizeof(mo->mh);
    for (i = 0; i < mo->nlc; i++) {
        fwrite(mo->lc[i], 1, mo->lc[i]->cmdsize, fp);
        fileofs += mo->lc[i]->cmdsize;
    }

    for (sk = sk_unknown; sk < sk_last; sk++) {
        //struct tcc_segment_command_64 *seg;
        if (skinfo[sk].seg_initial == 0 ||
	    (s1->output_type == TCC_OUTPUT_EXE && !mo->segment[sk]) ||
	    !mo->sk_to_sect[sk].s)
          continue;
        /*seg =*/ get_segment(mo, mo->segment[sk]);
        for (s = mo->sk_to_sect[sk].s; s; s = s->prev) {
            if (s->sh_type != SHT_NOBITS) {
                while (fileofs < s->sh_offset)
                  fputc(0, fp), fileofs++;
                if (s->sh_size) {
                    fwrite(s->data, 1, s->sh_size, fp);
                    fileofs += s->sh_size;
                }
            }
        }
    }
}

#ifdef CONFIG_NEW_MACHO
static int bind_rebase_cmp(const void *_a, const void *_b, void *arg)
{
    TCCState *s1 = arg;
    struct bind_rebase *a = (struct bind_rebase *) _a;
    struct bind_rebase *b = (struct bind_rebase *) _b;
    addr_t aa = s1->sections[a->section]->sh_addr + a->rel.r_offset;
    addr_t ab = s1->sections[b->section]->sh_addr + b->rel.r_offset;

    return aa > ab ? 1 : aa < ab ? -1 : 0;
}

ST_FUNC void bind_rebase_import(TCCState *s1, struct tcc_macho *mo)
{
    int i, j, k, bind_index, size, page_count, sym_index;
    const char *name;
    ElfW(Sym) *sym;
    unsigned char *data = mo->chained_fixups->data;
    struct tcc_segment_command_64 *seg;
    struct tcc_dyld_chained_fixups_header *header;
    struct tcc_dyld_chained_starts_in_image *image;
    struct tcc_dyld_chained_starts_in_segment *segment;
    struct tcc_dyld_chained_import *import;

    tcc_qsort(mo->bind_rebase, mo->n_bind_rebase, sizeof(struct bind_rebase),
	      bind_rebase_cmp, s1);
    for (i = 0; i < mo->n_bind_rebase - 1; i++)
	if (mo->bind_rebase[i].section == mo->bind_rebase[i + 1].section &&
	    mo->bind_rebase[i].rel.r_offset == mo->bind_rebase[i + 1].rel.r_offset) {
	    sym_index = ELFW(R_SYM)(mo->bind_rebase[i].rel.r_info);
            sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	    name = (char *) symtab_section->link->data + sym->st_name;
	    tcc_error("Overlap %s/%s %s:%s",
		      mo->bind_rebase[i].bind ? "bind" : "rebase",
		      mo->bind_rebase[i + 1].bind ? "bind" : "rebase",
		      s1->sections[mo->bind_rebase[i].section]->name, name);
	};
    header = (struct tcc_dyld_chained_fixups_header *) data;
    data += (sizeof(struct tcc_dyld_chained_fixups_header) + 7) & -8;
    header->starts_offset = data - mo->chained_fixups->data;
    header->imports_count = mo->n_bind;
    header->imports_format = TCC_DYLD_CHAINED_IMPORT;
    header->symbols_format = 0;
    size = sizeof(struct tcc_dyld_chained_starts_in_image) +
	   (mo->nseg - 1) * sizeof(uint32_t);
    image = (struct tcc_dyld_chained_starts_in_image *) data;
    data += (size + 7) & -8;
    image->seg_count = mo->nseg;
    for (i = (s1->output_type == TCC_OUTPUT_EXE); i < mo->nseg - 1; i++) {
        image->seg_info_offset[i] = (data - mo->chained_fixups->data) -
				    header->starts_offset;
	seg = get_segment(mo, i);
	page_count = (seg->vmsize + TCC_SEG_PAGE_SIZE - 1) / TCC_SEG_PAGE_SIZE;
	size = sizeof(struct tcc_dyld_chained_starts_in_segment) +
		      (page_count - 1) * sizeof(uint16_t);
        segment = (struct tcc_dyld_chained_starts_in_segment *) data;
        data += (size + 7) & -8;
        segment->size = size;
        segment->page_size = TCC_SEG_PAGE_SIZE;
#if 1
#define	PTR_64_OFFSET 0
#define	PTR_64_MASK   0x7FFFFFFFFFFULL
        segment->pointer_format = TCC_DYLD_CHAINED_PTR_64;
#else
#define	PTR_64_OFFSET 0x100000000ULL
#define	PTR_64_MASK   0xFFFFFFFFFFFFFFULL
        segment->pointer_format = TCC_DYLD_CHAINED_PTR_64_OFFSET;
#endif
        segment->segment_offset = seg->fileoff;
        segment->max_valid_pointer = 0;
        segment->page_count = page_count;
	// add bind/rebase
	bind_index = 0;
	k = 0;
	for (j = 0; j < page_count; j++) {
	    addr_t start = seg->vmaddr + j * TCC_SEG_PAGE_SIZE;
	    addr_t end = start + TCC_SEG_PAGE_SIZE;
	    void *last = NULL;
	    addr_t last_o = 0;
	    addr_t cur_o, cur;
	    struct tcc_dyld_chained_ptr_64_rebase *rebase;
	    struct tcc_dyld_chained_ptr_64_bind *bind;

	    segment->page_start[j] = TCC_DYLD_CHAINED_PTR_START_NONE;
	    for (; k < mo->n_bind_rebase; k++) {
	        Section *s = s1->sections[mo->bind_rebase[k].section];
		addr_t r_offset = mo->bind_rebase[k].rel.r_offset;
		addr_t addr = s->sh_addr + r_offset;

		if ((addr & 3) ||
		    (addr & (TCC_SEG_PAGE_SIZE - 1)) > TCC_SEG_PAGE_SIZE - PTR_SIZE)
		    tcc_error("Illegal rel_offset %s %lld",
			      s->name, (long long)r_offset);
		if (addr >= end)
		    break;
		if (addr >= start) {
		    cur_o = addr - start;
	            if (mo->bind_rebase[k].bind) {
		        if (segment->page_start[j] == TCC_DYLD_CHAINED_PTR_START_NONE)
			    segment->page_start[j] = cur_o;
		        else {
			    bind = (struct tcc_dyld_chained_ptr_64_bind *) last;
			    bind->next = (cur_o - last_o) / 4;
		        };
		        bind = (struct tcc_dyld_chained_ptr_64_bind *)
				    (s->data + r_offset);
		        last = bind;
		        last_o = cur_o;
		        bind->ordinal = bind_index;
		        bind->addend = 0;
		        bind->reserved = 0;
		        bind->next = 0;
		        bind->bind = 1;
		    }
		    else {
		        if (segment->page_start[j] == TCC_DYLD_CHAINED_PTR_START_NONE)
			    segment->page_start[j] = cur_o;
		        else {
			    rebase = (struct tcc_dyld_chained_ptr_64_rebase *) last;
			    rebase->next = (cur_o - last_o) / 4;
		        }
		        rebase = (struct tcc_dyld_chained_ptr_64_rebase *)
				    (s->data + r_offset);
		        last = rebase;
		        last_o = cur_o;
		        cur = (*(uint64_t *) (s->data + r_offset)) -
			      PTR_64_OFFSET;
		        rebase->target = cur & PTR_64_MASK;
		        rebase->high8 = cur >> (64 - 8);
			if (cur != ((uint64_t)rebase->high8 << (64 - 8)) + rebase->target)
			    tcc_error("rebase error");
		        rebase->reserved = 0;
		        rebase->next = 0;
		        rebase->bind = 0;
		    }
		}
		bind_index += mo->bind_rebase[k].bind;
	    }
	}
    }
    // add imports
    header->imports_offset = data - mo->chained_fixups->data;
    import = (struct tcc_dyld_chained_import *) data;
    data += mo->n_bind * sizeof (struct tcc_dyld_chained_import);
    header->symbols_offset = data - mo->chained_fixups->data;
    data++;
    for (i = 0, bind_index = 0; i < mo->n_bind_rebase; i++) {
	if (mo->bind_rebase[i].bind) {
	    import[bind_index].lib_ordinal =
		BIND_SPECIAL_DYLIB_FLAT_LOOKUP & 0xffu;
	    import[bind_index].name_offset =
		(data - mo->chained_fixups->data) - header->symbols_offset;
	    sym_index = ELFW(R_SYM)(mo->bind_rebase[i].rel.r_info);
	    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
	    import[bind_index].weak_import =
		ELFW(ST_BIND)(sym->st_info) == STB_WEAK;
	    name = (char *) symtab_section->link->data + sym->st_name;
            strcpy((char *) data, name);
	    data += strlen(name) + 1;
	    bind_index++;
	}
    }
    tcc_free(mo->bind_rebase);
}
#endif

ST_FUNC int macho_output_file(TCCState *s1, const char *filename)
{
    int fd, mode, file_type;
    FILE *fp;
    int i, ret = -1;
    struct tcc_macho mo;

    (void)memset(&mo, 0, sizeof(mo));

    file_type = s1->output_type;
    if (file_type == TCC_OUTPUT_OBJ)
        mode = 0666;
    else
        mode = 0777;
    unlink(filename);
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, mode);
    if (fd < 0 || (fp = fdopen(fd, "wb")) == NULL) {
        tcc_error_noabort("could not write '%s: %s'", filename, strerror(errno));
        return -1;
    };
    tcc_add_runtime(s1);
    tcc_macho_add_destructor(s1);
    resolve_common_syms(s1);
    create_symtab(s1, &mo);
    check_relocs(s1, &mo);
    ret = check_symbols(s1, &mo);
    if (!ret) {
	int save_output = s1->output_type;

        collect_sections(s1, &mo, filename);
        relocate_syms(s1, s1->symtab, 0);
	if (s1->output_type == TCC_OUTPUT_EXE)
            mo.ep->entryoff = get_sym_addr(s1, "main", 1, 1)
                            -     get_segment(&mo, 1)->vmaddr;
        if (s1->nb_errors)
          goto do_ret;
	// Macho uses bind/rebase instead of dynsym
	s1->output_type = TCC_OUTPUT_EXE;
        relocate_sections(s1);
	s1->output_type = save_output;
#ifdef CONFIG_NEW_MACHO
	bind_rebase_import(s1, &mo);
#endif
        convert_symbols(s1, &mo);
        if (s1->verbose)
            printf("<- %s\n", filename);
        macho_write(s1, &mo, fp);
    };

 do_ret:
    for (i = 0; i < mo.nlc; i++)
      tcc_free(mo.lc[i]);
    tcc_free(mo.seg2lc);
    tcc_free(mo.lc);
    tcc_free(mo.elfsectomacho);
    tcc_free(mo.e2msym);

    fclose(fp);
#ifdef CONFIG_CODESIGN
    if (!ret) {
	char command[1024];
	int retval;

	snprintf(command, sizeof(command), "codesign -f -s - %s", filename);
	retval = system (command);
	if (retval == -1 || !(WIFEXITED(retval) && WEXITSTATUS(retval) == 0))
	    tcc_error ("command failed '%s'", command);
    }
#endif
    return ret;
};

static uint32_t macho_swap32(uint32_t x)
{
  return (x >> 24) | (x << 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8);
}
#define SWAP(x) (swap ? macho_swap32(x) : (x))
#define tbd_parse_movepast(s) \
    (pos = (pos = strstr(pos, s)) ? pos + strlen(s) : NULL)
#define tbd_parse_movetoany(cs) (pos = strpbrk(pos, cs))
#define tbd_parse_skipws while (*pos && (*pos==' '||*pos=='\n')) ++pos
#define tbd_parse_tramplequote if(*pos=='\''||*pos=='"') tbd_parse_trample
#define tbd_parse_tramplespace if(*pos==' ') tbd_parse_trample
#define tbd_parse_trample *pos++=0

#ifdef TCC_IS_NATIVE
/* Looks for the active developer SDK set by xcode-select (or the default
   one set during installation.) */
ST_FUNC void tcc_add_macos_sdkpath(TCCState* s)
{
    char *sdkroot = NULL, *pos = NULL;
    void* xcs = dlopen("libxcselect.dylib", RTLD_GLOBAL | RTLD_LAZY);
    CString path;
    int (*f)(unsigned int, char**) = dlsym(xcs, "xcselect_host_sdk_path");
    cstr_new(&path);
    if (f) f(1, &sdkroot);
    if (sdkroot)
        pos = strstr(sdkroot,"SDKs/MacOSX");
    if (pos)
        cstr_printf(&path, "%.*s.sdk/usr/lib", (int)(pos - sdkroot + 11), sdkroot);
    /* must use free from libc directly */
#pragma push_macro("free")
#undef free
    free(sdkroot);
#pragma pop_macro("free")
    if (path.size)
        tcc_add_library_path(s, (char*)path.data);
    else
        tcc_add_library_path(s,
            "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib"
            ":" "/Applications/Xcode.app/Developer/SDKs/MacOSX.sdk/usr/lib"
            );
    cstr_free(&path);
}

ST_FUNC char* macho_tbd_soname(int fd) {
    char *soname, *data, *pos;
    char *ret = 0;
    pos = data = tcc_load_text(fd);
    if (!tbd_parse_movepast("install-name: ")) goto the_end;
    tbd_parse_skipws;
    tbd_parse_tramplequote;
    soname = pos;
    if (!tbd_parse_movetoany("\n \"'")) goto the_end;
    tbd_parse_trample;
    ret = tcc_strdup(soname);
the_end:
    tcc_free(data);
    return ret;
};
#endif /* TCC_IS_NATIVE */

ST_FUNC int macho_load_tbd(TCCState* s1, int fd, const char* filename, int lev)
{
    char *soname, *data, *pos;
    int ret = -1;

    pos = data = tcc_load_text(fd);
    if (!tbd_parse_movepast("install-name: ")) goto the_end;
    tbd_parse_skipws;
    tbd_parse_tramplequote;
    soname = pos;
    if (!tbd_parse_movetoany("\n \"'")) goto the_end;
    tbd_parse_trample;
    ret = 0;
    if (tcc_add_dllref(s1, soname, lev)->found)
        goto the_end;
    while (pos) {
        char* sym = NULL;
        int cont = 1;
        if (!tbd_parse_movepast("symbols: ")) break;
        if (!tbd_parse_movepast("[")) break;
        while (cont) {
            tbd_parse_skipws;
            tbd_parse_tramplequote;
            sym = pos;
            if (!tbd_parse_movetoany(",] \"'")) break;
            tbd_parse_tramplequote;
            tbd_parse_tramplespace;
            tbd_parse_skipws;
            if (*pos==0||*pos==']') cont=0;
            tbd_parse_trample;
            set_elf_sym(s1->dynsymtab_section, 0, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0, SHN_UNDEF, sym);
        };
    };

the_end:
    tcc_free(data);
    return ret;
};

ST_FUNC int macho_load_dll(TCCState * s1, int fd, const char* filename, int lev)
{
    unsigned char buf[sizeof(struct tcc_mach_header_64)];
    void *buf2;
    uint32_t machofs = 0;
    struct tcc_fat_header fh;
    struct tcc_mach_header mh;
    struct tcc_load_command *lc;
    int i, swap = 0;
    const char *soname = filename;
    struct tcc_nlist_64 *symtab = 0;
    uint32_t nsyms = 0;
    char *strtab = 0;
    uint32_t strsize = 0;
    uint32_t iextdef = 0;
    uint32_t nextdef = 0;

  again:
    if (full_read(fd, buf, sizeof(buf)) != sizeof(buf))
      return -1;
    memcpy(&fh, buf, sizeof(fh));
    if (fh.magic == TCC_FAT_MAGIC || fh.magic == TCC_FAT_CIGAM) {
        struct tcc_fat_arch *fa = load_data(fd, sizeof(fh),
                                        fh.nfat_arch * sizeof(*fa));
        swap = fh.magic == TCC_FAT_CIGAM;
        for (i = 0; i < SWAP(fh.nfat_arch); i++)
#ifdef TCC_TARGET_X86_64
          if (SWAP(fa[i].cputype) == TCC_CPU_TYPE_X86_64
              && SWAP(fa[i].cpusubtype) == TCC_CPU_SUBTYPE_X86_ALL)
#elif defined TCC_TARGET_ARM64
          if (SWAP(fa[i].cputype) == TCC_CPU_TYPE_ARM64
              && SWAP(fa[i].cpusubtype) == TCC_CPU_SUBTYPE_ARM64_ALL)
#endif
            break;
        if (i == SWAP(fh.nfat_arch)) {
            tcc_free(fa);
            return -1;
        };
        machofs = SWAP(fa[i].offset);
        tcc_free(fa);
        lseek(fd, machofs, SEEK_SET);
        goto again;
    }; else if (fh.magic == FAT_MAGIC_64 || fh.magic == FAT_CIGAM_64) {
        tcc_warning("%s: Mach-O fat 64bit files of type 0x%x not handled",
                    filename, fh.magic);
        return -1;
    };

    memcpy(&mh, buf, sizeof(mh));
    if (mh.magic != TCC_MH_MAGIC_64)
      return -1;
    dprintf("found Mach-O at %d\n", machofs);
    buf2 = load_data(fd, machofs + sizeof(struct tcc_mach_header_64), mh.sizeofcmds);
    for (i = 0, lc = buf2; i < mh.ncmds; i++) {
        dprintf("lc %2d: 0x%08x\n", i, lc->cmd);
        switch (lc->cmd) {
        case TCC_LC_SYMTAB:
        {
            struct tcc_symtab_command *sc = (struct tcc_symtab_command*)lc;
            nsyms = sc->nsyms;
            symtab = load_data(fd, machofs + sc->symoff, nsyms * sizeof(*symtab));
            strsize = sc->strsize;
            strtab = load_data(fd, machofs + sc->stroff, strsize);
            break;
        };
        case TCC_LC_ID_DYLIB:
        {
            struct tcc_dylib_command *dc = (struct tcc_dylib_command*)lc;
            soname = (char*)lc + dc->name;
            dprintf(" ID_DYLIB %d 0x%x 0x%x %s\n",
                    dc->timestamp, dc->current_version,
                    dc->compatibility_version, soname);
            break;
        };
        case TCC_LC_REEXPORT_DYLIB:
        {
            struct tcc_dylib_command *dc = (struct tcc_dylib_command*)lc;
            char *name = (char*)lc + dc->name;
            int subfd = open(name, O_RDONLY | O_BINARY);
            dprintf(" REEXPORT %s\n", name);
            if (subfd < 0)
              tcc_warning("can't open %s (reexported from %s)", name, filename);
            else {
                /* Hopefully the REEXPORTs never form a cycle, we don't check
                   for that!  */
                macho_load_dll(s1, subfd, name, lev + 1);
                close(subfd);
            };
            break;
        };
        case TCC_LC_DYSYMTAB:
        {
            struct tcc_dysymtab_command *dc = (struct tcc_dysymtab_command*)lc;
            iextdef = dc->iextdefsym;
            nextdef = dc->nextdefsym;
            break;
        };
        };
        lc = (struct tcc_load_command*) ((char*)lc + lc->cmdsize);
    };

    if (tcc_add_dllref(s1, soname, lev)->found)
        goto the_end;

    if (!nsyms || !nextdef)
      tcc_warning("%s doesn't export any symbols?", filename);

    //dprintf("symbols (all):\n");
    dprintf("symbols (exported):\n");
    dprintf("    n: typ sec   desc              value name\n");
    //for (i = 0; i < nsyms; i++) {
    for (i = iextdef; i < iextdef + nextdef; i++) {
        struct tcc_nlist_64 *sym = symtab + i;
        dprintf("%5d: %3d %3d 0x%04x 0x%016lx %s\n",
                i, sym->n_type, sym->n_sect, sym->n_desc, (long)sym->n_value,
                strtab + sym->n_strx);
        set_elf_sym(s1->dynsymtab_section, 0, 0,
                    ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE),
                    0, SHN_UNDEF, strtab + sym->n_strx);
    };

  the_end:
    tcc_free(strtab);
    tcc_free(symtab);
    tcc_free(buf2);
    return 0;
};
