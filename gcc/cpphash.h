/* Part of CPP library.
   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This header defines all the internal data structures and functions
   that need to be visible across files.  It's called cpphash.h for
   historical reasons.  */

#ifndef GCC_CPPHASH_H
#define GCC_CPPHASH_H

#include "hashtable.h"

#if defined HAVE_ICONV_H && defined HAVE_ICONV
#include <iconv.h>
#else
#define HAVE_ICONV 0
typedef int iconv_t;  /* dummy */
#endif

struct directive;		/* Deliberately incomplete.  */
struct pending_option;
struct op;
struct _cpp_strbuf;

typedef bool (*convert_f) (iconv_t, const unsigned char *, size_t,
			   struct _cpp_strbuf *);
struct cset_converter
{
  convert_f func;
  iconv_t cd;
};

#ifndef HAVE_UCHAR
typedef unsigned char uchar;
#endif
#define U (const uchar *)  /* Intended use: U"string" */

#define BITS_PER_CPPCHAR_T (CHAR_BIT * sizeof (cppchar_t))

/* Test if a sign is valid within a preprocessing number.  */
#define VALID_SIGN(c, prevc) \
  (((c) == '+' || (c) == '-') && \
   ((prevc) == 'e' || (prevc) == 'E' \
    || (((prevc) == 'p' || (prevc) == 'P') \
        && CPP_OPTION (pfile, extended_numbers))))

#define CPP_OPTION(PFILE, OPTION) ((PFILE)->opts.OPTION)
#define CPP_BUFFER(PFILE) ((PFILE)->buffer)
#define CPP_BUF_COLUMN(BUF, CUR) ((CUR) - (BUF)->line_base)
#define CPP_BUF_COL(BUF) CPP_BUF_COLUMN(BUF, (BUF)->cur)

/* Maximum nesting of cpp_buffers.  We use a static limit, partly for
   efficiency, and partly to limit runaway recursion.  */
#define CPP_STACK_MAX 200

/* Host alignment handling.  */
struct dummy
{
  char c;
  union
  {
    double d;
    int *p;
  } u;
};

#define DEFAULT_ALIGNMENT offsetof (struct dummy, u)
#define CPP_ALIGN2(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#define CPP_ALIGN(size) CPP_ALIGN2 (size, DEFAULT_ALIGNMENT)

/* Each macro definition is recorded in a cpp_macro structure.
   Variadic macros cannot occur with traditional cpp.  */
struct cpp_macro
{
  /* Parameters, if any.  */
  cpp_hashnode **params;

  /* Replacement tokens (ISO) or replacement text (traditional).  See
     comment at top of cpptrad.c for how traditional function-like
     macros are encoded.  */
  union
  {
    cpp_token *tokens;
    const uchar *text;
  } exp;

  /* Definition line number.  */
  fileline line;

  /* Number of tokens in expansion, or bytes for traditional macros.  */
  unsigned int count;

  /* Number of parameters.  */
  unsigned short paramc;

  /* If a function-like macro.  */
  unsigned int fun_like : 1;

  /* If a variadic macro.  */
  unsigned int variadic : 1;

  /* If macro defined in system header.  */
  unsigned int syshdr   : 1;

  /* Nonzero if it has been expanded or had its existence tested.  */
  unsigned int used     : 1;
};

#define _cpp_mark_macro_used(NODE) do {					\
  if ((NODE)->type == NT_MACRO && !((NODE)->flags & NODE_BUILTIN))	\
    (NODE)->value.macro->used = 1; } while (0)

/* A generic memory buffer, and operations on it.  */
typedef struct _cpp_buff _cpp_buff;
struct _cpp_buff
{
  struct _cpp_buff *next;
  unsigned char *base, *cur, *limit;
};

extern _cpp_buff *_cpp_get_buff (cpp_reader *, size_t);
extern void _cpp_release_buff (cpp_reader *, _cpp_buff *);
extern void _cpp_extend_buff (cpp_reader *, _cpp_buff **, size_t);
extern _cpp_buff *_cpp_append_extend_buff (cpp_reader *, _cpp_buff *, size_t);
extern void _cpp_free_buff (_cpp_buff *);
extern unsigned char *_cpp_aligned_alloc (cpp_reader *, size_t);
extern unsigned char *_cpp_unaligned_alloc (cpp_reader *, size_t);

#define BUFF_ROOM(BUFF) (size_t) ((BUFF)->limit - (BUFF)->cur)
#define BUFF_FRONT(BUFF) ((BUFF)->cur)
#define BUFF_LIMIT(BUFF) ((BUFF)->limit)

