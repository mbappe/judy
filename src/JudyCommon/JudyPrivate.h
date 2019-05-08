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
extern Word_t  j__MisComparesP;
extern Word_t  j__MisComparesM;
extern Word_t  j__GetCallsP;
extern Word_t  j__GetCallsM;
extern Word_t  j__GetCalls;
extern Word_t  j__SearchPopulation;
extern Word_t  j__DirectHits;

// if BUCKETSIZE not defined, default to one word
#ifndef BUCKETSIZE
#define BUCKETSIZE      (sizeof(Word_t))       // Bytes per bucket
#endif  // BUCKETSIZE

#define KEYSPERBUCKET(KEY)   (BUCKETSIZE / (KEY))

// Perhaps should change name to SEARCHCOMPAREMISES
#define  MISCOMPARESP(CNT)      (j__MisComparesP   += (Word_t)(CNT), j__GetCallsP++)
#define  MISCOMPARESM(CNT)      (j__MisComparesM   += (Word_t)(CNT), j__GetCallsM++)

#define  MISCOMPARESP1          (j__MisComparesP++)     // GETCALLS() must be called appropriately
#define  MISCOMPARESM1          (j__MisComparesM++)

#define  GETCALLSP              (j__GetCallsP++)        // for B1 & Binary search only
#define  GETCALLSM              (j__GetCallsM++)        // for B1 & Binary search only

#define  DIRECTHITS             (j__DirectHits++)
#define  SEARCHPOPULATION(CNT)  (j__SearchPopulation += (Word_t)(CNT), j__GetCalls++)
#else
#define  MISCOMPARESP(CNT)
#define  MISCOMPARESM(CNT)
#define  MISCOMPARESP1
#define  MISCOMPARESM1
#define  GETCALLSP
#define  GETCALLSM
#define  GETCALLS
#define  DIRECTHITS
#define  SEARCHPOPULATION(CNT)
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

#define cJU_BYTESPERCL 128              // cache line size in bytes.

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

#define BITMAPB_t uint64_t
#define BITMAPL_t uint64_t

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

    if (num >= (((Word_t)1) << (1 * 32)))
    {
        num >>= 32;
        __lb += 32;
    }

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
// (Pvoid_t), not PWord_t or Word_t for them, except use Pjll8_t for whole-word
// (top-level, root-level) leaves.  Value areas, however, are always whole
// words.
//
// Furthermore, use Pjll_t only for generic leaf pointers (for various size
// LeafLs).  Use Pjll8_t for Leaf8s.  Use Pleaf (with type uint8_t *, uint16_t
// *, etc) when the leaf index size is known.

#ifdef JUDY1
// Leaf structures
typedef struct J_UDY1_LEAF1_STRUCT
{
//    Word_t      jl1_subLeafPops; // temp
    Word_t      jl1_LastKey;
    uint8_t     jl1_Leaf[0];
} jll1_t, *Pjll1_t;

typedef struct J_UDY1_LEAF2_STRUCT
{
    Word_t      jl2_LastKey;
    uint16_t    jl2_Leaf[0];
} jll2_t, *Pjll2_t;

typedef struct J_UDY1_LEAF3_STRUCT
{
    Word_t      jl3_LastKey;
    uint8_t     jl3_Leaf[0];
} jll3_t, *Pjll3_t;

typedef struct J_UDY1_LEAF4_STRUCT
{
    Word_t      jl4_LastKey;
    uint32_t    jl4_Leaf[0];
} jll4_t, *Pjll4_t;

typedef struct J_UDY1_LEAF5_STRUCT
{
    Word_t      jl5_LastKey;
    uint8_t     jl5_Leaf[0];
} jll5_t, *Pjll5_t;

typedef struct J_UDY1_LEAF6_STRUCT
{
    Word_t      jl6_LastKey;
    uint8_t     jl6_Leaf[0];
} jll6_t, *Pjll6_t;

typedef struct J_UDY1_LEAF7_STRUCT
{
    Word_t      jl7_LastKey;
    uint8_t     jl7_Leaf[0];
} jll7_t, *Pjll7_t;

typedef struct J_UDY1_LEAF8_STRUCT
{
    Word_t      jl8_Population0;
    Word_t      jl8_Leaf[0];
} jllw_t, *Pjll8_t;

//typedef PWord_t Pjll8_t;  // pointer to root-level leaf (whole-word indexes).

#endif  // JUDY1


#ifdef JUDYL
// Leaf structures
typedef struct J_UDY1_LEAF1_STRUCT_UCOMP
{
    Word_t      jl1_LastKey;
    BITMAPL_t   jl1_Bitmaps[4 /* cJU_NUMSUBEXP */];
    Word_t      jl1_Values[256];
} jllu1_t, *Pjllu1_t;

typedef struct J_UDYL_LEAF1_STRUCT
{
    Word_t      jl1_LastKey;
//    Word_t      jl1_subLeafPops;        // temp
    uint8_t     jl1_Leaf[0];
} jll1_t, *Pjll1_t;

typedef struct J_UDYL_LEAF2_STRUCT
{
    Word_t      jl2_LastKey;
    uint16_t    jl2_Leaf[0];
} jll2_t, *Pjll2_t;

typedef struct J_UDYL_LEAF3_STRUCT
{
    Word_t      jl3_LastKey;
    uint8_t     jl3_Leaf[0];
} jll3_t, *Pjll3_t;

typedef struct J_UDYL_LEAF4_STRUCT
{
    Word_t      jl4_LastKey;
    uint32_t    jl4_Leaf[0];
} jll4_t, *Pjll4_t;

