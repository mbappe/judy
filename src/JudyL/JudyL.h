#ifndef _JUDYL_INCLUDED
#define _JUDYL_INCLUDED
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

// @(#) $Revision: 4.41 $ $Source: /judy/src/JudyL/JudyL.h $

// ****************************************************************************
//          JUDYL -- SMALL/LARGE AND/OR CLUSTERED/SPARSE ARRAYS
//
//                                    -by-
//
//                             Douglas L. Baskins
//                             dougbaskins@gmail.com
//
// Judy arrays are designed to be used instead of arrays.  The performance
// suggests the reason why Judy arrays are thought of as arrays, instead of
// trees.  They are remarkably memory efficient at all populations.
// Implemented as a hybrid digital tree (but really a state machine, see
// below), Judy arrays feature fast insert/retrievals, fast near neighbor
// searching, and contain a population tree for extremely fast ordinal related
// retrievals.
//
// CONVENTIONS:
//
// - The comments here refer to 32-bit [64-bit] systems.
//
// - BranchL, LeafL refer to linear branches and leaves (small populations),
//   except LeafL does not actually appear as such; rather, Leaf1..3 [Leaf1..7]
//   is used to represent leaf Index sizes, and Leaf8 refers to a Leaf with
//   full (Long) word Indexes, which is also a type of linear leaf.  Note that
//   root-level Leaf8 (Leaf4 [Leaf8]) leaves are called LEAF8.
//
// - BranchB, LeafB1 refer to bitmap branches and leaves (intermediate
//   populations).
//
// - BranchU refers to uncompressed branches.  An uncompressed branch has 256
//   JPs, some of which could be null.  Note:  All leaves are compressed (and
//   sorted), or else an expanse is full (FullPopu), so there is no LeafU
//   equivalent to BranchU.
//
// - "Popu" is short for "Population".
// - "Pop1" refers to actual population (base 1).
// - "Pop0" refers to Pop1 - 1 (base 0), the way populations are stored in data
//   structures.
//
// - Branches and Leaves are both named by the number of bytes in their Pop0
//   field.  In the case of Leaves, the same number applies to the Index sizes.
//
// - The representation of many numbers as hex is a relatively safe and
//   portable way to get desired bitpatterns as unsigned longs.
//
// - Some preprocessors cant handle single apostrophe characters within
//   #ifndef code, so here, delete all instead.


#include "JudyPrivate.h"        // includes Judy.h in turn.
#include "JudyPrivateBranch.h"  // support for branches.


// ****************************************************************************
// JUDYL ROOT POINTER (JRP) AND JUDYL POINTER (JP) TYPE FIELDS
// ****************************************************************************