/* #include types.  */
enum include_type {IT_INCLUDE, IT_INCLUDE_NEXT, IT_IMPORT, IT_CMDLINE};

union utoken
{
  const cpp_token *token;
  const cpp_token **ptoken;
};

/* A "run" of tokens; part of a chain of runs.  */
typedef struct tokenrun tokenrun;
struct tokenrun
{
  tokenrun *next, *prev;
  cpp_token *base, *limit;
};

/* Accessor macros for struct cpp_context.  */
#define FIRST(c) ((c)->u.iso.first)
#define LAST(c) ((c)->u.iso.last)
#define CUR(c) ((c)->u.trad.cur)
#define RLIMIT(c) ((c)->u.trad.rlimit)

typedef struct cpp_context cpp_context;
struct cpp_context
{
  /* Doubly-linked list.  */
  cpp_context *next, *prev;

  union
  {
    /* For ISO macro expansion.  Contexts other than the base context
       are contiguous tokens.  e.g. macro expansions, expanded
       argument tokens.  */
    struct
    {
      union utoken first;
      union utoken last;
    } iso;

    /* For traditional macro expansion.  */
    struct
    {
      const uchar *cur;
      const uchar *rlimit;
    } trad;
  } u;

  /* If non-NULL, a buffer used for storage related to this context.
     When the context is popped, the buffer is released.  */
  _cpp_buff *buff;

  /* For a macro context, the macro node, otherwise NULL.  */
  cpp_hashnode *macro;

  /* True if utoken element is token, else ptoken.  */
  bool direct_p;
};

struct lexer_state
{
  /* Nonzero if first token on line is CPP_HASH.  */
  unsigned char in_directive;

  /* Nonzero if in a directive that will handle padding tokens itself.
     #include needs this to avoid problems with computed include and
     spacing between tokens.  */
  unsigned char directive_wants_padding;

  /* True if we are skipping a failed conditional group.  */
  unsigned char skipping;

  /* Nonzero if in a directive that takes angle-bracketed headers.  */
  unsigned char angled_headers;

  /* Nonzero if in a #if or #elif directive.  */
  unsigned char in_expression;

  /* Nonzero to save comments.  Turned off if discard_comments, and in
     all directives apart from #define.  */
  unsigned char save_comments;

  /* Nonzero if lexing __VA_ARGS__ is valid.  */
  unsigned char va_args_ok;

  /* Nonzero if lexing poisoned identifiers is valid.  */
  unsigned char poisoned_ok;

  /* Nonzero to prevent macro expansion.  */
  unsigned char prevent_expansion;

  /* Nonzero when parsing arguments to a function-like macro.  */
  unsigned char parsing_args;

  /* Nonzero to skip evaluating part of an expression.  */
  unsigned int skip_eval;
};

/* Special nodes - identifiers with predefined significance.  */
struct spec_nodes
{
  cpp_hashnode *n_defined;		/* defined operator */
  cpp_hashnode *n_true;			/* C++ keyword true */
  cpp_hashnode *n_false;		/* C++ keyword false */
  cpp_hashnode *n__VA_ARGS__;		/* C99 vararg macros */
};

typedef struct _cpp_line_note _cpp_line_note;
struct _cpp_line_note
{
  /* Location in the clean line the note refers to.  */
  const uchar *pos;

  /* Type of note.  The 9 'from' trigraph characters represent those
     trigraphs, '\\' an escaped newline, ' ' an escaped newline with
     intervening space, and anything else is invalid.  */
  unsigned int type;
};

/* Represents the contents of a file cpplib has read in.  */
struct cpp_buffer
{
  const uchar *cur;		/* Current location.  */
  const uchar *line_base;	/* Start of current physical line.  */
  const uchar *next_line;	/* Start of to-be-cleaned logical line.  */

  const uchar *buf;		/* Entire character buffer.  */
  const uchar *rlimit;		/* Writable byte at end of file.  */

  _cpp_line_note *notes;	/* Array of notes.  */
  unsigned int cur_note;	/* Next note to process.  */
  unsigned int notes_used;	/* Number of notes.  */
  unsigned int notes_cap;	/* Size of allocated array.  */

  struct cpp_buffer *prev;

  /* Pointer into the file table; non-NULL if this is a file buffer.
     Used for include_next and to record control macros.  */
  struct _cpp_file *file;

  /* Value of if_stack at start of this file.
     Used to prohibit unmatched #endif (etc) in an include file.  */
  struct if_stack *if_stack;

  /* True if we need to get the next clean line.  */
  bool need_line;