typedef struct J_UDYL_LEAF5_STRUCT
{
    Word_t      jl5_LastKey;
    uint8_t     jl5_Leaf[0];
} jll5_t, *Pjll5_t;

typedef struct J_UDYL_LEAF6_STRUCT
{
    Word_t      jl6_LastKey;
    uint8_t     jl6_Leaf[0];
} jll6_t, *Pjll6_t;

typedef struct J_UDYL_LEAF7_STRUCT
{
    Word_t      jl7_LastKey;
    uint8_t     jl7_Leaf[0];
} jll7_t, *Pjll7_t;

typedef struct J_UDYL_LEAF8_STRUCT
{
    Word_t      jl8_Population0;
    Word_t      jl8_Leaf[0];
} jll8_t, *Pjll8_t;
#endif  // JUDYL

// typedef Pvoid_t Pjll_t;  // pointer to lower-level linear leaf.

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
//    jp.jp_Addr0        generic pointer to next-level node, except when used
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


#define P_JPM(  ADDR) ((Pjpm_t) (ADDR))  // root JPM.
#define P_JBL(  ADDR) ((Pjbl_t) (ADDR))  // BranchL.
#define P_JBB(  ADDR) ((Pjbb_t) (ADDR))  // BranchB.
#define P_JBU(  ADDR) ((Pjbu_t) (ADDR))  // BranchU.
#define P_JLL1( ADDR) ((Pjll1_t)(ADDR))  // Leaf1.
#define P_JLL2( ADDR) ((Pjll2_t)(ADDR))  // Leaf2.
#define P_JLL3( ADDR) ((Pjll3_t)(ADDR))  // Leaf3.
#define P_JLL4( ADDR) ((Pjll4_t)(ADDR))  // Leaf4.
#define P_JLL5( ADDR) ((Pjll5_t)(ADDR))  // Leaf5.
#define P_JLL6( ADDR) ((Pjll6_t)(ADDR))  // Leaf6.
#define P_JLL7( ADDR) ((Pjll7_t)(ADDR))  // Leaf7.
#define P_JLL8( ADDR) ((Pjll8_t)(ADDR))  // root leaf.
#define P_JLB1( ADDR) ((Pjlb_t) (ADDR))  // LeafB1.
#define P_JP(   ADDR) ((Pjp_t)  (ADDR))  // JP.

#ifdef JUDYL

// Strip hi 10 bits and low 4 bits
#define P_JV(   ADDR) ((Pjv_t)  (((Word_t)(ADDR)) & 0x3FFFFFFFFFFFF0))  // &value.


#endif  // JUDYL

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

// 0xffffffffffffffff = JU_LEASTBYTESMASK(0)
// 0x              ff = JU_LEASTBYTESMASK(1)
// 0x            ffff = JU_LEASTBYTESMASK(2)
// 0x          ffffff = JU_LEASTBYTESMASK(3)
// 0x        ffffffff = JU_LEASTBYTESMASK(4)
// 0x      ffffffffff = JU_LEASTBYTESMASK(5)
// 0x    ffffffffffff = JU_LEASTBYTESMASK(6)
// 0x  ffffffffffffff = JU_LEASTBYTESMASK(7)
// 0xffffffffffffffff = JU_LEASTBYTESMASK(8)

// BYTES must be 2..7 
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


// EXPORTED DATA AND FUNCTIONS:

#ifdef JUDY1
extern const uint16_t j__1_BranchBJPPopToWords[];
extern int      j__udy1Test(Pcvoid_t PArray, Word_t Index, P_JE);
#endif

#ifdef JUDYL
extern const uint16_t j__L_BranchBJPPopToWords[];
extern PPvoid_t j__udyLGet(Pcvoid_t PArray, Word_t Index, P_JE);
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
            if (I_ndex != i_ndex) return(~o_ff);                \
                                                                \
            MISCOMPARESP(o_ff);                                 \
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
                                                                \
    while(I_ndex > *P_leaf) P_leaf++;                           \
                                                                \
    _off = P_leaf - (LEAFTYPE_t *)(ADDR);                       \
    if (I_ndex != *P_leaf) return(~_off);                       \
 /*   MISCOMPARESP(_off);     */                                \
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
        GETCALLSP;                                              \
        m_id = (h_igh + l_ow) / 2;                              \
        if ((P_leaf)[m_id] <= (I_ndex))                         \
        {                                                       \
            if ((P_leaf)[m_id] == (I_ndex))                     \
            {                                                   \
                return(m_id);                                   \
            }                                                   \
            l_ow = m_id;                                        \
        }                                                       \
        else                                                    \
        {                                                       \
            h_igh = m_id;                                       \
        }                                                       \
    }                                                           \
    if ((h_igh == (POP1)) && ((I_ndex) > (P_leaf)[(POP1) - 1])) \
        return(~(POP1));                                        \
                                       /* Linear Search */      \
    while ((P_leaf)[l_ow] < (I_ndex))                           \
    {                                                           \
        l_ow++;                                                 \
        GETCALLSM;                                              \
    }                                                           \
    if ((P_leaf)[l_ow] != (I_ndex)) return(~l_ow);              \
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
        if (__LeafKey <= __Key)                                 \
        {                                                       \
            if (__LeafKey == __Key) return(m_id);               \
            l_ow = m_id;                                        \
        }                                                       \
        else                                                    \
        {                                                       \
            h_igh = m_id;                                       \
        }                                                       \
        MISCOMPARESP1;                                          \
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
        l_ow++;                                                 \
        COPYINDEX(__LeafKey, __PLeaf + (l_ow * (LFBTS)));       \
        MISCOMPARESP1;                                          \
    }                                                           \
    if (__LeafKey != __Key) return(~l_ow);                      \
    MISCOMPARESP(1);                                            \
    return(l_ow);                                               \
}


