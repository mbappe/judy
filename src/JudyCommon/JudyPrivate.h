#ifndef _JUDYPRIVATE_INCLUDED
#define _JUDYPRIVATE_INCLUDED
// _________________
//
// Copyright (C) 2000 - 2002 Hewlett-Packard Company
//
// This program is free software; you can redistribute it and/or modify it
// under the term of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// _________________

// @(#) $Revision: 1.9 $ $Source: /home/doug/judy-1.0.5_PSplit_goto_newLeaf3_U2_1K_1_L765_5th_cleanedup/src/JudyCommon/RCS/JudyPrivate.h,v $
//
// Header file for all Judy sources, for global but private (non-exported)
// declarations.

#include "Judy.h"

// ****************************************************************************
// A VERY BRIEF EXPLANATION OF A JUDY ARRAY
//
// A Judy array is, effectively, a digital tree (or Trie) with 256 element
// branches (nodes), and with "compression tricks" applied to low-population
// branches or leaves to save a lot of memory at the cost of relatively little
// CPU time or cache fills.
//
// In the actual implementation, a Judy array is level-less, and traversing the
// "tree" actually means following the states in a state machine (SM) as
// directed by the Index.  A Judy array is referred to here as an "SM", rather
// than as a "tree"; having "states", rather than "levels".
//
// Each branch or leaf in the SM decodes a portion ("digit") of the original
// Index; with 256-way branches there are 8 bits per digit.  There are 3 kinds
// of branches, called:  Linear, Bitmap and Uncompressed, of which the first 2
// are compressed to contain no NULL entries.
//
// An Uncompressed branch has a 1.0 cache line fill cost to decode 8 bits of
// (digit, part of an Index), but it might contain many NULL entries, and is
// therefore inefficient with memory if lightly populated.
//
// A Linear branch has a ~1.75 cache line fill cost when at maximum population.
// A Bitmap branch has ~2.0 cache line fills.  Linear and Bitmap branches are
// converted to Uncompressed branches when the additional memory can be
// amortized with larger populations.  Higher-state branches have higher
// priority to be converted.
//
// Linear branches can hold 28 elements (based on detailed analysis) -- thus 28
// expanses.  A Linear branch is converted to a Bitmap branch when the 29th
// expanse is required.
//
// A Bitmap branch could hold 256 expanses, but is forced to convert to an
// Uncompressed branch when 185 expanses are required.  Hopefully, it is
// converted before that because of population growth (again, based on detailed
// analysis and heuristics in the code).
//
// A path through the SM terminates to a leaf when the Index (or key)
// population in the expanse below a pointer will fit into 1 or 2 cache lines
// (~31..255 Indexes).  A maximum-population Leaf has ~1.5 cache line fill
// cost.
//
// Leaves are sorted arrays of Indexes, where the Index Sizes (IS) are:  0, 1,
// 8, 16, 24, 32, [40, 48, 56, 64] bits.  The IS depends on the "density"
// (population/expanse) of the values in the Leaf.  Zero bits are possible if
// population == expanse in the SM (that is, a full small expanse).
//
// Elements of a branches are called Judy Pointers (JPs).  Each JP object
// points to the next object in the SM, plus, a JP can decode an additional
// 2[6] bytes of an Index, but at the cost of "narrowing" the expanse
// represented by the next object in the SM.  A "narrow" JP (one which has
// decode bytes/digits) is a way of skipping states in the SM.
//
// Although counterintuitive, we think a Judy SM is optimal when the Leaves are
// stored at MINIMUM compression (narrowing, or use of Decode bytes).  If more
// aggressive compression was used, decompression of a leaf be required to
// insert an index.  Additional compression would save a little memory but not
// help performance significantly.


#ifdef A_PICTURE_IS_WORTH_1000_WORDS
*******************************************************************************

JUDY 32-BIT STATE MACHINE (SM) EXAMPLE, FOR INDEX = 0x02040103

The Index used in this example is purposely chosen to allow small, simple
examples below; each 1-byte "digit" from the Index has a small numeric value
that fits in one column.  In the drawing below:

   JRP  == Judy Root Pointer;

    C   == 1 byte of a 1..3 byte Population (count of Indexes) below this
           pointer.  Since this is shared with the Decode field, the combined
           sizes must be 3[7], that is, 1 word less 1 byte for the JP Type.

   The 1-byte field jp_Type is represented as:

   1..3 == Number of bytes in the population (Pop0) word of the Branch or Leaf
           below the pointer (note:  1..7 on 64-bit); indicates:
           - number of bytes in Decode field == 3 - this number;
           - number of bytes remaining to decode.
           Note:  The maximum is 3, not 4, because the 1st byte of the Index is
           always decoded digitally in the top branch.
   -B-  == JP points to a Branch (there are many kinds of Branches).
   -L-  == JP points to a Leaf (there are many kinds of Leaves).

   (2)  == Digit of Index decoded by position offset in branch (really
           0..0xff).

    4*  == Digit of Index necessary for decoding a "narrow" pointer, in a
           Decode field; replaces 1 missing branch (really 0..0xff).

    4+  == Digit of Index NOT necessary for decoding a "narrow" pointer, but
           used for fast traversal of the SM by Judy1Test() and JudyLGet()
           (see the code) (really 0..0xff).

    0   == Byte in a JPs Pop0 field that is always ignored, because a leaf
           can never contain more than 256 Indexes (Pop0 <= 255).

    +-----  == A Branch or Leaf; drawn open-ended to remind you that it could
    |          have up to 256 columns.
    +-----

    |
    |   == Pointer to next Branch or Leaf.
    V

    |
    O   == A state is skipped by using a "narrow" pointer.
    |

    < 1 > == Digit (Index) shown as an example is not necessarily in the
             position shown; is sorted in order with neighbor Indexes.
             (Really 0..0xff.)