  /* True if we have already warned about C++ comments in this file.
     The warning happens only for C89 extended mode with -pedantic on,
     or for -Wtraditional, and only once per file (otherwise it would
     be far too noisy).  */
  unsigned char warned_cplusplus_comments;

  /* True if we don't process trigraphs and escaped newlines.  True
     for preprocessed input, command line directives, and _Pragma
     buffers.  */
  unsigned char from_stage3;

  /* Nonzero means that the directory to start searching for ""
     include files has been calculated and stored in "dir" below.  */
  unsigned char search_cached;

  /* The directory of the this buffer's file.  Its NAME member is not
     allocated, so we don't need to worry about freeing it.  */
  struct cpp_dir dir;

  /* Used for buffer overlays by cpptrad.c.  */
  const uchar *saved_cur, *saved_rlimit;

  /* Descriptor for converting from the input character set to the
     source character set.  */
  struct cset_converter input_cset_desc;
};

/* A cpp_reader encapsulates the "state" of a pre-processor run.
   Applying cpp_get_token repeatedly yields a stream of pre-processor
   tokens.  Usually, there is only one cpp_reader object active.  */
struct cpp_reader
{
  /* Top of buffer stack.  */
  cpp_buffer *buffer;

  /* Overlaid buffer (can be different after processing #include).  */
  cpp_buffer *overlaid_buffer;

  /* Lexer state.  */
  struct lexer_state state;

  /* Source line tracking.  */
  struct line_maps *line_table;
  const struct line_map *map;
  fileline line;

  /* The line of the '#' of the current directive.  */
  fileline directive_line;

  /* Memory buffers.  */
  _cpp_buff *a_buff;		/* Aligned permanent storage.  */
  _cpp_buff *u_buff;		/* Unaligned permanent storage.  */
  _cpp_buff *free_buffs;	/* Free buffer chain.  */

  /* Context stack.  */
  struct cpp_context base_context;
  struct cpp_context *context;

  /* If in_directive, the directive if known.  */
  const struct directive *directive;

  /* Search paths for include files.  */
  struct cpp_dir *quote_include;	/* "" */
  struct cpp_dir *bracket_include;	/* <> */
  struct cpp_dir no_search_path;	/* No path.  */

  /* Chain of all hashed _cpp_file instances.  */
  struct _cpp_file *all_files;

  struct _cpp_file *main_file;

  /* File and directory hash table.  */
  struct htab *file_hash;
  struct file_hash_entry *file_hash_entries;
  unsigned int file_hash_entries_allocated, file_hash_entries_used;

  /* Nonzero means don't look for #include "foo" the source-file
     directory.  */
  bool quote_ignores_source_dir;

  /* Nonzero if any file has contained #pragma once or #import has
     been used.  */
  bool seen_once_only;

  /* Multiple include optimization.  */
  const cpp_hashnode *mi_cmacro;
  const cpp_hashnode *mi_ind_cmacro;
  bool mi_valid;

  /* Lexing.  */
  cpp_token *cur_token;
  tokenrun base_run, *cur_run;
  unsigned int lookaheads;

  /* Nonzero prevents the lexer from re-using the token runs.  */
  unsigned int keep_tokens;

  /* Error counter for exit code.  */
  unsigned int errors;

  /* Buffer to hold macro definition string.  */
  unsigned char *macro_buffer;
  unsigned int macro_buffer_len;

  /* Descriptor for converting from the source character set to the
     execution character set.  */
  struct cset_converter narrow_cset_desc;

  /* Descriptor for converting from the source character set to the
     wide execution character set.  */
  struct cset_converter wide_cset_desc;

  /* Date and time text.  Calculated together if either is requested.  */
  const uchar *date;
  const uchar *time;

  /* EOF token, and a token forcing paste avoidance.  */
  cpp_token avoid_paste;
  cpp_token eof;

  /* Opaque handle to the dependencies of mkdeps.c.  */
  struct deps *deps;

  /* Obstack holding all macro hash nodes.  This never shrinks.
     See cpphash.c */
  struct obstack hash_ob;

  /* Obstack holding buffer and conditional structures.  This is a
     real stack.  See cpplib.c.  */
  struct obstack buffer_ob;

  /* Pragma table - dynamic, because a library user can add to the
     list of recognized pragmas.  */
  struct pragma_entry *pragmas;

  /* Call backs to cpplib client.  */
  struct cpp_callbacks cb;

  /* Identifier hash table.  */
  struct ht *hash_table;

  /* Expression parser stack.  */
  struct op *op_stack, *op_limit;

  /* User visible options.  */
  struct cpp_options opts;