typedef enum            // uint8_t -- but C does not support this type of enum.
{

// JP NULL TYPES:
//
// There is a series of cJL_JPNULL* Types because each one pre-records a
// different Index Size for when the first Index is inserted in the previously
// null JP.  They must start >= 8 (three bits).
//
// Note:  These Types must be in sequential order for doing relative
// calculations between them.

        cJL_JPNULL1 = 1,
        cJL_JPNULL2,            // Index Size 2 bytes when 1 Index inserted.
        cJL_JPNULL3,            // Index Size 3 bytes when 1 Index inserted.
        cJL_JPNULL4,            // Index Size 4 bytes when 1 Index inserted.
        cJL_JPNULL5,            // Index Size 5 bytes when 1 Index inserted.
        cJL_JPNULL6,            // Index Size 6 bytes when 1 Index inserted.
        cJL_JPNULL7,            // Index Size 7 bytes when 1 Index inserted.
#define cJL_JPNULLMAX cJL_JPNULL7


// JP BRANCH TYPES:
//
// Note:  There are no state-1 branches; only leaves reside at state 1.

// Uncompressed branches:
//
// Note:  These Types must be in sequential order for doing relative
// calculations between them.

        cJL_JPBRANCH_U2,        // 2 bytes Pop0, 5 bytes Dcd.
        cJL_JPBRANCH_U3,        // 3 bytes Pop0, 4 bytes Dcd.
        cJL_JPBRANCH_U4,        // 4 bytes Pop0, 3 bytes Dcd.
        cJL_JPBRANCH_U5,        // 5 bytes Pop0, 2 bytes Dcd.
        cJL_JPBRANCH_U6,        // 6 bytes Pop0, 1 byte  Dcd.
        cJL_JPBRANCH_U7,        // 7 bytes Pop0, 0 bytes Dcd.
        cJL_JPBRANCH_U8,        // 8 bytes Pop0, 0 bytes Dcd.

        cJL_JPBRANCH1_L2,       // 2 bytes Pop0, 6 bytes Dcd.
        cJL_JPBRANCH1_L3,       // 3 bytes Pop0, 5 bytes Dcd.
        cJL_JPBRANCH1_L4,       // 4 bytes Pop0, 4 bytes Dcd.
        cJL_JPBRANCH1_L5,       // 5 bytes Pop0, 3 bytes Dcd.
        cJL_JPBRANCH1_L6,       // 6 bytes Pop0, 2 byte  Dcd.
        cJL_JPBRANCH1_L7,       // 7 bytes Pop0, 1 bytes Dcd.
        cJL_JPBRANCH1_L8,       // 8 bytes Pop0, 0 bytes Dcd.

// Linear branches:
//
// Note:  These Types must be in sequential order for doing relative
// calculations between them.

        cJL_JPBRANCH_L2,        // 2 bytes Pop0, 6 bytes Dcd.
        cJL_JPBRANCH_L3,        // 3 bytes Pop0, 5 bytes Dcd.
        cJL_JPBRANCH_L4,        // 4 bytes Pop0, 4 bytes Dcd.
        cJL_JPBRANCH_L5,        // 5 bytes Pop0, 3 bytes Dcd.
        cJL_JPBRANCH_L6,        // 6 bytes Pop0, 2 byte  Dcd.
        cJL_JPBRANCH_L7,        // 7 bytes Pop0, 1 bytes Dcd.
        cJL_JPBRANCH_L8,        // 8 bytes Pop0, 0 bytes Dcd.

// Bitmap branches:
//
// Note:  These Types must be in sequential order for doing relative
// calculations between them.

        cJL_JPBRANCH_B2,        // 2 bytes Pop0, 6 bytes Dcd.
        cJL_JPBRANCH_B3,        // 3 bytes Pop0, 5 bytes Dcd.
        cJL_JPBRANCH_B4,        // 4 bytes Pop0, 4 bytes Dcd.
        cJL_JPBRANCH_B5,        // 5 bytes Pop0, 3 bytes Dcd.
        cJL_JPBRANCH_B6,        // 6 bytes Pop0, 2 byte  Dcd.
        cJL_JPBRANCH_B7,        // 7 bytes Pop0, 1 bytes Dcd.
        cJL_JPBRANCH_B8,        // 8 bytes Pop0, 0 bytes Dcd.
#define cJL_JPMAXBRANCH   (cJL_JPBRANCH_B8) // Max Branch jp_type

// JP LEAF TYPES:
// Linear leaves:
//
// Note:  These Types must be in sequential order for doing relative
// calculations between them.
//
        cJL_JPLEAF1,            // 1 byte  Pop0, 7 bytes Dcd.
        cJL_JPLEAF2,            // 2 bytes Pop0, 6 bytes Dcd.
        cJL_JPLEAF3,            // 3 bytes Pop0, 5 bytes Dcd.
        cJL_JPLEAF4,            // 4 bytes Pop0, 4 bytes Dcd.
        cJL_JPLEAF5,            // 5 bytes Pop0, 3 bytes Dcd.
        cJL_JPLEAF6,            // 6 bytes Pop0, 2 byte  Dcd.
        cJL_JPLEAF7,            // 7 bytes Pop0, 1 bytes Dcd.
        cJL_JPLEAF8,            // 8 bytes Pop0, 0 bytes Dcd.

// Bitmap leaf; Index Size == 1:
//
// Note:  These are currently only supported at state 1.  At other states the
// bitmap would grow from 256 to 256^2, 256^3, ... bits, which would not be
// efficient..

        cJL_JPLEAF_B1U,          // 1] byte Pop0, 7 bytes Dcd.
#define cJL_JLEAFMAX (cJL_JPLEAF_B1U) // max Leaf jp_type

// Full population; Index Size == 1 virtual leaf:
//
// Note:  JudyL has no cJL_JPFULLPOPU1 equivalent to cJ1_JPFULLPOPU1, because
// in the JudyL case this could result in a values-only leaf of up to 256 words
// (value areas) that would be slow to insert/delete.


// JP IMMEDIATES; leaves (Indexes) stored inside a JP:
//
// The second numeric suffix is the Pop1 for each type.  As the Index Size
// increases, the maximum possible population decreases.
//
// Note:  These Types must be in sequential order in each group (Index Size),
// and the groups in correct order too, for doing relative calculations between
// them.  For example, since these Types enumerate the Pop1 values (unlike
// other JP Types where there is a Pop0 value in the JP), the maximum Pop1 for
// each Index Size is computable.
//
// All enums equal or above this point are cJL_JPIMMEDs.

        cJL_JPIMMED_1_01,       // Index Size = 1, Pop1 = 1.
        cJL_JPIMMED_2_01,       // Index Size = 2, Pop1 = 1.
        cJL_JPIMMED_3_01,       // Index Size = 3, Pop1 = 1.
        cJL_JPIMMED_4_01,       // Index Size = 4, Pop1 = 1.
        cJL_JPIMMED_5_01,       // Index Size = 5, Pop1 = 1.
        cJL_JPIMMED_6_01,       // Index Size = 6, Pop1 = 1.
        cJL_JPIMMED_7_01,       // Index Size = 7, Pop1 = 1.

        cJL_JPIMMED_1_02,       // Index Size = 1, Pop1 = 2.
        cJL_JPIMMED_1_03,       // Index Size = 1, Pop1 = 3.
        cJL_JPIMMED_1_04,       // Index Size = 1, Pop1 = 4.
        cJL_JPIMMED_1_05,       // Index Size = 1, Pop1 = 5.
        cJL_JPIMMED_1_06,       // Index Size = 1, Pop1 = 6.
        cJL_JPIMMED_1_07,       // Index Size = 1, Pop1 = 7.
        cJL_JPIMMED_1_08,       // Index Size = 1, Pop1 = 8.

        cJL_JPIMMED_2_02,       // Index Size = 2, Pop1 = 2.
        cJL_JPIMMED_2_03,       // Index Size = 2, Pop1 = 3.
        cJL_JPIMMED_2_04,       // Index Size = 2, Pop1 = 4.

        cJL_JPIMMED_3_02,       // Index Size = 3, Pop1 = 2.

        cJL_JPIMMED_4_02,       // Index Size = 4, Pop1 = 2. 58

// This special Type is merely a sentinel for doing relative calculations.
// This value should not be used in switch statements (to avoid allocating code
// for it), which is also why it appears at the end of the enum list.

        cJL_JPIMMED_CAP

} jpL_Type_t;