// Use bits of the Key (INDEX) to determine where to start search.
// Good for populations up to 2..255
// NOTE: Should not be used for Leaf8 because Leaf8 is the only
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
// cache-line hits to few as possible.

#define SEARCHBIDIRNATIVE(LEAFSIZE_t, PLEAF, POP1, KEY, START)  \
{                                                               \
    Word_t      __LeafKey;      /* current Key in Leaf */       \
    LEAFSIZE_t  __Key;          /* Key to search for */         \
    LEAFSIZE_t  *__PLeaf;       /* ^ toLeaf to search */        \
    int         __pos;          /* starting pos of search */    \
                                                                \
    if (START) assert((Word_t)(START) < (Word_t)(POP1));        \
    __pos    = (int)(START);                                    \
    __Key    = (LEAFSIZE_t)(KEY);                               \
    __PLeaf  = (LEAFSIZE_t *)(PLEAF);                           \
    __LeafKey = (Word_t)__PLeaf[__pos];                         \
                                                                \
    if (__LeafKey == __Key) /* 1st check if direct hit */       \
    {                                                           \
        DIRECTHITS;                                             \
        return(__pos);  /* Nailed it */                         \
    }                                                           \
    if (__Key > __LeafKey)     /* Search forward */             \
    {                                                           \
        for (__pos++; __pos < (POP1); __pos++)                  \
        {                                                       \
            __LeafKey = __PLeaf[__pos];                         \
            if (__LeafKey >= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISCOMPARESP(__pos - (START));              \
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
                    MISCOMPARESM((START) - __pos);              \
                    return(__pos);                              \
                }                                               \
                __pos++; /*  advance to hole */                 \
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
        DIRECTHITS;                                             \
        return(__pos);  /* Nailed it */                         \
    }                                                           \
    if (__Key > __LeafKey)                                      \
    {                   /* Search forward */                    \
        for (__pos++; __pos < (POP1); __pos++)                  \
        {                                                       \
            COPYINDEX(__LeafKey, __PLeaf + (__pos * (LFBTS)));  \
            if (__LeafKey >= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISCOMPARESP(__pos - (START));              \
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
            COPYINDEX(__LeafKey, __PLeaf + (__pos * (LFBTS)));  \
            if (__LeafKey <= __Key)                             \
            {                                                   \
                if (__LeafKey == __Key)                         \
                {                                               \
                    MISCOMPARESM((START) - __pos);              \
                    return(__pos);                              \
                }                                               \
                __pos++; /*  advance to hole */                 \
                break;                                          \
            }                                                   \
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
j__udyCount64Bits(uint64_t word64)
{
     return ((int)__builtin_popcountl(word64));
}

#else   // ! __POPCNT__

// Hopefully non-X86 compilers will recognize this and use popc replacement
static inline int
j__udyCount64Bits(uint64_t word64)
{
printf("\nOOps not using popcnt()\n");
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

#define j__udyCountBitsL j__udyCount64Bits
#define j__udyCountBitsB j__udyCount64Bits

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

/////#define  JU_LEAF8_POP0(JRP)                  (*P_JLL8(JRP))
#define  JU_LEAF8_POP0(JRP)                  (P_JLL8(JRP)->jl8_Population0)
#define cJU_JPFULLPOPU1_POP0                 (cJU_SUBEXPPERSTATE - 1)

#ifdef  JU_LITTLE_ENDIAN        // ====================================
#else  // BIG_ENDIAN =================================================
#endif  // BIG_ENDIAN =================================================

// NUMBER OF BITS IN A BRANCH OR LEAF BITMAP AND SUBEXPANSE:
//
// Note:  cJU_BITSPERBITMAP must be the same as the number of JPs in a branch.

#define cJU_BITSPERBITMAP cJU_SUBEXPPERSTATE

// Bitmaps are accessed in units of "subexpanses":

#define cJU_BITSPERSUBEXPB  (sizeof(BITMAPB_t) * cJU_BITSPERBYTE)       // 64
#define cJU_NUMSUBEXPB      (cJU_BITSPERBITMAP / cJU_BITSPERSUBEXPB)    // 4


#define cJU_BITSPERSUBEXPL  (sizeof(BITMAPL_t) * cJU_BITSPERBYTE)       // 64
#define cJU_NUMSUBEXPL      (cJU_BITSPERBITMAP / cJU_BITSPERSUBEXPL)    // 4


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

// This one is for a copy of same typeof PDST and PSRC
#define JU_COPYMEMODD(PDST,PSRC,POP1)                   \
    {                                                   \
        Word_t i_ndex = 0;                              \
        assert((POP1) > 0);                             \
        do { (PDST)[i_ndex] = (PSRC)[i_ndex]; }         \
        while (++i_ndex < (POP1));                      \
    }


#define JU_COPYMEM(PDST,PSRC,POP1)                      \
    {                                                   \
        assert((POP1) > 0);                             \
        if (sizeof(*(PDST)) == sizeof(*(PSRC)))         \
            memcpy(PDST, PSRC, sizeof(*(PDST)) * (POP1)); \
        else                                            \
            JU_COPYMEMODD((PDST),(PSRC),POP1);          \
    }



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
    (PINDEX)[0] = (uint8_t)((Word_t)(SOURCELONG) >> 48);        \
    (PINDEX)[1] = (uint8_t)((Word_t)(SOURCELONG) >> 40);        \
    (PINDEX)[2] = (uint8_t)((Word_t)(SOURCELONG) >> 32);        \
    (PINDEX)[3] = (uint8_t)((Word_t)(SOURCELONG) >> 24);        \
    (PINDEX)[4] = (uint8_t)((Word_t)(SOURCELONG) >> 16);        \
    (PINDEX)[5] = (uint8_t)((Word_t)(SOURCELONG) >>  8);        \
    (PINDEX)[6] = (uint8_t)((Word_t)(SOURCELONG))


// ****************************************************************************
// COMMON CODE FRAGMENTS (MACROS)
// ****************************************************************************
//
// These code chunks are shared between various source files.
//
// Replicate a KEY as many times as possible in a Word_t
// (if n keys do not exactly fit, right justify by shifting by remainder bits)

// Used to mask to single Key
#define KEYMASK(cbPK)           (((Word_t)1 << (cbPK)) - 1)

// Used to right justify the replicated Keys that have remainder bits - zero if Key 2 ^ n
//                              ( 64  - (  64  /   16 )  *   16 )))
#define REMb(cbPK)              (cbPW - ((cbPW / (cbPK)) * (cbPK)))

// Replicate a KEY as many times as possible in a Word_t
// (if n keys do not exactly fit, right justify by shifting by remainder bits)
#define REPKEY(cbPK, KEY)                                        \
    ((((Word_t)(-1) / KEYMASK(cbPK)) * (KEYMASK(cbPK) & (KEY))) >> REMb(cbPK))

#ifdef noPAD
#define JU_PADLEAF1(PJLL, POP1)
#define JU_PADLEAF2(PJLL, POP1)
#else   // noPAD
#define JU_PADLEAF1(PJLL, POP1)                         \
{                                                       \
    Word_t   __Pop0 = (POP1) - 1;                       \
    Word_t   __rounduppop1 = ((__Pop0 + 8) / 8) * 8;    \
    uint8_t *__Pleaf1 = (uint8_t *)(PJLL);              \
                                                        \
    for (int __i = (POP1); __i < __rounduppop1; __i++)  \
         __Pleaf1[__i] = __Pleaf1[__Pop0];              \
}

#define JU_PADLEAF2(PJLL, POP1)                         \
{                                                       \
    Word_t    __Pop0 = (POP1) - 1;                      \
    Word_t    __rounduppop1 = ((__Pop0 + 4) / 4) * 4;   \
    uint16_t *__Pleaf2 = (uint16_t *)(PJLL);            \
                                                        \
    for (int __i = (POP1); __i < __rounduppop1; __i++)  \
         __Pleaf2[__i] = __Pleaf2[__Pop0];              \
}
#endif  // noPAD

#ifdef  later
// #ifdef  PARALLEL1
// Macros for one word parallel searches
// cbPW = bits per Word
// KEYMASK = Key Mask
// KEY  = Key
// REMb = Remainder Bits in Word_t, if n keys do not exactly fit in Word_t
// cbPK = Bits Per Key

//#define cbPW        (sizeof(Word_t) * 8)

// WTF, compilers should warn about this or fix it
// Count_Least_Zero bits
//#define JU_CLZ(BUCKET)       ((Word_t)(BUCKET) > 0 ? (int)__builtin_clzl((Word_t)BUCKET) : (int)cbPW)

#define JU_POP0(BUCKET)      ((int)((cbPW - 1) - JU_CLZ(BUCKET)) / bitsPerKey)

// count trailing zero bits
#define JU_CTZ(BUCKET, BPK)     (__builtin_ctzl(BUCKET) / (BPK))

// Check if any Key in the Word_t has a zero value
#define HAS0KEY(cbPK, BUCKET)                                      \
    (((BUCKET) - REPKEY(cbPK, 1)) & ~(BUCKET) & REPKEY(cbPK, -1))

// How many keys offset until the one found (in highest bit in zero)
//#define KEYOFFSET(ZERO, cbPK)         (__builtin_ctzl(ZERO) / (cbPK))

// Mask to bits in Population0 
///#define Mskbs(cbPK, POP0)      ((1 << ((POP0) * (cbPK))))

// Calculate the population of a bucket
//#define JU_POP0(BUCKET, cbPK)      ((int)((cbPW - 1) - JU_CLZL(BUCKET)) / (cbPK))

// Mask to remove Keys that are not in the population
// broke?

//#define P_M(cbPK,POP0) (((Word_t)((1 << ((cbPK) - 1)) * 2) << ((POP0) * cbPK)) - 1)

// Experiment with different methods
#ifdef  JUDYL

#ifdef  ONE
static int j__udyBuckoff(Word_t Bucket, int bPK)
{
    Bucket &= -Bucket;                  // get rid of all but Lsb bit
    if ((Bucket >>= bPK) == 0) return(0);
    if ((Bucket >>= bPK) == 0) return(1);
    if ((Bucket >>= bPK) == 0) return(2);
    return(3);                  // only other posibility
}
#endif  // ONE

#ifdef  TWO
static int j__udyBuckoff(Word_t Bucket, int bPK)
{
    int offset;
    Word_t xxx;

    xxx = Bucket & -Bucket;          // get rid of all but Lsb bit
    for (offset = 0; xxx >>= bPK; offset++);
    return(offset);
}
#endif  // TWO

#ifdef  THREE 
//  Only slightly faster with 4Ghz i7-5960X/2666-14 RAM
//  Shame on Intel (bsfq instruction)
static int j__udyBuckoff(Word_t Bucket, int bPK)
{
    return (__builtin_ctzl(Bucket) / bPK);
}
#endif  // THREE 

#ifdef  FOUR 
//  Only slightly faster with 4Ghz i7-5960X/2666-14 RAM
//  Shame on Intel (bsfq instruction)
static int j__udyBuckoff(Word_t Bucket, int bPK)
{
    Bucket = (Bucket & -Bucket) - 1;    // get rid of all but Lsb bits
    return ((int)__builtin_popcountl(Bucket) / bPK);
}
#endif  // FOUR 

#endif  // JUDYL

// If matching Key in Bucket, then offset into Bucket is returned
// else -1 if no Key was found

// Note: bPK must be a compile time constant for high performance
static int
j__udyBucketHasKey(Word_t Bucket, Word_t Key, int bPK)
{
    Word_t      repLsbKey;
    Word_t      repMsbKey;
    Word_t      newBucket;

    assert (Bucket != 0);

//  Check special case of Key == 0
//  BUT, the leaf population can never be < 2, so Bucket can never be zero,
//  so save some conditional branches.
//  if (Key == 0)
//  {
//      if (Bucket & KeyMask) 
//          return (-1);        // no match and offset = -1
//      else
//          return (0);         // match and offset = 0
//  }

//  replicate the Lsb in every Key position (at compile time)
    repLsbKey = REPKEY(bPK, 1);

//  replicate the Msb in every Key position (at compile time)
    repMsbKey = repLsbKey << (bPK - 1);

//  make zero the searched for Key in new Bucket
    newBucket = Bucket ^ REPKEY(bPK, Key);

//  Magic, the Msb=1 is located in the matching Key position
    newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;

    printf("\n-----------  0x%lx, rep = 0x%lx\n", Key, newBucket);

    if (newBucket == 0)         // Key not found in Bucket
        return(-1);

#ifdef  JUDY1
    return(0);                  // Key was found in Bucket
#endif  // JUDY1

#ifdef  JUDYL
//  Calc offset 0..3 into one Bucket for Pjv_t
#ifdef  FIVE
    {
        uint16_t *bucket16 = (uint16_t *)&Bucket;
        int off = 0;

        while (bucket16[off] != (uint16_t)Key) off++;
        return(off);
    }
#else   // ! FIVE
    return (j__udyBuckoff(newBucket, bPK));
#endif  // ! FIVE
#endif  // JUDYL

}
// #endif  //  PARALLEL1
#endif  // later

// SET (REPLACE) ONE DIGIT IN AN INDEX:
//
// To avoid endian issues, use masking and ORing, which operates in a
// big-endian register, rather than treating the Index as an array of bytes,
// though that would be simpler, but would operate in endian-specific memory.
//
// TBD:  This contains two variable shifts, is that bad?

// Now only used for JudyByCount.c, JudyPrevNext.c and JudyPrevNextEmpty.c
#define JU_SETDIGIT(INDEX,DIGIT,STATE)                  \
        (INDEX) = ((INDEX) & (~cJU_MASKATSTATE(STATE))) \
                           | (((Word_t) (DIGIT))        \
                              << (((STATE) - 1) * cJU_BITSPERBYTE))

// Fast version for single LSB:

// Now only used for JudyByCount.c and JudyPrevNext.c
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

// Now only used for JudyByCount.c and JudyPrevNext.c
#define JU_SETDCD(INDEX,PJP,cSTATE)                             \
    (INDEX) = ((INDEX) & ~cJU_DCDMASK(cSTATE))                  \
                | (ju_DcdPop0(PJP) & cJU_DCDMASK(cSTATE))

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

#include <string.h> 
#define JU_INSERTINPLACE(PARRAY,POP1,OFFSET,INDEX)              \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) <= (Word_t) (POP1));           \
        {                                                       \
            Word_t n = ((POP1)-(OFFSET)) * sizeof(*(PARRAY));   \
            void *src  = (PARRAY) + (OFFSET);                   \
            void *dest = (PARRAY) + (OFFSET) + 1;               \
            memmove(dest, src, n);                              \
            (PARRAY)[OFFSET] = (INDEX);                         \
        }



// Variation for non-native Indexes, where cIS = Index Size
// and PByte must point to a uint8_t (byte); shift byte-by-byte:
//

#define JU_INSERTINPLACE3(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    Word_t __n = ((POP1)-(OFFSET)) * 3;                           \
    void   *src  = (PBYTE) + ((OFFSET) * 3);                    \
    void   *dest = (PBYTE) + ((OFFSET) * 3) + 3;                \
    memmove(dest, src, __n);                                      \
    JU_COPY3_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 3]), INDEX);   \
}


#define JU_INSERTINPLACE5(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    Word_t __n = ((POP1)-(OFFSET)) * 5;                           \
    void   *src  = (PBYTE) + ((OFFSET) * 5);                    \
    void   *dest = (PBYTE) + ((OFFSET) * 5) + 5;                \
    memmove(dest, src, __n);                                      \
    JU_COPY5_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 5]), INDEX);   \
}

