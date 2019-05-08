#ifndef _JUDY_PRIVATE_BRANCH_INCLUDED
#define _JUDY_PRIVATE_BRANCH_INCLUDED
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

// @(#) $Revision: 4.57 $ $Source: /judy/src/JudyCommon/JudyPrivateBranch.h $
//
// Header file for all Judy sources, for global but private (non-exported)
// declarations specific to branch support.
//
// See also the "Judy Shop Manual" (try judy/doc/int/JudyShopManual.*).


// ****************************************************************************
// JUDY POINTER (JP) SUPPORT
// ****************************************************************************
//
// This "rich pointer" object is pivotal to Judy execution.
//
// JP CONTAINING OTHER THAN IMMEDIATE INDEXES:
//
// If the JP points to a linear or bitmap leaf, jp_DcdPopO contains the
// Population-1 in LSbs and Decode (Dcd) bytes in the MSBs.  (In practice the
// Decode bits are masked off while accessing the Pop0 bits.)
//
// The Decode Size, the number of Dcd bytes available, is encoded in jpo_Type.
// It can also be thought of as the number of states "skipped" in the SM, where
// each state decodes 8 bits = 1 byte.
//
// TBD:  Dont need two structures, except possibly to force jp_Type to highest
// address!
//
// Note:  The jpo_u union is not required by HP-UX or Linux but Win32 because
// the cl.exe compiler otherwise refuses to pack a bitfield (DcdPopO) with
// anything else, even with the -Zp option.  This is pretty ugly, but
// fortunately portable, and its all hide-able by macros (see below).

//  ------------------------------------------------------------------
//                   J U D Y L  &  J U D Y 1
//  ------------------------------------------------------------------

//    =================================================================
//    |          Little Endian Branch/Leaf Pointer                    |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    |        Pointer to Branches/Leafs (48 Bits)    |jp_JPop|jp_Type|
//    |                      D C D  ~|~ Pop0 of Branch                |
//    =================================================================

//  ------------------------------------------------------------------
//                          J U D Y L 
//  ------------------------------------------------------------------
//    =================================================================
//    |          Little Endian Bitmap Leaf 6 bit decode Key           |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    | Pointer to Value area JudyL                   |jp_BPop|jp_Type|
//    |                    B i t  M a p  (64 bits - 6 bits decode_    |
//    =================================================================

//    =================================================================
//    |          Little Endian Immed_[1..7]_01 only 56 bit (2) Key    |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    |                   I M M E D 56  56 bit                |jp_Type|
//    |                   V     A     L     U     E                   |
//    =================================================================

//    =================================================================
//    |          Little Endian Immed 8 bit (8) decode Key             |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    | Pointer to IMMEDs VALUES in JudyL (48 bits)   |jp_IPop|jp_Type|
//    |Imm[7] |Imm[6] |Imm[5] |Imm[4] |Imm[3] |Imm[2] |Imm[1] |Imm[0] |
//    =================================================================
//    Note: 2 * 3 < 8,      5 * 1 < 8,     6 * 1 < 8,     7 * 1 < 8

//    =================================================================
//    |          Little Endian Immed 16 bit (4) decode Key            |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    | Pointer to IMMEDs Values in JudyL (48 bits)   |jp_IPop|jp_Type|
//    |     Immed[3]  |   Immed[2]    |   Immed[1]    |   Immed[0]    |
//    =================================================================

//    =================================================================
//    |          Little Endian Immed 32 bit (2) decode Key            |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    | Pointer to IMMEDs Values in JudyL (48 bits)   |jp_IPop|jp_Type|
//    |            Immed[1]           |            Immed[0]           |
//    =================================================================

//  ------------------------------------------------------------------
//                          J U D Y 1 
//  ------------------------------------------------------------------
//    =================================================================
//    |          Little Endian Immed 56/64 bit (2) Key                |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    |                   I M M E D X_56  56 bit Key          |jp_Type|
//    |                   I M M E D X_64  64 bit Key                  |
//    =================================================================

//    =================================================================
//    |          Little Endian Immed 32 bit (3) decode Key - Judy1    |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    |            Immed[0]           | spare 16 bits |jp_IPop|jp_Type|
//    |            Immed[2]           |            Immed[1]           |
//    =================================================================

//    =================================================================
//    |          Little Endian Immed 16 bit (7) decode Key            |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    |   Immed[2]   |   Immed[1]     |   Immed[0]    |jp_IPop|jp_Type|
//    |   Immed[6]   |   Immed[5]     |   Immed[4]    |  Immed[3]     |
//    =================================================================

//    =================================================================
//    |          Little Endian Immed 8 bit (15) decode Key            |
//    |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   | 
//    |Imm[6] |Imm[5] |Imm[4] |Imm[3] |Imm[2] |Imm[1] |Imm[0] |jp_Type|
//    |Imm[14]|Imm[13]Imm[12]|Imm[11]| Imm[10]|Imm[9] |Imm[8] |Imm[7] |
//    =================================================================
//    Note: (3) * 5 == 15,  (5) * 3 <= 15,  (6) * 2 <= 15,  (7) * 2 <= 15