// RELATED VALUES:

// Index Size (state) for leaf JP, and JP type based on Index Size (state):

#define JL_LEAFINDEXSIZE(jpType) ((jpType)    - cJL_JPLEAF1 + 1)
#define JL_LEAFTYPE(IndexSize)   ((IndexSize) + cJL_JPLEAF1 - 1)


// MAXIMUM POPULATIONS OF LINEAR LEAVES:
// Note: Current malloc() uses a min of 4 words per allocation!.  This
// means the 1,2,3 words malloced use the same amount of mmaped memory.
// Also 2048 words is same as 2063 words and 1024 is same as 1027.


//pre#define J_L_MAXB                (sizeof(Word_t) * 109) 
//pre#define ALLOCSIZES { 1, 3, 5, 9, 15, 25, 41, 67, 109, 177, 255, TERMINATOR }

//#define ALLOCSIZES { 3, 5, 7, 9, 11, 15, 23, 33, 47, 65, 95, 129, TERMINATOR } // in words.
//#define ALLOCSIZES { 3, 5, 9, 13, 17, 23, 29, 37, 45, 65, 85, 105, 129, TERMINATOR }

//#define J_L_MAXB                (sizeof(Word_t) * 255)
//#define ALLOCSIZES { 3, 5, 9, 15, 25, 41, 49, 57, 65, 109, 177, 255, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 11, 15, 23, 32, 47, 64, 95, 129, TERMINATOR } // in words.
//#define ALLOCSIZES { 1, 3, 5, 7, 13, 21, 35, 57, 93, 151, TERMINATOR }

