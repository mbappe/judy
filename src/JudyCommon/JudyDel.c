// Copyright (C) 2000 - 2019 Douglas L. Baskins
//
// This program is free software; you can redistribute it and/or modify it
// under the term of the GNU Lesser General Public License as published by the
// Free Software Foundation version 2.1 of the License.
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
// @(#) $Revision: 4.68 $ $Source: /judy/src/JudyCommon/JudyDel.c $
//
// Judy1Unset() and JudyLDel() functions for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.
//
// About HYSTERESIS:  In the Judy code, hysteresis means leaving around a
// nominally suboptimal (not maximally compressed) data structure after a
// deletion.  As a result, the shape of the tree for two identical index sets
// can differ depending on the insert/delete path taken to arrive at the index
// sets.  The purpose is to minimize worst-case behavior (thrashing) that could
// result from a series of intermixed insertions and deletions.  It also makes
// for MUCH simpler code, because instead of performing, "delete and then
// compress," it can say, "compress and then delete," where due to hysteresis,
// compression is not even attempted until the object IS compressible.
//
// In some cases the code has no choice and it must "ungrow" a data structure
// across a "phase transition" boundary without hysteresis.  In other cases the
// amount (such as "hysteresis = 1") is indicated by the number of JP deletions
// (in branches) or index deletions (in leaves) that can occur in succession
// before compressing the data structure.  (It appears that hysteresis <= 1 in
// all cases.)
//
// In general no hysteresis occurs when the data structure type remains the
// same but the allocated memory chunk for the node must shrink, because the
// relationship is hardwired and theres no way to know how much memory is
// allocated to a given data structure.  Hysteresis = 0 in all these cases.
//
// TBD:  Could this code be faster if memory chunk hysteresis were supported
// somehow along with data structure type hysteresis?
//
// TBD:  Should some of the assertions here be converted to product code that
// returns JU_ERRNO_CORRUPT?
//
// TBD:  Dougs code had an odd mix of function-wide and limited-scope
// variables.  Should some of the function-wide variables appear only in
// limited scopes, or more likely, vice-versa?

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef  JUDY1
#include "Judy1.h"
#else   // JUDYL 
#include "JudyL.h"
#endif  // JUDYL 

#include "JudyPrivate1L.h"

#ifdef TRACEJPD
#include "JudyPrintJP.c"
#endif

// These are defined to generic values in JudyCommon/JudyPrivateTypes.h:
//
// TBD:  These should be exported from a header file, but perhaps not, as they
// are only used here, and exported from JudyDecascade.c, which is a separate
// file for profiling reasons (to prevent inlining), but which potentially
// could be merged with this file, either in SoftCM or at compile-time:

#ifdef  JUDY1
extern int    j__udy1BranchBToBranchL(Pjp_t Pjp, Pjpm_t Pjpm);
extern int    j__udy1LeafB1ToLeaf1(Pjp_t, Pjpm_t);
extern Word_t j__udy1Leaf1ToLeaf2(uint16_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf2ToLeaf3(uint8_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf3ToLeaf4(uint32_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf4ToLeaf5(uint8_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf5ToLeaf6(uint8_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf6ToLeaf7(uint8_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf7ToLeafW(PWord_t, Pjp_t, Word_t, Pjpm_t);
#else   // JUDYL 
extern int    j__udyLBranchBToBranchL(Pjp_t Pjp, Pjpm_t Pjpm);
extern int    j__udyLLeafB1ToLeaf1(Pjp_t, Pjpm_t);
extern Word_t j__udyLLeaf1ToLeaf2(uint16_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf2ToLeaf3(uint8_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf3ToLeaf4(uint32_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf4ToLeaf5(uint8_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf5ToLeaf6(uint8_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf6ToLeaf7(uint8_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf7ToLeafW(PWord_t, Pjv_t, Pjp_t, Word_t, Pjpm_t);
#endif  // JUDYL 
// For convenience in the calling code; "M1" means "minus one":
// ****************************************************************************
// __ J U D Y   D E L   W A L K
//
// Given a pointer to a JP, an Index known to be valid, the number of bytes
// left to decode (== level in the tree), and a pointer to a global JPM, walk a
// Judy (sub)tree to do an unset/delete of that index, and possibly modify the
// JPM.  This function is only called internally, and recursively.  Unlike
// Judy1Test() and JudyLGet(), the extra time required for recursion should be
// negligible compared with the total.
//
// Return values:
//
// -1 error; details in JPM
//
//  0 Index already deleted (should never happen, Index is known to be valid)
//
//  1 previously valid Index deleted
//
//  2 same as 1, but in addition the JP now points to a BranchL containing a
//    single JP, which should be compressed into the parent branch (if there
//    is one, which is not the case for a top-level branch under a JPM)
DBGCODE(static uint8_t parentJPtype;);                                  // parent branch JP type.
FUNCTION static int
j__udyDelWalk(Pjp_t Pjp,                // current JP under which to delete.
              Word_t Index,             // to delete.
              Word_t ParentLevel,       // of parent branch.
              Pjpm_t Pjpm)              // for returning info to top level.
{
    Word_t    pop1;                     // of a leaf.
    Word_t    level;                    // of a leaf.
    uint8_t   digit;                    // from Index, in current branch.
    Word_t    PjllnewRaw;               // address of newly allocated leaf.
    Pjll_t    Pjllnew;
    int       offset;                   // within a branch.
    int       retcode;                  // return code: -1, 0, 1, 2.
#ifdef JUDYL
    Word_t    PjvRaw;                   // value area.
    Pjv_t     Pjv;
#endif   // JUDYL 
    DBGCODE(level = 0;);
  ContinueDelWalk:                     // for modifying state without recursing.

#ifdef TRACEJPD
            JudyPrintJP(Pjp, "d", __LINE__);
#endif

    switch (ju_Type(Pjp))               // entry:  Pjp, Index.
    {
// ****************************************************************************
// LINEAR BRANCH:
//
// MACROS FOR COMMON CODE:
//
// Check for population too high to compress a branch to a leaf, meaning just
// descend through the branch, with a purposeful off-by-one error that
// constitutes hysteresis = 1.  In other words, do not compress until the
// branchs CURRENT population fits in the leaf, even BEFORE deleting one
// index.
//
// Next is a label for branch-type-specific common code.  Variables pop1,
// level, digit, and Index are in the context.
// Support for generic calling of JudyLeaf*ToLeaf*() functions:
//
// Note:  Cannot use JUDYLCODE() because this contains a comma.
// During compression to a leaf, check if a JP contains nothing but a
// cJU_JPIMMED_*_01, in which case shortcut calling j__udyLeaf*ToLeaf*():
//
// Copy the index bytes from the jp_DcdPopO field (with possible truncation),
// and continue the branch-JP-walk loop.  Variables Pjp and Pleaf are in the
// context.
// Compress a BranchL into a leaf one index size larger:
//
// Allocate a new leaf, walk the JPs in the old BranchL and pack their contents
// into the new leaf (of type NewJPType), free the old BranchL, and finally
// restart the switch to delete Index from the new leaf.  (Note that all
// BranchLs are the same size.)  Variables Pjp, Pjpm, Pleaf, digit, and pop1
// are in the context.
//        ju_SetDcdPop0(PjpJP, DcdP0);
//        ju_SetLeafPop0(PjpJP, Pop1 - 1);
//        ju_SetJpType(PjpJP, cJU_JPLEAF2);
//        ju_SetBaLPntr(PjpJP, PjllRaw);
//        ju_SetIMM01(Pjp, Value, Key, Type)
//        ju_SetPjvPntr(Pjp, PjvPntr)
// Overall common code for initial BranchL deletion handling:
//
// Assert that Index is in the branch, then see if the BranchL should be kept
// or else compressed to a leaf.  Variables Index, Pjp, and pop1 are in the
// context.
// END OF MACROS, START OF CASES:
    case cJU_JPBRANCH_L2:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        pop1 = ju_BranchPop0(Pjp, 2) + 1;
        if (pop1 > (cJU_LEAF2_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((2) >= 2);
            level = (2);
            digit = JU_DIGITATSTATE(Index, 2);
            goto BranchLKeep;
        }
        assert(pop1 == (cJU_LEAF2_MAXPOP1));
        {
            uint16_t *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            if ((PjllnewRaw = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint16_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf, cJU_LEAF2_MAXPOP1););
            PjblRaw = ju_BaLPntr(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (2) - 2)
                {
                    *Pleaf++ = ju_DcdPop0((Pjbl->jbl_jp) + offset);
                    JUDYLCODE(*Pjv++ = ju_PImmVals((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 =    j__udyLeaf1ToLeaf2(Pleaf, /* null. */ (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 2), Pjpm);
#else   // JUDYL 
                pop1 =    j__udyLeaf1ToLeaf2(Pleaf, Pjv, (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 2), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint16_t *) (((Word_t)Pleaf) + ((2) * pop1));
                JUDYLCODE(Pjv += pop1;);
            } 
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (2)) == (cJU_LEAF2_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF2VALUEAREA(Pjllnew, cJU_LEAF2_MAXPOP1)) == (cJU_LEAF2_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF2);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_L3:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        pop1 = ju_BranchPop0(Pjp, 3) + 1;
        if (pop1 > (cJU_LEAF3_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((3) >= 2);
            level = (3);
            digit = JU_DIGITATSTATE(Index, 3);
            goto BranchLKeep;
        }
        assert(pop1 == (cJU_LEAF3_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            if ((PjllnewRaw = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pleaf, cJU_LEAF3_MAXPOP1););
            PjblRaw = ju_BaLPntr(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (3) - 2)
                {
                    JU_COPY3_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0((Pjbl->jbl_jp) + offset)));
                    Pleaf += (3);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
                
#ifdef  JUDY1
                pop1 = j__udyLeaf2ToLeaf3(Pleaf, /* null. */ (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 3), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf2ToLeaf3(Pleaf, Pjv, (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 3), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((3) * pop1));
                JUDYLCODE(Pjv += pop1;);
            } 
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (3)) == (cJU_LEAF3_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF3VALUEAREA(Pjllnew, cJU_LEAF3_MAXPOP1)) == (cJU_LEAF3_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF3);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_L4:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        pop1 = ju_BranchPop0(Pjp, 4) + 1;
        if (pop1 > (cJU_LEAF4_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((4) >= 2);
            level = (4);
            digit = JU_DIGITATSTATE(Index, 4);
            goto BranchLKeep;
        }
        assert(pop1 == (cJU_LEAF4_MAXPOP1));
        {
            uint32_t *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            if ((PjllnewRaw = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint32_t *)Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, cJU_LEAF4_MAXPOP1););
            PjblRaw = ju_BaLPntr(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (4) - 2)
                {
                    *Pleaf++ = ju_DcdPop0((Pjbl->jbl_jp) + offset);
                    JUDYLCODE(*Pjv++ = ju_PImmVals((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
                
#ifdef  JUDY1
                pop1 = j__udyLeaf3ToLeaf4(Pleaf, /* null. */ (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 4), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf3ToLeaf4(Pleaf, Pjv, (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 4), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint32_t *)(((Word_t)Pleaf) + ((4) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (4)) == (cJU_LEAF4_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF4VALUEAREA(Pjllnew, cJU_LEAF4_MAXPOP1)) == (cJU_LEAF4_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF4);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_L5:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        pop1 = ju_BranchPop0(Pjp, 5) + 1;
        if (pop1 > (cJU_LEAF5_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((5) >= 2);
            level = (5);
            digit = JU_DIGITATSTATE(Index, 5);
            goto BranchLKeep;
        }
        assert(pop1 == (cJU_LEAF5_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            if ((PjllnewRaw = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, cJU_LEAF5_MAXPOP1););
            PjblRaw = ju_BaLPntr(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (5) - 2)
                {
                    JU_COPY5_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0((Pjbl->jbl_jp) + offset)));
                    Pleaf += (5);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
                
#ifdef  JUDY1
                pop1 = j__udyLeaf4ToLeaf5(Pleaf, /* null. */ (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 5), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf4ToLeaf5(Pleaf, Pjv, (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 5), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((5) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (5)) == (cJU_LEAF5_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF5VALUEAREA(Pjllnew, cJU_LEAF5_MAXPOP1)) == (cJU_LEAF5_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF5);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_L6:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        pop1 = ju_BranchPop0(Pjp, 6) + 1;
        if (pop1 > (cJU_LEAF6_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((6) >= 2);
            level = (6);
            digit = JU_DIGITATSTATE(Index, 6);
            goto BranchLKeep;
        }
        assert(pop1 == (cJU_LEAF6_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            if ((PjllnewRaw = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, cJU_LEAF6_MAXPOP1););
            PjblRaw = ju_BaLPntr(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (6) - 2)
                {
                    JU_COPY6_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0((Pjbl->jbl_jp) + offset)));
                    Pleaf += (6);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
                
#ifdef  JUDY1
                pop1 = j__udyLeaf5ToLeaf6(Pleaf, /* null. */ (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 6), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf5ToLeaf6(Pleaf, Pjv, (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 6), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((6) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (6)) == (cJU_LEAF6_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF6VALUEAREA(Pjllnew, cJU_LEAF6_MAXPOP1)) == (cJU_LEAF6_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF6);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_L7:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        pop1 = ju_BranchPop0(Pjp, 7) + 1;
        if (pop1 > (cJU_LEAF7_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((7) >= 2);
            level = (7);
            digit = JU_DIGITATSTATE(Index, 7);
            goto BranchLKeep;
        }
        assert(pop1 == (cJU_LEAF7_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            if ((PjllnewRaw = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, cJU_LEAF7_MAXPOP1););
            PjblRaw = ju_BaLPntr(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (7) - 2)
                {
                    JU_COPY7_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0((Pjbl->jbl_jp) + offset)));
                    Pleaf += (7);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf6ToLeaf7(Pleaf, /* null. */ (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 7), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf6ToLeaf7(Pleaf, Pjv, (Pjbl->jbl_jp) + offset,
                                       JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 7), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((7) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (7)) == (cJU_LEAF7_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF7VALUEAREA(Pjllnew, cJU_LEAF7_MAXPOP1)) == (cJU_LEAF7_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF7);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
// A top-level BranchL is different and cannot use JU_BRANCHL():  Dont try to
// compress to a (LEAFW) leaf yet, but leave this for a later deletion
// (hysteresis > 0); and the next JP type depends on the system word size; so
// dont use JU_BRANCH_KEEP():
    case cJU_JPBRANCH_L:
    {
        Pjbl_t    Pjbl;
        Word_t    numJPs;
        level = cJU_ROOTSTATE;
        digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        // fall through:
// COMMON CODE FOR KEEPING AND DESCENDING THROUGH A BRANCHL:
//
// Come here with level and digit set.
      BranchLKeep:
        Pjbl = P_JBL(ju_BaLPntr(Pjp));
        numJPs = Pjbl->jbl_NumJPs;
        assert(numJPs > 0);
        DBGCODE(parentJPtype = ju_Type(Pjp););
// Search for a match to the digit (valid Index => must find digit):
        for (offset = 0; (Pjbl->jbl_Expanse[offset]) != digit; ++offset)
            assert(offset < numJPs - 1);
        Pjp = (Pjbl->jbl_jp) + offset;
// If not at a (deletable) JPIMMED_*_01, continue the walk (to descend through
// the BranchL):
        assert(level >= 2);
        if ((ju_Type(Pjp)) != cJU_JPIMMED_1_01 + level - 2)
            break;
// At JPIMMED_*_01:  Ensure the index is in the right expanse, then delete the
// Immed from the BranchL:
//
// Note:  A BranchL has a fixed size and format regardless of numJPs.
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));
        JU_DELETEINPLACE(Pjbl->jbl_Expanse, numJPs, offset, ignore);
        JU_DELETEINPLACEJP(Pjbl->jbl_jp, numJPs, offset, ignore);
// If only one index left in the BranchL, indicate this to the caller:
        return ((--(Pjbl->jbl_NumJPs) <= 1) ? 2 : 1);
    }                                   // case cJU_JPBRANCH_L.
// ****************************************************************************
// BITMAP BRANCH:
//
// MACROS FOR COMMON CODE:
//
// Note the reuse of common macros here, defined earlier:  JU_BRANCH_KEEP(),
// JU_PVALUE*.
//
// Compress a BranchB into a leaf one index size larger:
//
// Allocate a new leaf, walk the JPs in the old BranchB (one bitmap subexpanse
// at a time) and pack their contents into the new leaf (of type NewJPType),
// free the old BranchB, and finally restart the switch to delete Index from
// the new leaf.  Variables Pjp, Pjpm, Pleaf, digit, and pop1 are in the
// context.
//
// Note:  Its no accident that the interface to JU_BRANCHB_COMPRESS() is
// identical to JU_BRANCHL_COMPRESS().  Only the details differ in how to
// traverse the branchs JPs.
// Overall common code for initial BranchB deletion handling:
//
// Assert that Index is in the branch, then see if the BranchB should be kept
// or else compressed to a leaf.  Variables Index, Pjp, and pop1 are in the
// context.
// END OF MACROS, START OF CASES:
//
// Note:  Its no accident that the macro calls for these cases is nearly
// identical to the code for BranchLs.
    case cJU_JPBRANCH_B2:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        pop1 = ju_BranchPop0(Pjp, 2) + 1;
        if (pop1 > (cJU_LEAF2_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((2) >= 2);
            level = (2);
            digit = JU_DIGITATSTATE(Index, 2);
            goto BranchBKeep;
        }
        assert(pop1 == (cJU_LEAF2_MAXPOP1));
        {
            uint16_t *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            if ((PjllnewRaw = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint16_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf, cJU_LEAF2_MAXPOP1););
            PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb = P_JBB(PjbbRaw);
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           /* empty subexpanse */
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (2) - 2)
                    {
                        *Pleaf++ = ju_DcdPop0(Pjp2 + offset - 1);
                        JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf1ToLeaf2(Pleaf, /* null. */ Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 2), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf1ToLeaf2(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 2), Pjpm);
#endif  // JUDYL 
                    Pleaf = (uint16_t *) (((Word_t)Pleaf) + ((2) * pop1));
                    JUDYLCODE(Pjv += pop1;);
                } 
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (2)) == (cJU_LEAF2_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF2VALUEAREA(Pjllnew, cJU_LEAF2_MAXPOP1)) == (cJU_LEAF2_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF2);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_B3:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        pop1 = ju_BranchPop0(Pjp, 3) + 1;
        if (pop1 > (cJU_LEAF3_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((3) >= 2);
            level = (3);
            digit = JU_DIGITATSTATE(Index, 3);
            goto BranchBKeep;
        }
        assert(pop1 == (cJU_LEAF3_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            if ((PjllnewRaw = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pleaf, cJU_LEAF3_MAXPOP1););
            PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb = P_JBB(PjbbRaw);
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           /* empty subexpanse */
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (3) - 2)
                    {
                        JU_COPY3_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2 + offset - 1)));
                        Pleaf += (3);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf2ToLeaf3(Pleaf, /* null. */ Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 3), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf2ToLeaf3(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 3), Pjpm);
#endif  // JUDYL 
                    Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((3) * pop1));
                    JUDYLCODE(Pjv += pop1;);
                } 
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (3)) == (cJU_LEAF3_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF3VALUEAREA(Pjllnew, cJU_LEAF3_MAXPOP1)) == (cJU_LEAF3_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF3);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_B4:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        pop1 = ju_BranchPop0(Pjp, 4) + 1;
        if (pop1 > (cJU_LEAF4_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((4) >= 2);
            level = (4);
            digit = JU_DIGITATSTATE(Index, 4);
            goto BranchBKeep;
        }
        assert(pop1 == (cJU_LEAF4_MAXPOP1));
        {
            uint32_t *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            if ((PjllnewRaw = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint32_t *)Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, cJU_LEAF4_MAXPOP1););
            PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb = P_JBB(PjbbRaw);
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           /* empty subexpanse */
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (4) - 2)
                    {
                        *Pleaf++ = ju_DcdPop0(Pjp2 + offset - 1);
                        JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf3ToLeaf4(Pleaf, /* null. */ Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 4), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf3ToLeaf4(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 4), Pjpm);
#endif  // JUDYL 
                    Pleaf = (uint32_t *)(((Word_t)Pleaf) + ((4) * pop1));
                    JUDYLCODE(Pjv += pop1;);
                } 
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (4)) == (cJU_LEAF4_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF4VALUEAREA(Pjllnew, cJU_LEAF4_MAXPOP1)) == (cJU_LEAF4_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF4);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_B5:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        pop1 = ju_BranchPop0(Pjp, 5) + 1;
        if (pop1 > (cJU_LEAF5_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((5) >= 2);
            level = (5);
            digit = JU_DIGITATSTATE(Index, 5);
            goto BranchBKeep;
        }
        assert(pop1 == (cJU_LEAF5_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            if ((PjllnewRaw = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, cJU_LEAF5_MAXPOP1););
            PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb = P_JBB(PjbbRaw);
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           /* empty subexpanse */
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (5) - 2)
                    {
                        JU_COPY5_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2 + offset - 1)));
                        Pleaf += (5);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf4ToLeaf5(Pleaf, /* null. */ Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 5), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf4ToLeaf5(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 5), Pjpm);
#endif  // JUDYL 
                    Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((5) * pop1));
                    JUDYLCODE(Pjv += pop1;);
                } 
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (5)) == (cJU_LEAF5_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF5VALUEAREA(Pjllnew, cJU_LEAF5_MAXPOP1)) == (cJU_LEAF5_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF5);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_B6:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        pop1 = ju_BranchPop0(Pjp, 6) + 1;
        if (pop1 > (cJU_LEAF6_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((6) >= 2);
            level = (6);
            digit = JU_DIGITATSTATE(Index, 6);
            goto BranchBKeep;
        }
        assert(pop1 == (cJU_LEAF6_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            if ((PjllnewRaw = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, cJU_LEAF6_MAXPOP1););
            PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb = P_JBB(PjbbRaw);
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           /* empty subexpanse */
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (6) - 2)
                    {
                        JU_COPY6_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2 + offset - 1)));
                        Pleaf += (6);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf5ToLeaf6(Pleaf, /* null. */ Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 6), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf5ToLeaf6(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 6), Pjpm);
#endif  // JUDYL 
                    Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((6) * pop1));
                    JUDYLCODE(Pjv += pop1;);
                } 
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (6)) == (cJU_LEAF6_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF6VALUEAREA(Pjllnew, cJU_LEAF6_MAXPOP1)) == (cJU_LEAF6_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF6);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_B7:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        pop1 = ju_BranchPop0(Pjp, 7) + 1;
        if (pop1 > (cJU_LEAF7_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((7) >= 2);
            level = (7);
            digit = JU_DIGITATSTATE(Index, 7);
            goto BranchBKeep;
        }
        assert(pop1 == (cJU_LEAF7_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            if ((PjllnewRaw = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, cJU_LEAF7_MAXPOP1););
            PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb = P_JBB(PjbbRaw);
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           /* empty subexpanse */
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (7) - 2)
                    {
                        JU_COPY7_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2 + offset - 1)));
                        Pleaf += (7);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf6ToLeaf7(Pleaf, /* null. */ Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 7), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf6ToLeaf7(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 7), Pjpm);
#endif  // JUDYL 
                    Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((7) * pop1));
                    JUDYLCODE(Pjv += pop1;);
                } 
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (7)) == (cJU_LEAF7_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF7VALUEAREA(Pjllnew, cJU_LEAF7_MAXPOP1)) == (cJU_LEAF7_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF7);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
// A top-level BranchB is different and cannot use JU_BRANCHB():  Dont try to
// compress to a (LEAFW) leaf yet, but leave this for a later deletion
// (hysteresis > 0); and the next JP type depends on the system word size; so
// dont use JU_BRANCH_KEEP():
    case cJU_JPBRANCH_B:
    {
        Pjbb_t    Pjbb;                 // BranchB to modify.
        Word_t    subexp;               // current subexpanse number.
        Word_t    subexp2;              // in second-level loop.
        BITMAPB_t bitmap;               // portion for this subexpanse.
        BITMAPB_t bitmask;              // with digits bit set.
        Word_t    Pjp2Raw;              // one subexpanses subarray.
        Pjp_t     Pjp2;
        Word_t    numJPs;               // in one subexpanse.
        level = cJU_ROOTSTATE;
        digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        // fall through:
// COMMON CODE FOR KEEPING AND DESCENDING THROUGH A BRANCHB:
//
// Come here with level and digit set.
      BranchBKeep:
        Pjbb = P_JBB(ju_BaLPntr(Pjp));
        subexp = digit / cJU_BITSPERSUBEXPB;
        bitmap = JU_JBB_BITMAP(Pjbb, subexp);
        bitmask = JU_BITPOSMASKB(digit);
        assert(bitmap & bitmask);       // Index valid => digits bit is set.
        DBGCODE(parentJPtype = ju_Type(Pjp););
// Compute digits offset into the bitmap, with a fast method if all bits are
// set:
        offset = j__udyCountBitsB(bitmap & JU_MASKLOWEREXC(bitmask));
//            offset = ((bitmap == (cJU_FULLBITMAPB)) ?
//                      digit % cJU_BITSPERSUBEXPB :
//                      j__udyCountBitsB(bitmap & JU_MASKLOWEREXC(bitmask)));
        Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
        Pjp2 = P_JP(Pjp2Raw);
        assert(Pjp2 != (Pjp_t) NULL);   // valid subexpanse pointer.
// If not at a (deletable) JPIMMED_*_01, continue the walk (to descend through
// the BranchB):
        if (ju_Type(Pjp2 + offset) != cJU_JPIMMED_1_01 + level - 2)
        {
            Pjp = Pjp2 + offset;
            break;
        }
// At JPIMMED_*_01:  Ensure the index is in the right expanse, then delete the
// Immed from the BranchB:
        assert(ju_DcdPop0(Pjp2 + offset) == JU_TRIMTODCDSIZE(Index));
// If only one index is left in the subexpanse, free the JP array:
        if ((numJPs = j__udyCountBitsB(bitmap)) == 1)
        {
            j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ 1, Pjpm);
            JU_JBB_PJP(Pjbb, subexp) = 0;
        }
// Shrink JP array in-place:
        else if (JU_BRANCHBJPGROWINPLACE(numJPs - 1))
        {
            assert(numJPs > 0);
            JU_DELETEINPLACEJP(Pjp2, numJPs, offset, ignore);
        }
// JP array would end up too large; compress it to a smaller one:
        else
        {
            Word_t    PjpnewRaw;
            Pjp_t     Pjpnew;
            PjpnewRaw = j__udyAllocJBBJP(numJPs - 1, Pjpm);
            if (PjpnewRaw == 0)
                return (-1);
            Pjpnew = P_JP(PjpnewRaw);
            JU_DELETECOPY(Pjpnew, Pjp2, numJPs, offset, ignore);
            j__udyFreeJBBJP(Pjp2Raw, numJPs, Pjpm);     // old.
            JU_JBB_PJP(Pjbb, subexp) = PjpnewRaw;
        }
// Clear digits bit in the bitmap:
        JU_JBB_BITMAP(Pjbb, subexp) ^= bitmask;
// If the current subexpanse alone is still too large for a BranchL (with
// hysteresis = 1), the delete is all done:
        if (numJPs > cJU_BRANCHLMAXJPS)
            return (1);
// Consider shrinking the current BranchB to a BranchL:
//
// Check the numbers of JPs in other subexpanses in the BranchL.  Upon reaching
// the critical number of numJPs (which could be right at the start; again,
// with hysteresis = 1), its faster to just watch for any non-empty subexpanse
// than to count bits in each subexpanse.  Upon finding too many JPs, give up
// on shrinking the BranchB.
        for (subexp2 = 0; subexp2 < cJU_NUMSUBEXPB; ++subexp2)
        {
            if (subexp2 == subexp)
                continue;               // skip current subexpanse.
            if ((numJPs == cJU_BRANCHLMAXJPS) ?
                JU_JBB_BITMAP(Pjbb, subexp2) :
                ((numJPs += j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, subexp2))) > cJU_BRANCHLMAXJPS))
            {
                return (1);             // too many JPs, cannot shrink.
            }
        }
// Shrink current BranchB to a BranchL:
//
// Note:  In this rare case, ignore the return value, do not pass it to the
// caller, because the deletion is already successfully completed and the
// caller(s) must decrement population counts.  The only errors expected from
// this call are JU_ERRNO_NOMEM and JU_ERRNO_OVERRUN, neither of which is worth
// forwarding from this point.  See also 4.1, 4.8, and 4.15 of this file.
        (void)j__udyBranchBToBranchL(Pjp, Pjpm);
        return (1);
    }                                   // case.
// ****************************************************************************
// UNCOMPRESSED BRANCH:
//
// MACROS FOR COMMON CODE:
//
// Note the reuse of common macros here, defined earlier:  JU_PVALUE*.
//
// Compress a BranchU into a leaf one index size larger:
//
// Allocate a new leaf, walk the JPs in the old BranchU and pack their contents
// into the new leaf (of type NewJPType), free the old BranchU, and finally
// restart the switch to delete Index from the new leaf.  Variables Pjp, Pjpm,
// digit, and pop1 are in the context.
//
// Note:  Its no accident that the interface to JU_BRANCHU_COMPRESS() is
// nearly identical to JU_BRANCHL_COMPRESS(); just NullJPType is added.  The
// details differ in how to traverse the branchs JPs --
//
// -- and also, what to do upon encountering a cJU_JPIMMED_*_01 JP.  In
// BranchLs and BranchBs the JP must be deleted, but in a BranchU its merely
// converted to a null JP, and this is done by other switch cases, so the "keep
// branch" situation is simpler here and JU_BRANCH_KEEP() is not used.  Also,
// theres no code to convert a BranchU to a BranchB since counting the JPs in
// a BranchU is (at least presently) expensive, and besides, keeping around a
// BranchU is form of hysteresis.
// Overall common code for initial BranchU deletion handling:
//
// Assert that Index is in the branch, then see if a BranchU should be kept or
// else compressed to a leaf.  Variables level, Index, Pjp, and pop1 are in the
// context.
//
// Note:  BranchU handling differs from BranchL and BranchB as described above.
// END OF MACROS, START OF CASES:
//
// Note:  Its no accident that the macro calls for these cases is nearly
// identical to the code for BranchLs, with the addition of cJU_JPNULL*
// parameters only needed for BranchUs.
    case cJU_JPBRANCH_U2:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        pop1 = ju_BranchPop0(Pjp, 2) + 1;
        if (pop1 > (cJU_LEAF2_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (2);                /*          Pjp = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cLevel); */
            Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 2);
            break;                      /* descend to next level */
        }
        assert(pop1 == (cJU_LEAF2_MAXPOP1));
        {
            uint16_t *Pleaf;
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjp_t     Pjp2 = JU_JBU_PJP0(Pjp);
            Word_t    ldigit;           /* larger than uint8_t */
            if ((PjllnewRaw = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint16_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf, cJU_LEAF2_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == (cJU_JPNULL1))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01 + (2) - 2)
                {
                    *Pleaf++ = ju_DcdPop0(Pjp2);
                    JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf1ToLeaf2(Pleaf, /* null. */ Pjp2, JU_DIGITTOSTATE(ldigit, 2), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf1ToLeaf2(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 2), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint16_t *) (((Word_t)Pleaf) + ((2) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (2)) == (cJU_LEAF2_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF2VALUEAREA(Pjllnew, cJU_LEAF2_MAXPOP1)) == (cJU_LEAF2_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF2);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_U3:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        pop1 = ju_BranchPop0(Pjp, 3) + 1;
        if (pop1 > (cJU_LEAF3_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (3);                /*          Pjp = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cLevel); */
            Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 3);
            break;                      /* descend to next level */
        }
        assert(pop1 == (cJU_LEAF3_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjp_t     Pjp2 = JU_JBU_PJP0(Pjp);
            Word_t    ldigit;           /* larger than uint8_t */
            if ((PjllnewRaw = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pleaf, cJU_LEAF3_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == (cJU_JPNULL2))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01 + (3) - 2)
                {
                    JU_COPY3_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2)));
                    Pleaf += (3);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf2ToLeaf3(Pleaf, /* null. */ Pjp2, JU_DIGITTOSTATE(ldigit, 3), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf2ToLeaf3(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 3), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((3) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (3)) == (cJU_LEAF3_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF3VALUEAREA(Pjllnew, cJU_LEAF3_MAXPOP1)) == (cJU_LEAF3_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF3);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_U4:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        pop1 = ju_BranchPop0(Pjp, 4) + 1;
        if (pop1 > (cJU_LEAF4_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (4);                /*          Pjp = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cLevel); */
            Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 4);
            break;                      /* descend to next level */
        }
        assert(pop1 == (cJU_LEAF4_MAXPOP1));
        {
            uint32_t *Pleaf;
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjp_t     Pjp2 = JU_JBU_PJP0(Pjp);
            Word_t    ldigit;           /* larger than uint8_t */
            if ((PjllnewRaw = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint32_t *)Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, cJU_LEAF4_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == (cJU_JPNULL3))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01 + (4) - 2)
                {
                    *Pleaf++ = ju_DcdPop0(Pjp2);
                    JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf3ToLeaf4(Pleaf, /* null. */ Pjp2, JU_DIGITTOSTATE(ldigit, 4), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf3ToLeaf4(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 4), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint32_t *)(((Word_t)Pleaf) + ((4) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (4)) == (cJU_LEAF4_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF4VALUEAREA(Pjllnew, cJU_LEAF4_MAXPOP1)) == (cJU_LEAF4_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF4);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_U5:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        pop1 = ju_BranchPop0(Pjp, 5) + 1;
        if (pop1 > (cJU_LEAF5_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (5);                /*          Pjp = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cLevel); */
            Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 5);
            break;                      /* descend to next level */
        }
        assert(pop1 == (cJU_LEAF5_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjp_t     Pjp2 = JU_JBU_PJP0(Pjp);
            Word_t    ldigit;           /* larger than uint8_t */
            if ((PjllnewRaw = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, cJU_LEAF5_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == (cJU_JPNULL4))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01 + (5) - 2)
                {
                    JU_COPY5_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2)));
                    Pleaf += (5);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf4ToLeaf5(Pleaf, /* null. */ Pjp2, JU_DIGITTOSTATE(ldigit, 5), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf4ToLeaf5(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 5), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((5) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (5)) == (cJU_LEAF5_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF5VALUEAREA(Pjllnew, cJU_LEAF5_MAXPOP1)) == (cJU_LEAF5_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF5);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_U6:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        pop1 = ju_BranchPop0(Pjp, 6) + 1;
        if (pop1 > (cJU_LEAF6_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (6);                /*          Pjp = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cLevel); */
            Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 6);
            break;                      /* descend to next level */
        }
        assert(pop1 == (cJU_LEAF6_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjp_t     Pjp2 = JU_JBU_PJP0(Pjp);
            Word_t    ldigit;           /* larger than uint8_t */
            if ((PjllnewRaw = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, cJU_LEAF6_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == (cJU_JPNULL5))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01 + (6) - 2)
                {
                    JU_COPY6_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2)));
                    Pleaf += (6);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf5ToLeaf6(Pleaf, /* null. */ Pjp2, JU_DIGITTOSTATE(ldigit, 6), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf5ToLeaf6(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 6), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((6) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (6)) == (cJU_LEAF6_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF6VALUEAREA(Pjllnew, cJU_LEAF6_MAXPOP1)) == (cJU_LEAF6_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF6);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    case cJU_JPBRANCH_U7:
        assert(!ju_DcdNonMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        pop1 = ju_BranchPop0(Pjp, 7) + 1;
        if (pop1 > (cJU_LEAF7_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (7);                /*          Pjp = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cLevel); */
            Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 7);
            break;                      /* descend to next level */
        }
        assert(pop1 == (cJU_LEAF7_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjp_t     Pjp2 = JU_JBU_PJP0(Pjp);
            Word_t    ldigit;           /* larger than uint8_t */
            if ((PjllnewRaw = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, cJU_LEAF7_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == (cJU_JPNULL6))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01 + (7) - 2)
                {
                    JU_COPY7_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2)));
                    Pleaf += (7);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_PImmVals(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf6ToLeaf7(Pleaf, /* null. */ Pjp2, JU_DIGITTOSTATE(ldigit, 7), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf6ToLeaf7(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 7), Pjpm);
#endif  // JUDYL 
                Pleaf = (uint8_t *) (((Word_t)Pleaf) + ((7) * pop1));
                JUDYLCODE(Pjv += pop1;);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (7)) == (cJU_LEAF7_MAXPOP1));
            JUDYLCODE(assert((Pjv - JL_LEAF7VALUEAREA(Pjllnew, cJU_LEAF7_MAXPOP1)) == (cJU_LEAF7_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */
            ju_SetJpType(Pjp, cJU_JPLEAF7);     /* Pjp->Jp_Addr0 = PjllnewRaw;        */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
// A top-level BranchU is different and cannot use JU_BRANCHU():  Dont try to
// compress to a (LEAFW) leaf yet, but leave this for a later deletion
// (hysteresis > 0); just descend through the BranchU:
    case cJU_JPBRANCH_U:
        DBGCODE(parentJPtype = ju_Type(Pjp););
        level = cJU_ROOTSTATE;
//            Pjp   = P_JP(ju_BaLPntr(Pjp)) + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        Pjbu_t    Pjbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        break;
// ****************************************************************************
// LINEAR LEAF:
//
// State transitions while deleting an Index, the inverse of the similar table
// that appears in JudyIns.c:
//
// Note:  In JudyIns.c this table is not needed and does not appear until the
// Immed handling code; because once a Leaf is reached upon growing the tree,
// the situation remains simpler, but for deleting indexes, the complexity
// arises when leaves must compress to Immeds.
//
// Note:  There are other transitions possible too, not shown here, such as to
// a leaf one level higher.
//
// (Yes, this is very terse...  Study it and it will make sense.)
// (Note, parts of this diagram are repeated below for quick reference.)
//
//                      reformat JP here for Judy1 only, from word-1 to word-2
//                                                                     |
//           JUDY1               JUDY1                                 |
//                                                                     V
// (*) Leaf1 [[ => 1_15..08 ] => 1_07 => ... => 1_04 ] => 1_03 => 1_02 => 1_01
//     Leaf2 [[ => 2_07..04 ] => 2_03 => 2_02        ]                 => 2_01
//     Leaf3 [[ => 3_05..03 ] => 3_02                ]                 => 3_01
//     Leaf4 [[ => 4_03..02 ]]                                         => 4_01
//     Leaf5 [[ => 5_03..02 ]]                                         => 5_01
//     Leaf6 [[ => 6_02     ]]                                         => 6_01
//     Leaf7 [[ => 7_02     ]]                                         => 7_01
//
// (*) For Judy1 & 64-bit, go directly from a LeafB1 to cJU_JPIMMED_1_15; skip
//     Leaf1, as described in Judy1.h regarding cJ1_JPLEAF1.
//
// MACROS FOR COMMON CODE:
//
// (De)compress a LeafX into a LeafY one index size (cIS) larger (X+1 = Y):
//
// This is only possible when the current leaf is under a narrow pointer
// ((ParentLevel - 1) > cIS) and its population fits in a higher-level leaf.
// Variables ParentLevel, pop1, PjllnewRaw, Pjllnew, Pjpm, and Index are in the
// context.
//
// Note:  Doing an "uplevel" doesnt occur until the old leaf can be compressed
// up one level BEFORE deleting an index; that is, hysteresis = 1.
//
// Note:  LeafType, MaxPop1, NewJPType, and Alloc refer to the up-level leaf,
// not the current leaf.
//
// Note:  010327:  Fixed bug where the jp_DcdPopO next-uplevel digit (byte)
// above the current Pop0 value was not being cleared.  When upleveling, one
// digit in jp_DcdPopO "moves" from being part of the Dcd subfield to the Pop0
// subfield, but since a leaf maxpop1 is known to be <= 1 byte in size, the new
// Pop0 byte should always be zero.  This is easy to overlook because
// JU_JPLEAF_POP0() "knows" to only use the LSB of Pop0 (for efficiency) and
// ignore the other bytes...  Until someone uses cJU_POP0MASK() instead of
// JU_JPLEAF_POP0(), such as in JudyInsertBranch.c.
//
// TBD:  Should JudyInsertBranch.c use JU_JPLEAF_POP0() rather than
// cJU_POP0MASK(), for efficiency?  Does it know for sure its a narrow pointer
// under the leaf?  Not necessarily.
// For Leaf3, only support JU_LEAF_UPLEVEL on a 64-bit system, and for Leaf7,
// there is no JU_LEAF_UPLEVEL:
//
// Note:  Theres no way here to go from Leaf3 [Leaf7] to LEAFW on a 32-bit
// [64-bit] system.  Thats handled in the main code, because its different in
// that a JPM is involved.
// Compress a Leaf* with pop1 = 2, or a JPIMMED_*_02, into a JPIMMED_*_01:
//
// Copy whichever Index is NOT being deleted (and assert that the other one is
// found; Index must be valid).  This requires special handling of the Index
// bytes (and value area).  Variables Pjp, Index, offset, and Pleaf are in the
// context, offset is modified to the undeleted Index, and Pjp is modified
// including Jp_Addr0.
// Compress a Leaf* into a JPIMMED_*_0[2+]:
//
// This occurs as soon as its possible, with hysteresis = 0.  Variables pop1,
// Pleaf, offset, and Pjpm are in the context.
//
// TBD:  Explain why hysteresis = 0 here, rather than > 0.  Probably because
// the insert code assumes if the population is small enough, an Immed is used,
// not a leaf.
//
// The differences between Judy1 and JudyL with respect to value area handling
// are just too large for completely common code between them...  Oh well, some
// big ifdefs follow.
#ifdef JUDYL
// Pjv is also in the context.
// A complicating factor for JudyL & 32-bit is that Leaf2..3, and for JudyL &
// 64-bit Leaf 4..7, go directly to an Immed*_01, where the value is stored in
// Jp_Addr0 and not in a separate LeafV.  For efficiency, use the following
// macro in cases where it can apply; it is rigged to do the right thing.
// Unfortunately, this requires the calling code to "know" the transition table
// and call the right macro.
//
// This variant compresses a Leaf* with pop1 = 2 into a JPIMMED_*_01:
#endif  // JUDYL 
// See comments above about these:
//
// Note:  Here "23" means index size 2 or 3, and "47" means 4..7.
// Compress a Leaf* in place:
//
// Here hysteresis = 0 (no memory is wasted).  Variables pop1, Pleaf, and
// offset, and for JudyL, Pjv, are in the context.
// Compress a Leaf* into a smaller memory object of the same JP type:
//
// Variables PjllnewRaw, Pjllnew, Pleafpop1, Pjpm, PleafRaw, Pleaf, and offset
// are in the context.
// Overall common code for Leaf* deletion handling:
//
// See if the leaf can be:
// - (de)compressed to one a level higher (JU_LEAF_UPLEVEL()), or if not,
// - compressed to an Immediate JP (JU_LEAF_TOIMMED()), or if not,
// - shrunk in place (JU_LEAF_INPLACE()), or if none of those, then
// - shrink the leaf to a smaller chunk of memory (JU_LEAF_SHRINK()).
//
// Variables Pjp, pop1, Index, and offset are in the context.
// The *Up parameters refer to a leaf one level up, if there is any.
// END OF MACROS, START OF CASES:
//
// (*) Leaf1 [[ => 1_15..08 ] => 1_07 => ... => 1_04 ] => 1_03 => 1_02 => 1_01
#ifdef JUDYL
    case cJU_JPLEAF1:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 1));
        assert(ParentLevel > (1));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1;
        assert(((ParentLevel - 1) == (1)) || (pop1 >= (cJU_LEAF2_MAXPOP1)));
        if (((ParentLevel - 1) > (1)) /* under narrow pointer */  && (pop1 == (cJU_LEAF2_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA((uint16_t *) Pjllnew, cJU_LEAF2_MAXPOP1););
            (void)j__udyLeaf1ToLeaf2((uint16_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(1), /* TBD, Doug says */ Pjpm);
            D_cdP0 = (~cJU_MASKATSTATE((1) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF2);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf1(Pleaf, pop1, Index, 1 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF1VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED1_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED1_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm)) == 0)
                return (-1);
            JUDYLCODE(Pjvnew = P_JV(PjvnewRaw););
            JU_DELETECOPY((uint8_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 1);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 1);
            j__udyFreeJLL1(PjllRaw, pop1, Pjpm);        /* Pjp->jp_PValue = PjvnewRaw;      */
            ju_SetPjvPntr(Pjp, PjvnewRaw);      /* Pjp->jp_Type = (BaseJPType) - 2 + (MaxPop1); */
            ju_SetJpType(Pjp, (cJU_JPIMMED_1_02) - 2 + (cJU_IMMED1_MAXPOP1));
            return (1);
        }
        if (JU_LEAF1GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, pop1, offset, 1);
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL1(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF1VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY((uint8_t *) Pjllnew, Pleaf, pop1, offset, 1);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 1);
            j__udyFreeJLL1(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            return (1);
        }
    }
#endif  // JUDYL 
// A complicating factor is that for JudyL & 32-bit, a Leaf2 must go directly
// to an Immed 2_01 and a Leaf3 must go directly to an Immed 3_01:
//
// Leaf2 [[ => 2_07..04 ] => 2_03 => 2_02 ] => 2_01
// Leaf3 [[ => 3_05..03 ] => 3_02         ] => 3_01
//
// Hence use JU_LEAF_TOIMMED_23 instead of JU_LEAF_TOIMMED in the cases below,
// and also the parameters ToImmed and, for odd index sizes, CopyPIndex, are
// required.
    case cJU_JPLEAF2:
    {
        Word_t    PleafRaw;
        uint16_t *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint16_t *) P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1;
        assert(((ParentLevel - 1) == (2)) || (pop1 >= (cJU_LEAF3_MAXPOP1)));
        if (((ParentLevel - 1) > (2)) /* under narrow pointer */  && (pop1 == (cJU_LEAF3_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF3_MAXPOP1););
#ifdef  JUDY1
            (void)j__udyLeaf2ToLeaf3((uint8_t *) Pjllnew, /* null. */ Pjp, Index & cJU_DCDMASK(2), /* TBD, Doug says */
                                     Pjpm);
#else   // JUDYL 
            (void)j__udyLeaf2ToLeaf3((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(2), /* TBD, Doug says */ Pjpm);
#endif  // JUDYL 
            D_cdP0 = (~cJU_MASKATSTATE((2) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF3);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf2(Pleaf, pop1, Index, 2 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED2_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED2_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY((uint16_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 2);        /* Pjp->jp_Type = (BaseJPType) - 1 + (MaxPop1) - 1; */
            ju_SetJpType(Pjp, (cJU_JPIMMED_2_02) - 1 + (cJU_IMMED2_MAXPOP1) - 1);
            j__udyFreeJLL2(PjllRaw, pop1, Pjpm);
#else   // JUDYL 
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm)) == 0)
                return (-1);
            JUDYLCODE(Pjvnew = P_JV(PjvnewRaw););
            JU_DELETECOPY((uint16_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 2);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 2);
            j__udyFreeJLL2(PjllRaw, pop1, Pjpm);        /* Pjp->jp_PValue = PjvnewRaw;      */
            ju_SetPjvPntr(Pjp, PjvnewRaw);      /* Pjp->jp_Type = (BaseJPType) - 2 + (MaxPop1); */
            ju_SetJpType(Pjp, (cJU_JPIMMED_2_02) - 2 + (cJU_IMMED2_MAXPOP1));
#endif  // JUDYL 
            return (1);
        }
        if (JU_LEAF2GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, pop1, offset, 2);
#ifdef JUDYL
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL2(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF2VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY((uint16_t *) Pjllnew, Pleaf, pop1, offset, 2);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 2);
            j__udyFreeJLL2(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        if ((PjllnewRaw = j__udyAllocJLL2(pop1 - 1, Pjpm)) == 0)
            return (-1);
        Pjllnew = P_JLL(PjllnewRaw);
        JU_DELETECOPY((uint16_t *) Pjllnew, Pleaf, pop1, offset, 2);
        j__udyFreeJLL2(PleafRaw, pop1, Pjpm);   /*   Pjp->Jp_Addr0 = PjllnewRaw;     */
        ju_SetBaLPntr(Pjp, PjllnewRaw);
        return (1);
#endif  // JUDY1    
    }
// On 32-bit there is no transition to "uplevel" for a Leaf3, so use
// JU_LEAF_UPLEVEL64 instead of JU_LEAF_UPLEVEL:
    case cJU_JPLEAF3:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1;
        assert(((ParentLevel - 1) == (3)) || (pop1 >= (cJU_LEAF4_MAXPOP1)));
        if (((ParentLevel - 1) > (3)) /* under narrow pointer */  && (pop1 == (cJU_LEAF4_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA((uint32_t *)Pjllnew, cJU_LEAF4_MAXPOP1););
#ifdef  JUDY1
            (void)j__udyLeaf3ToLeaf4((uint32_t *)Pjllnew, /* null. */ Pjp, Index & cJU_DCDMASK(3), /* TBD, Doug says */
                                     Pjpm);
#else   // JUDYL 
            (void)j__udyLeaf3ToLeaf4((uint32_t *)Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(3), /* TBD, Doug says */ Pjpm);
#endif  // JUDYL 
            D_cdP0 = (~cJU_MASKATSTATE((3) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF4);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf3(Pleaf, pop1, Index, 3 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED3_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED3_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD((uint8_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 3);     /* Pjp->jp_Type = (BaseJPType) - 1 + (MaxPop1) - 1; */
            ju_SetJpType(Pjp, (cJU_JPIMMED_3_02) - 1 + (cJU_IMMED3_MAXPOP1) - 1);
            j__udyFreeJLL3(PjllRaw, pop1, Pjpm);
#else   // JUDYL 
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm)) == 0)
                return (-1);
            JUDYLCODE(Pjvnew = P_JV(PjvnewRaw););
            JU_DELETECOPY_ODD((uint8_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 3);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 3);
            j__udyFreeJLL3(PjllRaw, pop1, Pjpm);        /* Pjp->jp_PValue = PjvnewRaw;      */
            ju_SetPjvPntr(Pjp, PjvnewRaw);      /* Pjp->jp_Type = (BaseJPType) - 2 + (MaxPop1); */
            ju_SetJpType(Pjp, (cJU_JPIMMED_3_02) - 2 + (cJU_IMMED3_MAXPOP1));
#endif  // JUDYL 
            return (1);
        }
        if (JU_LEAF3GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, pop1, offset, 3);
#ifdef JUDYL
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL3(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF3VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 3);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 3);
            j__udyFreeJLL3(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        if ((PjllnewRaw = j__udyAllocJLL3(pop1 - 1, Pjpm)) == 0)
            return (-1);
        Pjllnew = P_JLL(PjllnewRaw);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 3);
        j__udyFreeJLL3(PleafRaw, pop1, Pjpm);   /*   Pjp->Jp_Addr0 = PjllnewRaw;     */
        ju_SetBaLPntr(Pjp, PjllnewRaw);
        return (1);
#endif  // JUDY1    
    }
// A complicating factor is that for JudyL & 64-bit, a Leaf[4-7] must go
// directly to an Immed [4-7]_01:
//
// Leaf4 [[ => 4_03..02 ]] => 4_01
// Leaf5 [[ => 5_03..02 ]] => 5_01
// Leaf6 [[ => 6_02     ]] => 6_01
// Leaf7 [[ => 7_02     ]] => 7_01
//
// Hence use JU_LEAF_TOIMMED_47 instead of JU_LEAF_TOIMMED in the cases below.
    case cJU_JPLEAF4:
    {
        Word_t    PleafRaw;
        uint32_t *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint32_t *)P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1;
        assert(((ParentLevel - 1) == (4)) || (pop1 >= (cJU_LEAF5_MAXPOP1)));
        if (((ParentLevel - 1) > (4)) /* under narrow pointer */  && (pop1 == (cJU_LEAF5_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF5_MAXPOP1););
#ifdef  JUDY1
            (void)j__udyLeaf4ToLeaf5((uint8_t *) Pjllnew, /* null. */ Pjp, Index & cJU_DCDMASK(4), /* TBD, Doug says */
                                     Pjpm);
#else   // JUDYL 
            (void)j__udyLeaf4ToLeaf5((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(4), /* TBD, Doug says */ Pjpm);
#endif  // JUDYL 
            D_cdP0 = (~cJU_MASKATSTATE((4) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF5);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf4(Pleaf, pop1, Index, 4 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED4_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED4_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY((uint32_t *)ju_PImmed1(Pjp), Pleaf, pop1, offset, 4); /* Pjp->jp_Type = (BaseJPType) - 1 + (MaxPop1) - 1; */
            ju_SetJpType(Pjp, (cJ1_JPIMMED_4_02) - 1 + (cJU_IMMED4_MAXPOP1) - 1);
            j__udyFreeJLL4(PjllRaw, pop1, Pjpm);
#else   // JUDYL 
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                uint8_t   T_ype = ju_Type(Pjp);
                offset = (Pleaf[0] == JU_LEASTBYTES(Index, 4)); /* undeleted Ind */
                assert(Pleaf[offset ? 0 : 1] == JU_LEASTBYTES(Index, 4));
                D_cdP0 = (Index & cJU_DCDMASK(4)) | Pleaf[offset];
                JUDYLCODE(A_ddr = Pjv[offset];);                  /*   JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
                ju_SetBaLPntr(Pjp, A_ddr);
                ju_SetDcdPop0(Pjp, D_cdP0);
                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL4(PjllRaw, pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
            ju_SetJpType(Pjp, cJU_JPIMMED_4_01);
#endif  // JUDYL 
            return (1);
        }
        if (JU_LEAF4GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, pop1, offset, 4);
#ifdef JUDYL
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL4(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF4VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY((uint32_t *)Pjllnew, Pleaf, pop1, offset, 4);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 4);
            j__udyFreeJLL4(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        if ((PjllnewRaw = j__udyAllocJLL4(pop1 - 1, Pjpm)) == 0)
            return (-1);
        Pjllnew = P_JLL(PjllnewRaw);
        JU_DELETECOPY((uint32_t *)Pjllnew, Pleaf, pop1, offset, 4);
        j__udyFreeJLL4(PleafRaw, pop1, Pjpm);   /*   Pjp->Jp_Addr0 = PjllnewRaw;     */
        ju_SetBaLPntr(Pjp, PjllnewRaw);
        return (1);
#endif  // JUDY1    
    }
    case cJU_JPLEAF5:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1;
        assert(((ParentLevel - 1) == (5)) || (pop1 >= (cJU_LEAF6_MAXPOP1)));
        if (((ParentLevel - 1) > (5)) /* under narrow pointer */  && (pop1 == (cJU_LEAF6_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF6_MAXPOP1););
#ifdef  JUDY1
            (void)j__udyLeaf5ToLeaf6((uint8_t *) Pjllnew, /* null. */ Pjp, Index & cJU_DCDMASK(5), /* TBD, Doug says */
                                     Pjpm);
#else   // JUDYL 
            (void)j__udyLeaf5ToLeaf6((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(5), /* TBD, Doug says */ Pjpm);
#endif  // JUDYL 
            D_cdP0 = (~cJU_MASKATSTATE((5) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF6);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf5(Pleaf, pop1, Index, 5 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED5_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED5_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD((uint8_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 5);     /* Pjp->jp_Type = (BaseJPType) - 1 + (MaxPop1) - 1; */
            ju_SetJpType(Pjp, (cJ1_JPIMMED_5_02) - 1 + (cJU_IMMED5_MAXPOP1) - 1);
            j__udyFreeJLL5(PjllRaw, pop1, Pjpm);
#else   // JUDYL 
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                uint8_t   T_ype = ju_Type(Pjp);
                offset = j__udySearchLeaf5(Pleaf, 2, Index, 5 * 8);
                assert(offset >= 0);    /* Index must be valid */
                JU_COPY5_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 5]));
                D_cdP0 |= Index & cJU_DCDMASK(5);
                JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                  /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
                ju_SetBaLPntr(Pjp, A_ddr);
                ju_SetDcdPop0(Pjp, D_cdP0);
                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL5(PjllRaw, pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
            ju_SetJpType(Pjp, cJU_JPIMMED_5_01);
#endif  // JUDYL 
            return (1);
        }
        if (JU_LEAF5GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, pop1, offset, 5);
#ifdef JUDYL
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL5(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF5VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 5);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 5);
            j__udyFreeJLL5(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        if ((PjllnewRaw = j__udyAllocJLL5(pop1 - 1, Pjpm)) == 0)
            return (-1);
        Pjllnew = P_JLL(PjllnewRaw);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 5);
        j__udyFreeJLL5(PleafRaw, pop1, Pjpm);   /*   Pjp->Jp_Addr0 = PjllnewRaw;     */
        ju_SetBaLPntr(Pjp, PjllnewRaw);
        return (1);
#endif  // JUDY1    
    }
    case cJU_JPLEAF6:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1;
        assert(((ParentLevel - 1) == (6)) || (pop1 >= (cJU_LEAF7_MAXPOP1)));
        if (((ParentLevel - 1) > (6)) /* under narrow pointer */  && (pop1 == (cJU_LEAF7_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF7_MAXPOP1););
#ifdef  JUDY1
            (void)j__udyLeaf6ToLeaf7((uint8_t *) Pjllnew, /* null. */ Pjp, Index & cJU_DCDMASK(6), /* TBD, Doug says */
                                     Pjpm);
#else   // JUDYL 
            (void)j__udyLeaf6ToLeaf7((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(6), /* TBD, Doug says */ Pjpm);
#endif  // JUDYL 
            D_cdP0 = (~cJU_MASKATSTATE((6) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF7);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf6(Pleaf, pop1, Index, 6 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED6_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED6_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD((uint8_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 6);     /* Pjp->jp_Type = (BaseJPType) - 1 + (MaxPop1) - 1; */
            ju_SetJpType(Pjp, (cJ1_JPIMMED_6_02) - 1 + (cJU_IMMED6_MAXPOP1) - 1);
            j__udyFreeJLL6(PjllRaw, pop1, Pjpm);
#else   // JUDYL 
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                uint8_t   T_ype = ju_Type(Pjp);
                offset = j__udySearchLeaf6(Pleaf, 2, Index, 6 * 8);
                assert(offset >= 0);    /* Index must be valid */
                JU_COPY6_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 6]));
                D_cdP0 |= Index & cJU_DCDMASK(6);
                JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                  /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
                ju_SetBaLPntr(Pjp, A_ddr);
                ju_SetDcdPop0(Pjp, D_cdP0);
                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL6(PjllRaw, pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
            ju_SetJpType(Pjp, cJU_JPIMMED_6_01);
#endif  // JUDYL 
            return (1);
        }
        if (JU_LEAF6GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, pop1, offset, 6);
#ifdef JUDYL
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL6(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF6VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 6);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 6);
            j__udyFreeJLL6(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        if ((PjllnewRaw = j__udyAllocJLL6(pop1 - 1, Pjpm)) == 0)
            return (-1);
        Pjllnew = P_JLL(PjllnewRaw);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 6);
        j__udyFreeJLL6(PleafRaw, pop1, Pjpm);   /*   Pjp->Jp_Addr0 = PjllnewRaw;     */
        ju_SetBaLPntr(Pjp, PjllnewRaw);
        return (1);
#endif  // JUDY1    
    }
// There is no transition to "uplevel" for a Leaf7, so use JU_LEAF_UPLEVEL_NONE
// instead of JU_LEAF_UPLEVEL, and ignore all of the parameters to that macro:
    case cJU_JPLEAF7:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        PleafRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PleafRaw);
        pop1 = ju_LeafPop0(Pjp) + 1; /* null. */ ;
        offset = j__udySearchLeaf7(Pleaf, pop1, Index, 7 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, pop1););
        assert(pop1 > (cJU_IMMED7_MAXPOP1));
        if ((pop1 - 1) == (cJU_IMMED7_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_BaLPntr(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD((uint8_t *) ju_PImmed1(Pjp), Pleaf, pop1, offset, 7);     /* Pjp->jp_Type = (BaseJPType) - 1 + (MaxPop1) - 1; */
            ju_SetJpType(Pjp, (cJ1_JPIMMED_7_02) - 1 + (cJU_IMMED7_MAXPOP1) - 1);
            j__udyFreeJLL7(PjllRaw, pop1, Pjpm);
#else   // JUDYL 
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                uint8_t   T_ype = ju_Type(Pjp);
                offset = j__udySearchLeaf7(Pleaf, 2, Index, 7 * 8);
                assert(offset >= 0);    /* Index must be valid */
                JU_COPY7_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 7]));
                D_cdP0 |= Index & cJU_DCDMASK(7);
                JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                  /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
                ju_SetBaLPntr(Pjp, A_ddr);
                ju_SetDcdPop0(Pjp, D_cdP0);
                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL7(PjllRaw, pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
            ju_SetJpType(Pjp, cJU_JPIMMED_7_01);
#endif  // JUDYL 
            return (1);
        }
        if (JU_LEAF7GROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, pop1, offset, 7);
#ifdef JUDYL
            /**/ JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
            return (1);
        }
        {
            /**/ Pjv_t Pjvnew;
            if ((PjllnewRaw = j__udyAllocJLL7(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            /**/ Pjvnew = JL_LEAF7VALUEAREA(Pjllnew, pop1 - 1);
            JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 7);
            /**/ JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, 7);
            j__udyFreeJLL7(PleafRaw, pop1, Pjpm);       /*  Pjp->Jp_Addr0 = PjllnewRaw;    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        if ((PjllnewRaw = j__udyAllocJLL7(pop1 - 1, Pjpm)) == 0)
            return (-1);
        Pjllnew = P_JLL(PjllnewRaw);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, pop1, offset, 7);
        j__udyFreeJLL7(PleafRaw, pop1, Pjpm);   /*   Pjp->Jp_Addr0 = PjllnewRaw;     */
        ju_SetBaLPntr(Pjp, PjllnewRaw);
        return (1);
#endif  // JUDY1    
    }
// ****************************************************************************
// BITMAP LEAF:
    case cJU_JPLEAF_B1:
    {
#ifdef JUDYL
        Word_t    PjvnewRaw;            // new value area.
        Pjv_t     Pjvnew;
        Word_t    subexp;               // 1 of 8 subexpanses in bitmap.
        Pjlb_t    Pjlb;                 // pointer to bitmap part of the leaf.
        BITMAPL_t bitmap;               // for one subexpanse.
        BITMAPL_t bitmask;              // bit set for Indexs digit.
#endif  // JUDYL 
        assert(!ju_DcdNonMatchKey(Index, Pjp, 1));
        assert(ParentLevel > 1);
        // valid Index:
        assert(JU_BITMAPTESTL(P_JLB(ju_BaLPntr(Pjp)), Index));
        pop1 = ju_LeafPop0(Pjp) + 1;
// Like a Leaf1, see if its under a narrow pointer and can become a Leaf2
// (hysteresis = 1):
//  This code may not work!!!!! (dlb) and probally not good for a large Leaf2
        assert(((ParentLevel - 1) == (1)) || (pop1 >= (cJU_LEAF2_MAXPOP1)));
        if (((ParentLevel - 1) > (1)) /* under narrow pointer */  && (pop1 == (cJU_LEAF2_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            if ((PjllnewRaw = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm)) == 0)
                return (-1);
            Pjllnew = P_JLL(PjllnewRaw);
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA((uint16_t *) Pjllnew, cJU_LEAF2_MAXPOP1););
#ifdef  JUDY1
            (void)j__udyLeaf1ToLeaf2((uint16_t *) Pjllnew, /* null. */ Pjp, Index & cJU_DCDMASK(1), /* TBD, Doug says */
                                     Pjpm);
#else   // JUDYL 
            (void)j__udyLeaf1ToLeaf2((uint16_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(1), /* TBD, Doug says */ Pjpm);
#endif  // JUDYL 
            D_cdP0 = (~cJU_MASKATSTATE((1) + 1)) & ju_DcdPop0(Pjp);     /*    JU_JPSETADT(Pjp, PjllnewRaw, D_cdP0, NewJPType);    */
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF2);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
#ifdef  JUDY1
// Handle the unusual special case, on Judy1 64-bit only, where a LeafB1 goes
// directly to a JPIMMED_1_15; as described in comments in Judy1.h and
// JudyIns.c.  Copy 1-byte indexes from old LeafB1 to the Immed:
        if ((pop1 - 1) == cJU_IMMED1_MAXPOP1)   // hysteresis = 0.
        {
            Word_t    PjlbRaw;          // bitmap in old leaf.
            Pjlb_t    Pjlb;
            uint8_t  *Pleafnew;         // JPIMMED as a pointer.
            Word_t    ldigit;           // larger than uint8_t.
            PjlbRaw = ju_BaLPntr(Pjp);
            Pjlb = P_JLB(PjlbRaw);
            Pleafnew = ju_PImmed1(Pjp);
            JU_BITMAPCLEARL(Pjlb, Index);       // unset Indexs bit.
// TBD:  This is very slow, there must be a better way:
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit)
            {
                if (JU_BITMAPTESTL(Pjlb, ldigit))
                {
                    *Pleafnew++ = ldigit;
                    assert((Pleafnew - ju_PImmed1(Pjp)) <= cJU_IMMED1_MAXPOP1);
                }
            }
            j__udyFreeJLB1(PjlbRaw, Pjpm);
//                Pjp->jp_Type = cJ1_JPIMMED_1_15;
            ju_SetJpType(Pjp, cJ1_JPIMMED_1_15);
#else   // JUDYL 
// Compress LeafB1 to a Leaf1:
//
// Note:  4.37 of this file contained alternate code for Judy1 only that simply
// cleared the bit and allowed the LeafB1 to go below cJU_LEAF1_MAXPOP1.  This
// was the ONLY case where a malloc failure was not fatal; however, it violated
// the critical assumption that the tree is always kept in least-compressed
// form.
        if (pop1 == cJU_LEAF1_MAXPOP1)  // hysteresis = 1.
        {
            if (j__udyLeafB1ToLeaf1(Pjp, Pjpm) == -1)
                return (-1);
            goto ContinueDelWalk;       // delete Index in new Leaf1.
        }
// This is very different from Judy1 because of the need to manage the value
// area:
//
// Get last byte to decode from Index, and pointer to bitmap leaf:
        digit = JU_DIGITATSTATE(Index, 1);
        Pjlb = P_JLB(ju_BaLPntr(Pjp));
// Prepare additional values:
        subexp = digit / cJU_BITSPERSUBEXPL;    // which subexpanse.
        bitmap = JU_JLB_BITMAP(Pjlb, subexp);   // subexps 32-bit map.
        PjvRaw = JL_JLB_PVALUE(Pjlb, subexp);   // corresponding values.
        Pjv = P_JV(PjvRaw);
        bitmask = JU_BITPOSMASKL(digit);        // mask for Index.
        assert(bitmap & bitmask);       // Index must be valid.
        if (bitmap == cJU_FULLBITMAPL)  // full bitmap, take shortcut:
        {
            pop1 = cJU_BITSPERSUBEXPL;
            offset = digit % cJU_BITSPERSUBEXPL;
        }
        else                            // compute subexpanse pop1 and value area offset:
        {
            pop1 = j__udyCountBitsL(bitmap);
            offset = j__udyCountBitsL(bitmap & (bitmask - 1));
        }
// Handle solitary Index remaining in subexpanse:
        if (pop1 == 1)
        {
            j__udyLFreeJV(PjvRaw, 1, Pjpm);
            JL_JLB_PVALUE(Pjlb, subexp) = 0;
            JU_JLB_BITMAP(Pjlb, subexp) = 0;
#endif  // JUDYL 
            return (1);
        }
#ifdef  JUDY1
        // unset Indexs bit:
        JU_BITMAPCLEARL(P_JLB(ju_BaLPntr(Pjp)), Index);
#else   // JUDYL 
// Shrink value area in place or move to a smaller value area:
        if (JL_LEAFVGROWINPLACE(pop1 - 1))      // hysteresis = 0.
        {
            JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
        }
        else
        {
            if ((PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, ignore);
            j__udyLFreeJV(PjvRaw, pop1, Pjpm);
            JL_JLB_PVALUE(Pjlb, subexp) = PjvnewRaw;
        }
        JU_JLB_BITMAP(Pjlb, subexp) ^= bitmask; // clear Indexs bit.
#endif  // JUDYL 
        return (1);
    }                                   // case.
// ****************************************************************************
#ifdef  JUDY1
// FULL POPULATION LEAF:
//
// Convert to a LeafB1 and delete the index.  Hysteresis = 0; none is possible.
//
// Note:  Earlier the second assertion below said, "== 2", but in fact the
// parent could be at a higher level if a fullpop is under a narrow pointer.
    case cJ1_JPFULLPOPU1:
    {
        Word_t    PjlbRaw;
        Pjlb_t    Pjlb;
        Word_t    subexp;
        assert(!ju_DcdNonMatchKey(Index, Pjp, 2));
        assert(ParentLevel > 1);        // see above.
        if ((PjlbRaw = j__udyAllocJLB1(Pjpm)) == 0)
            return (-1);
        Pjlb = P_JLB(PjlbRaw);
// Fully populate the leaf, then unset Indexs bit:
        for (subexp = 0; subexp < cJU_NUMSUBEXPL; ++subexp)
            JU_JLB_BITMAP(Pjlb, subexp) = cJU_FULLBITMAPL;
        JU_BITMAPCLEARL(Pjlb, Index);
//            Pjp->Jp_Addr0 = PjlbRaw;
        ju_SetBaLPntr(Pjp, PjlbRaw);
//            Pjp->jp_Type = cJU_JPLEAF_B1;
        ju_SetJpType(Pjp, cJU_JPLEAF_B1);
        return (1);
    }
// ****************************************************************************
#endif  // JUDY1    
// IMMEDIATE JP:
//
// If theres just the one Index in the Immed, convert the JP to a JPNULL*
// (should only happen in a BranchU); otherwise delete the Index from the
// Immed.  See the state transitions table elsewhere in this file for a summary
// of which Immed types must be handled.  Hysteresis = 0; none is possible with
// Immeds.
//
// MACROS FOR COMMON CODE:
//
// Single Index remains in cJU_JPIMMED_*_01; convert JP to null:
//
// Variables Pjp and parentJPtype are in the context.
//
// Note:  cJU_JPIMMED_*_01 should only be encountered in BranchUs, not in
// BranchLs or BranchBs (where its improper to merely modify the JP to be a
// null JP); that is, BranchL and BranchB code should have already handled
// any cJU_JPIMMED_*_01 by different means.
// Convert cJ*_JPIMMED_*_02 to cJU_JPIMMED_*_01:
//
// Move the undeleted Index, whichever does not match the least bytes of Index,
// from undecoded-bytes-only (in jp_1Index or jp_LIndex as appropriate) to
// jp_DcdPopO (full-field).  Pjp, Index, and offset are in the context.
// Variation for "odd" cJ*_JPIMMED_*_02 JP types, which are very different from
// "even" types because they use leaf search code and odd-copy macros:
//
// Note:  JudyL 32-bit has no "odd" JPIMMED_*_02 types.
// Core code for deleting one Index (and for JudyL, its value area) from a
// larger Immed:
//
// Variables Pleaf, pop1, and offset are in the context.
#ifdef JUDYL
// For JudyL the value area might need to be shrunk:
#endif  // JUDYL 
// Delete one Index from a larger Immed where no restructuring is required:
//
// Variables pop1, Pjp, offset, and Index are in the context.
// END OF MACROS, START OF CASES:
// Single Index remains in Immed; convert JP to null:
    case cJU_JPIMMED_1_01:
        assert(parentJPtype == (cJU_JPBRANCH_U2));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL1);
        return (1);
    case cJU_JPIMMED_2_01:
        assert(parentJPtype == (cJU_JPBRANCH_U3));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL2);
        return (1);
    case cJU_JPIMMED_3_01:
        assert(parentJPtype == (cJU_JPBRANCH_U4));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL3);
        return (1);
    case cJU_JPIMMED_4_01:
        assert(parentJPtype == (cJU_JPBRANCH_U5));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL4);
        return (1);
    case cJU_JPIMMED_5_01:
        assert(parentJPtype == (cJU_JPBRANCH_U6));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL5);
        return (1);
    case cJU_JPIMMED_6_01:
        assert(parentJPtype == (cJU_JPBRANCH_U7));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL6);
        return (1);
    case cJU_JPIMMED_7_01:
        assert(parentJPtype == (cJU_JPBRANCH_U));
        assert(ju_DcdPop0(Pjp) == JU_TRIMTODCDSIZE(Index));     /*  JU_JPSETADT(Pjp, 0, 0, NewJPType);  */
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL7);
        return (1);
// Multiple Indexes remain in the Immed JP; delete the specified Index:
    case cJU_JPIMMED_1_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (1));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
#endif  // JUDYL 
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = (Pleaf[0] == JU_LEASTBYTES(Index, 1));     /* undeleted Ind */
            assert(Pleaf[offset ? 0 : 1] == JU_LEASTBYTES(Index, 1));
            D_cdP0 = (Index & cJU_DCDMASK(1)) | Pleaf[offset];
            JUDYLCODE(A_ddr = Pjv[offset];);                      /*   JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }
#ifdef  JUDYL
        j__udyLFreeJV(PjvRaw, 2, Pjpm); /* Pjp->jp_Type = (NewJPType);   */
#endif  // JUDYL 
        ju_SetJpType(Pjp, cJU_JPIMMED_1_01);
        return (1);
    }
    case cJU_JPIMMED_1_03:
    case cJU_JPIMMED_1_04:
    case cJU_JPIMMED_1_05:
    case cJU_JPIMMED_1_06:
    case cJU_JPIMMED_1_07:
#ifdef  JUDY1
    case cJ1_JPIMMED_1_08:
    case cJ1_JPIMMED_1_09:
    case cJ1_JPIMMED_1_10:
    case cJ1_JPIMMED_1_11:
    case cJ1_JPIMMED_1_12:
    case cJ1_JPIMMED_1_13:
    case cJ1_JPIMMED_1_14:
    case cJ1_JPIMMED_1_15:
#endif  // JUDY1    
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (1));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
#endif  // JUDYL 
        pop1 = (ju_Type(Pjp)) - (cJU_JPIMMED_1_02) + 2;
        offset = j__udySearchLeaf1(Pleaf, pop1, Index, 1 * 8);
        assert(offset >= 0);            /* Index must be valid */
#ifdef  JUDY1
        JU_DELETEINPLACE(Pleaf, pop1, offset, 1);;      /*  --(Pjp->jp_Type);       */
#else   // JUDYL 
        if (JL_LEAFVGROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, pop1, offset, 1);
            JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
        }
        else
        {
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETEINPLACE(Pleaf, pop1, offset, 1);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, ignore);
            j__udyLFreeJV(PjvRaw, pop1, Pjpm);  /*    Pjp->jp_PValue = PjvnewRaw;     */
            ju_SetPjvPntr(Pjp, PjvnewRaw);
        }                              /*   --(Pjp->jp_Type);       */
#endif  // JUDYL 
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJU_JPIMMED_2_02:
    {
        uint16_t *Pleaf;
        assert((ParentLevel - 1) == (2));
        Pleaf = (uint16_t *) ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
#endif  // JUDYL 
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = (Pleaf[0] == JU_LEASTBYTES(Index, 2));     /* undeleted Ind */
            assert(Pleaf[offset ? 0 : 1] == JU_LEASTBYTES(Index, 2));
            D_cdP0 = (Index & cJU_DCDMASK(2)) | Pleaf[offset];
            JUDYLCODE(A_ddr = Pjv[offset];);                      /*   JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }                              /*  Pjp->jp_Type = (NewJPType);   */
#ifdef  JUDYL
        j__udyLFreeJV(PjvRaw, 2, Pjpm); /* Pjp->jp_Type = (NewJPType);   */
#endif  // JUDYL 
        ju_SetJpType(Pjp, cJU_JPIMMED_2_01);
        return (1);
    }
    case cJU_JPIMMED_2_03:
#ifdef  JUDY1
    case cJ1_JPIMMED_2_04:
    case cJ1_JPIMMED_2_05:
    case cJ1_JPIMMED_2_06:
    case cJ1_JPIMMED_2_07:
#endif  // JUDY1    
    {
        uint16_t *Pleaf;
        assert((ParentLevel - 1) == (2));
        Pleaf = (uint16_t *) ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
#endif  // JUDYL 
        pop1 = (ju_Type(Pjp)) - (cJU_JPIMMED_2_02) + 2;
        offset = j__udySearchLeaf2(Pleaf, pop1, Index, 2 * 8);
        assert(offset >= 0);            /* Index must be valid */
#ifdef  JUDY1
        JU_DELETEINPLACE(Pleaf, pop1, offset, 2);;      /*  --(Pjp->jp_Type);       */
#else   // JUDYL 
        if (JL_LEAFVGROWINPLACE(pop1 - 1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, pop1, offset, 2);
            JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
        }
        else
        {
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETEINPLACE(Pleaf, pop1, offset, 2);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, ignore);
            j__udyLFreeJV(PjvRaw, pop1, Pjpm);  /*    Pjp->jp_PValue = PjvnewRaw;     */
            ju_SetPjvPntr(Pjp, PjvnewRaw);
        }                              /*   --(Pjp->jp_Type);       */
#endif  // JUDYL 
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJU_JPIMMED_3_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (3));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
#endif  // JUDYL 
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = j__udySearchLeaf3(Pleaf, 2, Index, 3 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY3_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 3]));
            D_cdP0 |= Index & cJU_DCDMASK(3);
            JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                      /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }                              /* Pjp->jp_Type = (NewJPType);   */
#ifdef  JUDYL
        j__udyLFreeJV(PjvRaw, 2, Pjpm); /* Pjp->jp_Type = (NewJPType);   */
#endif  // JUDYL 
        ju_SetJpType(Pjp, cJU_JPIMMED_3_01);
        return (1);
    }
#ifdef  JUDY1
    case cJ1_JPIMMED_3_03:
    case cJ1_JPIMMED_3_04:
    case cJ1_JPIMMED_3_05:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (3));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
        pop1 = (ju_Type(Pjp)) - (cJU_JPIMMED_3_02) + 2;
        offset = j__udySearchLeaf3(Pleaf, pop1, Index, 3 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JU_DELETEINPLACE_ODD(Pleaf, pop1, offset, 3);;  /*  --(Pjp->jp_Type);       */
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJ1_JPIMMED_4_02:
    {
        uint32_t *Pleaf;
        assert((ParentLevel - 1) == (4));
        Pleaf = (uint32_t *)ju_PImmed1(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = (Pleaf[0] == JU_LEASTBYTES(Index, 4));     /* undeleted Ind */
            assert(Pleaf[offset ? 0 : 1] == JU_LEASTBYTES(Index, 4));
            D_cdP0 = (Index & cJU_DCDMASK(4)) | Pleaf[offset];
            JUDYLCODE(A_ddr = Pjv[offset];);                      /*   JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }                              /*  Pjp->jp_Type = (NewJPType);   */
        ju_SetJpType(Pjp, cJU_JPIMMED_4_01);
        return (1);
    }
    case cJ1_JPIMMED_4_03:
    {
        uint32_t *Pleaf;
        assert((ParentLevel - 1) == (4));
        Pleaf = (uint32_t *)ju_PImmed1(Pjp);
        pop1 = (ju_Type(Pjp)) - (cJ1_JPIMMED_4_02) + 2;
        offset = j__udySearchLeaf4(Pleaf, pop1, Index, 4 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JU_DELETEINPLACE(Pleaf, pop1, offset, 4);;      /*  --(Pjp->jp_Type);       */
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJ1_JPIMMED_5_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (5));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = j__udySearchLeaf5(Pleaf, 2, Index, 5 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY5_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 5]));
            D_cdP0 |= Index & cJU_DCDMASK(5);
            JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                      /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }                              /* Pjp->jp_Type = (NewJPType);   */
        ju_SetJpType(Pjp, cJU_JPIMMED_5_01);
        return (1);
    }
    case cJ1_JPIMMED_5_03:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (5));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
        pop1 = (ju_Type(Pjp)) - (cJ1_JPIMMED_5_02) + 2;
        offset = j__udySearchLeaf5(Pleaf, pop1, Index, 5 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JU_DELETEINPLACE_ODD(Pleaf, pop1, offset, 5);;  /*  --(Pjp->jp_Type);       */
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJ1_JPIMMED_6_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (6));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = j__udySearchLeaf6(Pleaf, 2, Index, 6 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY6_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 6]));
            D_cdP0 |= Index & cJU_DCDMASK(6);
            JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                      /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }                              /* Pjp->jp_Type = (NewJPType);   */
        ju_SetJpType(Pjp, cJU_JPIMMED_6_01);
        return (1);
    }
    case cJ1_JPIMMED_7_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (7));
        Pleaf = (uint8_t *) ju_PImmed1(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            uint8_t   T_ype = ju_Type(Pjp);
            offset = j__udySearchLeaf7(Pleaf, 2, Index, 7 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY7_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 7]));
            D_cdP0 |= Index & cJU_DCDMASK(7);
            JUDYLCODE(A_ddr = Pjv[offset ? 0 : 1];);                      /*       JU_JPSETADT(Pjp, A_ddr, D_cdP0, T_ype);       */
            ju_SetBaLPntr(Pjp, A_ddr);
            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, T_ype);
        }                              /* Pjp->jp_Type = (NewJPType);   */
        ju_SetJpType(Pjp, cJU_JPIMMED_7_01);
        return (1);
    }