typedef struct J_UDY_POINTER_OTHERS     // JPO.
{
        union
        {
            Word_t  j_po_Addr0;         // only the hi 48 bits are available
            uint8_t j_po_jp_Type[2];    // jp_Type & jp_LeafPop1 in lo 16 bits
        };
        union
        {
            Word_t  j_po_Addr1;         // 2nd Word_t
            Word_t  j_po_subLeafPops;
            uint8_t j_po_subPop1[8];
        };
} jpo_t;

// JP CONTAINING IMMEDIATE INDEXES:
//
// Note:  Actually when Pop1 = 1, jpi_t is not used, and the least bytes of the
// single Index are stored n j_po_DcdPopO, for both Judy1 and JudyL, so for
// JudyL the j_po_Addr field can hold the target value.
//
// TBD:  Revise this structure to not overload j_po_DcdPopO this way?  The
// current arrangement works, its just confusing.

#ifdef  JUDY1
typedef struct _JUDY_POINTER_IMMED1  
{
        union 
        {   // jp_Type is low addrs and each array is 2 Word_t
            uint8_t  j_pi_1Index1[(2 * sizeof(Word_t))/1];   // 15 Keys
            uint16_t j_pi_1Index2[(2 * sizeof(Word_t))/2];   // 7 Keys
            uint32_t j_pi_1Index4[(2 * sizeof(Word_t))/4];   // 3 Keys
        };
} jpi_t;
#endif  // JUDY1

#ifdef  JUDYL
typedef struct _JUDY_POINTER_IMMEDL      // JPI.
{
        Word_t   j_pi_Space;       // (48 bit) pointer to Values
        union 
        {
            uint8_t  j_pi_LIndex1[sizeof(Word_t)];      // 8 Keys in 2nd word
            uint16_t j_pi_LIndex2[sizeof(Word_t)/2];    // 4 Keys in 2nd word
            uint32_t j_pi_LIndex4[sizeof(Word_t)/4];    // 2 Keys in 2nd word
        };
} jpi_t;
#endif  // JUDYL

// UNION OF JP TYPES:
//
// A branch is an array of cJU_BRANCHUNUMJPS (256) of this object, or an
// alternate data type such as:  A linear branch which is a list of 2..7 JPs,
// or a bitmap branch which contains 8 lists of 0..32 JPs.  JPs reside only in
// branches of a Judy SM.

typedef union J_UDY_POINTER         // JP.
{
        jpo_t j_po;                 // other than immediate Keys.
        jpi_t j_pi;                 // immediate Keys.
} jp_t, *Pjp_t;

// For coding convenience:
#ifdef  JUDY1
// These are offset by one key to make room for jp_Type - no jp_Ipop for 1,3,5,6,7
#define jp_PIndex1 j_pi.j_pi_1Index1    // for storing uint8_t  Keys in 1st  word.
#define jp_PIndex2 j_pi.j_pi_1Index2    // for storing uint16_t Keys in 1st  word.
#define jp_PIndex4 j_pi.j_pi_1Index4    // for storing uint32_t Keys in 1st  word.
#endif  //JUDY1

#ifdef  JUDYL
#define jp_PIndex1 j_pi.j_pi_LIndex1    // for storing uint8_t in first word.
#define jp_PIndex2 j_pi.j_pi_LIndex2    // for storing uint16_t in first word
#define jp_PIndex4 j_pi.j_pi_LIndex4    // for storing uint16_t in first word
#define jp_PValue  j_pi.j_pi_PValue          // Pointer to Values for IMMED_[1..7]_[02..15]
#endif  // JUDYL

#define jp_Type     j_po.j_po_jp_Type[0] // jp_Type
#define jp_LeafPop1 j_po.j_po_jp_Type[1] // jp_LeafPop1
#define jp_Addr0    j_po.j_po_Addr0      // only the hi 48 bits are available
#define jp_Addr1    j_po.j_po_Addr1      // 2nd Word in jp
#define jp_subPop1  j_po.j_po_subPop1    // uint8_t[] 8 octants in array
#define jp_subLeafPops  j_po.j_po_subLeafPops    // Word_t -- 8 octants in Word_t

// Mask off the high byte from INDEX to it can be compared to DcdPopO:
#define JU_TrimToIMM01(INDEX) ((cJU_ALLONES >> cJU_BITSPERBYTE) & (INDEX))

#define cJU_POP0MASK(cPopBytes) JU_LEASTBYTESMASK(cPopBytes)

// EXTRACT VALUES FROM JP:
//
// Masks for the bytes in the Dcd and Pop0 parts of jp_DcdPopO:
//
// cJU_DCDMASK() consists of a mask that excludes the (LSb) Pop0 bytes and
// also, just to be safe, the top byte of the word, since jp_DcdPopO is 1 byte
// less than a full word.
//
// Note:  These are constant macros (cJU) because cPopBytes should be a
// constant.  Also note cPopBytes == state in the SM.

#define cJU_DCDMASK(cPopBytes) (~JU_LEASTBYTESMASK(cPopBytes))

// DETERMINE IF AN INDEX IS (NOT) IN A JP EXPANSE:
//#define JU_DCDNOTMATCHINDEX(INDEX,PJP,POP0BYTES) (((INDEX) ^ ju_DcdPop0(PJP)) & cJU_DCDMASK(POP0BYTES))

//
//      Get routines
//

static uint8_t ju_Type(Pjp_t Pjp)
        { return(Pjp->jp_Type); } 

static Word_t  ju_PntrInJp(Pjp_t Pjp)
        { return(Pjp->jp_Addr0 >> 16); } 

