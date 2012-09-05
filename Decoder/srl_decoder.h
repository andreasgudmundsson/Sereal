#ifndef SRL_DECODER_H_
#define SRL_DECODER_H_

#include "EXTERN.h"
#include "perl.h"
#include "assert.h"

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
#define ERROR_UNTERMINATED(dec, tag,str) MYCROAK("Tag SRL_HDR_%s %s was not terminated properly at ofs %lu with %lu to go", tag_name[tag & 127], str,dec->pos - dec->buf_start,dec->buf_end - dec->pos)
#define ERROR_BAD_COPY(dec, tag) MYCROAK("While processing tag SRL_HDR_%s encountered a bad COPY tag", tag_name[tag & 127])
#define ERROR_UNEXPECTED(dec, tag, msg) MYCROAK("Unexpected tag %s while expecting %s", tag_name[(tag || *(dec)->pos) & 127], msg)
#define ERROR_PANIC(dec, msg) MYCROAK("Panic: %s", msg);

/* If set, the decoder struct needs to be cleared instead of freed at
 * the end of a deserialization operation */
#define SRL_F_REUSE_DECODER 1UL
/* If set, then the decoder destructor was already pushed to the
 * callback stack */
#define SRL_F_DECODER_DESTRUCTOR_OK 2UL
/* Non-persistent flag! */
#define SRL_F_DECODER_NEEDS_FINALIZE 4UL

#define SRL_DEC_HAVE_OPTION(dec, flag_num) ((dec)->flags & flag_num)
#define SRL_DEC_SET_OPTION(dec, flag_num) ((dec)->flags |= flag_num)
#define SRL_DEC_UNSET_OPTION(dec, flag_num) ((dec)->flags &= ~flag_num)
#define SRL_DEC_VOLATILE_FLAGS (SRL_F_DECODER_NEEDS_FINALIZE)
#define SRL_DEC_RESET_VOLATILE_FLAGS(dec) ((dec)->flags &= ~SRL_DEC_VOLATILE_FLAGS)