//#define J_L_MAXB                (sizeof(Word_t) * 69) 
//#define J_L_MAXB                (sizeof(Word_t) * 117) 
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 15, 19, 23, 27, 33, 39, 47, 55, 67, 81, 97, 117, 141, 169, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 17, 21, 27, 33, 41, 51, 65, 83, 103, 129, 159, 199, 249, 321, 519, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 17, 21, 27, 33, 41, 51, 65, 83, 103, 129, 159, 199, 289, 321, 519, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 17, 21, 27, 33, 41, 51, 65, 83, 103, 129, 159, 199, 291, 321, 519, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 27, 33, 41, 51, 65, 79, 95, 131, 195, 291, 321, 519, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 15, 17, 27, 43, 69, 111, 179, 289, 467, TERMINATOR }
//#define J_L_MAXB                (sizeof(Word_t) * 289) 
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 15, 17, 25, 39, 63, 101, 163, 261, 417, TERMINATOR }
//#define ALLOCSIZES { 3, 5, 7, 9, 11, 13, 15, 17, 19, 27, 41, 37, 53, 73, 95, 145, 205, 289, 321, 519, TERMINATOR }
//#define J_L_MAXB                (sizeof(Word_t) * 289)                // picked from   ^^^
//
// Square root 2 multiplyer at 9, L2=230, L1=128
#define ALLOCSIZES { 3, 5, 7, 9, 13, 19, 27, 37, 51, 73, 101, 145, 203, 289, 321, 576, TERMINATOR }
#define J_L_MAXB                (sizeof(Word_t) * 289)     // picked from   ^^^

//#define cJL_LEAF1_MAXWORDS       (419)
//#define cJL_LEAF2_MAXWORDS       (320)

#define cJL_LEAFB1_MAXPOP1       (256)

#ifndef cJL_LEAF1_MAXPOP1
//#define cJL_LEAF1_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (1 + cJU_BYTESPERWORD)) too big
#define cJL_LEAF1_MAXPOP1       (128)
#endif // cJL_LEAF1_MAXPOP1

#ifndef cJL_LEAF2_MAXPOP1
// absolute maximum is 255
#define cJL_LEAF2_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (2 + cJU_BYTESPERWORD))
#endif

#define cJL_LEAF3_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (3 + cJU_BYTESPERWORD))
#define cJL_LEAF4_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (4 + cJU_BYTESPERWORD))
#define cJL_LEAF5_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (5 + cJU_BYTESPERWORD))
#define cJL_LEAF6_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (6 + cJU_BYTESPERWORD))
#define cJL_LEAF7_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (7 + cJU_BYTESPERWORD))
#define cJL_LEAF8_MAXPOP1       ((J_L_MAXB - cJU_BYTESPERWORD) / (8 + cJU_BYTESPERWORD))