// temp for now -- low 16 bits available
static Word_t  ju_PntrInJp1(Pjp_t Pjp)
        { return(Pjp->jp_Addr1 >> 16); } 

static Word_t  ju_DcdPop0(Pjp_t Pjp)
        { return(Pjp->jp_Addr1); } 

static Word_t  ju_LeafPop1(Pjp_t Pjp)
        { return(Pjp->jp_LeafPop1); }   // TEMP - should be below

static Word_t  ju_BranchPop0(Pjp_t Pjp, int level)
        { return(Pjp->jp_Addr1 & JU_LEASTBYTESMASK(level)); }

//      return non-zero if DCD does not match
static Word_t  ju_DcdNotMatch(Word_t Key, Word_t DcdPop0, int level)
        { 
#ifdef  PCASNOT
              Word_t res = (Key ^ DcdPop0) & cJU_DCDMASK(level); 
if (res) printf("\n+FAILED DCD Check++++ ju_DcdNotMatch: DcdP0 = 0x%016lx Key = 0x%016lx mask = 0x%016lx level = %d\n", DcdPop0, Key, cJU_DCDMASK(level), level);
else     printf("\n+PASSED DCD Check++++ ju_DcdNotMatch: DcdP0 = 0x%016lx Key = 0x%016lx mask = 0x%016lx level = %d\n", DcdPop0, Key, cJU_DCDMASK(level), level);
#endif  // PCASNOT
              return((Key ^ DcdPop0) & cJU_DCDMASK(level)); 
        }

static Word_t  ju_DcdNotMatchKey(Word_t Key, Pjp_t Pjp, int level)
        { 
            return (ju_DcdNotMatch(Key, ju_DcdPop0(Pjp), level));
        }

// More generic version for different DCD(Pop0) positions
static Word_t  ju_DcdNotMatchKeyLeaf(Word_t KEY, Word_t DCD, int LEVEL)
        { return((KEY ^ DCD) & cJU_DCDMASK(LEVEL)); }

static Word_t ju_IMM01Key(Pjp_t Pjp)    // only low 56 bits of Key
        { return(Pjp->jp_Addr0 >> 8); } // shift out jp_Type

// Return Value in Immed_x_01
static Word_t ju_ImmVal_01(Pjp_t Pjp)
        { return(Pjp->jp_Addr1); }


//
//      Set routines
//

static void ju_SetIMM01(Pjp_t Pjp, Word_t Value, Word_t Key, uint8_t Type)
        { 
            Pjp->jp_Addr0 = (Key << 8) | Type;
//            Pjp->jp_Type  = Type; // lo-byte of Addr0 (see above)
            Pjp->jp_Addr1 = Value;
        } 

static void ju_SetLeafPop1(Pjp_t Pjp, Word_t LeafPop1)
        { Pjp->jp_LeafPop1 = (uint8_t)LeafPop1; }       // TEMP!!!!!

static void ju_SetPntrInJp(Pjp_t Pjp, Word_t PntrInJp)
        { 
            Word_t Types = Pjp->jp_Addr0 & 0xFFFF;
            Pjp->jp_Addr0 = (PntrInJp << 16) | Types; 
        }

// temp for now -- low 16 bits available
static void ju_SetPntrInJp1(Pjp_t Pjp, Word_t PntrInJp)
        { 
            Word_t Types = Pjp->jp_Addr1 & 0xFFFF;
            Pjp->jp_Addr1 = (PntrInJp << 16) | Types; 
        }

static void ju_SetJpType(Pjp_t Pjp, uint8_t Type)
        { Pjp->jp_Type  = Type; }

static void ju_SetDcdPop0(Pjp_t Pjp, Word_t DcdPop0)
        { Pjp->jp_Addr1 = DcdPop0; }

// Temp
static void ju_SetDcdPop0Leaf(Word_t PJLL, Word_t DcdPop0)
        { ((PWord_t)(PJLL))[0] = DcdPop0; }        // 1st Word in Leaf

#ifdef  JUDYL
//
//      JUDYL Get routines
//
// Return pointer to Value in Immed_[1..8]_01
static Pjv_t ju_PImmVal_01(Pjp_t Pjp)
        { return((Pjv_t)(&Pjp->jp_Addr1)); }

static uint8_t *ju_PImmed1(Pjp_t Pjp)
        { return(Pjp->jp_PIndex1); }   // used for 1,3,5,6,7 Keys

#define ju_PImmed3 ju_PImmed1
#define ju_PImmed5 ju_PImmed1
#define ju_PImmed6 ju_PImmed1
#define ju_PImmed7 ju_PImmed1

static uint16_t *ju_PImmed2(Pjp_t Pjp)
        { return(Pjp->jp_PIndex2); }

static uint32_t *ju_PImmed4(Pjp_t Pjp)
        { return(Pjp->jp_PIndex4); }
#endif  // JUDYL

#ifdef  JUDY1

//
//      JUDY1 Get routines
//

static uint8_t *ju_PImmed1(Pjp_t Pjp)   // used for 1,3,5,6,7 Keys
        { return(Pjp->jp_PIndex1 + 1); }

#define ju_PImmed3 ju_PImmed1
#define ju_PImmed5 ju_PImmed1
#define ju_PImmed6 ju_PImmed1
#define ju_PImmed7 ju_PImmed1

