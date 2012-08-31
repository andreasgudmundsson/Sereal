#ifndef SRL_DECODER_H_
#define SRL_DECODER_H_

#include "EXTERN.h"
#include "perl.h"
#include "assert.h"

#define SRL_DECODER_NEEDS_FINALIZE 1

typedef struct PTABLE * ptable_ptr;
typedef struct {
    unsigned char *buf_start;           /* ptr to "physical" start of input buffer */
    unsigned char *buf_end;             /* ptr to end of input buffer */
    unsigned char *pos;                 /* ptr to current position within input buffer */
    unsigned char *save_pos;

    U32 flags;                          /* flag-like options: See F_* defines in srl_decoder.c */
    unsigned int depth;                 /* current Perl-ref recursion depth */
    ptable_ptr ref_seenhash;            /* ptr table for avoiding circular refs */
    ptable_ptr ref_stashes;             /* ptr table for tracking stashes we will bless into - key: ofs, value: stash */
    ptable_ptr ref_bless_av;            /* ptr table for tracking which objects need to be bless - key: ofs, value: mortal AV (of refs)  */
    AV* weakref_av;
} srl_decoder_t;

/* constructor; don't need destructor, this sets up a callback */
srl_decoder_t *srl_build_decoder_struct(pTHX_ HV *opt);

/* main routine */
SV *srl_decode_into(pTHX_ srl_decoder_t *dec, SV *src, SV *into);

/* Explicit destructor */
void srl_destroy_decoder(pTHX_ srl_decoder_t *dec);

/* destructor hook - called automagically */
void srl_decoder_destructor_hook(pTHX_ void *p);

#define BUF_POS(dec) ((dec)->pos)
#define BUF_SPACE(dec) ((dec)->buf_end - (dec)->pos)
#define BUF_POS_OFS(dec) ((dec)->pos - (dec)->buf_start)
#define BUF_SIZE(dec) ((dec)->buf_end - (dec)->buf_start)
#define BUF_NOT_DONE(dec) ((dec)->pos < (dec)->buf_end)
#define BUF_DONE(dec) ((dec)->pos >= (dec)->buf_end)


#define MYCROAK(fmt, args...) croak("Sereal: Error in %s line %u: " fmt, __FILE__, __LINE__ , ## args)
#define ERROR(msg) MYCROAK("%s", msg)
#define ERRORf1(fmt,var) MYCROAK(fmt, (var))
#define ERRORf2(fmt,var1,var2) MYCROAK(fmt, (var1),(var2))
#define ERROR_UNIMPLEMENTED(dec,tag,str) \
    MYCROAK("Tag %u %s is unimplemented at ofs: %d", tag,str, BUF_POS_OFS(dec)); 
#define ERROR_UNTERMINATED(dec, tag,str) MYCROAK("Tag %u %s was not terminated properly at ofs %lu with %lu to go", tag, str,dec->pos - dec->buf_start,dec->buf_end - dec->pos)
#define ERROR_BAD_COPY(dec, tag) MYCROAK("While processing tag %u encountered a bad COPY tag", tag)
#define ERROR_UNEXPECTED(dec, tag, msg) MYCROAK("Unexpected tag %u while expecting %s", (tag || *(dec)->pos), msg)
#define ERROR_PANIC(dec, msg) MYCROAK("Panic: %s", msg);

/* if set, the decoder struct needs to be cleared instead of freed at
 * the end of a deserialization operation */
#define SRL_F_REUSE_DECODER 1UL
#define SRL_DEC_HAVE_OPTION(dec, flag_num) ((dec)->flags & flag_num)


#endif