  /* Special nodes - identifiers with predefined significance to the
     preprocessor.  */
  struct spec_nodes spec_nodes;

  /* Whether cpplib owns the hashtable.  */
  bool our_hashtable;

  /* Traditional preprocessing output buffer (a logical line).  */
  struct
  {
    uchar *base;
    uchar *limit;
    uchar *cur;
    fileline first_line;
  } out;

  /* Used to save the original line number during traditional
     preprocessing.  */
  unsigned int saved_line;

  /* A saved list of the defined macros, for dependency checking
     of precompiled headers.  */
  struct cpp_savedstate *savedstate;
};

/* Character classes.  Based on the more primitive macros in safe-ctype.h.
   If the definition of `numchar' looks odd to you, please look up the
   definition of a pp-number in the C standard [section 6.4.8 of C99].

   In the unlikely event that characters other than \r and \n enter
   the set is_vspace, the macro handle_newline() in cpplex.c must be
   updated.  */
#define _dollar_ok(x)	((x) == '$' && CPP_OPTION (pfile, dollars_in_ident))

#define is_idchar(x)	(ISIDNUM(x) || _dollar_ok(x))
#define is_numchar(x)	ISIDNUM(x)
#define is_idstart(x)	(ISIDST(x) || _dollar_ok(x))
#define is_numstart(x)	ISDIGIT(x)
#define is_hspace(x)	ISBLANK(x)
#define is_vspace(x)	IS_VSPACE(x)
#define is_nvspace(x)	IS_NVSPACE(x)
#define is_space(x)	IS_SPACE_OR_NUL(x)

/* This table is constant if it can be initialized at compile time,
   which is the case if cpp was compiled with GCC >=2.7, or another
   compiler that supports C99.  */
#if HAVE_DESIGNATED_INITIALIZERS
extern const unsigned char _cpp_trigraph_map[UCHAR_MAX + 1];
#else
extern unsigned char _cpp_trigraph_map[UCHAR_MAX + 1];
#endif

/* Macros.  */

#define CPP_IN_SYSTEM_HEADER(PFILE) ((PFILE)->map && (PFILE)->map->sysp)
#define CPP_PEDANTIC(PF) CPP_OPTION (PF, pedantic)
#define CPP_WTRADITIONAL(PF) CPP_OPTION (PF, warn_traditional)

/* In cpperror.c  */
extern int _cpp_begin_message (cpp_reader *, int, fileline, unsigned int);

/* In cppmacro.c */
extern void _cpp_free_definition (cpp_hashnode *);
extern bool _cpp_create_definition (cpp_reader *, cpp_hashnode *);
extern void _cpp_pop_context (cpp_reader *);
extern void _cpp_push_text_context (cpp_reader *, cpp_hashnode *,
				    const uchar *, size_t);
extern bool _cpp_save_parameter (cpp_reader *, cpp_macro *, cpp_hashnode *);
extern bool _cpp_arguments_ok (cpp_reader *, cpp_macro *, const cpp_hashnode *,
			       unsigned int);
extern const uchar *_cpp_builtin_macro_text (cpp_reader *, cpp_hashnode *);
int _cpp_warn_if_unused_macro (cpp_reader *, cpp_hashnode *, void *);
/* In cpphash.c */
extern void _cpp_init_hashtable (cpp_reader *, hash_table *);
extern void _cpp_destroy_hashtable (cpp_reader *);

/* In cppfiles.c */
typedef struct _cpp_file _cpp_file;
extern _cpp_file *_cpp_find_file (cpp_reader *, const char *fname,
				  cpp_dir *start_dir, bool fake);
extern bool _cpp_find_failed (_cpp_file *);
extern void _cpp_mark_file_once_only (cpp_reader *, struct _cpp_file *);
extern void _cpp_fake_include (cpp_reader *, const char *);
extern bool _cpp_stack_file (cpp_reader *, _cpp_file*, bool);
extern bool _cpp_stack_include (cpp_reader *, const char *, int,
				enum include_type);
extern int _cpp_compare_file_date (cpp_reader *, const char *, int);
extern void _cpp_report_missing_guards (cpp_reader *);
extern void _cpp_init_files (cpp_reader *);
extern void _cpp_cleanup_files (cpp_reader *);
extern void _cpp_pop_file_buffer (cpp_reader *, struct _cpp_file *);
extern bool _cpp_save_file_entries (cpp_reader *pfile, FILE *f);
extern bool _cpp_read_file_entries (cpp_reader *, FILE *);

/* In cppexp.c */
extern bool _cpp_parse_expr (cpp_reader *);
extern struct op *_cpp_expand_op_stack (cpp_reader *);