#define JU_INSERTINPLACE6(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    Word_t __n = ((POP1)-(OFFSET)) * 6;                           \
    void   *src  = (PBYTE) + ((OFFSET) * 6);                    \
    void   *dest = (PBYTE) + ((OFFSET) * 6) + 6;                \
    memmove(dest, src, __n);                                      \
    JU_COPY6_LONG_TO_PINDEX(&((PBYTE)[(OFFSET) * 6]), INDEX);   \
}

#define JU_INSERTINPLACE7(PBYTE,POP1,OFFSET,INDEX)              \
{                                                               \
    Word_t __n = ((POP1)-(OFFSET)) * 7;                         \
    void   *src  = (PBYTE) + ((OFFSET) * 7);                    \
    void   *dest = (PBYTE) + ((OFFSET) * 7) + 7;                \
    memmove(dest, src, __n);                                    \
    JU_COPY7_LONG_TO_PINDEX((PBYTE) + ((OFFSET) * 7), INDEX);   \
}


// Counterparts to the above for deleting an Index:
//
// "Shift down" the array elements starting at the Index to be deleted.

#define JU_DELETEINPLACE(PARRAY,POP1,OFFSET)                    \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            void *src  = (PARRAY) + (OFFSET) + 1;               \
            void *dest = (PARRAY) + (OFFSET);                   \
            Word_t __n = ((POP1)-(OFFSET)) * sizeof(*(PARRAY));   \
                                                                \
            memmove(dest, src, __n - 1);                          \
     /*       (PARRAY)[(POP1) - 1] = 0;     zero pad */         \
        }