#endif  // JUDY1    
// ****************************************************************************
// INVALID JP TYPE:
    default:
        JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
        return (-1);
    }                                   // switch
// PROCESS JP -- RECURSIVELY:
//
// For non-Immed JP types, if successful, post-decrement the population count
// at this level, or collapse a BranchL if necessary by copying the remaining
// JP in the BranchL to the parent (hysteresis = 0), which implicitly creates a
// narrow pointer if there was not already one in the hierarchy.
    assert(level);
    retcode = j__udyDelWalk(Pjp, Index, level, Pjpm);
    assert(retcode != 0);               // should never happen.
    if ((ju_Type(Pjp)) < cJU_JPIMMED_1_01)      // not an Immed.
    {
        switch (retcode)
        {
        case 1:
        {
//                jp_t JP = *Pjp;
            Word_t    DcdP0;
            DcdP0 = ju_DcdPop0(Pjp) - 1;        // decrement count.
//                JU_JPSETADT(Pjp, JP.Jp_Addr0, DcdP0, ju_Type(&JP)); 
            ju_SetDcdPop0(Pjp, DcdP0);
            break;
        }
        case 2:                        // collapse BranchL to single JP; see above:
        {
            Word_t    PjblRaw = ju_BaLPntr(Pjp);
            Pjbl_t    Pjbl = P_JBL(PjblRaw);
            *Pjp = Pjbl->jbl_jp[0];
            j__udyFreeJBL(PjblRaw, Pjpm);
            retcode = 1;
        }
        }
    }
    return (retcode);
}                                       // j__udyDelWalk()