static uint16_t *ju_PImmed2(Pjp_t Pjp)  // 2 byte Keys
        { return(Pjp->jp_PIndex2 + 1); }

static uint32_t *ju_PImmed4(Pjp_t Pjp)  // 4 byte Keys
        { return(Pjp->jp_PIndex4 + 1); }
#endif  // JUDY1

// ****************************************************************************
// JUDY POINTER (JP) -- RELATED MACROS AND CONSTANTS
// ****************************************************************************

// Get from jp_DcdPopO the Pop0 for various branch JP Types:
//
// Note:  There are no simple macros for cJU_BRANCH* Types because their
// populations must be added up and dont reside in an already-calculated
// place.

#define JU_JPBRANCH_POP0(PJP,cPopBytes) \
        (ju_DcdPop0(PJP) & JU_LEASTBYTESMASK(cPopBytes))

// METHOD FOR DETERMINING IF OBJECTS HAVE ROOM TO GROW:
//
// J__U_GROWCK() is a generic method to determine if an object can grow in
// place, based on whether the next population size (one more) would use the
// same space.

#define J__U_GROWCK(POP1,MAXPOP1,POPTOWORDS) \
        (POPTOWORDS[POP1] == POPTOWORDS[(POP1) + 1])
////        (((POP1) != (MAXPOP1)) && (POPTOWORDS[POP1] == POPTOWORDS[(POP1) + 1]))

#define JU_BRANCHBJPGROWINPLACE(NumJPs) \
        J__U_GROWCK(NumJPs, cJU_BITSPERSUBEXPB, j__U_BranchBJPPopToWords)

// NUMBER OF JPs IN AN UNCOMPRESSED BRANCH:
//
// An uncompressed branch is simply an array of 256 Judy Pointers (JPs).  It is
// a minimum cacheline fill object.  Define it here before its first needed.

#define cJU_BRANCHUNUMJPS  cJU_SUBEXPPERSTATE


// ****************************************************************************
// JUDY BRANCH LINEAR (JBL) SUPPORT
// ****************************************************************************
//
// A linear branch is a way of compressing empty expanses (null JPs) out of an
// uncompressed 256-way branch, when the number of populated expanses is so
// small that even a bitmap branch is excessive.
//
// The maximum number of JPs in a Judy linear branch:
//
// Note:  This number results in a 1-cacheline sized structure.  Previous
// versions had a larger struct so a linear branch didnt become a bitmap
// branch until the memory consumed was even, but for speed, its better to
// switch "sooner" and keep a linear branch fast.

#ifndef cJU_BRANCHLMAXJPS
#define cJU_BRANCHLMAXJPS 7
#endif  // cJU_BRANCHLMAXJPS


// LINEAR BRANCH STRUCT:
//
// 1-byte count, followed by array of byte-sized expanses, followed by JPs.

typedef struct J__UDY_BRANCH_LINEAR
        {
            Word_t  jbl_RootStruct;                 // ^ to root Structure
            Word_t  jbl_DCDPop0;                    // DCDPop0
            uint8_t jbl_NumJPs;                     // num of JPs (Pjp_t), 1..N.
            uint8_t jbl_Expanse[cJU_BRANCHLMAXJPS]; // 1..7 MSbs of pop exps.
            jp_t    jbl_jp     [cJU_BRANCHLMAXJPS]; // JPs for populated exps.
        } jbl_t, * Pjbl_t;

// ****************************************************************************
// JUDY LEAF MISC rouines
// ****************************************************************************

// make a mask from sub-expanse number (0..15) for suming prep
#define ADDTOSUBLEAF16(SUBEXP16) ((Word_t)0x1 << ((SUBEXP16) * 4))

#define MASK16SUB(EXP16)        ((Word_t)1 << ((EXP16 * 4)) - 1)
#define MASK16KEY(SUBEXPNUM16)    ((Word_t)0xF << ((SUBEXPNUM  * 4)) - 1)

// make a mask from sub-expanse number (0..32) for suming prep
//#define ADDTOSUBLEAF8(SUBEXP8)  ((Word_t)0x1 << ((SUBEXP8) * 8))

#define MASK8SUB(EXP8)          ((Word_t)1 << ((EXP8  * 8)) - 1)
#define MASK8KEY(EXP8,SUBEXPNUM8)    ((Word_t)0x3F << ((EXP8  * 4)) - 1)

// Get the sum Total Population of a 8 sub-expanse Word_t
//  Note: the max sum is 255 or an overflow will occur
static int j__udySubLeaf8Pops(Word_t SubPops8)
{
    return((SubPops8 * (Word_t)0x0101010101010101) >> 56);
}

// Convert a 16 sub-expanse POPS Word_t to 8 sub-expanse POPS Word_t
// Note: the max sum is 240 = 15 * 16
static Word_t j__udyConv16To8(Word_t SubPops16)
{
    Word_t oddpops = SubPops16 & (Word_t)0x0f0f0f0f0f0f0f0f;

//  Parallel sum the even pops with the odd pops
    return((((SubPops16 ^ oddpops) >> 4) + oddpops));
}

// Get the sum total Population of a 16 sub-expanse Word_t
static Word_t j__udySubLeaf16Pops(Word_t SubPops16)
{
//  Convert to 8 and Sum all 8 bytes using a mpy
    return(j__udySubLeaf8Pops(j__udyConv16To8(SubPops16)));
}