#define JU_DELETEINPLACE(PARRAY,POP1,OFFSET)                    \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            void *src  = (PARRAY) + (OFFSET) + 1;               \
            void *dest = (PARRAY) + (OFFSET);                   \
            Word_t __n = ((POP1)-(OFFSET)) * sizeof(*(PARRAY));   \
            memmove(dest, src, __n - 1);                          \
     /*       (PARRAY)[(POP1) - 1] = 0;     zero pad */         \
        }

#define JU_DELETEINPLACE_ODD(PBYTE,POP1,OFFSET,cIS)             \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            void *src  = (PBYTE) + (OFFSET * cIS) + cIS;        \
            void *dest = (PBYTE) + (OFFSET * cIS);              \
            Word_t __n = ((POP1)-(OFFSET)) * cIS;                 \
            memmove(dest, src, __n - cIS);                        \
        }


// INSERT/DELETE AN INDEX WHILE COPYING OTHERS:
//
// Copy PSource[] to PDest[], where PSource[] has Pop1 elements (Indexes),
// inserting Index at PDest[Offset].  Unlike JU_*INPLACE*() above, these macros
// are used when moving Indexes from one memory object to another.

#define JU_INSERTCOPY(PDEST,PSOURCE,POP1,OFFSET,INDEX)          \
    assert((long) (POP1) > 0);                                  \
    assert((Word_t) (OFFSET) <= (Word_t) (POP1));               \
    {                                                           \
        uint8_t *src  = (uint8_t *)(PSOURCE);                         \
        uint8_t *dest = (uint8_t *)(PDEST);                           \
        int   cIS = sizeof(*(PDEST));                           \
        Word_t __n = (OFFSET) * cIS;                            \
        memcpy((void *)dest, (void *)src, __n);                 \
        dest += __n;                                            \
        src  += __n;                                            \
        (PDEST)[OFFSET] = (INDEX);          /* insert new */    \
        __n = ((POP1)-(OFFSET)) * cIS;                          \
        dest += cIS;                                            \
        memcpy((void *)dest, (void *)src, __n);                 \
    }