// MAXIMUM POPULATIONS OF IMMEDIATE JPs:
//
// These specify the maximum Population of immediate JPs with various Index
// Sizes (== sizes of remaining undecoded Index bits).  Since the JP Types enum
// already lists all the immediates in order by state and size, calculate these
// values from it to avoid redundancy.

#define cJL_IMMED1_MAXPOP1  (8)                         // 8
//#define cJL_IMMED1_MAXPOP1  (cJU_BYTESPERWORD / 1)      // 8
#define cJL_IMMED2_MAXPOP1  (cJU_BYTESPERWORD / 2)      // 4
#define cJL_IMMED3_MAXPOP1  (cJU_BYTESPERWORD / 3)      // 2
#define cJL_IMMED4_MAXPOP1  (cJU_BYTESPERWORD / 4)      // 2
#define cJL_IMMED5_MAXPOP1  (cJU_BYTESPERWORD / 5)      // 1
#define cJL_IMMED6_MAXPOP1  (cJU_BYTESPERWORD / 6)      // 1
#define cJL_IMMED7_MAXPOP1  (cJU_BYTESPERWORD / 7)      // 1


// ****************************************************************************
// JUDYL LEAF BITMAP (JLLB) SUPPORT
// ****************************************************************************
//
// Assemble bitmap leaves out of smaller units that put bitmap subexpanses
// close to their associated pointers.  Why not just use a bitmap followed by a
// series of pointers?  (See 4.27.)  Turns out this wastes a cache fill on
// systems with smaller cache lines than the assumed value cJU_WORDSPERCL.

#define JL_JLB_BITMAP(Pjlb, Subexp)     ((Pjlb)->jLlb_Bitmap[Subexp])
#define JL_JLB_PVALUE(Pjlb)  ((Pjlb)->jLlb_PV)
//
typedef struct J__UDYL_LEAF_BITMAP
{
        Word_t    jlb_LastKey;          // now in the jp_t DcdPop0????
        BITMAPL_t jLlb_Bitmap[cJU_NUMSUBEXPL];
        jv_t      jLlb_PV[256];
} jLlb_t, * PjLlb_t;

// Words per bitmap leaf:

#define cJL_WORDSPERLEAFB1U  (sizeof(jLlb_t) / cJU_BYTESPERWORD)


// ****************************************************************************
// MEMORY ALLOCATION SUPPORT
// ****************************************************************************

// ARRAY-GLOBAL INFORMATION:
//
// At the cost of an occasional additional cache fill, this object, which is
// pointed at by a JRP and in turn points to a JP_BRANCH*, carries array-global
// information about a JudyL array that has sufficient population to amortize
// the cost.  The jpm_Pop0 field prevents having to add up the total population
// for the array in insert, delete, and count code.  The jpm_JP field prevents
// having to build a fake JP for entry to a state machine; 
//
// Note:  Struct fields are ordered to keep "hot" data in the first 8 words
// (see left-margin comments) for machines with 8-word cache lines, and to keep
// sub-word fields together for efficient packing.

typedef struct J_UDYL_POPULATION_AND_MEMORY
{
/* 1 */ Word_t    jpm_Pop0;             // total population-1 in array.
/* 1 */ Word_t    jpm_TotalMemWords;    // words allocated in array.
/* 2 */ jp_t      jpm_JP[1];            // JP to first branch or Leaf7; see above.
/* 1 */ Pjv_t     jpm_PValue;           // pointer to value to return.
// Note:  Field names match PJError_t for convenience in macros:
/* 1 */ JU_Errno_t je_Errno;            // one of the enums in Judy.h.
/* 0  */ int      je_ErrID;             // often an internal source line number.
} jLpm_t, *PjLpm_t;


// TABLES FOR DETERMINING IF LEAVES HAVE ROOM TO GROW:
//
// These tables indicate if a given memory chunk can support growth of a given
// object into wasted (rounded-up) memory in the chunk.  Note:  This violates
// the hiddenness of the JudyMalloc code.