/* In cpplex.c */
extern void _cpp_process_line_notes (cpp_reader *, int);
extern void _cpp_clean_line (cpp_reader *);
extern bool _cpp_get_fresh_line (cpp_reader *);
extern bool _cpp_skip_block_comment (cpp_reader *);
extern cpp_token *_cpp_temp_token (cpp_reader *);
extern const cpp_token *_cpp_lex_token (cpp_reader *);
extern cpp_token *_cpp_lex_direct (cpp_reader *);
extern int _cpp_equiv_tokens (const cpp_token *, const cpp_token *);
extern void _cpp_init_tokenrun (tokenrun *, unsigned int);

/* In cppinit.c.  */
extern void _cpp_maybe_push_include_file (cpp_reader *);

/* In cpplib.c */
extern int _cpp_test_assertion (cpp_reader *, unsigned int *);
extern int _cpp_handle_directive (cpp_reader *, int);
extern void _cpp_define_builtin (cpp_reader *, const char *);
extern char ** _cpp_save_pragma_names (cpp_reader *);
extern void _cpp_restore_pragma_names (cpp_reader *, char **);
extern void _cpp_do__Pragma (cpp_reader *);
extern void _cpp_init_directives (cpp_reader *);
extern void _cpp_init_internal_pragmas (cpp_reader *);
extern void _cpp_do_file_change (cpp_reader *, enum lc_reason, const char *,
				 unsigned int, unsigned int);
extern void _cpp_pop_buffer (cpp_reader *);
extern uchar *_cpp_input_to_utf8 (cpp_reader *, const unsigned char *, cppchar_t);
extern void _cpp_init_iconv_buffer (cpp_reader *, const char *);
extern void _cpp_close_iconv_buffer (cpp_reader *);

/* In cpptrad.c.  */
extern bool _cpp_scan_out_logical_line (cpp_reader *, cpp_macro *);
extern bool _cpp_read_logical_line_trad (cpp_reader *);
extern void _cpp_overlay_buffer (cpp_reader *pfile, const uchar *, size_t);
extern void _cpp_remove_overlay (cpp_reader *);
extern bool _cpp_create_trad_definition (cpp_reader *, cpp_macro *);
extern bool _cpp_expansions_different_trad (const cpp_macro *,
					    const cpp_macro *);
extern uchar *_cpp_copy_replacement_text (const cpp_macro *, uchar *);
extern size_t _cpp_replacement_text_len (const cpp_macro *);

/* In cppcharset.c.  */
extern cppchar_t _cpp_valid_ucn (cpp_reader *, const uchar **,
				 const uchar *, int);
extern void _cpp_destroy_iconv (cpp_reader *);
extern bool _cpp_interpret_string_notranslate (cpp_reader *, const cpp_string *,
					       cpp_string *);

/* Utility routines and macros.  */
#define DSC(str) (const uchar *)str, sizeof str - 1
#define xnew(T)		(T *) xmalloc (sizeof(T))
#define xcnew(T)	(T *) xcalloc (1, sizeof(T))
#define xnewvec(T, N)	(T *) xmalloc (sizeof(T) * (N))
#define xcnewvec(T, N)	(T *) xcalloc (N, sizeof(T))
#define xobnew(O, T)	(T *) obstack_alloc (O, sizeof(T))

/* These are inline functions instead of macros so we can get type
   checking.  */
static inline int ustrcmp (const uchar *, const uchar *);
static inline int ustrncmp (const uchar *, const uchar *, size_t);
static inline size_t ustrlen (const uchar *);
static inline uchar *uxstrdup (const uchar *);
static inline uchar *ustrchr (const uchar *, int);
static inline int ufputs (const uchar *, FILE *);

static inline int
ustrcmp (const uchar *s1, const uchar *s2)
{
  return strcmp ((const char *)s1, (const char *)s2);
}

static inline int
ustrncmp (const uchar *s1, const uchar *s2, size_t n)
{
  return strncmp ((const char *)s1, (const char *)s2, n);
}

static inline size_t
ustrlen (const uchar *s1)
{
  return strlen ((const char *)s1);
}

static inline uchar *
uxstrdup (const uchar *s1)
{
  return (uchar *) xstrdup ((const char *)s1);
}

static inline uchar *
ustrchr (const uchar *s1, int c)
{
  return (uchar *) strchr ((const char *)s1, c);
}

static inline int
ufputs (const uchar *s, FILE *f)
{
  return fputs ((const char *)s, f);
}

#endif /* ! GCC_CPPHASH_H */