#define JU_INSERTCOPY3(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
    assert((long) (POP1) > 0);                                  \
    assert((Word_t) (OFFSET) <= (Word_t) (POP1));               \
    {                                                           \
        uint8_t *src  = (uint8_t *)(PSOURCE);                         \
        uint8_t *dest = (uint8_t *)(PDEST);                           \
        int   cIS = 3;                                          \
        Word_t __n = (OFFSET) * cIS;                            \
        memcpy((void *)dest, (void *)src, __n);                 \
        dest += __n;                                            \
        src  += __n;                                            \
                                                                \
        JU_COPY3_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * cIS]), INDEX); \
                                                                \
        __n = ((POP1)-(OFFSET)) * cIS;                          \
        dest += cIS;                                            \
        memcpy((void *)dest, (void *)src, __n);                 \
    }

#define JU_INSERTCOPY5(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
    assert((long) (POP1) > 0);                                  \
    assert((Word_t) (OFFSET) <= (Word_t) (POP1));               \
    {                                                           \
        uint8_t *src  = (uint8_t *)(PSOURCE);                         \
        uint8_t *dest = (uint8_t *)(PDEST);                           \
        int   cIS = 5;                                          \
        Word_t __n = (OFFSET) * cIS;                            \
        memcpy((void *)dest, (void *)src, __n);                 \
        dest += __n;                                            \
        src  += __n;                                            \
                                                                \
        JU_COPY5_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * cIS]), INDEX); \
                                                                \
        __n = ((POP1)-(OFFSET)) * cIS;                          \
        dest += cIS;                                            \
        memcpy((void *)dest, (void *)src, __n);                 \
    }

#define JU_INSERTCOPY6(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
    assert((long) (POP1) > 0);                                  \
    assert((Word_t) (OFFSET) <= (Word_t) (POP1));               \
    {                                                           \
        uint8_t *src  = (uint8_t *)(PSOURCE);                         \
        uint8_t *dest = (uint8_t *)(PDEST);                           \
        int   cIS = 6;                                          \
        Word_t __n = (OFFSET) * cIS;                            \
        memcpy((void *)dest, (void *)src, __n);                 \
        dest += __n;                                            \
        src  += __n;                                            \
                                                                \
        JU_COPY6_LONG_TO_PINDEX(&((PDEST)[(OFFSET) * cIS]), INDEX); \
                                                                \
        __n = ((POP1)-(OFFSET)) * cIS;                          \
        dest += cIS;                                            \
        memcpy((void *)dest, (void *)src, __n);                 \
    }