extern const uint16_t j__L_LeafB1PopToWords[256 + 2];
extern const uint16_t j__L_Leaf1PopToWords[cJL_LEAF1_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf2PopToWords[cJL_LEAF2_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf3PopToWords[cJL_LEAF3_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf4PopToWords[cJL_LEAF4_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf5PopToWords[cJL_LEAF5_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf6PopToWords[cJL_LEAF6_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf7PopToWords[cJL_LEAF7_MAXPOP1 + 2];
extern const uint16_t j__L_Leaf8PopToWords[cJL_LEAF8_MAXPOP1 + 2];
extern const uint16_t j__L_LeafVPopToWords[];

// These tables indicate where value areas start:

extern const uint16_t j__L_Leaf1Offset    [cJL_LEAF1_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf2Offset    [cJL_LEAF2_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf3Offset    [cJL_LEAF3_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf4Offset    [cJL_LEAF4_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf5Offset    [cJL_LEAF5_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf6Offset    [cJL_LEAF6_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf7Offset    [cJL_LEAF7_MAXPOP1 + 1];
extern const uint16_t j__L_Leaf8Offset    [cJL_LEAF8_MAXPOP1 + 1];

// Also define macros to hide the details in the code using these tables.

#define JL_LEAFB1GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, 256, j__L_LeafB1PopToWords)
#define JL_LEAF1GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF1_MAXPOP1, j__L_Leaf1PopToWords)
#define JL_LEAF2GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF2_MAXPOP1, j__L_Leaf2PopToWords)
#define JL_LEAF3GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF3_MAXPOP1, j__L_Leaf3PopToWords)
#define JL_LEAF4GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF4_MAXPOP1, j__L_Leaf4PopToWords)
#define JL_LEAF5GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF5_MAXPOP1, j__L_Leaf5PopToWords)
#define JL_LEAF6GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF6_MAXPOP1, j__L_Leaf6PopToWords)
#define JL_LEAF7GROWINPLACE(Pop1) \
        J__U_GROWCK(Pop1, cJL_LEAF7_MAXPOP1, j__L_Leaf7PopToWords)
#define JL_LEAF8GROWINPLACE(Pop1) \
    J__U_GROWCK(Pop1, cJL_LEAF8_MAXPOP1, j__L_Leaf8PopToWords)

// now only used for Immed1..2s Value area
#define JL_LEAFVGROWINPLACE(Pop1)                       \
    J__U_GROWCK(Pop1, cJU_BITSPERSUBEXPL,  j__L_LeafVPopToWords)

// Check if LeafB1 is ready for uncompressed Value area
#define JL_MAXCOMPRESSEDB1(POP1)                        \
   (j__L_LeafB1PopToWords[POP1] == j__L_LeafB1PopToWords[256])

// Uncompress LeafB1 while copy -- add/delete Value AFTER copy
// Pjlb1new must have 256 size of Value area
// Pjlb1new must be zeroed
#define JL_COPYUNCOMPB1(PJLB1NEW, PJLB1OLD)                     \
{                                                               \
    *(PJLB1NEW) = *(PJLB1OLD);        /* Copy Bitmaps etc */    \
    Pjv_t _Pjvnew = JL_JLB_PVALUE(PJLB1NEW);                    \
    Pjv_t _Pjvold = JL_JLB_PVALUE(PJLB1OLD);                    \
                                                                \
    int _valoff = 0;                                            \
    for (int _sube = 0; _sube < 4; _sube++)                     \
    {                                                           \
        BITMAPL_t   _bitm = JL_JLB_BITMAP(PJLB1NEW, _sube);     \
        int         _dig  = _sube * sizeof(BITMAPL_t) * 8;      \
        for (; _bitm != 0; _bitm >>= 1, _dig++)                 \
        {                                                       \
            if (_bitm & 1) _Pjvnew[_dig] = _Pjvold[_valoff++];  \
        }                                                       \
    }                                                           \
}

// Check if Leaf1 is ready for uncompressed Value area
#define JL_MAXCOMPRESSEDL1(POP1)                                \
   (j__L_Leaf1PopToWords[POP1] == j__L_Leaf1PopToWords[256])

// Uncompress Leaf1 while copy -- add/delete Value AFTER copy
// PLeaf1new must have 256 size of Value area
// Pjlb1new must be zeroed
#define JL_COPYUNCOMPL1(PLEAFNEW, PLEAFOLD, POP1)               \
{                                                               \
    uint8_t *_PnewLeaf = (PLEAFNEW)->jl1_Leaf;                  \
    uint8_t *_PoldLeaf = (PLEAFOLD)->jl1_Leaf;                  \
    Pjv_t _Pjvnew = JL_LEAF1VALUEAREA(PLEAFNEW, 256);           \
    Pjv_t _Pjvold = JL_LEAF1VALUEAREA(PLEAFOLD, POP1);          \
                                                                \
    for (int _idx = 0; _idx < (POP1); _idx++)                   \
    {                                                           \
        uint8_t _Key    = _PoldLeaf[_idx];                      \
 /* printf("copy Key = 0x%x, Value = 0x%lx\n", _Key, _Pjvold [_idx]); */ \
        _PnewLeaf[_Key] = 1;                                    \
        _Pjvnew  [_Key] = _Pjvold [_idx];                       \
    }                                                           \
}

#ifdef OBSOLETE
// From compressed to uncompressed -- add/delete Value after copy
#define JL_COPYUNCOMPRESSED(PJVNEW,PJV,BITMAP)                  \
{                                                               \
    BITMAPL_t   _bitm = (BITMAP);                               \
    int         _idx;                                           \
    int         _digit;                                         \
                                                                \
    for (_idx = _digit = 0; _bitm != 0; _bitm >>= 1, _digit++)  \
    {                                                           \
        if (_bitm & 1)                                          \
        {                                                       \
            (PJVNEW)[_digit] = (PJV)[_idx++];                   \
        }                                                       \
    }                                                           \
}

// From uncompressed to compressed -- add/delete Value before copy
#define JL_COPYCOMPRESSED(PJVNEW,PJV,BITMAP)                    \
{                                                               \
    BITMAPL_t   _bitm = (BITMAP);                               \
    int         _idx;                                           \
    int         _digit;                                         \
                                                                \
    for (_idx = _digit = 0; _bitm != 0; _bitm >>= 1, _digit++)  \
    {                                                           \
        if (_bitm & 1)                                          \
        {                                                       \
            (PJVNEW)[_idx++] = (PJV)[_digit];                   \
        }                                                       \
    }                                                           \
}
#endif  // OBSOLETE


#define JL_LEAF1VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf1Offset[Pop1])
#define JL_LEAF2VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf2Offset[Pop1])
#define JL_LEAF3VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf3Offset[Pop1])
#define JL_LEAF4VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf4Offset[Pop1])
#define JL_LEAF5VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf5Offset[Pop1])
#define JL_LEAF6VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf6Offset[Pop1])
#define JL_LEAF7VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf7Offset[Pop1])
#define JL_LEAF8VALUEAREA(Pjv,Pop1)  (((Pjv_t)(Pjv)) + j__L_Leaf8Offset[Pop1])

#define JL_LEAFB1POPTOWORDS(Pop1)       (j__L_LeafB1PopToWords[Pop1])
#define JL_LEAF1POPTOWORDS(Pop1)        (j__L_Leaf1PopToWords[Pop1])
#define JL_LEAF2POPTOWORDS(Pop1)        (j__L_Leaf2PopToWords[Pop1])
#define JL_LEAF3POPTOWORDS(Pop1)        (j__L_Leaf3PopToWords[Pop1])
#define JL_LEAF4POPTOWORDS(Pop1)        (j__L_Leaf4PopToWords[Pop1])
#define JL_LEAF5POPTOWORDS(Pop1)        (j__L_Leaf5PopToWords[Pop1])
#define JL_LEAF6POPTOWORDS(Pop1)        (j__L_Leaf6PopToWords[Pop1])
#define JL_LEAF7POPTOWORDS(Pop1)        (j__L_Leaf7PopToWords[Pop1])
#define JL_LEAF8POPTOWORDS(Pop1)        (j__L_Leaf8PopToWords[Pop1])
#define JL_LEAFVPOPTOWORDS(Pop1)        (j__L_LeafVPopToWords[Pop1])


// FUNCTIONS TO ALLOCATE OBJECTS:

PjLpm_t j__udyLAllocJLPM(void);                         // constant size.

Word_t  j__udyLAllocJBL(          PjLpm_t);             // constant size.

#ifdef  NEWJBB
Word_t  j__udyLAllocJBB(    int,  PjLpm_t);             // constant size.
#else   // NEWJBB
Word_t  j__udyLAllocJBB(          PjLpm_t);             // constant size.
#endif  // NEWJBB

Word_t  j__udyLAllocJBBJP(Word_t, PjLpm_t);
Word_t  j__udyLAllocJBU(          PjLpm_t);             // constant size.

Word_t  j__udyLAllocJLL1( Word_t, PjLpm_t);
Word_t  j__udyLAllocJLL2( Word_t, PjLpm_t);
Word_t  j__udyLAllocJLL3( Word_t, PjLpm_t);

Word_t  j__udyLAllocJLL4( Word_t, PjLpm_t);
Word_t  j__udyLAllocJLL5( Word_t, PjLpm_t);
Word_t  j__udyLAllocJLL6( Word_t, PjLpm_t);
Word_t  j__udyLAllocJLL7( Word_t, PjLpm_t);
Word_t  j__udyLAllocJLL8( Word_t         );             // no PjLpm_t needed.
Word_t  j__udyLAllocJLB1U(        PjLpm_t);             // uncompressed
Word_t  j__udyLAllocJV(   Word_t, PjLpm_t);

// FUNCTIONS TO FREE OBJECTS:

void    j__udyLFreeJLPM( PjLpm_t,        PjLpm_t);      // constant size.

void    j__udyLFreeJBL(  Word_t,         PjLpm_t);      // constant size.
void    j__udyLFreeJBB(  Word_t,         PjLpm_t);      // constant size.
#ifdef  NEWJBB
#else   // NEWJBB
void    j__udyLFreeJBBJP(Word_t, Word_t, PjLpm_t);
#endif  // NEWJBB
void    j__udyLFreeJBU(  Word_t,         PjLpm_t);      // constant size.

void    j__udyLFreeJLL1( Word_t, Word_t, PjLpm_t);
void    j__udyLFreeJLL2( Word_t, Word_t, PjLpm_t);
void    j__udyLFreeJLL3( Word_t, Word_t, PjLpm_t);

void    j__udyLFreeJLL4( Word_t, Word_t, PjLpm_t);
void    j__udyLFreeJLL5( Word_t, Word_t, PjLpm_t);
void    j__udyLFreeJLL6( Word_t, Word_t, PjLpm_t);
void    j__udyLFreeJLL7( Word_t, Word_t, PjLpm_t);

void    j__udyLFreeJLL8( Word_t, Word_t, PjLpm_t);
void    j__udyLFreeJLB1U(Word_t,         PjLpm_t);      // constant size.
void    j__udyLFreeJV(   Word_t, Word_t, PjLpm_t);
Word_t  j__udyLFreeSM(   Pjp_t,          PjLpm_t);      // everything below Pjp.

#endif // ! _JUDYL_INCLUDED