// ****************************************************************************
// J U D Y   1   U N S E T
// J U D Y   L   D E L
//
// Main entry point.  See the manual entry for details.
#ifdef  JUDY1
FUNCTION int Judy1Unset(PPvoid_t PPArray,       // in which to delete.
                        Word_t Index,           // to delete.
                        PJError_t PJError)      // optional, for returning error info.
#else   // JUDYL 
FUNCTION int JudyLDel(PPvoid_t PPArray,         // in which to delete.
                      Word_t Index,             // to delete.
                      PJError_t PJError)        // optional, for returning error info.
#endif  // JUDYL 
{
    Word_t    pop1;                     // population of leaf.
    int       offset;                   // at which to delete Index.
#ifdef  JUDY1
    int       retcode;                  // return code from Judy1Test().
#else   // JUDYL 
    PPvoid_t  PPvalue;                  // pointer from JudyLGet().
#endif  // JUDYL 
// CHECK FOR NULL ARRAY POINTER (error by caller):
    if (PPArray == (PPvoid_t) NULL)
    {
        JU_SET_ERRNO(PJError, JU_ERRNO_NULLPPARRAY);
        return (JERRI);
    }
// CHECK IF INDEX IS INVALID:
//
// If so, theres nothing to do.  This saves a lot of time.  Pass through
// PJError, if any, from the "get" function.
#ifdef  JUDY1
    if ((retcode = Judy1Test(*PPArray, Index, PJError)) == JERRI)
#else   // JUDYL 
    if ((PPvalue = JudyLGet(*PPArray, Index, PJError)) == PPJERR)
#endif  // JUDYL 
        return (JERRI);
#ifdef  JUDY1
    if (retcode == 0)
#else   // JUDYL 
    if (PPvalue == (PPvoid_t) NULL)
#endif  // JUDYL 
        return (0);
// ****************************************************************************
// PROCESS TOP LEVEL (LEAFW) BRANCHES AND LEAVES:
// ****************************************************************************
// LEAFW LEAF, OTHER SIZE:
//
// Shrink or convert the leaf as necessary.  Hysteresis = 0; none is possible.
    if (JU_LEAFW_POP0(*PPArray) < cJU_LEAFW_MAXPOP1)    // must be a LEAFW
    {
#ifdef JUDYL
        Pjv_t     Pjv;                  // current value area.
        Pjv_t     Pjvnew;               // value area in new leaf.
#endif  // JUDYL 
        Pjllw_t    Pjllw = P_JLLW(*PPArray);       // first word of leaf.
        Pjllw_t    Pjllwnew;              // replacement leaf.
//        pop1 = Pjllw[0] + 1;            // first word of leaf is pop0.
        pop1 = Pjllw->jlw_Population0 + 1; // first word of leaf is pop0.
// Delete single (last) Index from array:
        if (pop1 == 1)
        {
            j__udyFreeJLLW(Pjllw, /* pop1 = */ 1, (Pjpm_t) NULL);
            *PPArray = (Pvoid_t)NULL;
            return (1);
        }
// Locate Index in compressible leaf:
//        offset = j__udySearchLeafW(Pjllw + 1, pop1, Index);
        offset = j__udySearchLeafW(Pjllw->jlw_Leaf, pop1, Index);
        assert(offset >= 0);            // Index must be valid.
#ifdef JUDYL
        Pjv = JL_LEAFWVALUEAREA(Pjllw, pop1);
#endif  // JUDYL 
// Delete Index in-place:
//
// Note:  "Grow in place from pop1 - 1" is the logical inverse of, "shrink in
// place from pop1."  Also, Pjllw points to the count word, so skip that for
// doing the deletion.
        if (JU_LEAFWGROWINPLACE(pop1 - 1))
        {
//            JU_DELETEINPLACE(Pjllw + 1, pop1, offset, ignore);
                JU_DELETEINPLACE(Pjllw->jlw_Leaf, pop1, offset, ignore);
#ifdef JUDYL
//              also delete from value area:
            JU_DELETEINPLACE(Pjv, pop1, offset, ignore);
#endif  // JUDYL 
//            --(Pjllw[0]);                // decrement population.
            --(Pjllw->jlw_Population0);  // decrement population.
            return (1);
        }
// Allocate new leaf for use in either case below:
        Pjllwnew = j__udyAllocJLLW(pop1 - 1);
        JU_CHECKALLOC(Pjllw_t, Pjllwnew, JERRI);
// Shrink to smaller LEAFW:
//
// Note:  Skip the first word = pop0 in each leaf.
//        Pjllwnew[0] = (pop1 - 1) - 1;
        Pjllwnew->jlw_Population0 = (pop1 - 1) - 1;
//        JU_DELETECOPY(Pjllwnew + 1, Pjllw + 1, pop1, offset, ignore);
        JU_DELETECOPY(Pjllwnew->jlw_Leaf, Pjllw->jlw_Leaf, pop1, offset, ignore);
#ifdef JUDYL
        Pjvnew = JL_LEAFWVALUEAREA(Pjllwnew, pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, pop1, offset, ignore);
#endif  // JUDYL 
        j__udyFreeJLLW(Pjllw, pop1, (Pjpm_t) NULL);
////        *PPArray = (Pvoid_t)  Pjllwnew | cJU_LEAFW);
        *PPArray = (Pvoid_t)Pjllwnew;
        return (1);
    }
    else
// ****************************************************************************
// JRP BRANCH:
//
// Traverse through the JPM to do the deletion unless the population is small
// enough to convert immediately to a LEAFW.
    {
        Pjpm_t    Pjpm;
        Pjp_t     Pjp;                  // top-level JP to process.
        Word_t    digit;                // in a branch.
#ifdef JUDYL
        Pjv_t     Pjv;                  // to value area.
#endif  // JUDYL 
        Pjllw_t    Pjllwnew;              // replacement leaf.
        Pjpm = P_JPM(*PPArray);         // top object in array (tree).
        Pjp = &(Pjpm->jpm_JP);          // next object (first branch or leaf).
        assert(((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_L)
               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_B) || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_U));

#ifdef TRACEJPD
        Pjpm = P_JPM(*PPArray);
        j__udyIndex = Index;
        j__udyPopulation = Pjpm->jpm_Pop0;
#endif

// WALK THE TREE 
//
// Note:  Recursive code in j__udyDelWalk() knows how to collapse a lower-level
// BranchL containing a single JP into the parent JP as a narrow pointer, but
// the code here cant do that for a top-level BranchL.  The result can be
// PArray -> JPM -> BranchL containing a single JP.  This situation is
// unavoidable because a JPM cannot contain a narrow pointer; the BranchL is
// required in order to hold the top digit decoded, and it does not collapse to
// a LEAFW until the population is low enough.
//
// TBD:  Should we add a topdigit field to JPMs so they can hold narrow
// pointers?
        if (j__udyDelWalk(Pjp, Index, cJU_ROOTSTATE, Pjpm) == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
            return (JERRI);
        }
        --(Pjpm->jpm_Pop0);             // success; decrement total population.
        if ((Pjpm->jpm_Pop0 + 1) != cJU_LEAFW_MAXPOP1)
        {
            return (1);
        }
// COMPRESS A BRANCH[LBU] TO A LEAFW:
//
        Pjllwnew = j__udyAllocJLLW(cJU_LEAFW_MAXPOP1);
        PWord_t   PLeafWnew = Pjllwnew->jlw_Leaf;
        JU_CHECKALLOC(Pjllw_t, Pjllwnew, JERRI);
// Plug leaf into root pointer and set population count:
////        *PPArray  = (Pvoid_t) ((Word_t) Pjllwnew | cJU_LEAFW);
        *PPArray = (Pvoid_t)Pjllwnew;
#ifdef JUDYL
        Pjv = JL_LEAFWVALUEAREA(Pjllwnew, cJU_LEAFW_MAXPOP1);
#endif  // JUDYL 
//        *Pjllwnew++ = cJU_LEAFW_MAXPOP1 - 1;     // set pop0.
        Pjllwnew->jlw_Population0 = cJU_LEAFW_MAXPOP1 - 1;       // set pop0.
        switch (ju_Type(Pjp))
        {
// JPBRANCH_L:  Copy each JPs indexes to the new LEAFW and free the old
// branch:
        case cJU_JPBRANCH_L:
        {
            Word_t    PjblRaw = ju_BaLPntr(Pjp);
            Pjbl_t    Pjbl = P_JBL(PjblRaw);
            for (offset = 0; offset < Pjbl->jbl_NumJPs; ++offset)
            {
#ifdef  JUDY1
                pop1 = j__udyLeaf7ToLeafW(PLeafWnew,      /* null. */
                        (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], cJU_BYTESPERWORD), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf7ToLeafW(PLeafWnew, Pjv,
                        (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], cJU_BYTESPERWORD), Pjpm);
#endif  // JUDYL 
//                Pjllwnew += pop1;        // advance through indexes.
                PLeafWnew += pop1;         // advance through indexes.
#ifdef JUDYL
                Pjv += pop1;            // advance through values.
#endif  // JUDYL 
            }
            j__udyFreeJBL(PjblRaw, Pjpm);
            break;                      // delete Index from new LEAFW.
        }
// JPBRANCH_B:  Copy each JPs indexes to the new LEAFW and free the old
// branch, including each JP subarray:
        case cJU_JPBRANCH_B:
        {
            Word_t    PjbbRaw = ju_BaLPntr(Pjp);
            Pjbb_t    Pjbb = P_JBB(PjbbRaw);
            Word_t    subexp;           // current subexpanse number.
            BITMAPB_t bitmap;           // portion for this subexpanse.
            Word_t    Pjp2Raw;          // one subexpanses subarray.
            Pjp_t     Pjp2;
            for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
            {
                if ((bitmap = JU_JBB_BITMAP(Pjbb, subexp)) == 0)
                    continue;           // skip empty subexpanse.
                digit = subexp * cJU_BITSPERSUBEXPB;
                Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
                Pjp2 = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);
// Walk through bits for all possible sub-subexpanses (digits); increment
// offset for each populated subexpanse; until no more set bits:
                for (offset = 0; bitmap != 0; bitmap >>= 1, ++digit)
                {
                    if (!(bitmap & 1))  // skip empty sub-subexpanse.
                        continue;
#ifdef  JUDY1
                    pop1 = j__udyLeaf7ToLeafW(PLeafWnew,  /* null. */
                            Pjp2 + offset, JU_DIGITTOSTATE(digit, cJU_BYTESPERWORD), Pjpm);
#else   // JUDYL 
                    pop1 = j__udyLeaf7ToLeafW(PLeafWnew, Pjv,
                            Pjp2 + offset, JU_DIGITTOSTATE(digit, cJU_BYTESPERWORD), Pjpm);
#endif  // JUDYL 
                    PLeafWnew += pop1;    // advance through indexes.
#ifdef JUDYL
                    Pjv += pop1;        // advance through values.
#endif  // JUDYL 
                    ++offset;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            j__udyFreeJBB(PjbbRaw, Pjpm);
            break;                      // delete Index from new LEAFW.
        }                               // case cJU_JPBRANCH_B.
// JPBRANCH_U:  Copy each JPs indexes to the new LEAFW and free the old
// branch:
        case cJU_JPBRANCH_U:
        {
            Word_t    PjbuRaw = ju_BaLPntr(Pjp);
            Pjbu_t    Pjbu = P_JBU(PjbuRaw);
            Word_t    ldigit;           // larger than uint8_t.
            for (Pjp = Pjbu->jbu_jp, ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++Pjp, ++ldigit)
            {
// Shortcuts, to save a little time for possibly big branches:
                if ((ju_Type(Pjp)) == cJU_JPNULLMAX)    // skip null JP.
                    continue;
// TBD:  Should the following shortcut also be used in BranchL and BranchB
// code?
                if ((ju_Type(Pjp)) == cJU_JPIMMED_7_01)
                {                       // single Immed:
                    *PLeafWnew++ = JU_DIGITTOSTATE(ldigit, cJU_BYTESPERWORD) | ju_DcdPop0(Pjp);   // rebuild Index.
#ifdef JUDYL
                    *Pjv++ = ju_PImmVals(Pjp);  // copy value area.
#endif  // JUDYL 
                    continue;
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf7ToLeafW(PLeafWnew,      /* null. */
                                          Pjp, JU_DIGITTOSTATE(ldigit, cJU_BYTESPERWORD), Pjpm);
#else   // JUDYL 
                pop1 = j__udyLeaf7ToLeafW(PLeafWnew, Pjv, Pjp, JU_DIGITTOSTATE(ldigit, cJU_BYTESPERWORD), Pjpm);
#endif  // JUDYL 
                PLeafWnew += pop1;        // advance through indexes.
#ifdef JUDYL
                Pjv += pop1;            // advance through values.
#endif  // JUDYL 
            }
            j__udyFreeJBU(PjbuRaw, Pjpm);
            break;                      // delete Index from new LEAFW.
        }                               // case cJU_JPBRANCH_U.
// INVALID JP TYPE in jpm_t struct
        default:
            JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
            return (JERRI);
        }                               // end switch on sub-JP type.
// FREE JPM (no longer needed):
        j__udyFreeJPM(Pjpm, (Pjpm_t) NULL);
        return (1);
    }
 /*NOTREACHED*/}                        // Judy1Unset() / JudyLDel()