#define JU_INSERTCOPY7(PDEST,PSOURCE,POP1,OFFSET,INDEX)         \
    assert((long) (POP1) > 0);                                  \
    assert((Word_t) (OFFSET) <= (Word_t) (POP1));               \
    {                                                           \
        uint8_t *src  = (uint8_t *)(PSOURCE);                   \
        uint8_t *dest = (uint8_t *)(PDEST);                     \
        int   cIS = 7;                                          \
        Word_t __n = (OFFSET) * cIS;                            \
        memcpy((void *)dest, (void *)src, __n);                 \
        dest += __n;                                            \
        src  += __n;                                            \
                                                                \
        assert(dest == (uint8_t *)(PDEST) + ((OFFSET) * cIS));  \
    /*  JU_COPY7_LONG_TO_PINDEX(&((PDEST)(OFFSET) * cIS]), INDEX); */ \
        JU_COPY7_LONG_TO_PINDEX(dest, (INDEX));                 \
                                                                \
        __n = ((POP1)-(OFFSET)) * cIS;                          \
        dest += cIS;                                            \
        memcpy((void *)dest, (void *)src, __n);                 \
    }

#ifdef orig
void static JU_INSERTCOPY7(uint8_t *dest, uint8_t *src,  Word_t POP1, int OFFSET, Word_t Key)
{
    assert((long) (POP1) > 0);
    assert((Word_t) (OFFSET) <= (Word_t) (POP1));
    {                                           
        int   cIS = 7;                         
        Word_t __n = (OFFSET) * cIS;          
        memcpy((void *)dest, (void *)src, __n);
        dest += __n;                          
        src  += __n;                         
                                                     
        JU_COPY7_LONG_TO_PINDEX(dest, Key);     
                                                   
        __n = ((POP1)-(OFFSET)) * cIS;            
        dest += cIS;                             
        memcpy((void *)dest, (void *)src, __n);
    }
}
#endif  // orig

#define JU_DELETECOPY_ODD(PDEST,PSOURCE,POP1,OFFSET,cIS)        \
        assert((long) (POP1) > 0);                              \
        assert((Word_t) (OFFSET) < (Word_t) (POP1));            \
        {                                                       \
            uint8_t *src  = (uint8_t *)(PSOURCE);               \
            uint8_t *dest = (uint8_t *)(PDEST);                 \
            Word_t __n = (OFFSET) * cIS;                        \
            memcpy((void *)dest, (void *)src, __n);             \
            dest += __n;                                        \
            src  += __n + cIS;   /* skip deleted Key */         \
            __n = (((POP1 - 1)-(OFFSET)) * cIS);                \
            memcpy((void *)dest, (void *)src, __n);             \
        }

#define JU_DELETECOPY(PDEST,PSOURCE,POP1,OFFSET)                \
    JU_DELETECOPY_ODD(PDEST,PSOURCE,POP1,OFFSET,sizeof(*(PDEST)))


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

#define JU_RET_FOUND_LEAF8(PJLL8,POP1,OFFSET)   JU_RET_FOUND

#define JU_RET_FOUND_JPM(Pjpm)                  JU_RET_FOUND
#define JU_RET_FOUND_PVALUE(Pjv,OFFSET)         JU_RET_FOUND

#define JU_RET_FOUND_LEAF1(Pjll1,POP1,OFFSET)   JU_RET_FOUND
#define JU_RET_FOUND_LEAF2(Pjll2,POP1,OFFSET)   JU_RET_FOUND
#define JU_RET_FOUND_LEAF3(Pjll3,POP1,OFFSET)   JU_RET_FOUND
#define JU_RET_FOUND_LEAF4(Pjll4,POP1,OFFSET)   JU_RET_FOUND
#define JU_RET_FOUND_LEAF5(Pjll5,POP1,OFFSET)   JU_RET_FOUND
#define JU_RET_FOUND_LEAF6(Pjll6,POP1,OFFSET)   JU_RET_FOUND
#define JU_RET_FOUND_LEAF7(Pjll7,POP1,OFFSET)   JU_RET_FOUND
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

// THESE are left over for Count & NextPrev !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define JU_RET_FOUND_PVALUE(Pjv,OFFSET) return((PPvoid_t) ((Pjv) + OFFSET))

#define JU_RET_FOUND_LEAF8(PJLL8,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF8VALUEAREA(PJLL8, POP1) + (OFFSET)))

#define JU_RET_FOUND_LEAF1(Pjll1,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF1VALUEAREA(Pjll1, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF2(Pjll2,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF2VALUEAREA(Pjll2, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF3(Pjll3,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF3VALUEAREA(Pjll3, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF4(Pjll4,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF4VALUEAREA(Pjll4, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF5(Pjll5,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF5VALUEAREA(Pjll5, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF6(Pjll5,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF6VALUEAREA(Pjll6, POP1) + (OFFSET)))
#define JU_RET_FOUND_LEAF7(Pjll7,POP1,OFFSET) \
                return((PPvoid_t) (JL_LEAF7VALUEAREA(Pjll7, POP1) + (OFFSET)))

// Note:  Here jp_Addr0 is a value area itself and not an address, so P_JV() is
// not needed:

//#define JU_RET_FOUND_IMM_01(PJP)  return((PPvoid_t) (&((PJP)->jp_PValue)))
#define JU_RET_FOUND_IMM_01(PJP)  return((PPvoid_t) ju_PImmVal_01(PJP))

// Note:  Here jp_Addr0 is a pointer to a separately-mallocd value area, so
// P_JV() is required; likewise for JL_JLB_PVALUE:

//#            return((PPvoid_t) (P_JV((PJP)->jp_PValue) + (OFFSET)))
#define JU_RET_FOUND_IMM(PJP,OFFSET) \
            return((PPvoid_t) (P_JV(ju_PntrInJp(PJP)) + (OFFSET)))

#ifndef BMVALUE

#define JU_RET_FOUND_LEAF_B1(PJLB,SUBEXP,OFFSET) \
            return((PPvoid_t) (JL_JLB_PVALUE(PJLB) + (OFFSET)))

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
                JU_ERRNO(PJError) = JU_ERRNO(Pjpm);             \
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