Note that this example shows every possibly topology to reach a leaf in a
32-bit Judy SM, although this is a very subtle point!

                                                                          STATE or`
                                                                          LEVEL
     +---+    +---+    +---+    +---+    +---+    +---+    +---+    +---+
     |RJP|    |RJP|    |RJP|    |RJP|    |RJP|    |RJP|    |RJP|    |RJP|
     L---+    B---+    B---+    B---+    B---+    B---+    B---+    B---+
     |        |        |        |        |        |        |        |
     |        |        |        |        |        |        |        |
     V        V (2)    V (2)    V (2)    V (2)    V (2)    V (2)    V (2)
     +------  +------  +------  +------  +------  +------  +------  +------
Four |< 2 >   |  0     |  4*    |  C     |  4*    |  4*    |  C     |  C
byte |< 4 >   |  0     |  0     |  C     |  1*    |  C     |  C     |  C     4
Index|< 1 >   |  C     |  C     |  C     |  C     |  C     |  C     |  C
Leaf |< 3 >   |  3     |  2     |  3     |  1     |  2     |  3     |  3
     +------  +--L---  +--L---  +--B---  +--L---  +--B---  +--B---  +--B---
                 |        |        |        |        |        |        |
                /         |       /         |        |       /        /
               /          |      /          |        |      /        /
              |           |     |           |        |     |        |
              V           |     V   (4)     |        |     V   (4)  V   (4)
              +------     |     +------     |        |     +------  +------
    Three     |< 4 >      |     |    4+     |        |     |    4+  |    4+
    byte Index|< 1 >      O     |    0      O        O     |    1*  |    C   3
    Leaf      |< 3 >      |     |    C      |        |     |    C   |    C
              +------     |     |    2      |        |     |    1   |    2
                         /      +----L-     |        |     +----L-  +----B-
                        /            |      |        |          |        |
                       |            /       |       /          /        /
                       |           /        |      /          /        /
                       |          /         |     |          /        /
                       |         /          |     |         /        /
                       |        |           |     |        |        |
                       V        V           |     V(1)     |        V(1)
                       +------  +------     |     +------  |        +------
          Two byte     |< 1 >   |< 1 >      |     | 4+     |        | 4+
          Index Leaf   |< 3 >   |< 3 >      O     | 1+     O        | 1+     2
                       +------  +------    /      | C      |        | C
                                          /       | 1      |        | 1
                                         |        +-L----  |        +-L----
                                         |          |      |          |
                                         |         /       |         /
                                         |        |        |        |
                                         V        V        V        V
                                         +------  +------  +------  +------
                    One byte Index Leaf  |< 3 >   |< 3 >   |< 3 >   |< 3 >   1
                                         +------  +------  +------  +------


#endif // A_PICTURE_IS_WORTH_1000_WORDS


// ****************************************************************************
// MISCELLANEOUS GLOBALS:
//
// PLATFORM-SPECIFIC CONVENIENCE MACROS:
//
// These are derived from context (set by cc or in system header files) or
// based on JU_<PLATFORM> macros from make_includes/platform.*.mk.  We decided
// on 011018 that any macro reliably derivable from context (cc or headers) for
// ALL platforms supported by Judy is based on that derivation, but ANY
// exception means to stop using the external macro completely and derive from
// JU_<PLATFORM> instead.

// Other miscellaneous stuff:

#ifndef _BOOL_T
#define _BOOL_T
typedef int bool_t;
#endif

#define FUNCTION                // null; easy to find functions.

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef TRACE            // turn on all other tracing in the code:
#define TRACEJPI 1      // JP traversals in JudyIns.c and JudyDel.c.
#define TRACEJPG 1      // JP traversals in retrieval code, JudyGet.c.
#define TRACECF  1      // cache fills in JudyGet.c.
#define TRACEMI  1      // malloc calls in JudyMallocIF.c.
#define TRACEMF  1      // malloc calls at a lower level in JudyMalloc.c.
#endif

// Well maybe someday the compiler will set define instead of this kludge
#define JU_LITTLE_ENDIAN (((union { unsigned x; unsigned char c; }){1}).c)

// SUPPORT FOR DEBUG-ONLY CODE:
//
// By convention, use -DDEBUG to enable both debug-only code AND assertions in
// the Judy sources.
//
// Invert the sense of assertions, so they are off unless explicitly requested,
// in a uniform way.
//
// Note:  It is NOT appropriate to put this in Judy.h; it would mess up
// application code.

#ifndef DEBUG
#define NDEBUG 1                // must be 1 for "#if".
#endif

// Shorthand notations to avoid #ifdefs for single-line conditional statements:
//
// Warning:  These cannot be used around compiler directives, such as
// "#include", nor in the case where Code contains a comma other than nested
// within parentheses or quotes.

#ifndef DEBUG
#define DBGCODE(Code) // null.
#else
#define DBGCODE(Code) Code
#endif

#ifdef JUDY1
#define JUDY1CODE(Code) Code
#define JUDYLCODE(Code) // null.
#endif

#ifdef JUDYL
#define JUDYLCODE(Code) Code
#define JUDY1CODE(Code) // null.
#endif

// Enable measurements for average number of compare misses in search routines

#ifdef  SEARCHMETRICS
Word_t  j__MissCompares;
// Word_t  j__SearchPopulation;
Word_t  j__SearchGets;
Word_t  j__DirectHits;

// if BUCKETSIZE not defined, default to one word
#ifndef BUCKETSIZE
#define BUCKETSIZE      (sizeof(Word_t))       // Bytes per bucket
#endif  // BUCKETSIZE

#define KEYSPERBUCKET(KEY)   (BUCKETSIZE / (KEY))

// Perhaps should change name to SEARCHCOMPAREMISSES
#define  MISSCOMPARES(CNT)      (j__MissCompares   += (Word_t)(CNT))

#ifdef  DIRINWORDS

// on a bucket basis
#define  DIRECTHITS(_START, POS, KEY)                    \
    (j__DirectHits += (((_START) / KEYSPERBUCKET(KEY) == ((POS) / KEYSPERBUCKET(KEY)) ? 1 : 0)))
#else  // DIRINWORDS
// on a key basis
#define  DIRECTHITS(_START, POS, KEY)  (j__DirectHits += 1)

#endif  // DIRINWORDS

// #define  SEARCHPOPULATION(CNT)  (j__SearchPopulation += (Word_t)(CNT))
#else
#define STARTSEARCH             // null
#define  MISSCOMPARES(CNT)      // null
#define  DIRECTHITS(_START, POS, KEY)   // null
// #define  SEARCHPOPULATION(CNT)  // null
#endif  // ! SEARCHMETRICS

#include <assert.h>

// ****************************************************************************
// FUNDAMENTAL CONSTANTS FOR MACHINE
// ****************************************************************************

// Machine (CPU) cache line size:
//
// NOTE:  A leaf size of 2 cache lines maximum is the target (optimal) for
// Judy.  Its hard to obtain a machines cache line size at compile time, but
// if the machine has an unexpected cache line size, its not devastating if
// the following constants end up causing leaves that are 1 cache line in size,
// or even 4 cache lines in size.  The assumed 32-bit system has 16-word =
// 64-byte cache lines, and the assumed 64-bit system has 16-word = 128-byte
// cache lines.

#ifdef JU_64BIT
#define cJU_BYTESPERCL 128              // cache line size in bytes.
#else
#define cJU_BYTESPERCL  64              // cache line size in bytes.
#endif

// Bits Per Byte:


// Bytes Per Word and Bits Per Word, latter assuming sizeof(byte) is 8 bits:
//
// Expect 32 [64] bits per word.

#define cJU_BITSPERBYTE 0x8
#define cJU_BYTESPERWORD (sizeof(Word_t))
#define cBPW             (cJU_BYTESPERWORD)

#define cJU_BITSPERWORD  (cBPW * cJU_BITSPERBYTE)
#define cbPW             (cJU_BITSPERWORD)

#define JU_BYTESTOWORDS(BYTES) \
        (((BYTES) + cJU_BYTESPERWORD - 1) / cJU_BYTESPERWORD)

// A word that is all-ones, normally equal to -1UL, but safer with ~0:

#define cJU_ALLONES  (~(Word_t)0)

// Note, these are forward references, but thats OK:

#define cJU_FULLBITMAPB ((BITMAPB_t) cJU_ALLONES)
#define cJU_FULLBITMAPL ((BITMAPL_t) cJU_ALLONES)

// ****************************************************************************
// Return base 2 log (floor) of num -or- most significant bit number of num
//
// Note 1: This routine would be very slow if the compiler did not 
//     recognize it and change to use an SSE2 instruction.
//
// Note 2: measured at about < 1nS on Intel(R) Core(TM) i5-4670K CPU @ 4.5GHz
//
// Note 3: using clz instruction (32bit only) faster and requires __builtin_clz()
//        clz == c_ount number of l_eading z_ero bits in a word. When subtracted
//        from number of bit -1 in a word it gives you fls() or log2()
//
// If passing a constant, the time is zero, since the compiler calculates it.

// Note: With the gcc, icc and clang compilers, this routine turns into a 
// "bsr" instruction and therefore about a clock in time.

#ifdef __GNUC__

static inline int
j__log2(Word_t num)
{
    assert(num != 0);           // zero is undefined

//  This produces a "bsr" instruction on X86 family
    return ((cbPW - 1) - __builtin_clzl(num));
}

#else   // not __GNUC__ -- hope compiler can recognize it

static inline int
j__log2(Word_t num)
{
    int       __lb = 0;

    assert(num != 0);           // log of zero is undefined

#ifdef JU_64BIT
    if (num >= (((Word_t)1) << (1 * 32)))
    {
        num >>= 32;
        __lb += 32;
    }
#endif  // JU_64BIT

    if (num >= (((Word_t)1) << (1 * 16)))
    {
        num >>= 16;
        __lb += 16;
    }
    if (num >= (((Word_t)1) << (1 * 8)))
    {
        num >>= 8;
        __lb += 8;
    }
    if (num >= (((Word_t)1) << (1 * 4)))
    {
        num >>= 4;
        __lb += 4;
    }
    if (num >= (((Word_t)1) << (1 * 2)))
    {
        num >>= 2;
        __lb += 2;
    }
    if (num >= (((Word_t)1) << (1 * 1)))
    {
        __lb += 1;
    }
    return (__lb);
}
#endif  // not __GNUC__

// ****************************************************************************
// MISCELLANEOUS JUDY-SPECIFIC DECLARATIONS
// ****************************************************************************

// ROOT STATE:
//
// State at the start of the Judy SM, based on 1 byte decoded per state; equal
// to the number of bytes per Index to decode.

#define cJU_ROOTSTATE (cBPW)


// SUBEXPANSES PER STATE:
//
// Number of subexpanses per state traversed, which is the number of JPs in a
// branch (actual or theoretical) and the number of bits in a bitmap.

#define cJU_SUBEXPPERSTATE  256


// LEAF AND VALUE POINTERS:
//
// Some other basic object types are in declared in JudyPrivateBranch.h
// (Pjbl_t, Pjbb_t, Pjbu_t, Pjp_t) or are Judy1/L-specific (Pjlb_t).  The
// few remaining types are declared below.
//
// Note:  Leaf pointers are cast to different-sized objects depending on the
// leafs level, but are at least addresses (not just numbers), so use void *
// (Pvoid_t), not PWord_t or Word_t for them, except use Pjlw_t for whole-word
// (top-level, root-level) leaves.  Value areas, however, are always whole
// words.
//
// Furthermore, use Pjll_t only for generic leaf pointers (for various size
// LeafLs).  Use Pjlw_t for LeafWs.  Use Pleaf (with type uint8_t *, uint16_t
// *, etc) when the leaf index size is known.

typedef PWord_t Pjlw_t;  // pointer to root-level leaf (whole-word indexes).
typedef Pvoid_t Pjll_t;  // pointer to lower-level linear leaf.

#ifdef JUDYL
typedef PWord_t Pjv_t;   // pointer to JudyL value area.
#endif

// POINTER PREPARATION MACROS:
//
// These macros are used to strip malloc-namespace-type bits from a pointer +
// malloc-type word (which references any Judy mallocd object that might be
// obtained from other than a direct call of malloc()), prior to dereferencing
// the pointer as an address.  The malloc-type bits allow Judy mallocd objects
// to come from different "malloc() namespaces".
//
//    (root pointer)    (JRP, see above)
//    jp.jp_Addr        generic pointer to next-level node, except when used
//                      as a JudyL Immed01 value area
//    JU_JBB_PJP        macro hides jbbs_Pjp (pointer to JP subarray)
//    JL_JLB_PVALUE     macro hides jLlbs_PV_Raw (pointer to value subarray)
//
// When setting one of these fields or passing an address to j__udyFree*(), the
// "raw" memory address is used; otherwise the memory address must be passed
// through one of the macros below before its dereferenced.
//
// Note:  After much study, the typecasts below appear in the macros rather
// than at the point of use, which is both simpler and allows the compiler to
// do type-checking.


#define P_JLW(  ADDR) ((Pjlw_t) (ADDR))  // root leaf.
#define P_JPM(  ADDR) ((Pjpm_t) (ADDR))  // root JPM.
#define P_JBL(  ADDR) ((Pjbl_t) (ADDR))  // BranchL.
#define P_JBB(  ADDR) ((Pjbb_t) (ADDR))  // BranchB.
#define P_JBU(  ADDR) ((Pjbu_t) (ADDR))  // BranchU.
#define P_JLL(  ADDR) ((Pjll_t) (ADDR))  // LeafL.
#define P_JLB(  ADDR) ((Pjlb_t) (ADDR))  // LeafB1.
#define P_JP(   ADDR) ((Pjp_t)  (ADDR))  // JP.

#define P_JV(   ADDR) ((Pjv_t)  (ADDR))  // &value.

#ifdef  LATER
#ifdef JUDYL

#ifdef  JU_64BIT
// Strip hi 10 bits and low 4 bits
#define P_JV(   ADDR) ((Pjv_t)  (((Word_t)(ADDR)) & 0x3FFFFFFFFFFFF0))  // &value.
#endif  // JU_64BIT

#ifndef JU_64BIT
// Strip low 3 bits
#define P_JV(   ADDR) ((Pjv_t)  (((Word_t)(ADDR)) & 0xFFFFFFF8))  // &value.
#endif  // JU_32BIT

#endif  // JUDYL
#endif  //   LATER

// LEAST BYTES:
//
// Mask for least bytes of a word, and a macro to perform this mask on an
// Index.
//
// Note:  This macro has been problematic in the past to get right and to make
// portable.  Its not OK on all systems to shift by the full word size.  This
// macro should allow shifting by 1..N bytes, where N is the word size, but
// should produce a compiler warning if the macro is called with Bytes == 0.
//
// Warning:  JU_LEASTBYTESMASK() is not a constant macro unless Bytes is a
// constant; otherwise it is a variable shift, which is expensive on some
// processors.

#define JU_LEASTBYTESMASK(BYTES) \
        (((Word_t)0x100 << (cJU_BITSPERBYTE * ((BYTES) - 1))) - 1)

#define JU_LEASTBYTES(INDEX,BYTES)  ((INDEX) & JU_LEASTBYTESMASK(BYTES))


// BITS IN EACH BITMAP SUBEXPANSE FOR BITMAP BRANCH AND LEAF:
//
// The bits per bitmap subexpanse times the number of subexpanses equals a
// constant (cJU_SUBEXPPERSTATE).  You can also think of this as a compile-time
// choice of "aspect ratio" for bitmap branches and leaves (which can be set
// independently for each).
//
// A default aspect ratio is hardwired here if not overridden at compile time,
// such as by "EXTCCOPTS=-DBITMAP_BRANCH16x16 make".

#ifndef JU_64BIT
//////////////////////////////#if (! (defined(BITMAP_BRANCH8x32) || defined(BITMAP_BRANCH16x16) || defined(BITMAP_BRANCH32x8)))
#define BITMAP_BRANCH32x8 1     // 32 bits per subexpanse, 8 subexpanses.
/////////////////////#endif
#endif  // JU_32BIT

#ifdef  JU_64BIT
////////////////#if (! (defined(BITMAP_BRANCH8x32) || defined(BITMAP_BRANCH16x16) || defined(BITMAP_BRANCH32x8) || defined(BITMAP_BRANCH64x4)))
#define BITMAP_BRANCH64x4 1     // 64 bits per subexpanse, 4 subexpanses.
////////////////#endif
#endif  // JU_64BIT

#ifdef BITMAP_BRANCH8x32
//////////////////////#define BITMAPB_t uint8_t
#endif

#ifdef BITMAP_BRANCH16x16
//////////////////////#define BITMAPB_t uint16_t
#endif

#ifdef BITMAP_BRANCH32x8
#define BITMAPB_t uint32_t
#endif

#ifdef BITMAP_BRANCH64x4
#define BITMAPB_t uint64_t
#endif  

// Note:  For bitmap leaves, BITMAP_LEAF64x4 is only valid for 64 bit:
//
// Note:  Choice of aspect ratio mostly matters for JudyL bitmap leaves.  For
// Judy1 the choice doesnt matter much -- the code generated for different
// BITMAP_LEAF* values choices varies, but correctness and performance are the
// same.

#ifndef JU_64BIT
//////////////////////#if (! (defined(BITMAP_LEAF8x32) || defined(BITMAP_LEAF16x16) || defined(BITMAP_LEAF32x8)))
#define BITMAP_LEAF32x8         // 32 bits per subexpanse, 8 subexpanses.
//////////////////////#endif
#endif  // 32_BIT

#ifdef  JU_64BIT
//////////////////////#if (! (defined(BITMAP_LEAF8x32) || defined(BITMAP_LEAF16x16) || defined(BITMAP_LEAF32x8) || defined(BITMAP_LEAF64x4)))
#define BITMAP_LEAF64x4         // 64 bits per subexpanse, 4 subexpanses.
//////////////////////#endif
#endif // JU_64BIT

#ifdef BITMAP_LEAF8x32
//////////////////////#define BITMAPL_t uint8_t
#endif

#ifdef BITMAP_LEAF16x16
//////////////////////#define BITMAPL_t uint16_t
#endif

#ifdef BITMAP_LEAF32x8
#define BITMAPL_t uint32_t
#endif

#ifdef BITMAP_LEAF64x4
#define BITMAPL_t uint64_t
#endif


// EXPORTED DATA AND FUNCTIONS:

#ifdef JUDY1
extern const uint8_t j__1_BranchBJPPopToWords[];
#endif

#ifdef JUDYL
extern const uint8_t j__L_BranchBJPPopToWords[];
#endif

// Conversion size to Linear for a Binary Search
#ifndef LENSIZE
#define LENSIZE 4       // experimentally best - haswell in-cache
#endif  // LENSIZE

// The non-native searches (Leaf Key size = 3,5,6,7)
#define SEARCHLINARNONNAT(ADDR,POP1,INDEX,LFBTS,COPYINDEX,START)\
{                                                               \
    uint8_t *P_leaf;                                            \
    uint8_t *P_leafEnd;                                         \
    uint8_t  MSBIndex;                                          \
    Word_t   i_ndex;                                            \
    Word_t   I_ndex = JU_LEASTBYTES((INDEX), (LFBTS));          \
    (void)(START);                                              \
                                                                \
    P_leaf    = (uint8_t *)(ADDR);                              \
    P_leafEnd = P_leaf + (((POP1) - 1) * (LFBTS));              \
    COPYINDEX(i_ndex, P_leafEnd);                               \
    if (I_ndex > i_ndex) return(~(POP1));                       \
                                                                \
/*  These 2 lines of code are for high byte only candidate   */ \
    MSBIndex   = (uint8_t)(I_ndex >> (((LFBTS) - 1) * 8));      \
    while (MSBIndex > *P_leaf) P_leaf += (LFBTS);               \
                                                                \
    for (;;)                                                    \
    {                                                           \
        int     o_ff = 0;                                       \
        COPYINDEX(i_ndex, P_leaf);                              \
        if (I_ndex <= i_ndex)                                   \
        {                                                       \
            o_ff = (P_leaf - (uint8_t *)(ADDR)) / (LFBTS);      \
                                                                \
            MISSCOMPARES(o_ff);                                 \
                                                                \
            if (I_ndex != i_ndex) o_ff = ~o_ff;                 \
            return(o_ff);                                       \
        }                                                       \
        P_leaf += LFBTS;                                        \
    }                                                           \
}

// The native searches (Leaf Key size = 1,2,4
#define SEARCHLINARNATIVE(LEAFTYPE_t,ADDR,POP1,INDEX, START)    \
{                                                               \
    LEAFTYPE_t *P_leaf = (LEAFTYPE_t *)(ADDR);                  \
    LEAFTYPE_t  I_ndex = (INDEX); /* with masking */            \
    int         _off;                                           \
    (void)(START);                                              \
                                                                \
    if (I_ndex > P_leaf[(POP1) - 1]) return(~(POP1));           \
    while(I_ndex > *P_leaf) P_leaf++;                           \
    _off = P_leaf - (LEAFTYPE_t *)(ADDR);                       \
                                                                \
    MISSCOMPARES(_off);                                         \
                                                                \
    if (I_ndex != *P_leaf) _off = ~_off;                        \
    return(_off);                                               \
}

// This is binary search should be smarter and faster
// search in order middle, then half remainder until LENSIZE, then 8 linear

#define SEARCHBINARYNATIVE(LEAFTYPE_t,ADDR,POP1,INDEX, START)   \
{                                                               \
    LEAFTYPE_t *P_leaf = (LEAFTYPE_t *)(ADDR);                  \
    LEAFTYPE_t I_ndex = (LEAFTYPE_t)INDEX; /* truncate HiBits */\
    int       l_ow  = 0;                                        \
    int       h_igh = (int)(POP1);                              \
    int       m_id;                                             \
    (void)(START);                                              \
                                                                \
    while ((h_igh - l_ow) > LENSIZE)   /* Binary Search */      \
    {                                                           \
        MISSCOMPARES(1);                                        \
                                                                \
        m_id = (h_igh + l_ow) / 2;                              \
        if ((P_leaf)[m_id] <= (I_ndex))                         \
        {                                                       \
            if ((P_leaf)[m_id] == (I_ndex)) return(m_id);       \
            l_ow = m_id;                                        \
        }                                                       \
        else                                                    \
        {                                                       \
            h_igh = m_id;                                       \
        }                                                       \
    }                                                           \
                                                                \
    if ((h_igh == (POP1)) && ((I_ndex) > (P_leaf)[(POP1) - 1])) \
        return(~(POP1));                                        \
                                       /* Linear Search */      \
    while ((P_leaf)[l_ow] < (I_ndex))                           \
    {                                                           \
        MISSCOMPARES(1);                                        \
                                                                \
        l_ow++;                                                 \
    }                                                           \
    if ((P_leaf)[l_ow] != (I_ndex)) l_ow = ~l_ow;               \
    return(l_ow);                                               \
}

#define SEARCHBINARYNONNAT(ADDR,POP1,KEY,LFBTS,COPYINDEX,START) \
{                                                               \
    Word_t      __LeafKey;      /* current Key in Leaf */       \
    Word_t      __Key;          /* Key to search for */         \
    uint8_t      *__PLeaf;      /* ^ to Leaf to search */       \
    int         l_ow  = 0;                                      \
    int         h_igh = (int)(POP1);                            \
    int         m_id;                                           \
                                                                \
    __Key       = JU_LEASTBYTES((KEY), (LFBTS));                \
    __PLeaf = (uint8_t *)(ADDR);                                \
    (void)(START);                                              \
                                                                \
    while ((h_igh - l_ow) > LENSIZE)   /* Binary Search */      \
    {                                                           \
        m_id = (h_igh + l_ow) / 2;                              \
        COPYINDEX(__LeafKey, __PLeaf + (m_id * (LFBTS)));       \
                                                                \
        MISSCOMPARES(1);                                        \
                                                                \
        if (__LeafKey <= __Key)                                 \
        {                                                       \
            if (__LeafKey == __Key) return(m_id);               \
            l_ow = m_id;                                        \
        }                                                       \
        else                                                    \
        {                                                       \
            h_igh = m_id;                                       \
        }                                                       \
    }                                                           \
    if (h_igh == (POP1))                                        \
    {                                                           \
       COPYINDEX(__LeafKey, __PLeaf + (((POP1) - 1) * (LFBTS)));\
       if (__Key > __LeafKey) return(~(POP1));                  \
    }                                                           \
                   /* Linear Search */                          \
    COPYINDEX(__LeafKey, __PLeaf + (l_ow * (LFBTS)));           \
    while (__LeafKey < __Key)                                   \
    {                                                           \
        MISSCOMPARES(1);                                        \
                                                                \
        l_ow++;                                                 \
        COPYINDEX(__LeafKey, __PLeaf + (l_ow * (LFBTS)));       \
    }                                                           \
    if (__LeafKey != __Key) l_ow = ~l_ow;                       \
    return(l_ow);                                               \
}


// Use bits of the Key (INDEX) to determine where to start search.
// Good for populations up to 2..255
// NOTE: Should not be used for LeafW because LeafW is the only
// Leaf that can have a population == 1

// This macro can be used to "guess" the offset into a Leaf to 
// begin the search, given the Population and siginificant bits
// of the Leaf.  I.E.
//
//    Word_t MSb = (Word_t)(PLeaf[0] ^ PLeaf[(POP1) - 1]);
//    LOG2 = j__log2(MSb) + 1;
//    
//                     - or -
//
//    LOG2 = (cbPW - __builtin_clzl(MSb);
//
// This should be done in the JudyIns routine and passed to the
// JudyGet routine.
// Note:  The second form should work with a population == 1


// This routine will do a linear Search forward or reverse 
// beginning with element START.  It is specifically designed
// to "touch" as few bytes of RAM as possible, therefore limiting
// cache-line hits to few as possible.  Measurements show the
// typical accesses to RAM are about 3 adjacent elements.

#ifdef  EXPERMENT1
//             posidx = j__udySearchLeaf1IMM(Pjll, Pop1, Index)
//
#define LEAFKEY(LEAF, POS) (((LEAF) >> ((POS) * 8)) & 0xff)

#define SEARCHBIDIRNATIVEI(LEAFSIZE_t, PLEAF, POP1, KEY, START) \
{                                                               \
    Word_t      __Array;        /* up to 7 byte immed_01 */     \
    Word_t      __LeafKey;      /* current Key in Leaf */       \
    LEAFSIZE_t  __Key;          /* Key to search for */         \
    int         __pos;          /* starting pos of search */    \
                                                                \
    assert((Word_t)(START) < (Word_t)(POP1));                   \
    __Array  = *((PWord_t)(PLEAF));                             \
    __pos    = (int)(START);                                    \
    __Key    = (LEAFSIZE_t)(KEY);                               \
    __LeafKey = LEAFKEY(__Array,__pos);                         \
                                                                \
    if (__Key >= __LeafKey)     /* Search forward */            \
    {                                                           \
        if (__LeafKey == __Key) /* 1st check if direct hit */   \
        {                                                       \
            DIRECTHITS(START, __pos, sizeof(__Key));            \
            return(__pos);      /* Nailed it */                 \
        }                                                       \
        for (__pos++; __pos < (POP1); __pos++)                  \
        {                                                       \
            __LeafKey = LEAFKEY(__Array,__pos);                 \
            if (__LeafKey >= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISSCOMPARES(__pos - (START));              \
            /*      DIRECTHITS((START), __pos, sizeof(__Key));*/\
                    return(__pos);                              \
                }                                               \
                break;                                          \
            }                                                   \
        }                                                       \
    }                                                           \
    else                /* Search in reverse */                 \
    {                                                           \
        while (__pos)                                           \
        {                                                       \
            __pos--;                                            \
            __LeafKey = LEAFKEY(__Array,__pos);                 \
                                                                \
            if (__LeafKey <= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISSCOMPARES((START) - __pos);              \
             /*     DIRECTHITS((START), __pos, sizeof(__Key));*/\
                    return(__pos);                              \
                }                                               \
                __pos++;  /* advance to hole */                 \
                break;                                          \
            }                                                   \
        }                                                       \
    }                                                           \
    return (~__pos);       /* ones comp location of hole */     \
}
#endif  // EXPERMENT1 

#define SEARCHBIDIRNATIVE(LEAFSIZE_t, PLEAF, POP1, KEY, START)  \
{                                                               \
    Word_t      __LeafKey;      /* current Key in Leaf */       \
    LEAFSIZE_t  __Key;          /* Key to search for */         \
    LEAFSIZE_t  *__PLeaf;       /* ^ toLeaf to search */        \
    int         __pos;          /* starting pos of search */    \
                                                                \
    assert((Word_t)(START) < (Word_t)(POP1));                   \
    __pos    = (int)(START);                                    \
    __Key    = (LEAFSIZE_t)(KEY);                               \
    __PLeaf  = (LEAFSIZE_t *)(PLEAF);                           \
    __LeafKey = (Word_t)__PLeaf[__pos];                         \
                                                                \
    if (__Key >= __LeafKey)     /* Search forward */            \
    {                                                           \
        if (__LeafKey == __Key) /* 1st check if direct hit */   \
        {                                                       \
            DIRECTHITS((START), __pos, sizeof(__Key));          \
            return(__pos);  /* Nailed it */                     \
        }                                                       \
        for (__pos++; __pos < (POP1); __pos++)                  \
        {                                                       \
            __LeafKey = __PLeaf[__pos];                         \
            if (__LeafKey >= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISSCOMPARES(__pos - (START));              \
             /*     DIRECTHITS((START), __pos, sizeof(__Key));*/\
                    return(__pos);                              \
                }                                               \
                break;                                          \
            }                                                   \
        }                                                       \
    }                                                           \
    else                /* Search in reverse */                 \
    {                                                           \
        while (__pos)                                           \
        {                                                       \
            __pos--;                                            \
            __LeafKey = __PLeaf[__pos];                         \
                                                                \
            if (__LeafKey <= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISSCOMPARES((START) - __pos);              \
             /*     DIRECTHITS((START), __pos, sizeof(__Key));*/\
                    return(__pos);                              \
                }                                               \
                __pos++;  /* advance to hole */                 \
                break;                                          \
            }                                                   \
        }                                                       \
    }                                                           \
    return (~__pos);       /* ones comp location of hole */     \
}

// This routine will do a linear search forward or reverse 
// beginning with element START 

#define SEARCHBIDIRNONNAT(ADDR,POP1,KEY,LFBTS,COPYINDEX,START)  \
{                                                               \
    Word_t      __LeafKey;      /* current Key in Leaf */       \
    Word_t      __Key;          /* Key to search for */         \
    uint8_t      *__PLeaf;      /* ^ to Leaf to search */       \
    int         __pos;          /* starting pos of search */    \
                                                                \
    assert((Word_t)(START) < (Word_t)(POP1));                   \
    __pos       = (int)(START);                                 \
    __Key       = JU_LEASTBYTES((KEY), (LFBTS));                \
    __PLeaf     = (uint8_t *)(ADDR);                            \
    COPYINDEX(__LeafKey, __PLeaf + (__pos * (LFBTS)));          \
                                                                \
    if (__LeafKey == __Key)                                     \
    {                                                           \
        DIRECTHITS((START), __pos, sizeof(__Key));              \
                                                                \
        return(__pos);  /* Nailed it */                         \
    }                                                           \
    MISSCOMPARES(1);                                            \
                                                                \
    if (__Key > __LeafKey)                                      \
    {                   /* Search forward */                    \
        for (__pos++; __pos < (POP1); __pos++)                  \
        {                                                       \
            COPYINDEX(__LeafKey, __PLeaf + (__pos * (LFBTS)));  \
            if (__LeafKey >= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
             /*     DIRECTHITS((START), __pos, sizeof(__Key));*/\
                    return(__pos);                              \
                }                                               \
                break;                                          \
            }                                                   \
            MISSCOMPARES(1);                                    \
        }                                                       \
    }                                                           \
    else                /* Search in reverse */                 \
    {                                                           \
        while (__pos)                                           \
        {                                                       \
            __pos--;                                            \
            COPYINDEX(__LeafKey, __PLeaf + (__pos * (LFBTS)));  \
            if (__LeafKey <= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
             /*     DIRECTHITS((START), __pos, sizeof(__Key));*/\
                    return(__pos);                              \
                }                                               \
                __pos++;  /* advance to hole */                 \
                break;                                          \
            }                                                   \
            MISSCOMPARES(1);                                    \
        }                                                       \
    }                                                           \
    return (~__pos);       /* ones comp location of hole */     \
}

// The native searches (Leaf Key size = 1,2,4,8)
#if (! defined(SEARCH_BINARY)) && (! defined(SEARCH_LINEAR)) && (! defined(SEARCH_PSPLIT))
// default a search leaf method
#define SEARCH_PSPLIT 
#endif  // (! defined(SEARCH_BINARY)) && (! defined(SEARCH_LINEAR)) && (! defined(SEARCH_PSPLIT))


// search in order 1,2,3,4..
#ifdef  SEARCH_LINEAR
#define SEARCHLEAFNATIVE        SEARCHLINARNATIVE
#define SEARCHLEAFNONNAT        SEARCHLINARNONNAT
#endif  // SEARCH_LINEAR


// search in order middle, then half remainder
#ifdef SEARCH_BINARY
#define SEARCHLEAFNATIVE        SEARCHBINARYNATIVE
#define SEARCHLEAFNONNAT        SEARCHBINARYNONNAT
#endif // SEARCH_BINARY


#ifdef SEARCH_PSPLIT
#define SEARCHLEAFNATIVE        SEARCHBIDIRNATIVE
#define SEARCHLEAFNONNAT        SEARCHBIDIRNONNAT
#endif  // SEARCH_PSPLIT

// Fast way to count bits set in 8..32[64]-bit int:
//
// For performance, j__udyCountBits*() are written to take advantage of
// platform-specific features where available.
//
// A good compiler should recognize this code sequence and decide if a
// popcnt and/or a imull instruction should be used. (dlb2012)

#ifdef __POPCNT__

static inline int
j__udyCount32Bits(uint32_t word32)
{
     return ((int)__builtin_popcount(word32));
}

static inline int
j__udyCount64Bits(uint64_t word64)
{
     return ((int)__builtin_popcountl(word64));
}

#else   // ! __POPCNT__

// Hopefully non-X86 compilers will recognize this and use popc replacement
static inline int
j__udyCount32Bits(uint32_t word32)
{
//  Calculate each nibble to have counts of 0..4 bits in each nibble.
    word32 -= (word32 >> 1) & (uint32_t)0x55555555;
    word32 = ((word32 >> 2) & (uint32_t)0x33333333) + 
                   (word32  & (uint32_t)0x33333333);

//  Odd nibbles += even nibbles (in parallel)
    word32 += word32 >> 4;

//  Clean out the even nibbles for some calculating space
    word32 &= (uint32_t)0x0F0F0F0F;

//  Now sum the 4 bytes of bit counts of 0..8 bits each in odd nibble.
    word32 *= (uint32_t)0x01010101;               // sums bytes (1 instruction)
    word32  = word32 >> (32 - 8);       // sum in high byte

    return ((int)word32);
}

static inline int
j__udyCount64Bits(uint64_t word64)
{
//  Calculate each nibble to have counts of 0..4 bits in each nibble.
    word64 -= (word64 >> 1) & (uint64_t)0x5555555555555555;
    word64 = ((word64 >> 2) & (uint64_t)0x3333333333333333) + 
                    (word64 & (uint64_t)0x3333333333333333);

//  Odd nibbles += even nibbles (in parallel)
    word64 += word64 >> 4;

//  Clean out the even nibbles for some calculating space
    word64 &= (uint64_t)0x0F0F0F0F0F0F0F0F;    // sums bytes (1 instruction)

//  Now sum the 8 bytes of bit counts of 0..8 bits each in odd nibble.
    word64 *= (uint64_t)0x0101010101010101;
    word64  = word64 >> (64 - 8);       // sum in high byte

    return ((int)word64);                       // 0..64
}
#endif  // ! __POPCNT__

// Always 32 bits or less for 32 and 64Bit machines

#ifndef JU_64BIT
#define j__udyCountBitsL j__udyCount32Bits
#define j__udyCountBitsB j__udyCount32Bits
#endif  // JU_32BIT

#ifdef  JU_64BIT
#define j__udyCountBitsL j__udyCount64Bits

#ifdef BITMAP_BRANCH64x4
#define j__udyCountBitsB j__udyCount64Bits
#else   // 32x8 Bits
#define j__udyCountBitsB j__udyCount32Bits
#endif  // ! BITMAP_BRANCH64x4

#endif  // JU_64BIT

// GET POP0:
//
// Get from jp_DcdPopO the Pop0 for various JP Types.
//
// Notes:
//
// - Different macros require different parameters...
//
// - There are no simple macros for cJU_BRANCH* Types because their
//   populations must be added up and dont reside in an already-calculated
//   place.  (TBD:  This is no longer true, now its in the JPM.)
//
// - cJU_JPIMM_POP0() is not defined because it would be redundant because the
//   Pop1 is already encoded in each enum name.
//
// - A linear or bitmap leaf Pop0 cannot exceed cJU_SUBEXPPERSTATE - 1 (Pop0 =
//   0..255), so use a simpler, faster macro for it than for other JP Types.
//
// - Avoid any complex calculations that would slow down the compiled code.
//   Assume these macros are only called for the appropriate JP Types.
//   Unfortunately theres no way to trigger an assertion here if the JP type
//   is incorrect for the macro, because these are merely expressions, not
//   statements.

#define  JU_LEAFW_POP0(JRP)                  (*P_JLW(JRP))
#define cJU_JPFULLPOPU1_POP0                 (cJU_SUBEXPPERSTATE - 1)

// GET JP Type:
// Since bit fields greater than 32 bits are not supported in some compilers
// the jp_DcdPopO field is expanded to include the jp_Type in the high 8 bits
// of the Word_t.
// First the read macro:

#define JU_JPTYPE(PJP)          ((PJP)->jp_Type)

#ifndef  SWAP
#define JU_JPLEAF_POP0(PJP)     ((PJP)->jp_Addr1 & 0xFF)

#ifdef  JU_LITTLE_ENDIAN        // ====================================
#define JU_JPDCDPOP0(PJP)   JU_TRIMTODCDSIZE((PJP)->jp_Addr1)

#define JU_JPSETADT(PJP,ADDR,DCDPOP0,TYPE)                              \
{                                                                       \
    (PJP)->jp_Addr  = (ADDR);                                           \
    (PJP)->jp_Addr1 = (JU_TRIMTODCDSIZE(DCDPOP0)                        \
            | (((Word_t)(TYPE) << ((sizeof(Word_t)-1) * 8))));          \
}
#endif  // LITTLE_ENDIAN =================================================

#else   // SWAP

// define the JU_SWAP macro

#ifndef JU_64BIT
#define JU_SWAP(WORD)   ((Word_t)__builtin_bswap32(WORD))
#endif  // JU_32BIT

#ifdef  JU_64BIT
#define JU_SWAP(WORD)   ((Word_t)__builtin_bswap64(WORD))
#endif  // JU_64BIT

#define JU_JPLEAF_POP0(PJP)     ((PJP)->jp_DcdP0[cBPW - 2])

#ifdef  JU_LITTLE_ENDIAN        // ====================================
#define JU_JPDCDPOP0(PJP)  (JU_SWAP((PJP)->jp_Addr1) >> 8)

#define JU_JPSETADT(PJP,ADDR,DCDPOP0,TYPE)                              \
{                                                                       \
    (PJP)->jp_Addr  = (ADDR);                                           \
    (PJP)->jp_Addr1 = JU_SWAP(((DCDPOP0) << 8) | (uint8_t)(TYPE));      \
}

#else   // BIG_ENDIAN =================================================

#define JU_JPDCDPOP0(PJP)  (((PJP)->jp_Addr1) >> 8)

#define JU_JPSETADT(PJP,ADDR,DCDPOP0,TYPE)                              \
{                                                                       \
    (PJP)->jp_Addr  = (ADDR);                                           \
    (PJP)->jp_Addr1 = ((DCDPOP0) << 8) | (uint8_t)(TYPE);               \
}
#endif  // BIG_ENDIAN =================================================

#endif  // SWAP

// NUMBER OF BITS IN A BRANCH OR LEAF BITMAP AND SUBEXPANSE:
//
// Note:  cJU_BITSPERBITMAP must be the same as the number of JPs in a branch.

#define cJU_BITSPERBITMAP cJU_SUBEXPPERSTATE

// Bitmaps are accessed in units of "subexpanses":

#define cJU_BITSPERSUBEXPB  (sizeof(BITMAPB_t) * cJU_BITSPERBYTE)       // 4[8]
#define cJU_NUMSUBEXPB      (cJU_BITSPERBITMAP / cJU_BITSPERSUBEXPB)    // 128[64]

#define cJU_BITSPERSUBEXPL  (sizeof(BITMAPL_t) * cJU_BITSPERBYTE)
#define cJU_NUMSUBEXPL      (cJU_BITSPERBITMAP / cJU_BITSPERSUBEXPL)


// MASK FOR A SPECIFIED BIT IN A BITMAP:
//
// Warning:  If BitNum is a variable, this results in a variable shift that is
// expensive, at least on some processors.  Use with caution.
//
// Warning:  BitNum must be less than cJU_BITSPERWORD, that is, 0 ..
// cJU_BITSPERWORD - 1, to avoid a truncated shift on some machines.
//
#define JU_BITPOSMASKB(BITNUM) ((BITMAPB_t)1 << ((BITNUM) % cJU_BITSPERSUBEXPB))
#define JU_BITPOSMASKL(BITNUM) ((BITMAPL_t)1 << ((BITNUM) % cJU_BITSPERSUBEXPL))


// TEST/SET/CLEAR A BIT IN A BITMAP LEAF:
//
// Test if a byte-sized Digit (portion of Index) has a corresponding bit set in
// a bitmap, or set a byte-sized Digits bit into a bitmap, by looking up the
// correct subexpanse and then checking/setting the correct bit.
//
// Note:  Mask higher bits, if any, for the convenience of the user of this
// macro, in case they pass a full Index, not just a digit.  If the caller has
// a true 8-bit digit, make it of type uint8_t and the compiler should skip the
// unnecessary mask step.

#define JU_SUBEXPL(DIGIT) (((DIGIT) / cJU_BITSPERSUBEXPL) & (cJU_NUMSUBEXPL-1))

#define JU_BITMAPTESTL(PJLB, INDEX)  \
    (JU_JLB_BITMAP(PJLB, JU_SUBEXPL(INDEX)) &  JU_BITPOSMASKL(INDEX))

#define JU_BITMAPSETL(PJLB, INDEX)   \
    (JU_JLB_BITMAP(PJLB, JU_SUBEXPL(INDEX)) |= JU_BITPOSMASKL(INDEX))

#define JU_BITMAPCLEARL(PJLB, INDEX) \
    (JU_JLB_BITMAP(PJLB, JU_SUBEXPL(INDEX)) ^= JU_BITPOSMASKL(INDEX))


// MAP BITMAP BIT OFFSET TO DIGIT:
//
// Given a digit variable to set, a bitmap branch or leaf subexpanse (base 0),
// the bitmap (BITMAP*_t) for that subexpanse, and an offset (Nth set bit in
// the bitmap, base 0), compute the digit (also base 0) corresponding to the
// subexpanse and offset by counting all bits in the bitmap until offset+1 set
// bits are seen.  Avoid expensive variable shifts.  Offset should be less than
// the number of set bits in the bitmap; assert this.
//
// If theres a better way to do this, I dont know what it is.

#define JU_BITMAPDIGITB(DIGIT,SUBEXP,BITMAP,OFFSET)             \
        {                                                       \
            BITMAPB_t bitmap = (BITMAP); int remain = (OFFSET); \
            (DIGIT) = (SUBEXP) * cJU_BITSPERSUBEXPB;            \
                                                                \
            while ((remain -= (bitmap & 1)) >= 0)               \
            {                                                   \
                bitmap >>= 1; ++(DIGIT);                        \
                assert((DIGIT) < ((SUBEXP) + 1) * cJU_BITSPERSUBEXPB); \
            }                                                   \
        }

#define JU_BITMAPDIGITL(DIGIT,SUBEXP,BITMAP,OFFSET)             \
        {                                                       \
            BITMAPL_t bitmap = (BITMAP); int remain = (OFFSET); \
            (DIGIT) = (SUBEXP) * cJU_BITSPERSUBEXPL;            \
                                                                \
            while ((remain -= (bitmap & 1)) >= 0)               \
            {                                                   \
                bitmap >>= 1; ++(DIGIT);                        \
                assert((DIGIT) < ((SUBEXP) + 1) * cJU_BITSPERSUBEXPL); \
            }                                                   \
        }


// MASKS FOR PORTIONS OF 32-BIT WORDS:
//
// These are useful for bitmap subexpanses.
//
// "LOWER"/"HIGHER" means bits representing lower/higher-valued Indexes.  The
// exact order of bits in the word is explicit here but is hidden from the
// caller.
//
// "EXC" means exclusive of the specified bit; "INC" means inclusive.
//
// In each case, BitPos is either "JU_BITPOSMASK*(BitNum)", or a variable saved
// from an earlier call of that macro; either way, it must be a 32-bit word
// with a single bit set.  In the first case, assume the compiler is smart
// enough to optimize out common subexpressions.
//
// The expressions depend on unsigned decimal math that should be universal.

#define JU_MASKLOWEREXC( BITPOS)  ((BITPOS) - 1)
#define JU_MASKLOWERINC( BITPOS)  (JU_MASKLOWEREXC(BITPOS) | (BITPOS))
#define JU_MASKHIGHERINC(BITPOS)  (-(BITPOS))
#define JU_MASKHIGHEREXC(BITPOS)  (JU_MASKHIGHERINC(BITPOS) ^ (BITPOS))


// ****************************************************************************
// SUPPORT FOR NATIVE INDEX SIZES
// ****************************************************************************
//
// Copy a series of generic objects (uint8_t, uint16_t, uint32_t, Word_t) from
// one place to another.

#ifdef  MEMMOVE
#define JU_COPYMEM(PDST,PSRC,POP1)                      \
    {                                                   \
        Word_t i_ndex = 0;                              \
        assert((POP1) > 0);                             \
        do { (PDST)[i_ndex] = (PSRC)[i_ndex]; } \
        while (++i_ndex < (POP1));                      \
    }
#else   // ! MEMMOVE
#define JU_COPYMEM(PDST,PSRC,POP1)                      \
    {                                                   \
        Word_t i_ndex = 0;                              \
        assert((POP1) > 0);                             \
        do { (PDST)[i_ndex] = (PSRC)[i_ndex]; } \
        while (++i_ndex < (POP1));                      \
    }
#endif  // ! MEMMOVE


// ****************************************************************************
// SUPPORT FOR NON-NATIVE INDEX SIZES
// ****************************************************************************
//
// Copy a 3-byte Index pointed by a uint8_t * to a Word_t:
//
#define JU_COPY3_PINDEX_TO_LONG(DESTLONG,PINDEX)        \
    DESTLONG  = (Word_t)(PINDEX)[0] << 16;              \
    DESTLONG += (Word_t)(PINDEX)[1] <<  8;              \
    DESTLONG += (Word_t)(PINDEX)[2]

// Copy a Word_t to a 3-byte Index pointed at by a uint8_t *:

#define JU_COPY3_LONG_TO_PINDEX(PINDEX,SOURCELONG)      \
    (PINDEX)[0] = (uint8_t)((SOURCELONG) >> 16);        \
    (PINDEX)[1] = (uint8_t)((SOURCELONG) >>  8);        \
    (PINDEX)[2] = (uint8_t)((SOURCELONG))

#ifdef JU_64BIT

// Copy a 5-byte Index pointed by a uint8_t * to a Word_t:
//
#define JU_COPY5_PINDEX_TO_LONG(DESTLONG,PINDEX)        \
    DESTLONG  = (Word_t)(PINDEX)[0] << 32;              \
    DESTLONG += (Word_t)(PINDEX)[1] << 24;              \
    DESTLONG += (Word_t)(PINDEX)[2] << 16;              \
    DESTLONG += (Word_t)(PINDEX)[3] <<  8;              \
    DESTLONG += (Word_t)(PINDEX)[4]

// Copy a Word_t to a 5-byte Index pointed at by a uint8_t *:

#define JU_COPY5_LONG_TO_PINDEX(PINDEX,SOURCELONG)      \
    (PINDEX)[0] = (uint8_t)((SOURCELONG) >> 32);        \
    (PINDEX)[1] = (uint8_t)((SOURCELONG) >> 24);        \
    (PINDEX)[2] = (uint8_t)((SOURCELONG) >> 16);        \
    (PINDEX)[3] = (uint8_t)((SOURCELONG) >>  8);        \
    (PINDEX)[4] = (uint8_t)((SOURCELONG))

// Copy a 6-byte Index pointed by a uint8_t * to a Word_t:
//
#define JU_COPY6_PINDEX_TO_LONG(DESTLONG,PINDEX)        \
    DESTLONG  = (Word_t)(PINDEX)[0] << 40;              \
    DESTLONG += (Word_t)(PINDEX)[1] << 32;              \
    DESTLONG += (Word_t)(PINDEX)[2] << 24;              \
    DESTLONG += (Word_t)(PINDEX)[3] << 16;              \
    DESTLONG += (Word_t)(PINDEX)[4] <<  8;              \
    DESTLONG += (Word_t)(PINDEX)[5]

// Copy a Word_t to a 6-byte Index pointed at by a uint8_t *:

#define JU_COPY6_LONG_TO_PINDEX(PINDEX,SOURCELONG)      \
    (PINDEX)[0] = (uint8_t)((SOURCELONG) >> 40);        \
    (PINDEX)[1] = (uint8_t)((SOURCELONG) >> 32);        \
    (PINDEX)[2] = (uint8_t)((SOURCELONG) >> 24);        \
    (PINDEX)[3] = (uint8_t)((SOURCELONG) >> 16);        \
    (PINDEX)[4] = (uint8_t)((SOURCELONG) >>  8);        \
    (PINDEX)[5] = (uint8_t)((SOURCELONG))

// Copy a 7-byte Index pointed by a uint8_t * to a Word_t:
//
#define JU_COPY7_PINDEX_TO_LONG(DESTLONG,PINDEX)        \
    DESTLONG  = (Word_t)(PINDEX)[0] << 48;              \
    DESTLONG += (Word_t)(PINDEX)[1] << 40;              \
    DESTLONG += (Word_t)(PINDEX)[2] << 32;              \
    DESTLONG += (Word_t)(PINDEX)[3] << 24;              \
    DESTLONG += (Word_t)(PINDEX)[4] << 16;              \
    DESTLONG += (Word_t)(PINDEX)[5] <<  8;              \
    DESTLONG += (Word_t)(PINDEX)[6]

// Copy a Word_t to a 7-byte Index pointed at by a uint8_t *:

#define JU_COPY7_LONG_TO_PINDEX(PINDEX,SOURCELONG)      \
    (PINDEX)[0] = (uint8_t)((SOURCELONG) >> 48);        \
    (PINDEX)[1] = (uint8_t)((SOURCELONG) >> 40);        \
    (PINDEX)[2] = (uint8_t)((SOURCELONG) >> 32);        \
    (PINDEX)[3] = (uint8_t)((SOURCELONG) >> 24);        \
    (PINDEX)[4] = (uint8_t)((SOURCELONG) >> 16);        \
    (PINDEX)[5] = (uint8_t)((SOURCELONG) >>  8);        \
    (PINDEX)[6] = (uint8_t)((SOURCELONG))

#endif // JU_64BIT

// ****************************************************************************
// COMMON CODE FRAGMENTS (MACROS)
// ****************************************************************************
//
// These code chunks are shared between various source files.


// SET (REPLACE) ONE DIGIT IN AN INDEX:
//
// To avoid endian issues, use masking and ORing, which operates in a
// big-endian register, rather than treating the Index as an array of bytes,
// though that would be simpler, but would operate in endian-specific memory.
//
// TBD:  This contains two variable shifts, is that bad?

#define JU_SETDIGIT(INDEX,DIGIT,STATE)                  \
        (INDEX) = ((INDEX) & (~cJU_MASKATSTATE(STATE))) \
                           | (((Word_t) (DIGIT))        \
                              << (((STATE) - 1) * cJU_BITSPERBYTE))

// Fast version for single LSB:

#define JU_SETDIGIT1(INDEX,DIGIT) (INDEX) = ((INDEX) & ~0xff) | (DIGIT)


// SET (REPLACE) "N" LEAST DIGITS IN AN INDEX:

#define JU_SETDIGITS(INDEX,INDEX2,cSTATE) \
        (INDEX) = ((INDEX ) & (~JU_LEASTBYTESMASK(cSTATE))) \
                | ((INDEX2) & ( JU_LEASTBYTESMASK(cSTATE)))

// COPY DECODE BYTES FROM JP TO INDEX:
//
// Modify Index digit(s) to match the bytes in jp_DcdPopO in case one or more
// branches are skipped and the digits are significant.  Its probably faster
// to just do this unconditionally than to check if its necessary.
//
// To avoid endian issues, use masking and ORing, which operates in a
// big-endian register, rather than treating the Index as an array of bytes,
// though that would be simpler, but would operate in endian-specific memory.
//
// WARNING:  Must not call JU_LEASTBYTESMASK (via cJU_DCDMASK) with Bytes =
// cJU_ROOTSTATE or a bad mask is generated, but there are no Dcd bytes to copy
// in this case anyway.  In fact there are no Dcd bytes unless State <
// cJU_ROOTSTATE - 1, so dont call this macro except in those cases.
//
// TBD:  It would be nice to validate jp_DcdPopO against known digits to ensure
// no corruption, but this is non-trivial.

#define JU_SETDCD(INDEX,PJP,cSTATE)                             \
    (INDEX) = ((INDEX) & ~cJU_DCDMASK(cSTATE))                  \
                | (JU_JPDCDPOP0(PJP) & cJU_DCDMASK(cSTATE))

// INSERT/DELETE AN INDEX IN-PLACE IN MEMORY:
//
// Given a pointer to an array of "even" (native), same-sized objects
// (indexes), the current population of the array, an offset in the array, and
// a new Index to insert, "shift up" the array elements (Indexes) above the
// insertion point and insert the new Index.  Assume there is sufficient memory
// to do this.
//
// In these macros, "i_offset" is an index offset, and "b_off" is a byte
// offset for odd Index sizes.
//
// Note:  Endian issues only arise for insertion, not deletion, and even for
// insertion, they are transparent when native (even) objects are used, and
// handled explicitly for odd (non-native) Index sizes.
//
// Note:  The following macros are tricky enough that there is some test code
// for them appended to this file.

#ifdef  MEMMOVE
#include <string.h> 
#define JU_INSERTINPLACE(PARRAY,POP1,OFFSET,INDEX)              \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) <= (Word_t) (POP1));           \
        {                                                       \
            size_t n = ((POP1)-(OFFSET)) * sizeof(*(PARRAY));   \
            void   *src  = (void *)((PARRAY) + (OFFSET));       \
            void   *dest = src + sizeof(*(PARRAY));             \
            memmove(dest, src, n);                              \
            (PARRAY)[OFFSET] = (INDEX);                         \
        }

#else   // ! MEMMOVE
#define JU_INSERTINPLACE(PARRAY,POP1,OFFSET,INDEX)              \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) <= (Word_t) (POP1));           \
        {                                                       \
            int    i_offset = (int)(POP1);                      \
                                                                \
            while (i_offset-- > (OFFSET))                       \
                (PARRAY)[i_offset + 1] = (PARRAY)[i_offset];    \
                                                                \
            (PARRAY)[OFFSET] = (INDEX);                         \
        }
#endif  // ! MEMMOVE


// Variation for non-native Indexes, where cIS = Index Size
// and PByte must point to a uint8_t (byte); shift byte-by-byte:
//

#ifdef  MEMMOVE
#define JU_INSERTINPLACE3(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    size_t n = ((POP1)-(OFFSET)) * 3;                           \
    void   *src  = (void *)((PBYTE) + ((OFFSET) * 3));          \
    void   *dest = src + 3;                                     \
    memmove(dest, src, n);                                      \
    JU_COPY3_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 3]), INDEX);   \
}
#else   // ! MEMMOVE
#define JU_INSERTINPLACE3(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    int    i_off = POP1;                                        \
                                                                \
    while (i_off-- > (OFFSET))                                  \
    {                                                           \
        Word_t  i_dx = i_off * 3;                               \
        (PBYTE)[i_dx + 0 + 3] = (PBYTE)[i_dx + 0];              \
        (PBYTE)[i_dx + 1 + 3] = (PBYTE)[i_dx + 1];              \
        (PBYTE)[i_dx + 2 + 3] = (PBYTE)[i_dx + 2];              \
    }                                                           \
    JU_COPY3_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 3]), INDEX);   \
}
#endif  // ! MEMMOVE

#ifdef JU_64BIT

#ifdef  MEMMOVE
#define JU_INSERTINPLACE5(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    size_t n = ((POP1)-(OFFSET)) * 5;                           \
    void   *src  = (void *)((PBYTE) + ((OFFSET) * 5));          \
    void   *dest = src + 5;                                     \
    memmove(dest, src, n);                                      \
    JU_COPY5_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 5]), INDEX);   \
}
#else   // ! MEMMOVE
#define JU_INSERTINPLACE5(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    int    i_off = POP1;                                        \
                                                                \
    while (i_off-- > (OFFSET))                                  \
    {                                                           \
        Word_t  i_dx = i_off * 5;                               \
        (PBYTE)[i_dx + 0 + 5] = (PBYTE)[i_dx + 0];              \
        (PBYTE)[i_dx + 1 + 5] = (PBYTE)[i_dx + 1];              \
        (PBYTE)[i_dx + 2 + 5] = (PBYTE)[i_dx + 2];              \
        (PBYTE)[i_dx + 3 + 5] = (PBYTE)[i_dx + 3];              \
        (PBYTE)[i_dx + 4 + 5] = (PBYTE)[i_dx + 4];              \
    }                                                           \
    JU_COPY5_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 5]), INDEX);   \
}
#endif  // ! MEMMOVE

#ifdef  MEMMOVE
#define JU_INSERTINPLACE6(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    size_t n = ((POP1)-(OFFSET)) * 6;                           \
    void   *src  = (void *)((PBYTE) + ((OFFSET) * 6));          \
    void   *dest = src + 6;                                     \
    memmove(dest, src, n);                                      \
    JU_COPY6_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 6]), INDEX);   \
}
#else   // ! MEMMOVE
#define JU_INSERTINPLACE6(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    int    i_off = POP1;                                        \
                                                                \
    while (i_off-- > (OFFSET))                                  \
    {                                                           \
        Word_t  i_dx = i_off * 6;                               \
        (PBYTE)[i_dx + 0 + 6] = (PBYTE)[i_dx + 0];              \
        (PBYTE)[i_dx + 1 + 6] = (PBYTE)[i_dx + 1];              \
        (PBYTE)[i_dx + 2 + 6] = (PBYTE)[i_dx + 2];              \
        (PBYTE)[i_dx + 3 + 6] = (PBYTE)[i_dx + 3];              \
        (PBYTE)[i_dx + 4 + 6] = (PBYTE)[i_dx + 4];              \
        (PBYTE)[i_dx + 5 + 6] = (PBYTE)[i_dx + 5];              \
    }                                                           \
    JU_COPY6_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 6]), INDEX);   \
}
#endif  // ! MEMMOVE

#ifdef  MEMMOVE
#define JU_INSERTINPLACE7(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    size_t n = ((POP1)-(OFFSET)) * 7;                           \
    void   *src  = (void *)((PBYTE) + ((OFFSET) * 7));          \
    void   *dest = src + 7;                                     \
    memmove(dest, src, n);                                      \
    JU_COPY7_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 7]), INDEX);   \
}
#else   // ! MEMMOVE
#define JU_INSERTINPLACE7(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    int    i_off = POP1;                                        \
                                                                \
    while (i_off-- > (OFFSET))                                  \
    {                                                           \
        Word_t  i_dx = i_off * 7;                               \
        (PBYTE)[i_dx + 0 + 7] = (PBYTE)[i_dx + 0];              \
        (PBYTE)[i_dx + 1 + 7] = (PBYTE)[i_dx + 1];              \
        (PBYTE)[i_dx + 2 + 7] = (PBYTE)[i_dx + 2];              \
        (PBYTE)[i_dx + 3 + 7] = (PBYTE)[i_dx + 3];              \
        (PBYTE)[i_dx + 4 + 7] = (PBYTE)[i_dx + 4];              \
        (PBYTE)[i_dx + 5 + 7] = (PBYTE)[i_dx + 5];              \
        (PBYTE)[i_dx + 6 + 7] = (PBYTE)[i_dx + 6];              \
    }                                                           \
    JU_COPY7_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 7]), INDEX);   \
}
#endif  // ! MEMMOVE

#endif // JU_64BIT

// Counterparts to the above for deleting an Index:
//
// "Shift down" the array elements starting at the Index to be deleted.

#define JU_DELETEINPLACE(PARRAY,POP1,OFFSET,IGNORE)             \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            int    i_offset = (OFFSET);                         \
                                                                \
            while (++i_offset < (int)(POP1))                    \
            {                                                   \
                (PARRAY)[i_offset - 1] = (PARRAY)[i_offset];    \
            }                                                   \
     /*       (PARRAY)[(POP1) - 1] = 0;     zero pad */         \
            (PARRAY)[i_offset - 1] = 0;     /* zero pad */      \
        }

#define JU_DELETEINPLACEJP(PARRAY,POP1,OFFSET,IGNORE)           \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            int    i_offset = (OFFSET);                         \
            while (++i_offset < (int)(POP1))                    \
            {                                                   \
                (PARRAY)[i_offset - 1] = (PARRAY)[i_offset];    \
            }                                                   \
        }

// Variation for odd-byte-sized (non-native) Indexes, where cIS = Index Size
// and PByte must point to a uint8_t (byte); copy byte-by-byte:
//
// Note:  If cIS == 1, JU_DELETEINPLACE_ODD == JU_DELETEINPLACE.
//
// Note:  There are no endian issues here because bytes are just shifted as-is,
// not converted to/from an Index.

#define JU_DELETEINPLACE_ODD(PBYTE,POP1,OFFSET,cIS)             \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            int    b_off = (int)(((OFFSET) + 1) * (cIS)) - 1;   \
                                                                \
            while (++b_off < (int)((POP1) * (cIS)))             \
                (PBYTE)[b_off - (cIS)] = (PBYTE)[b_off];        \
        }


// INSERT/DELETE AN INDEX WHILE COPYING OTHERS:
//
// Copy PSource[] to PDest[], where PSource[] has Pop1 elements (Indexes),
// inserting Index at PDest[Offset].  Unlike JU_*INPLACE*() above, these macros
// are used when moving Indexes from one memory object to another.

#define JU_INSERTCOPY(PDEST,PSOURCE,POP1,OFFSET,INDEX)          \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) <= (Word_t) (POP1));           \
        {                                                       \
            int    i_offset;                                    \
                                                                \
            for (i_offset = 0; i_offset < (int)(OFFSET); ++i_offset) \
                (PDEST)[i_offset] = (PSOURCE)[i_offset];        \
                                                                \
            (PDEST)[i_offset] = (INDEX);                        \
                                                                \
            for (/* null */; i_offset < (int)(POP1); ++i_offset)\
                (PDEST)[i_offset + 1] = (PSOURCE)[i_offset];    \
        }

#define JU_INSERTCOPY3(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
assert((long) (POP1) > 0);                                      \
assert((Word_t) (OFFSET) <= (Word_t) (POP1));                   \
{                                                               \
    int    o_ff;                                                \
                                                                \
    for (o_ff = 0; o_ff < (int)(OFFSET); o_ff++)                \
    {                                                           \
        Word_t  i_dx = o_ff * 3;                                \
        (PDEST)[i_dx + 0] = (PSOURCE)[i_dx + 0];                \
        (PDEST)[i_dx + 1] = (PSOURCE)[i_dx + 1];                \
        (PDEST)[i_dx + 2] = (PSOURCE)[i_dx + 2];                \
    }                                                           \
    JU_COPY3_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * 3]), INDEX);   \
                                                                \
    for (/* null */; o_ff < (int)(POP1); o_ff++)                \
    {                                                           \
        Word_t  i_dx = o_ff * 3;                                \
        (PDEST)[i_dx + 0 + 3] = (PSOURCE)[i_dx + 0];            \
        (PDEST)[i_dx + 1 + 3] = (PSOURCE)[i_dx + 1];            \
        (PDEST)[i_dx + 2 + 3] = (PSOURCE)[i_dx + 2];            \
    }                                                           \
}

#ifdef JU_64BIT

#define JU_INSERTCOPY5(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
assert((long) (POP1) > 0);                                      \
assert((Word_t) (OFFSET) <= (Word_t) (POP1));                   \
{                                                               \
    int    o_ff;                                                \
                                                                \
    for (o_ff = 0; o_ff < (int)(OFFSET); o_ff++)                \
    {                                                           \
        int     i_dx = o_ff * 5;                                \
        (PDEST)[i_dx + 0] = (PSOURCE)[i_dx + 0];                \
        (PDEST)[i_dx + 1] = (PSOURCE)[i_dx + 1];                \
        (PDEST)[i_dx + 2] = (PSOURCE)[i_dx + 2];                \
        (PDEST)[i_dx + 3] = (PSOURCE)[i_dx + 3];                \
        (PDEST)[i_dx + 4] = (PSOURCE)[i_dx + 4];                \
    }                                                           \
    JU_COPY5_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * 5]), INDEX);   \
                                                                \
    for (/* null */; o_ff < (int)(POP1); o_ff++)                \
    {                                                           \
        int     i_dx = o_ff * 5;                                \
        (PDEST)[i_dx + 0 + 5] = (PSOURCE)[i_dx + 0];            \
        (PDEST)[i_dx + 1 + 5] = (PSOURCE)[i_dx + 1];            \
        (PDEST)[i_dx + 2 + 5] = (PSOURCE)[i_dx + 2];            \
        (PDEST)[i_dx + 3 + 5] = (PSOURCE)[i_dx + 3];            \
        (PDEST)[i_dx + 4 + 5] = (PSOURCE)[i_dx + 4];            \
    }                                                           \
}

#define JU_INSERTCOPY6(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
assert((long) (POP1) > 0);                                      \
assert((Word_t) (OFFSET) <= (Word_t) (POP1));                   \
{                                                               \
    int    o_ff;                                                \
                                                                \
    for (o_ff = 0; o_ff < (int)(OFFSET); o_ff++)                \
    {                                                           \
        Word_t  i_dx = o_ff * 6;                                \
        (PDEST)[i_dx + 0] = (PSOURCE)[i_dx + 0];                \
        (PDEST)[i_dx + 1] = (PSOURCE)[i_dx + 1];                \
        (PDEST)[i_dx + 2] = (PSOURCE)[i_dx + 2];                \
        (PDEST)[i_dx + 3] = (PSOURCE)[i_dx + 3];                \
        (PDEST)[i_dx + 4] = (PSOURCE)[i_dx + 4];                \
        (PDEST)[i_dx + 5] = (PSOURCE)[i_dx + 5];                \
    }                                                           \
    JU_COPY6_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * 6]), INDEX);   \
                                                                \
    for (/* null */; o_ff < (int)(POP1); o_ff++)                \
    {                                                           \
        int     i_dx = o_ff * 6;                                \
        (PDEST)[i_dx + 0 + 6] = (PSOURCE)[i_dx + 0];            \
        (PDEST)[i_dx + 1 + 6] = (PSOURCE)[i_dx + 1];            \
        (PDEST)[i_dx + 2 + 6] = (PSOURCE)[i_dx + 2];            \
        (PDEST)[i_dx + 3 + 6] = (PSOURCE)[i_dx + 3];            \
        (PDEST)[i_dx + 4 + 6] = (PSOURCE)[i_dx + 4];            \
        (PDEST)[i_dx + 5 + 6] = (PSOURCE)[i_dx + 5];            \
    }                                                           \
}

#define JU_INSERTCOPY7(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
assert((long) (POP1) > 0);                                      \
assert((Word_t) (OFFSET) <= (Word_t) (POP1));                   \
{                                                               \
    int    o_ff;                                                \
                                                                \
    for (o_ff = 0; o_ff < (int)(OFFSET); o_ff++)                \
    {                                                           \
        Word_t  i_dx = o_ff * 7;                                \
        (PDEST)[i_dx + 0] = (PSOURCE)[i_dx + 0];                \
        (PDEST)[i_dx + 1] = (PSOURCE)[i_dx + 1];                \
        (PDEST)[i_dx + 2] = (PSOURCE)[i_dx + 2];                \
        (PDEST)[i_dx + 3] = (PSOURCE)[i_dx + 3];                \
        (PDEST)[i_dx + 4] = (PSOURCE)[i_dx + 4];                \
        (PDEST)[i_dx + 5] = (PSOURCE)[i_dx + 5];                \
        (PDEST)[i_dx + 6] = (PSOURCE)[i_dx + 6];                \
    }                                                           \
    JU_COPY7_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * 7]), INDEX);   \
                                                                \
    for (/* null */; o_ff < (int)(POP1); o_ff++)                \
    {                                                           \
        int     i_dx = o_ff * 7;                                \
        (PDEST)[i_dx + 0 + 7] = (PSOURCE)[i_dx + 0];            \
        (PDEST)[i_dx + 1 + 7] = (PSOURCE)[i_dx + 1];            \
        (PDEST)[i_dx + 2 + 7] = (PSOURCE)[i_dx + 2];            \
        (PDEST)[i_dx + 3 + 7] = (PSOURCE)[i_dx + 3];            \
        (PDEST)[i_dx + 4 + 7] = (PSOURCE)[i_dx + 4];            \
        (PDEST)[i_dx + 5 + 7] = (PSOURCE)[i_dx + 5];            \
        (PDEST)[i_dx + 6 + 7] = (PSOURCE)[i_dx + 6];            \
    }                                                           \
}

#endif // JU_64BIT

// Counterparts to the above for deleting an Index:

#define JU_DELETECOPY(PDEST,PSOURCE,POP1,OFFSET,IGNORE)         \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            int    i_offset;                                    \
                                                                \
            for (i_offset = 0; i_offset < (OFFSET); ++i_offset) \
                (PDEST)[i_offset] = (PSOURCE)[i_offset];        \
                                                                \
            for (++i_offset; i_offset < (int)(POP1); ++i_offset)\
                (PDEST)[i_offset - 1] = (PSOURCE)[i_offset];    \
        }

// Variation for odd-byte-sized (non-native) Indexes, where cIS = Index Size;
// copy byte-by-byte:
//
// Note:  There are no endian issues here because bytes are just shifted as-is,
// not converted to/from an Index.
//
// Note:  If cIS == 1, JU_DELETECOPY_ODD == JU_DELETECOPY, at least in concept.

#define JU_DELETECOPY_ODD(PDEST,PSOURCE,POP1,OFFSET,cIS)                \
        assert((long) (POP1) > 0);                                      \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));                    \
        {                                                               \
            uint8_t *_Pdest   = (uint8_t *) (PDEST);                    \
            uint8_t *_Psource = (uint8_t *) (PSOURCE);                  \
            int      b_off;                                             \
                                                                        \
            for (b_off = 0; b_off < (int)((OFFSET) * (cIS)); ++b_off)   \
                *_Pdest++ = *_Psource++;                                \
                                                                        \
            _Psource += (cIS);                                          \
                                                                        \
            for (b_off += (cIS); b_off < (int)((POP1) * (cIS)); ++b_off)\
                *_Pdest++ = *_Psource++;                                \
        }


// GENERIC RETURN CODE HANDLING FOR JUDY1 (NO VALUE AREAS) AND JUDYL (VALUE
// AREAS):
//
// This common code hides Judy1 versus JudyL details of how to return various
// conditions, including a pointer to a value area for JudyL.
//
// First, define an internal variation of JERR called JERRI (I = int) to make
// lint happy.  We accidentally shipped to 11.11 OEUR with all functions that
// return int or Word_t using JERR, which is type Word_t, for errors.  Lint
// complains about this for functions that return int.  So, internally use
// JERRI for error returns from the int functions.  Experiments show that
// callers which compare int Foo() to (Word_t) JERR (~0UL) are OK, since JERRI
// sign-extends to match JERR.

#define JERRI ((int) ~0)                // see above.

#ifdef JUDY1

#define JU_RET_FOUND    return(1)
#define JU_RET_NOTFOUND return(0)

// For Judy1, these all "fall through" to simply JU_RET_FOUND, since there is no
// value area pointer to return:

#define JU_RET_FOUND_LEAFW(PJLW,POP1,OFFSET)    JU_RET_FOUND

#define JU_RET_FOUND_JPM(Pjpm)                  JU_RET_FOUND
#define JU_RET_FOUND_PVALUE(Pjv,OFFSET)         JU_RET_FOUND

#ifndef JU_64BIT
#define JU_RET_FOUND_LEAF1(Pjll,POP1,OFFSET)    JU_RET_FOUND
#endif

#define JU_RET_FOUND_LEAF2(Pjll,POP1,OFFSET)    JU_RET_FOUND
#define JU_RET_FOUND_LEAF3(Pjll,POP1,OFFSET)    JU_RET_FOUND
#ifdef JU_64BIT
#define JU_RET_FOUND_LEAF4(Pjll,POP1,OFFSET)    JU_RET_FOUND
#define JU_RET_FOUND_LEAF5(Pjll,POP1,OFFSET)    JU_RET_FOUND
#define JU_RET_FOUND_LEAF6(Pjll,POP1,OFFSET)    JU_RET_FOUND
#define JU_RET_FOUND_LEAF7(Pjll,POP1,OFFSET)    JU_RET_FOUND
#endif
#define JU_RET_FOUND_IMM_01(Pjp)                JU_RET_FOUND
#define JU_RET_FOUND_IMM(Pjp,OFFSET)            JU_RET_FOUND

// Note:  No JudyL equivalent:

#define JU_RET_FOUND_FULLPOPU1                   JU_RET_FOUND
#define JU_RET_FOUND_LEAF_B1(PJLB,SUBEXP,OFFSET) JU_RET_FOUND

#else // JUDYL

//      JU_RET_FOUND            // see below; must NOT be defined for JudyL.
#define JU_RET_NOTFOUND return((PPvoid_t) NULL)

// For JudyL, the location of the value area depends on the JP type and other
// factors:
//
// TBD:  The value areas should be accessed via data structures, here and in
// Dougs code, not by hard-coded address calculations.
//
// This is useful in insert/delete code when the value area is returned from
// lower levels in the JPM:

#define JU_RET_FOUND_JPM(Pjpm)  return((PPvoid_t) ((Pjpm)->jpm_PValue))

// This is useful in insert/delete code when the value area location is already
// computed:

#define JU_RET_FOUND_PVALUE(Pjv,OFFSET) return((PPvoid_t) ((Pjv) + OFFSET))

#define JU_RET_FOUND_LEAFW(PJLW,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAFWVALUEAREA(PJLW, POP1) + (OFFSET)))

#define JU_RET_FOUND_LEAF1(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF1VALUEAREA(Pjll, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF2(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF2VALUEAREA(Pjll, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF3(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF3VALUEAREA(Pjll, POP1) + (OFFSET)))
#ifdef JU_64BIT
#define JU_RET_FOUND_LEAF4(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF4VALUEAREA(Pjll, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF5(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF5VALUEAREA(Pjll, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF6(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF6VALUEAREA(Pjll, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF7(Pjll,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF7VALUEAREA(Pjll, POP1) + (OFFSET)))
#endif

// Note:  Here jp_Addr is a value area itself and not an address, so P_JV() is
// not needed:

#define JU_RET_FOUND_IMM_01(PJP)  return((PPvoid_t) (&((PJP)->jp_PValue)))

// Note:  Here jp_Addr is a pointer to a separately-mallocd value area, so
// P_JV() is required; likewise for JL_JLB_PVALUE:

#define JU_RET_FOUND_IMM(PJP,OFFSET) \
            return((PPvoid_t) (P_JV((PJP)->jp_PValue) + (OFFSET)))

#ifndef BMVALUE

#define JU_RET_FOUND_LEAF_B1(PJLB,SUBEXP,OFFSET) \
            return((PPvoid_t) (P_JV(JL_JLB_PVALUE(PJLB, SUBEXP)) + (OFFSET)))

#else   // BMVALUE

#define _bm(P,S)  ((Word_t)((P)->jLlb_jLlbs[S].jLlbs_Bitmap))
#define _bmi(P,S) ((_bm(P,S) & -_bm(P,S)) == (_bm(P,S)))

#define JU_RET_FOUND_LEAF_B1(PJLB,SUBEXP,OFFSET)                \
    return((PPvoid_t) ((_bmi(PJLB,SUBEXP)) ?                    \
    ((Pjv_t)(&(PJLB)->jLlb_jLlbs[SUBEXP].jLlbs_PV_Raw))         \
    : (P_JV((PJLB)->jLlb_jLlbs[SUBEXP].jLlbs_PV_Raw)) + (OFFSET)))

#endif  // BMVALUE


#endif // JUDYL


// GENERIC ERROR HANDLING:
//
// This is complicated by variations in the needs of the callers of these
// macros.  Only use JU_SET_ERRNO() for PJError, because it can be null; use
// JU_SET_ERRNO_NONNULL() for Pjpm, which is never null, and also in other
// cases where the pointer is known not to be null (to save dead branches).
//
// Note:  Most cases of JU_ERRNO_OVERRUN or JU_ERRNO_CORRUPT should result in
// an assertion failure in debug code, so they are more likely to be caught, so
// do that here in each macro.

#define JU_SET_ERRNO(PJError, JErrno)                   \
        {                                               \
            assert((JErrno) != JU_ERRNO_OVERRUN);       \
            assert((JErrno) != JU_ERRNO_CORRUPT);       \
                                                        \
            if (PJError != (PJError_t) NULL)            \
            {                                           \
                JU_ERRNO(PJError) = (JErrno);           \
                JU_ERRID(PJError) = __LINE__;           \
            }                                           \
        }

// Variation for callers who know already that PJError is non-null; and, it can
// also be Pjpm (both PJError_t and Pjpm_t have je_* fields), so only assert it
// for null, not cast to any specific pointer type:

#define JU_SET_ERRNO_NONNULL(PJError, JErrno)           \
        {                                               \
            assert((JErrno) != JU_ERRNO_OVERRUN);       \
            assert((JErrno) != JU_ERRNO_CORRUPT);       \
            assert(PJError);                            \
                                                        \
            JU_ERRNO(PJError) = (JErrno);               \
            JU_ERRID(PJError) = __LINE__;               \
        }

// Variation to copy error info from a (required) JPM to an (optional)
// PJError_t:
//
// Note:  The assertions above about JU_ERRNO_OVERRUN and JU_ERRNO_CORRUPT
// should have already popped, so they are not needed here.

#define JU_COPY_ERRNO(PJError, Pjpm)                            \
        {                                                       \
            if (PJError)                                        \
            {                                                   \
                JU_ERRNO(PJError) = (uint8_t)JU_ERRNO(Pjpm);    \
                JU_ERRID(PJError) = JU_ERRID(Pjpm);             \
            }                                                   \
        }

// For JErrno parameter to previous macros upon return from Judy*Alloc*():
//
// The memory allocator returns an address of 0 for out of memory,
// 1..sizeof(Word_t)-1 for corruption (an invalid pointer), otherwise a valid
// pointer.

#define JU_ALLOC_ERRNO(ADDR) \
        (((void *) (ADDR) != (void *) NULL) ? JU_ERRNO_OVERRUN : JU_ERRNO_NOMEM)

#define JU_CHECKALLOC(Type,Ptr,Retval)                  \
        if ((Ptr) < (Type) cBPW)                        \
        {                                               \
            JU_SET_ERRNO(PJError, JU_ALLOC_ERRNO(Ptr)); \
            return(Retval);                             \
        }

// Cheap and dirty search for matching branchL
static inline int j__udySearchBranchL(uint8_t *Lst, int pop1, uint8_t Exp)
{
    SEARCHLINARNATIVE(uint8_t, Lst, pop1, Exp, 0); 
}

// Leaf search routines

// This calculates a proportional division of Pop1 and significant hi-bits
// to produce a statical starting offset to begin search.

static inline int  FSPLIT(Word_t FIRST, Word_t LAST, Word_t KEY, Word_t POP1)
{
    int Start;

    if (KEY > LAST)  return (~POP1);
    if (KEY < FIRST) return (~0);

#ifdef DOUB
    Start = (double)(KEY - FIRST) * (double)(POP1 - 1) / (double)(LAST - FIRST);
#else   // DOUB
    Start = ((KEY - FIRST) * (POP1 - 1)) / (LAST - FIRST);
#endif  // DOUB

    return(Start);
}

// -------------------------------------------------------
// THIS NEEDS REVIEW!!!!!! Key mask and log are not the same
// Used to mask to single Key
//      = JU_LEASTBYTES((KEY), (LFBTS)); 
//
#define MskK(cbPK, KEY)         ((Word_t)(KEY) & (((Word_t)1 << (cbPK)) - 1))
#define KEYMASK(cbPK)           (((Word_t)1 << (cbPK)) - 1)


#ifdef ONEWAY
#define NSPLIT(POP, KEY, LOG2)                                  \
        ((((((Word_t)(KEY)) << (cbPW - (LOG2))) >> (cbPW - (LOG2))) * (POP)) >> (LOG2))
#else   // ! ONEWAY
#define NSPLIT(POP, KEY, LOG2)  (((Word_t)(KEY) * (POP)) >> (LOG2))
#endif  // ! ONEWAY
// End of review THIS NEEDS REVIEW!!!!!! Key mask and log are not the same
// -------------------------------------------------------

static inline int j__udySearchLeaf1(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
    int Start;

//    Start = NSPLIT(LeafPop1, Index, (cbPW - __builtin_clzl(((uint8_t *)Pjll)[0] ^ ((uint8_t *)Pjll)[LeafPop1 - 1])));

//  The vast majority of these are == 8, so hard code 8 
    Start = NSPLIT(LeafPop1, (uint8_t)Index, 1 * 8); 
    SEARCHLEAFNATIVE(uint8_t,  Pjll, LeafPop1, Index, Start); 
}

static inline int j__udySearchLeaf2(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
    int Start;

//    Start = NSPLIT(LeafPop1, Index, (cbPW - __builtin_clzl(((uint16_t *)Pjll)[0] ^ ((uint16_t *)Pjll)[LeafPop1 - 1])));

    Start = NSPLIT(LeafPop1, (uint16_t)Index, 2 * 8); 
    SEARCHLEAFNATIVE(uint16_t,  Pjll, LeafPop1, Index, Start); 
}

static inline int j__udySearchLeaf3(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
//  The vast majority of these are == 24, so hard code 24 
    int Start = NSPLIT(LeafPop1, JU_LEASTBYTES(Index, 3), 3 * 8); 
    SEARCHLEAFNONNAT(Pjll, LeafPop1, Index, 3, JU_COPY3_PINDEX_TO_LONG, Start); 
}

#ifdef JU_64BIT
static inline int j__udySearchLeaf4(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
    int Start = NSPLIT(LeafPop1, (uint32_t)Index, 4 * 8); 
    SEARCHLEAFNATIVE(uint32_t,  Pjll, LeafPop1, Index, Start); 
}

static inline int j__udySearchLeaf5(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
    int Start = NSPLIT(LeafPop1, JU_LEASTBYTES(Index, 5), 5 * 8); 
    SEARCHLEAFNONNAT(Pjll, LeafPop1, Index, 5, JU_COPY5_PINDEX_TO_LONG, Start); 
}

static inline int j__udySearchLeaf6(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
    int Start = NSPLIT(LeafPop1, JU_LEASTBYTES(Index, 6), 6 * 8); 
    SEARCHLEAFNONNAT(Pjll, LeafPop1, Index, 6, JU_COPY6_PINDEX_TO_LONG, Start); 
}

static inline int j__udySearchLeaf7(Pjll_t Pjll, int LeafPop1, Word_t Index)
{
    int Start = NSPLIT(LeafPop1, JU_LEASTBYTES(Index, 7), 7 * 8); 
    SEARCHLEAFNONNAT(Pjll, LeafPop1, Index, 7, JU_COPY7_PINDEX_TO_LONG, Start); 
}
#endif // JU_64BIT


#ifdef NSLEAFW

static inline int j__udySearchLeafW(Pjlw_t Pjlw, int LeafPop1, Word_t Index)
{

      if (LeafPop1 > 16)
      {
          int Start = 0;

//        Check id the Index is within the bounds of the leaf
          if (Index > Pjlw[LeafPop1 - 1]) return (~(LeafPop1)); // redundant from below
          if (Index < Pjlw[0]) return (~0);

          Word_t AIndex = Index - Pjlw[0];              // see above check
          Word_t Diff = Pjlw[LeafPop1 - 1] - Pjlw[0];   // no error possible

          Start = AIndex * (LeafPop1 - 1) / Diff;
          SEARCHBIDIRNATIVE(Word_t,  Pjlw, LeafPop1, Index, Start); 
      }
      else
      {
          SEARCHLINARNATIVE(Word_t,  Pjlw, LeafPop1, Index, 0); 
      }
}

#else	// NSLEAFW

static inline int j__udySearchLeafW(Pjlw_t Pjlw, int LeafPop1, Word_t Index)
{
//    SEARCHLINARNATIVE(Word_t,  Pjlw, LeafPop1, Index, 0); 
    SEARCHBINARYNATIVE(Word_t, Pjlw, LeafPop1, Index, 0); 
}
#endif	// NSLEAFW
#endif // ! _JUDYPRIVATE_INCLUDED