// ****************************************************************************
// JUDY BRANCH BITMAP (JBB) SUPPORT
// ****************************************************************************
//
// A bitmap branch is a way of compressing empty expanses (null JPs) out of
// uncompressed 256-way branch.  This costs 1 additional cache line fill, but
// can save a lot of memory when it matters most, near the leaves, and
// typically there will be only one at most in the path to any Index (leaf).
//
// The bitmap indicates which of the cJU_BRANCHUNUMJPS (256) JPs in the branch
// are NOT null, that is, their expanses are populated.  The jbb_t also
// contains N pointers to "mini" Judy branches ("subexpanses") of up to M JPs
// each (see BITMAP_BRANCHMxN, for example, BITMAP_BRANCH32x8), where M x N =
// cJU_BRANCHUNUMJPS.  These are dynamically allocated and never contain
// cJ*_JPNULL* jp_Types.  An empty subexpanse is represented by no bit sets in
// the corresponding subexpanse bitmap, in which case the corresponding
// jbbs_Pjp pointers value is unused.
//
// Note that the number of valid JPs in each 1-of-N subexpanses is determined
// by POPULATION rather than by EXPANSE -- the desired outcome to save memory
// when near the leaves.  Note that the memory required for 185 JPs is about as
// much as an uncompressed 256-way branch, therefore 184 is set as the maximum.
// However, it is expected that a conversion to an uncompressed 256-way branch
// will normally take place before this limit is reached for other reasons,
// such as improving performance when the "wasted" memory is well amortized by
// the population under the branch, preserving an acceptable overall
// bytes/Index in the Judy array.
//
// The number of pointers to arrays of JPs in the Judy bitmap branch:
//
// Note:  The numbers below are the same in both 32 and 64 bit systems.

//#define cJU_BRANCHBMAXJPS  184          // maximum JPs for bitmap branches.
#define cJU_BRANCHBMAXJPS  86          // maximum JPs for bitmap branches.

// Convenience wrappers for referencing BranchB bitmaps or JP subarray
// pointers:
//
// Note:  JU_JBB_PJP produces a "raw" memory address that must pass through
// P_JP before use, except when freeing memory:

#define JU_JBB_BITMAP(Pjbb, SubExp)  ((Pjbb)->jbb_jbbs[SubExp].jbbs_Bitmap)
#define JU_JBB_PJP(   Pjbb, SubExp)  ((Pjbb)->jbb_jbbs[SubExp].jbbs_Pjp)

#define JU_SUBEXPB(Digit) (((Digit) / cJU_BITSPERSUBEXPB) & (cJU_NUMSUBEXPB-1))

#define JU_BITMAPTESTB(Pjbb, Index) \
        (JU_JBB_BITMAP(Pjbb, JU_SUBEXPB(Index)) &  JU_BITPOSMASKB(Index))

#define JU_BITMAPSETB(Pjbb, Index)  \
        (JU_JBB_BITMAP(Pjbb, JU_SUBEXPB(Index)) |= JU_BITPOSMASKB(Index))

// Note:  JU_BITMAPCLEARB is not defined because the code does it a faster way.

typedef struct J__UDY_BRANCH_BITMAP_SUBEXPANSE
        {
            BITMAPB_t jbbs_Bitmap;
            Word_t    jbbs_Pjp;

        } jbbs_t;

// static size
typedef struct J__UDY_BRANCH_BITMAP
        {
            Word_t jbb_RootStruct;                  // ^ to root Structure
            Word_t jbb_DCDPop0;                     // DCDPop0
            jbbs_t jbb_jbbs   [cJU_NUMSUBEXPB];
//            Word_t jbbs_Pjp[];
            Word_t jbb_NumJPs;         // number of pointers in jp_t s

        } jbb_t, * Pjbb_t;

#define JU_BRANCHJP_NUMJPSTOWORDS(NumJPs) (j__U_BranchBJPPopToWords[NumJPs])

// ****************************************************************************
// JUDY BITMAP LEAF2
// ****************************************************************************

#define cJLB2_EXP       ((Word_t)65336)
#define cJLB2_BM        (cJLB2_EXP/cbPW)     // 2k[1k]
#define cJLB2_WORDS     (sizeof(jlb2_t[cJLB2_BM]) / cBPW)

// ****************************************************************************
// JUDY BRANCH UNCOMPRESSED (JBU) SUPPORT
// ****************************************************************************

typedef struct J__UDY_BRANCH_UNCOMPRESSED
{
//        Word_t jbu_RootStruct;                  // ^ to root Structure
        Word_t jbu_DCDPop0;                     // DCDPop0
        Word_t jbu_NumJPs;                     // number of Pointers in jp_t s
        jp_t   jbu_jp     [cJU_BRANCHUNUMJPS];  // JPs for populated exp.
//        jp_t   jbu_jp[0];                     // JPs for populated exp.
} jbu_t, * Pjbu_t;


// ****************************************************************************
// OTHER SUPPORT FOR JUDY STATE MACHINES (SMs)
// ****************************************************************************

// OBJECT SIZES IN WORDS:
//
// Word_ts per various JudyL structures that have constant sizes.
// cJU_WORDSPERJP should always be 2; this is fundamental to the Judy
// structures.