//#define JU_CHECKALLOC(Type,Ptr,Retval) 
//        {                               
//            JU_SET_ERRNO(PJError, JU_ALLOC_ERRNO(Ptr));
//            return(Retval);                           
//        }


// Leaf search routines

// This calculates a proportional division of Pop1 and significant hi-bits
// to produce a statical starting offset to begin search.
#define PSPLIT(POP, KEY, LOG2)  (((Word_t)(KEY) * (POP)) >> (LOG2))

// Cheap and dirty search for matching branchL
static inline int j__udySearchBranchL(uint8_t *Lst, int pop1, uint8_t Exp)
{
    SEARCHLINARNATIVE(uint8_t, Lst, pop1, Exp, 0); 
}

static inline int j__udySearchRawLeaf1(uint8_t *PLeaf1, int LeafPop1, Word_t Index, int Start)
{
    SEARCHLEAFNATIVE(uint8_t, PLeaf1, LeafPop1, (uint8_t)Index, Start); 
}

static inline int j__udySearchLeaf1(Pjll1_t Pjll1, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 1);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
//    printf("Pjll1->jl1_Leaf = 0x%016lx, LeafPop1 = %d, Index = 0x%016lx, Start = %d\n", (Word_t)Pjll1->jl1_Leaf, (int)LeafPop1, Index, (int)Start);
    SEARCHLEAFNATIVE(uint8_t, Pjll1->jl1_Leaf, LeafPop1, Index, Start); 
}

static inline int j__udySearchImmed1(uint8_t *Pleaf1, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 1);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNATIVE(uint8_t,  Pleaf1, LeafPop1, Index, Start); 
}

static inline int j__udySearchLeaf2(Pjll2_t Pjll2, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 2);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNATIVE(uint16_t,  Pjll2->jl2_Leaf, LeafPop1, Index, Start); 
}

static inline int j__udySearchImmed2(uint16_t *Pleaf2, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 2);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNATIVE(uint16_t,  Pleaf2, LeafPop1, Index, Start); 
}

static inline int j__udySearchLeaf3(Pjll3_t Pjll3, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 3);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNONNAT(Pjll3->jl3_Leaf, LeafPop1, Index, 3, JU_COPY3_PINDEX_TO_LONG, Start); 
}

static inline int j__udySearchImmed3(uint8_t *Pleaf3, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 3);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNONNAT(Pleaf3, LeafPop1, Index, 3, JU_COPY3_PINDEX_TO_LONG, Start); 
}

static inline int j__udySearchLeaf4(Pjll4_t Pjll4, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 4);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNATIVE(uint32_t, Pjll4->jl4_Leaf, LeafPop1, Index, Start); 
}

static inline int j__udySearchImmed4(uint32_t *Pleaf4, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 4);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNATIVE(uint32_t,  Pleaf4, LeafPop1, Index, Start); 
}

static inline int j__udySearchLeaf5(Pjll5_t Pjll5, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 5);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNONNAT(Pjll5->jl5_Leaf, LeafPop1, Index, 5, JU_COPY5_PINDEX_TO_LONG, Start); 
}

static inline int j__udySearchImmed5(uint8_t *Pleaf5, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 5);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLEAFNONNAT(Pleaf5, LeafPop1, Index, 5, JU_COPY5_PINDEX_TO_LONG, Start); 
}

static inline int j__udySearchLeaf6(Pjll6_t Pjll6, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 6);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
//    SEARCHLEAFNONNAT(Pjll6->jl6_Leaf, LeafPop1, Index, 6, JU_COPY6_PINDEX_TO_LONG, Start); 
    SEARCHLINARNONNAT(Pjll6->jl6_Leaf, LeafPop1, Index, 6, JU_COPY6_PINDEX_TO_LONG, 0); 
}

static inline int j__udySearchImmed6(uint8_t *Pleaf6, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 6);
//    int Start = PSPLIT(LeafPop1, Index, Expanse); 
    SEARCHLINARNONNAT(Pleaf6, LeafPop1, Index, 6, JU_COPY6_PINDEX_TO_LONG, 0); 
}

static inline int j__udySearchLeaf7(Pjll7_t Pjll7, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 7);
    int Start = PSPLIT(LeafPop1, Index, Expanse); 
//    SEARCHLEAFNONNAT(Pjll7->jl7_Leaf, LeafPop1, Index, 7, JU_COPY7_PINDEX_TO_LONG, Start); 
    SEARCHLINARNONNAT(Pjll7->jl7_Leaf, LeafPop1, Index, 7, JU_COPY7_PINDEX_TO_LONG, 0); 
}

static inline int j__udySearchImmed7(uint8_t *Pleaf7, int LeafPop1, Word_t Index, int Expanse)
{
    SEARCHPOPULATION(LeafPop1);
    Index = JU_LEASTBYTES(Index, 7);
//    int Start = PSPLIT(LeafPop1, Index, Expanse); 
//    SEARCHLEAFNONNAT(Pleaf7, LeafPop1, Index, 7, JU_COPY7_PINDEX_TO_LONG, Start); 
    SEARCHLINARNONNAT(Pleaf7, LeafPop1, Index, 7, JU_COPY7_PINDEX_TO_LONG, 0); 
}

static inline int j__udySearchLeaf8(Pjll8_t Pjll8, int LeafPop1, Word_t Index)
{
    SEARCHPOPULATION(LeafPop1);
//    SEARCHLINARNATIVE(Word_t,  PLeaf8, LeafPop1, Index, 0); 
    SEARCHBINARYNATIVE(Word_t, Pjll8->jl8_Leaf, LeafPop1, Index, 0); 
}
#endif // ! _JUDYPRIVATE_INCLUDED
