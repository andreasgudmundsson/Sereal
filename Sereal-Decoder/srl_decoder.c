#include "srl_decoder.h"

#include "ppport.h"

#define PERL_NO_GET_CONTEXT

#include "ptable.h"
#include "srl_protocol.h"

#define SRL_SET_FBIT(ptr) (*ptr |= 0b10000000)

/* This is fired when we exit the Perl pseudo-block.
 * It frees our decoder and all. Put decoder-level cleanup
 * logic here so that we can simply use croak/longjmp for
 * exception handling. Makes life vastly easier!
 */
void srl_destructor_hook(void *p)
{
    srl_decoder_t *dec = (srl_decoder_t *)p;
    /* Exception cleanup. Under normal operation, we should have
     * assigned NULL to buf_start after we're done. */
    PTABLE_free(dec->ref_seenhash);
    PTABLE_free(dec->str_seenhash);
    Safefree(dec);
}

/* Builds the C-level configuration and state struct.
 * Automatically freed at scope boundary. */
srl_decoder_t *
build_decoder_struct(pTHX_ HV *opt, SV *src)
{
    STRLEN len;
    srl_decoder_t *dec;
    /* SV **svp; */

    Newx(dec, 1, srl_decoder_t);
    /* Register our structure for destruction on scope exit */
    SAVEDESTRUCTOR(&srl_destructor_hook, (void *)dec);
    dec->depth = 0;
    dec->flags = 0;

    dec->ref_seenhash = PTABLE_new();
    dec->str_seenhash = PTABLE_new();
    dec->buf_start= dec->pos= (unsigned char*)SvPV(src, len);
    dec->buf_end= dec->buf_start + len;

    /* load options */
    if (opt != NULL) {
        /* if ( (svp = hv_fetchs(opt, "undef_blessed", 0)) && SvTRUE(*svp))
          dec->flags |= F_UNDEF_BLESSED;
        */
    }

    return dec;
}


int
srl_read_header(pTHX_ srl_decoder_t *dec)
{
    /* works for now: 3 byte magic string + proto version + 1 byte varint that indicates zero-length header 
    DEBUG_ASSERT_BUF_SANE(dec);
    srl_buf_cat_str_s(dec, SRL_MAGIC_STRING "\x01");
    srl_buf_cat_char(dec, '\0');
    */
}

SV *
srl_read_sv(pTHX_ srl_decoder_t *dec)
{
    STRLEN len;
    SV *ret= NULL;
    U8 tag= *dec->pos++;
    U8 track= tag & SRL_TRACK_FLAG;
    tag= tag & ~SRL_TRACK_FLAG;
    
    if (tag & SRL_HDR_ASCII) {
        len= (STRLEN)(tag & SRL_HDR_ASCI_LEN_MASK);
        ret= newSVpvn(dec->pos,len);
        dec->pos += len;
    } else if ( tag & SRL_HDR_TINYINT_KEY_MASK == 0) {
        U8 v= tag & SRL_HDR_TINYINT_VAL_MASK;
        if (tag & SRL_TINYINT_SIGN) {
            ret= newSViv(-v);
        } else {
            ret= newSVuv(v);
        }
    } else {
        switch (tag & SRL_HDR_TYPE_BITS) {
            case 0x00:/*varint*/            break;
            case 0x01:/*zigzag*/            break;

            case 0x02:/*float*/             break;
            case 0x03:/*double*/            break;
            case 0x04:/*long double*/       break;

            case 0x05:
            case 0x06:
            case 0x07:/* reserved */        break;

            case 0x08:
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x0E:
            case 0x0F: /* array of int */   break;

            case 0x10: /* ref */            break;
            case 0x11: /* reuse */          break;
            case 0x12: /* hash */
            case 0x13: /* array */          break;
            case 0x14: /* bless */          break;
            case 0x15: /* blessv */         break;
            case 0x16: /* weaken */         break;
            case 0x17: /* reserved */       break;
            
            case 0x18: 
            case 0x19: /* string */         break;

            case 0x1A: /* alias */          break;
            case 0x1B: /* copy */           break;
            case 0x1C: /* undef */          break;
            case 0x1D: /* regexp */         break;
            case 0x1E: /* reserved */       break;
            case 0x1F: /* pad */            break;
            default:
                /* error */
                break;
        }
    }
}