#define cJU_WORDSPERJP (sizeof(jp_t)   / cJU_BYTESPERWORD)
#define cJU_WORDSPERCL (cJU_BYTESPERCL / cJU_BYTESPERWORD)


// OPPORTUNISTIC UNCOMPRESSION:
//
// Define populations at which a BranchL or BranchB must convert to BranchU.
// Earlier conversion is possible with good memory efficiency -- see below.

#ifndef noU

// Max population below BranchL, then convert to BranchU:
// Note: if no conversion is allowed, the cost is 3 - 6 nS in Get/Test speed

#ifndef JU_BRANCHL_MAX_POP
#define JU_BRANCHL_MAX_POP      1000
#endif  // ! JU_BRANCHL_MAX_POP

// Minimum global population increment before next conversion of a BranchB to a
// BranchU:
//
// This is was done to allow malloc() to coalesce memory before the next big
// (~512 words) allocation.

//#define JU_BTOU_POP_INCREMENT    300
#define JU_BTOU_POP_INCREMENT    150

// Min/max population below BranchB, then convert to BranchU:

#ifndef JU_BRANCHB_MIN_POP
#define JU_BRANCHB_MIN_POP       135
#endif  // ! JU_BRANCHB_MIN_POP

#define JU_BRANCHB_MAX_POP       750

#else // noU

// These are set up to have conservative conversion schedules to BranchU:

#define JU_BRANCHL_MAX_POP      ((Word_t)-1)
//#define JU_BTOU_POP_INCREMENT      300
#define JU_BRANCHB_MIN_POP        1000
#define JU_BRANCHB_MAX_POP      ((Word_t)-1)

#endif // noU


// MISCELLANEOUS MACROS:

// Get N most significant bits from the shifted Index word:
//
// As Index words are decoded, they are shifted left so only relevant,
// undecoded Index bits remain.

#define JU_BITSFROMSFTIDX(SFTIDX, N)  ((SFTIDX) >> (cJU_BITSPERWORD - (N)))

// TBD:  I have my doubts about the necessity of these macros (dlb):

// Produce 1-digit mask at specified state:
#define cJU_MASKATSTATE(State)  (((Word_t)0xff) << (((State) - 1) * cJU_BITSPERBYTE))

// Get byte (digit) from Index at the specified state, right justified:
//
// Note:  State must be 1..cJU_ROOTSTATE, and Digits must be 1..(cJU_ROOTSTATE
// - 1), but theres no way to assert these within an expression.

#define JU_DIGITATSTATE(Index,cState) \
         ((uint8_t)((Index) >> (((cState) - 1) * cJU_BITSPERBYTE)))

// Similarly, place byte (digit) at correct position for the specified state:
//
// Note:  Cast digit to a Word_t first so there are no complaints or problems
// about shifting it more than 32 bits on a 64-bit system, say, when it is a
// uint8_t from jbl_Expanse[].  (Believe it or not, the C standard says to
// promote an unsigned char to a signed int; -Ac does not do this, but -Ae
// does.)
//
// Also, to make lint happy, cast the whole result again because apparently
// shifting a Word_t does not result in a Word_t!

#define JU_DIGITTOSTATE(Digit,cState) \
        ((Word_t) (((Word_t) (Digit)) << (((cState) - 1) * cJU_BITSPERBYTE)))

#endif // ! _JUDY_PRIVATE_BRANCH_INCLUDED


#ifdef TEST_INSDEL

// ****************************************************************************
// TEST CODE FOR INSERT/DELETE MACROS
// ****************************************************************************
//
// To use this, compile a temporary *.c file containing:
//
//      #define DEBUG
//      #define JUDY_ASSERT
//      #define TEST_INSDEL
//      #include "JudyPrivate.h"
//      #include "JudyPrivateBranch.h"
//
// Use a command like this:  cc -Ae +DD64 -I. -I JudyCommon -o t t.c
// For best results, include +DD64 on a 64-bit system.
//
// This test code exercises some tricky macros, but the output must be studied
// manually to verify it.  Assume that for even-index testing, whole words
// (Word_t) suffices.

#include <stdio.h>

#define INDEXES 3               // in each array.


// ****************************************************************************
// I N I T
//
// Set up variables for next test.  See usage.

FUNCTION void Init (
        int       base,
        PWord_t   PeIndex,
        PWord_t   PoIndex,
        PWord_t   Peleaf,       // always whole words.
        uint8_t * Poleaf3,
        uint8_t * Poleaf5,
        uint8_t * Poleaf6,
        uint8_t * Poleaf7)
{
        int offset;

        *PeIndex = 99;

        for (offset = 0; offset <= INDEXES; ++offset)
            Peleaf[offset] = base + offset;

        for (offset = 0; offset < (INDEXES + 1) * 3; ++offset)
            Poleaf3[offset] = base + offset;


        *PoIndex = (91L << 56) | (92L << 48) | (93L << 40) | (94L << 32)
                 | (95L << 24) | (96L << 16) | (97L <<  8) |  98L;

        for (offset = 0; offset < (INDEXES + 1) * 5; ++offset)
            Poleaf5[offset] = base + offset;

        for (offset = 0; offset < (INDEXES + 1) * 6; ++offset)
            Poleaf6[offset] = base + offset;

        for (offset = 0; offset < (INDEXES + 1) * 7; ++offset)
            Poleaf7[offset] = base + offset;

} // Init()