/*/ run this script with: perl -x srl_decoder.h
#!perl -w
use strict;
my (%sym, %val);
sub f {
    my $pfx= shift;
    for my $i ($sym{$pfx . "_LOW"} .. $sym{$pfx . "_HIGH"}) {
            next if $val{$i};
            $sym{$pfx."_".$i}=$i;
            $val{$i}= $pfx . "_". $i;
    }
}
{
    open my $fh,"<", "srl_protocol.h"
        or die "srl_protocol.h: $!";
    while (<$fh>) {
        if(/^#define\s+SRL_HDR_(\S+)\s+\(\(char\)(\d+)\)/i) {
            $sym{$1}=$2;
            $val{$2}= $1;
        }
    }
    close $fh;
    foreach my $pfx (keys %sym) {
        if ($pfx=~/^(.*)_LOW/) {
                f($1)
        }
    }
}
{
    open my $fh,"<", $0
        or die "$0:$!";
    rename $0,"$0.bak";
    open my $out,">", $0
        or die "$0:$!";
    while (<$fh>) {
        print $out $_;
        last if /^static const char \* const tag_name/;
    }

    print $out join(",\n",map {
                    sprintf qq(\t/).qq(* # %3d 0x%02x 0b%08b *).qq(/ "%s"),$_,$_,$_,$val{$_}
            } 0 .. 127 )
            . "\n};\n#endif\n/"."* do not put anything below the #endif! *"."/\n";
    close $out;
    close $fh;
}
__END__

The above perl script can be used to regenerate the following data structure.
Have fun. ;-)
*/
static const char * const tag_name[] = {
        /* #   0 0x00 0b00000000 */ "POS_LOW",
        /* #   1 0x01 0b00000001 */ "POS_1",
        /* #   2 0x02 0b00000010 */ "POS_2",
        /* #   3 0x03 0b00000011 */ "POS_3",
        /* #   4 0x04 0b00000100 */ "POS_4",
        /* #   5 0x05 0b00000101 */ "POS_5",
        /* #   6 0x06 0b00000110 */ "POS_6",
        /* #   7 0x07 0b00000111 */ "POS_7",
        /* #   8 0x08 0b00001000 */ "POS_8",
        /* #   9 0x09 0b00001001 */ "POS_9",
        /* #  10 0x0a 0b00001010 */ "POS_10",
        /* #  11 0x0b 0b00001011 */ "POS_11",
        /* #  12 0x0c 0b00001100 */ "POS_12",
        /* #  13 0x0d 0b00001101 */ "POS_13",
        /* #  14 0x0e 0b00001110 */ "POS_14",
        /* #  15 0x0f 0b00001111 */ "POS_HIGH",
        /* #  16 0x10 0b00010000 */ "NEG_LOW",
        /* #  17 0x11 0b00010001 */ "NEG_17",
        /* #  18 0x12 0b00010010 */ "NEG_18",
        /* #  19 0x13 0b00010011 */ "NEG_19",
        /* #  20 0x14 0b00010100 */ "NEG_20",
        /* #  21 0x15 0b00010101 */ "NEG_21",
        /* #  22 0x16 0b00010110 */ "NEG_22",
        /* #  23 0x17 0b00010111 */ "NEG_23",
        /* #  24 0x18 0b00011000 */ "NEG_24",
        /* #  25 0x19 0b00011001 */ "NEG_25",
        /* #  26 0x1a 0b00011010 */ "NEG_26",
        /* #  27 0x1b 0b00011011 */ "NEG_27",
        /* #  28 0x1c 0b00011100 */ "NEG_28",
        /* #  29 0x1d 0b00011101 */ "NEG_29",
        /* #  30 0x1e 0b00011110 */ "NEG_30",
        /* #  31 0x1f 0b00011111 */ "NEG_HIGH",
        /* #  32 0x20 0b00100000 */ "VARINT",
        /* #  33 0x21 0b00100001 */ "ZIGZAG",
        /* #  34 0x22 0b00100010 */ "FLOAT",
        /* #  35 0x23 0b00100011 */ "DOUBLE",
        /* #  36 0x24 0b00100100 */ "LONG_DOUBLE",
        /* #  37 0x25 0b00100101 */ "UNDEF",
        /* #  38 0x26 0b00100110 */ "STRING",
        /* #  39 0x27 0b00100111 */ "STRING_UTF8",
        /* #  40 0x28 0b00101000 */ "REFP",
        /* #  41 0x29 0b00101001 */ "REFN",
        /* #  42 0x2a 0b00101010 */ "HASH",
        /* #  43 0x2b 0b00101011 */ "ARRAY",
        /* #  44 0x2c 0b00101100 */ "BLESS",
        /* #  45 0x2d 0b00101101 */ "BLESSV",
        /* #  46 0x2e 0b00101110 */ "ALIAS",
        /* #  47 0x2f 0b00101111 */ "COPY",
        /* #  48 0x30 0b00110000 */ "WEAKEN",
        /* #  49 0x31 0b00110001 */ "REGEXP",
        /* #  50 0x32 0b00110010 */ "RESERVED1_LOW",
        /* #  51 0x33 0b00110011 */ "RESERVED1_51",
        /* #  52 0x34 0b00110100 */ "RESERVED1_52",
        /* #  53 0x35 0b00110101 */ "RESERVED1_53",
        /* #  54 0x36 0b00110110 */ "RESERVED1_54",
        /* #  55 0x37 0b00110111 */ "RESERVED1_55",
        /* #  56 0x38 0b00111000 */ "RESERVED1_56",
        /* #  57 0x39 0b00111001 */ "RESERVED1_57",
        /* #  58 0x3a 0b00111010 */ "RESERVED1_58",
        /* #  59 0x3b 0b00111011 */ "RESERVED1_HIGH",
        /* #  60 0x3c 0b00111100 */ "EXTEND",
        /* #  61 0x3d 0b00111101 */ "PACKET_START",
        /* #  62 0x3e 0b00111110 */ "LIST",
        /* #  63 0x3f 0b00111111 */ "PAD",
        /* #  64 0x40 0b01000000 */ "RESERVED2_LOW",
        /* #  65 0x41 0b01000001 */ "RESERVED2_65",
        /* #  66 0x42 0b01000010 */ "RESERVED2_66",
        /* #  67 0x43 0b01000011 */ "RESERVED2_67",
        /* #  68 0x44 0b01000100 */ "RESERVED2_68",
        /* #  69 0x45 0b01000101 */ "RESERVED2_69",
        /* #  70 0x46 0b01000110 */ "RESERVED2_70",
        /* #  71 0x47 0b01000111 */ "RESERVED2_71",
        /* #  72 0x48 0b01001000 */ "RESERVED2_72",
        /* #  73 0x49 0b01001001 */ "RESERVED2_73",
        /* #  74 0x4a 0b01001010 */ "RESERVED2_74",
        /* #  75 0x4b 0b01001011 */ "RESERVED2_75",
        /* #  76 0x4c 0b01001100 */ "RESERVED2_76",
        /* #  77 0x4d 0b01001101 */ "RESERVED2_77",
        /* #  78 0x4e 0b01001110 */ "RESERVED2_78",
        /* #  79 0x4f 0b01001111 */ "RESERVED2_79",
        /* #  80 0x50 0b01010000 */ "RESERVED2_80",
        /* #  81 0x51 0b01010001 */ "RESERVED2_81",
        /* #  82 0x52 0b01010010 */ "RESERVED2_82",
        /* #  83 0x53 0b01010011 */ "RESERVED2_83",
        /* #  84 0x54 0b01010100 */ "RESERVED2_84",
        /* #  85 0x55 0b01010101 */ "RESERVED2_85",
        /* #  86 0x56 0b01010110 */ "RESERVED2_86",
        /* #  87 0x57 0b01010111 */ "RESERVED2_87",
        /* #  88 0x58 0b01011000 */ "RESERVED2_88",
        /* #  89 0x59 0b01011001 */ "RESERVED2_89",
        /* #  90 0x5a 0b01011010 */ "RESERVED2_90",
        /* #  91 0x5b 0b01011011 */ "RESERVED2_91",
        /* #  92 0x5c 0b01011100 */ "RESERVED2_92",
        /* #  93 0x5d 0b01011101 */ "RESERVED2_93",
        /* #  94 0x5e 0b01011110 */ "RESERVED2_94",
        /* #  95 0x5f 0b01011111 */ "RESERVED2_HIGH",
        /* #  96 0x60 0b01100000 */ "ASCII_LOW",
        /* #  97 0x61 0b01100001 */ "ASCII_97",
        /* #  98 0x62 0b01100010 */ "ASCII_98",
        /* #  99 0x63 0b01100011 */ "ASCII_99",
        /* # 100 0x64 0b01100100 */ "ASCII_100",
        /* # 101 0x65 0b01100101 */ "ASCII_101",
        /* # 102 0x66 0b01100110 */ "ASCII_102",
        /* # 103 0x67 0b01100111 */ "ASCII_103",
        /* # 104 0x68 0b01101000 */ "ASCII_104",
        /* # 105 0x69 0b01101001 */ "ASCII_105",
        /* # 106 0x6a 0b01101010 */ "ASCII_106",
        /* # 107 0x6b 0b01101011 */ "ASCII_107",
        /* # 108 0x6c 0b01101100 */ "ASCII_108",
        /* # 109 0x6d 0b01101101 */ "ASCII_109",
        /* # 110 0x6e 0b01101110 */ "ASCII_110",
        /* # 111 0x6f 0b01101111 */ "ASCII_111",
        /* # 112 0x70 0b01110000 */ "ASCII_112",
        /* # 113 0x71 0b01110001 */ "ASCII_113",
        /* # 114 0x72 0b01110010 */ "ASCII_114",
        /* # 115 0x73 0b01110011 */ "ASCII_115",
        /* # 116 0x74 0b01110100 */ "ASCII_116",
        /* # 117 0x75 0b01110101 */ "ASCII_117",
        /* # 118 0x76 0b01110110 */ "ASCII_118",
        /* # 119 0x77 0b01110111 */ "ASCII_119",
        /* # 120 0x78 0b01111000 */ "ASCII_120",
        /* # 121 0x79 0b01111001 */ "ASCII_121",
        /* # 122 0x7a 0b01111010 */ "ASCII_122",
        /* # 123 0x7b 0b01111011 */ "ASCII_123",
        /* # 124 0x7c 0b01111100 */ "ASCII_124",
        /* # 125 0x7d 0b01111101 */ "ASCII_125",
        /* # 126 0x7e 0b01111110 */ "ASCII_126",
        /* # 127 0x7f 0b01111111 */ "ASCII_HIGH"
};
#endif
/* do not put anything below the #endif! */