// ****************************************************************************
// P R I N T   L E A F
//
// Print the byte values in a leaf.

FUNCTION void PrintLeaf (
        char *    Label,        // for output.
        int       IOffset,      // insertion offset in array.
        int       Indsize,      // index size in bytes.
        uint8_t * PLeaf)        // array of Index bytes.
{
        int       offset;       // in PLeaf.
        int       byte;         // in one word.

        (void) printf("%s %u: ", Label, IOffset);

        for (offset = 0; offset <= INDEXES; ++offset)
        {
            for (byte = 0; byte < Indsize; ++byte)
                (void) printf("%2d", PLeaf[(offset * Indsize) + byte]);

            (void) printf(" ");
        }

        (void) printf("\n");

} // PrintLeaf()


// ****************************************************************************
// M A I N
//
// Test program.

FUNCTION main()
{
        Word_t  eIndex;                         // even, to insert.
        Word_t  oIndex;                         // odd,  to insert.
        Word_t  eleaf [ INDEXES + 1];           // even leaf, index size 4.
        uint8_t oleaf3[(INDEXES + 1) * 3];      // odd leaf,  index size 3.
        uint8_t oleaf5[(INDEXES + 1) * 5];      // odd leaf,  index size 5.
        uint8_t oleaf6[(INDEXES + 1) * 6];      // odd leaf,  index size 6.
        uint8_t oleaf7[(INDEXES + 1) * 7];      // odd leaf,  index size 7.
        Word_t  eleaf_2 [ INDEXES + 1];         // same, but second arrays:
        uint8_t oleaf3_2[(INDEXES + 1) * 3];
        uint8_t oleaf5_2[(INDEXES + 1) * 5];
        uint8_t oleaf6_2[(INDEXES + 1) * 6];
        uint8_t oleaf7_2[(INDEXES + 1) * 7];
        int     ioffset;                // index insertion offset.

#define INIT        Init( 0, & eIndex, & oIndex, eleaf,   oleaf3, \
                         oleaf5,   oleaf6,   oleaf7)
#define INIT2 INIT; Init(50, & eIndex, & oIndex, eleaf_2, oleaf3_2, \
                         oleaf5_2, oleaf6_2, oleaf7_2)

#define WSIZE sizeof (Word_t)           // shorthand.

#ifdef PRINTALL                 // to turn on "noisy" printouts.
#define PRINTLEAF(Label,IOffset,Indsize,PLeaf) \
        PrintLeaf(Label,IOffset,Indsize,PLeaf)
#else
#define PRINTLEAF(Label,IOffset,Indsize,PLeaf)  \
        if (ioffset == 0)                       \
        PrintLeaf(Label,IOffset,Indsize,PLeaf)
#endif

        (void) printf(
"In each case, tests operate on an initial array of %d indexes.  Even-index\n"
"tests set index values to 0,1,2...; odd-index tests set byte values to\n"
"0,1,2...  Inserted indexes have a value of 99 or else byte values 91,92,...\n",
                        INDEXES);

        (void) puts("\nJU_INSERTINPLACE():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, WSIZE, (uint8_t *) eleaf);
            JU_INSERTINPLACE(eleaf, INDEXES, ioffset, eIndex);
            PrintLeaf("After ", ioffset, WSIZE, (uint8_t *) eleaf);
        }

        (void) puts("\nJU_INSERTINPLACE3():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 3, oleaf3);
            JU_INSERTINPLACE3(oleaf3, INDEXES, ioffset, oIndex);
            PrintLeaf("After ", ioffset, 3, oleaf3);
        }

        (void) puts("\nJU_INSERTINPLACE5():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 5, oleaf5);
            JU_INSERTINPLACE5(oleaf5, INDEXES, ioffset, oIndex);
            PrintLeaf("After ", ioffset, 5, oleaf5);
        }

        (void) puts("\nJU_INSERTINPLACE6():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 6, oleaf6);
            JU_INSERTINPLACE6(oleaf6, INDEXES, ioffset, oIndex);
            PrintLeaf("After ", ioffset, 6, oleaf6);
        }

        (void) puts("\nJU_INSERTINPLACE7():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 7, oleaf7);
            JU_INSERTINPLACE7(oleaf7, INDEXES, ioffset, oIndex);
            PrintLeaf("After ", ioffset, 7, oleaf7);
        }

        (void) puts("\nJU_DELETEINPLACE():");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, WSIZE, (uint8_t *) eleaf);
            JU_DELETEINPLACE(eleaf, INDEXES, ioffset);
            PrintLeaf("After ", ioffset, WSIZE, (uint8_t *) eleaf);
        }

        (void) puts("\nJU_DELETEINPLACE_ODD(3):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 3, oleaf3);
            JU_DELETEINPLACE_ODD(oleaf3, INDEXES, ioffset, 3);
            PrintLeaf("After ", ioffset, 3, oleaf3);
        }

        (void) puts("\nJU_DELETEINPLACE_ODD(5):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 5, oleaf5);
            JU_DELETEINPLACE_ODD(oleaf5, INDEXES, ioffset, 5);
            PrintLeaf("After ", ioffset, 5, oleaf5);
        }

        (void) puts("\nJU_DELETEINPLACE_ODD(6):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 6, oleaf6);
            JU_DELETEINPLACE_ODD(oleaf6, INDEXES, ioffset, 6);
            PrintLeaf("After ", ioffset, 6, oleaf6);
        }

        (void) puts("\nJU_DELETEINPLACE_ODD(7):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT;
            PRINTLEAF("Before", ioffset, 7, oleaf7);
            JU_DELETEINPLACE_ODD(oleaf7, INDEXES, ioffset, 7);
            PrintLeaf("After ", ioffset, 7, oleaf7);
        }

        (void) puts("\nJU_INSERTCOPY():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, WSIZE, (uint8_t *) eleaf);
            PRINTLEAF("Before, dest", ioffset, WSIZE, (uint8_t *) eleaf_2);
            JU_INSERTCOPY(eleaf_2, eleaf, INDEXES, ioffset, eIndex);
            PRINTLEAF("After,  src ", ioffset, WSIZE, (uint8_t *) eleaf);
            PrintLeaf("After,  dest", ioffset, WSIZE, (uint8_t *) eleaf_2);
        }

        (void) puts("\nJU_INSERTCOPY3():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 3, oleaf3);
            PRINTLEAF("Before, dest", ioffset, 3, oleaf3_2);
            JU_INSERTCOPY3(oleaf3_2, oleaf3, INDEXES, ioffset, oIndex);
            PRINTLEAF("After,  src ", ioffset, 3, oleaf3);
            PrintLeaf("After,  dest", ioffset, 3, oleaf3_2);
        }

        (void) puts("\nJU_INSERTCOPY5():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 5, oleaf5);
            PRINTLEAF("Before, dest", ioffset, 5, oleaf5_2);
            JU_INSERTCOPY5(oleaf5_2, oleaf5, INDEXES, ioffset, oIndex);
            PRINTLEAF("After,  src ", ioffset, 5, oleaf5);
            PrintLeaf("After,  dest", ioffset, 5, oleaf5_2);
        }

        (void) puts("\nJU_INSERTCOPY6():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 6, oleaf6);
            PRINTLEAF("Before, dest", ioffset, 6, oleaf6_2);
            JU_INSERTCOPY6(oleaf6_2, oleaf6, INDEXES, ioffset, oIndex);
            PRINTLEAF("After,  src ", ioffset, 6, oleaf6);
            PrintLeaf("After,  dest", ioffset, 6, oleaf6_2);
        }

        (void) puts("\nJU_INSERTCOPY7():");

        for (ioffset = 0; ioffset <= INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 7, oleaf7);
            PRINTLEAF("Before, dest", ioffset, 7, oleaf7_2);
            JU_INSERTCOPY7(oleaf7_2, oleaf7, INDEXES, ioffset, oIndex);
            PRINTLEAF("After,  src ", ioffset, 7, oleaf7);
            PrintLeaf("After,  dest", ioffset, 7, oleaf7_2);
        }

        (void) puts("\nJU_DELETECOPY():");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, WSIZE, (uint8_t *) eleaf);
            PRINTLEAF("Before, dest", ioffset, WSIZE, (uint8_t *) eleaf_2);
            JU_DELETECOPY(eleaf_2, eleaf, INDEXES, ioffset, ignore);
            PRINTLEAF("After,  src ", ioffset, WSIZE, (uint8_t *) eleaf);
            PrintLeaf("After,  dest", ioffset, WSIZE, (uint8_t *) eleaf_2);
        }

        (void) puts("\nJU_DELETECOPY_ODD(3):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 3, oleaf3);
            PRINTLEAF("Before, dest", ioffset, 3, oleaf3_2);
            JU_DELETECOPY_ODD(oleaf3_2, oleaf3, INDEXES, ioffset, 3);
            PRINTLEAF("After,  src ", ioffset, 3, oleaf3);
            PrintLeaf("After,  dest", ioffset, 3, oleaf3_2);
        }

        (void) puts("\nJU_DELETECOPY_ODD(5):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 5, oleaf5);
            PRINTLEAF("Before, dest", ioffset, 5, oleaf5_2);
            JU_DELETECOPY_ODD(oleaf5_2, oleaf5, INDEXES, ioffset, 5);
            PRINTLEAF("After,  src ", ioffset, 5, oleaf5);
            PrintLeaf("After,  dest", ioffset, 5, oleaf5_2);
        }

        (void) puts("\nJU_DELETECOPY_ODD(6):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 6, oleaf6);
            PRINTLEAF("Before, dest", ioffset, 6, oleaf6_2);
            JU_DELETECOPY_ODD(oleaf6_2, oleaf6, INDEXES, ioffset, 6);
            PRINTLEAF("After,  src ", ioffset, 6, oleaf6);
            PrintLeaf("After,  dest", ioffset, 6, oleaf6_2);
        }

        (void) puts("\nJU_DELETECOPY_ODD(7):");

        for (ioffset = 0; ioffset < INDEXES; ++ioffset)
        {
            INIT2;
            PRINTLEAF("Before, src ", ioffset, 7, oleaf7);
            PRINTLEAF("Before, dest", ioffset, 7, oleaf7_2);
            JU_DELETECOPY_ODD(oleaf7_2, oleaf7, INDEXES, ioffset, 7);
            PRINTLEAF("After,  src ", ioffset, 7, oleaf7);
            PrintLeaf("After,  dest", ioffset, 7, oleaf7_2);
        }

        return(0);

} // main()

#endif // TEST_INSDEL
