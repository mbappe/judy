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
#else  // JUDYL
#include "JudyL.h"
#endif // JUDYL

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
extern int j__udy1BranchBToBranchL(Pjp_t Pjp, Pjpm_t Pjpm);
extern int j__udy1LeafB1ToLeaf1(Pjp_t, Pjpm_t);
extern Word_t j__udy1Leaf1orB1ToLeaf2(uint16_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf2ToLeaf3(uint8_t  *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf3ToLeaf4(uint32_t *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf4ToLeaf5(uint8_t  *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf5ToLeaf6(uint8_t  *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf6ToLeaf7(uint8_t  *, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udy1Leaf7ToLeaf8(PWord_t, Pjp_t, Word_t, Pjpm_t);
#else  // JUDYL
extern int j__udyLBranchBToBranchL(Pjp_t Pjp, Pjpm_t Pjpm);
extern int j__udyLLeafB1ToLeaf1(Pjp_t, Pjpm_t);
extern Word_t j__udyLLeaf1orB1ToLeaf2(uint16_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf2ToLeaf3(uint8_t  *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf3ToLeaf4(uint32_t *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf4ToLeaf5(uint8_t  *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf5ToLeaf6(uint8_t  *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf6ToLeaf7(uint8_t  *, Pjv_t, Pjp_t, Word_t, Pjpm_t);
extern Word_t j__udyLLeaf7ToLeaf8(PWord_t, Pjv_t, Pjp_t, Word_t, Pjpm_t);
#endif // JUDYL

// The process is reversable by subtracting 1
#ifdef  JUDY1
#define	JU_LEAF1DELINPLACE(POP1)	J1_LEAF1GROWINPLACE((POP1) - 1)
#define	JU_LEAF2DELINPLACE(POP1)	J1_LEAF2GROWINPLACE((POP1) - 1)
#define	JU_LEAF3DELINPLACE(POP1)	J1_LEAF3GROWINPLACE((POP1) - 1)
#define	JU_LEAF4DELINPLACE(POP1)	J1_LEAF4GROWINPLACE((POP1) - 1)
#define	JU_LEAF5DELINPLACE(POP1)	J1_LEAF5GROWINPLACE((POP1) - 1)
#define	JU_LEAF6DELINPLACE(POP1)	J1_LEAF6GROWINPLACE((POP1) - 1)
#define	JU_LEAF7DELINPLACE(POP1)	J1_LEAF7GROWINPLACE((POP1) - 1)
#define	JU_LEAF8DELINPLACE(POP1)	J1_LEAF8GROWINPLACE((POP1) - 1)
#else   // JUDYL
#define	JU_LEAF1DELINPLACE(POP1)	JL_LEAF1GROWINPLACE((POP1) - 1)
#define	JU_LEAF2DELINPLACE(POP1)	JL_LEAF2GROWINPLACE((POP1) - 1)
#define	JU_LEAF3DELINPLACE(POP1)	JL_LEAF3GROWINPLACE((POP1) - 1)
#define	JU_LEAF4DELINPLACE(POP1)	JL_LEAF4GROWINPLACE((POP1) - 1)
#define	JU_LEAF5DELINPLACE(POP1)	JL_LEAF5GROWINPLACE((POP1) - 1)
#define	JU_LEAF6DELINPLACE(POP1)	JL_LEAF6GROWINPLACE((POP1) - 1)
#define	JU_LEAF7DELINPLACE(POP1)	JL_LEAF7GROWINPLACE((POP1) - 1)
#define	JU_LEAF8DELINPLACE(POP1)	JL_LEAF8GROWINPLACE((POP1) - 1)
#define JL_LEAFVDELINPLACE(POP1)        JL_LEAFVGROWINPLACE((POP1) - 1)
#endif  // JUDYL

#define JU_BRANCHBJPDELINPLACE(numJPs)  JU_BRANCHBJPGROWINPLACE((numJPs) - 1)

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
#ifdef LATER
DBGCODE(static uint8_t parentJPtype;);                                  // parent branch JP type.
FUNCTION static int
j__udyDelWalk(Pjp_t Pjp,                // current JP under which to delete.
              Word_t Index,             // to delete.
              Word_t ParentLevel,       // of parent branch.
              Pjpm_t Pjpm)              // for returning info to top level.
{
    Word_t    Pop1;                     // of a leaf.
    Word_t    level;                    // of a leaf.
    uint8_t   digit;                    // from Index, in current branch.
    int       offset;                   // within a branch.
    int       retcode;                  // return code: -1, 0, 1, 2.
#ifdef JUDYL
    Word_t    PjvRaw;                   // value area.
    Pjv_t     Pjv;
#endif // JUDYL
    DBGCODE(level = 0;);
  ContinueDelWalk:                     // for modifying state without recursing.

#ifdef TRACEJPD
    JudyPrintJP(Pjp, "d", __LINE__, Index, Pjpm);
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
// Next is a label for branch-type-specific common code.  Variables Pop1,
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
// BranchLs are the same size.)  Variables Pjp, Pjpm, Pleaf, digit, and Pop1
// are in the context.
//        ju_SetDcdPop0(PjpJP, DcdP0);
//        ju_SetLeafPop1(PjpJP, Pop1);
//        ju_SetJpType(PjpJP, cJU_JPLEAF2);
//        ju_SetPntrInJp(PjpJP, PjllRaw);
//        ju_SetIMM01(Pjp, Value, Key, Type)
//        ju_SetPntrInJp(Pjp, PjvPntr)
// Overall common code for initial BranchL deletion handling:
//
// Assert that Index is in the branch, then see if the BranchL should be kept
// or else compressed to a leaf.  Variables Index, Pjp, and Pop1 are in the
// context.
// END OF MACROS, START OF CASES:
    case cJU_JPBRANCH_L2:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        Pop1 = ju_BranchPop0(Pjp, 2) + 1;
        if (Pop1 > (cJU_LEAF2_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((2) >= 2);
            level = (2);
            digit = JU_DIGITATSTATE(Index, 2);
            goto BranchLKeep;
        }
        assert(Pop1 == (cJU_LEAF2_MAXPOP1));
        {
            Word_t PjllnewRaw2 = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm);
            if (PjllnewRaw2 == 0)
                return (-1);
            Pjll2_t Pjll2 = P_JLL2(PjllnewRaw2);
            uint16_t *Pleaf2 = Pjll2->jl2_Leaf;
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf2, cJU_LEAF2_MAXPOP1););
            Word_t PjblRaw = ju_PntrInJp(Pjp);
            Pjbl_t Pjbl = P_JBL(PjblRaw);
            int    numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                Word_t pop1;
                if (ju_Type(Pjbl->jbl_jp + offset) == cJU_JPIMMED_1_01)
                {
                    *Pleaf2++ = ju_IMM01Key(Pjbl->jbl_jp + offset);
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjbl->jbl_jp + offset););
                    continue;           /* for-loop */
                }
//printf("Line = %d\n", __LINE__);
#ifdef  JUDY1
                pop1 = j__udyLeaf1orB1ToLeaf2(Pleaf2,      Pjbl->jbl_jp + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 2), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf1orB1ToLeaf2(Pleaf2, Pjv, Pjbl->jbl_jp + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 2), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf2 += pop1;
            }
            assert(((((Word_t)Pleaf2) - ((Word_t)Pjllnew)) / (2)) == (cJU_LEAF2_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF2VALUEAREA(Pjllnew, cJU_LEAF2_MAXPOP1)) ==
                       (cJU_LEAF2_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm); /* Pjp->jp_Type = (NewJPType); */

            ju_SetJpType(Pjp, cJU_JPLEAF2);
            ju_SetLeafPop1(Pjp, cJU_LEAF2_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw2);

assert(ju_LeafPop1(Pjp) == Pop1);

            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_L3:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        Pop1 = ju_BranchPop0(Pjp, 3) + 1;
        if (Pop1 > (cJU_LEAF3_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((3) >= 2);
            level = (3);
            digit = JU_DIGITATSTATE(Index, 3);
            goto BranchLKeep;
        }
        assert(Pop1 == (cJU_LEAF3_MAXPOP1));
        {
            uint8_t  *Pleaf3;
            Pjbl_t    Pjbl;
            int       numJPs;
            Word_t PjllnewRaw3 = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm);
            if (PjllnewRaw3 == 0)
                return (-1);
            Pjll3_t Pjll3   = P_JLL3(PjllnewRaw3);
            uint8_t *Pleaf3 = Pjll3 = Pjll3_jl3_Leaf;
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pjll3, cJU_LEAF3_MAXPOP1););
            Word_t PjblRaw3 = ju_PntrInJp(Pjp);
            Pjbl3 = P_JBL(PjblRaw3);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                Word_t pop1;
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_02)
                {
                    JU_COPY3_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key((Pjbl->jbl_jp) + offset)));
                    Pleaf += (3);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf2ToLeaf3(Pleaf,      (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 3), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf2ToLeaf3(Pleaf, Pjv, (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 3), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (3) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (3)) == (cJU_LEAF3_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF3VALUEAREA(Pjllnew, cJU_LEAF3_MAXPOP1)) ==
                       (cJU_LEAF3_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm); /* Pjp->jp_Type = (NewJPType); */

            ju_SetJpType(Pjp, cJU_JPLEAF3);
            ju_SetLeafPop1(Pjp, cJU_LEAF3_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw3);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_L4:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        Pop1 = ju_BranchPop0(Pjp, 4) + 1;
        if (Pop1 > (cJU_LEAF4_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((4) >= 2);
            level = (4);
            digit = JU_DIGITATSTATE(Index, 4);
            goto BranchLKeep;
        }
        assert(Pop1 == (cJU_LEAF4_MAXPOP1));
        {
            uint32_t *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            PjllnewRaw4 = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm);
            if (PjllnewRaw4 == 0)
                return (-1);
            Pjll4_t Pjll4 = P_JLL4(PjllnewRaw4);
            Pleaf = (uint32_t *)Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, cJU_LEAF4_MAXPOP1););
            PjblRaw = ju_PntrInJp(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                Word_t pop1;
                if (ju_Type(Pjbl->jbl_jp + offset) == cJU_JPIMMED_1_03)
                {
                    *Pleaf++ = ju_IMM01Key(Pjbl->jbl_jp + offset);
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjbl->jbl_jp + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf3ToLeaf4(Pleaf,      (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 4), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf3ToLeaf4(Pleaf, Pjv, (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 4), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (4)) == (cJU_LEAF4_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF4VALUEAREA(Pjllnew, cJU_LEAF4_MAXPOP1)) ==
                       (cJU_LEAF4_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm); /* Pjp->jp_Type = (NewJPType); */

            ju_SetJpType(Pjp, cJU_JPLEAF4);
            ju_SetLeafPop1(Pjp, cJU_LEAF4_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw4);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_L5:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        Pop1 = ju_BranchPop0(Pjp, 5) + 1;
        if (Pop1 > (cJU_LEAF5_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((5) >= 2);
            level = (5);
            digit = JU_DIGITATSTATE(Index, 5);
            goto BranchLKeep;
        }
        assert(Pop1 == (cJU_LEAF5_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            PjllnewRaw5 = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm);
            if (PjllnewRaw5 == 0)
                return (-1);
            Pjll5_t Pjll5 = P_JLL5(PjllnewRaw5);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pjll5, cJU_LEAF5_MAXPOP1););
            PjblRaw = ju_PntrInJp(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                Word_t pop1;
                if (ju_Type(Pjbl->jbl_jp + offset) == cJU_JPIMMED_1_04)
                {
                    JU_COPY5_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key(Pjbl->jbl_jp + offset)));
                    Pleaf += (5);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf4ToLeaf5(Pleaf,      (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 5), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf4ToLeaf5(Pleaf, Pjv, (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 5), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (5) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (5)) == (cJU_LEAF5_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF5VALUEAREA(Pjllnew, cJU_LEAF5_MAXPOP1)) ==
                       (cJU_LEAF5_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, (int)numJPs, Pjpm); /* Pjp->jp_Type = (NewJPType); */

            ju_SetJpType(Pjp, cJU_JPLEAF5);
            ju_SetLeafPop1(Pjp, cJU_LEAF5_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw5);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_L6:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        Pop1 = ju_BranchPop0(Pjp, 6) + 1;
        if (Pop1 > (cJU_LEAF6_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((6) >= 2);
            level = (6);
            digit = JU_DIGITATSTATE(Index, 6);
            goto BranchLKeep;
        }
        assert(Pop1 == (cJU_LEAF6_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            PjllnewRaw6 = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm);
            if (PjllnewRaw6 == 0)
                return (-1);
            Pjll6_t Pjll6 = P_JLL6(PjllnewRaw6);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pjll6, cJU_LEAF6_MAXPOP1););
            PjblRaw = ju_PntrInJp(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                Word_t pop1;
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_05)
                {
                    JU_COPY6_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key(Pjbl->jbl_jp + offset)));
                    Pleaf += (6);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf5ToLeaf6(Pleaf,      (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 6), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf5ToLeaf6(Pleaf, Pjv, (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 6), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (6) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (6)) == (cJU_LEAF6_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF6VALUEAREA(Pjllnew, cJU_LEAF6_MAXPOP1)) ==
                       (cJU_LEAF6_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, (int)numJPs, Pjpm); /* Pjp->jp_Type = (NewJPType); */

            ju_SetJpType(Pjp, cJU_JPLEAF6);
            ju_SetLeafPop1(Pjp, cJU_LEAF6_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw6);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_L7:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        Pop1 = ju_BranchPop0(Pjp, 7) + 1;
        if (Pop1 > (cJU_LEAF7_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((7) >= 2);
            level = (7);
            digit = JU_DIGITATSTATE(Index, 7);
            goto BranchLKeep;
        }
        assert(Pop1 == (cJU_LEAF7_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjblRaw;
            Pjbl_t    Pjbl;
            int       numJPs;
            PjllnewRaw7 = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm);
            if (PjllnewRaw7 == 0)
                return (-1);
            Pjll7_t Pjll7 = P_JLL7(PjllnewRaw7);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pjll7, cJU_LEAF7_MAXPOP1););
            PjblRaw = ju_PntrInJp(Pjp);
            Pjbl = P_JBL(PjblRaw);
            numJPs = Pjbl->jbl_NumJPs;
            for (offset = 0; offset < numJPs; ++offset)
            {
                Word_t pop1;
                if (ju_Type((Pjbl->jbl_jp) + offset) == cJU_JPIMMED_1_01 + (7) - 2)
                {
                    Word_t pop1;
                    JU_COPY7_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key((Pjbl->jbl_jp) + offset)));
                    Pleaf += (7);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01((Pjbl->jbl_jp) + offset););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf6ToLeaf7(Pleaf,      (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 7), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf6ToLeaf7(Pleaf, Pjv, (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], 7), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (7) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (7)) == (cJU_LEAF7_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF7VALUEAREA(Pjllnew, cJU_LEAF7_MAXPOP1)) ==
                       (cJU_LEAF7_MAXPOP1)););
            j__udyFreeJBL(PjblRaw, Pjpm); /* Pjp->jp_Type = (NewJPType); */

            ju_SetJpType(Pjp, cJU_JPLEAF7);
            ju_SetLeafPop1(Pjp, cJU_LEAF7_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw7);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
// A top-level BranchL is different and cannot use JU_BRANCHL():  Dont try to
// compress to a (LEAF8) leaf yet, but leave this for a later deletion
// (hysteresis > 0); and the next JP type depends on the system word size; so
// dont use JU_BRANCH_KEEP():
    case cJU_JPBRANCH_L8:
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
        Pjbl = P_JBL(ju_PntrInJp(Pjp));
        numJPs = Pjbl->jbl_NumJPs;
        assert(numJPs > 0);
        DBGCODE(parentJPtype = ju_Type(Pjp););

//      Search for a match to the digit (valid Index => must find digit):
        offset = j__udySearchBranchL(Pjbl->jbl_Expanse, Pjbl->jbl_NumJPs, digit);
        assert(offset >= 0);

//        for (offset = 0; (Pjbl->jbl_Expanse[offset]) != digit; ++offset)
//            assert(offset < numJPs - 1);

        Pjp = Pjbl->jbl_jp + offset;
// If not at a (deletable) JPIMMED_*_01, continue the walk (to descend through
// the BranchL):
        assert(level >= 2);
        if ((ju_Type(Pjp)) != cJU_JPIMMED_1_01 + level - 2)
            break;
// At JPIMMED_*_01:  Ensure the index is in the right expanse, then delete the
// Immed from the BranchL:
//
// Note:  A BranchL has a fixed size and format regardless of numJPs.
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        JU_DELETEINPLACE(Pjbl->jbl_Expanse, numJPs, offset);
        JU_DELETEINPLACE(Pjbl->jbl_jp, numJPs, offset);
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
// the new leaf.  Variables Pjp, Pjpm, Pleaf, digit, and Pop1 are in the
// context.
//
// Note:  Its no accident that the interface to JU_BRANCHB_COMPRESS() is
// identical to JU_BRANCHL_COMPRESS().  Only the details differ in how to
// traverse the branchs JPs.
// Overall common code for initial BranchB deletion handling:
//
// Assert that Index is in the branch, then see if the BranchB should be kept
// or else compressed to a leaf.  Variables Index, Pjp, and Pop1 are in the
// context.
// END OF MACROS, START OF CASES:
//
// Note:  Its no accident that the macro calls for these cases is nearly
// identical to the code for BranchLs.
//
//  Replace Branch_Bx with Leafx, if the BranchBx falls below max population of Leafx
    case cJU_JPBRANCH_B2:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        Pop1 = ju_BranchPop0(Pjp, 2) + 1;


        if (Pop1 > cJU_LEAF2_MAXPOP1) /* hysteresis = 1 */
        {
            level = (2);
            digit = JU_DIGITATSTATE(Index, 2);
            goto BranchBKeep;
        }
//      BranchB2 to Leaf2
        assert(Pop1 == cJU_LEAF2_MAXPOP1);
        {
//          Pop of Branch_B2 is low enough to put into a Leaf2
            PjllnewRaw2 = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm);
            if (PjllnewRaw2 == 0)
                return (-1);
            Pjll2_t Pjllnew2  = P_JLL2(PjllnewRaw2);
            uint16_t *PLeaf2 = (uint16_t *) Pjllnew;
            JUDYLCODE(Pjv   = JL_LEAF2VALUEAREA(Pjll2, cJU_LEAF2_MAXPOP1););
            Word_t PjbbRaw  = ju_PntrInJp(Pjp);
            Pjbb_t Pjbb     = P_JBB(PjbbRaw);

//printf("\n=====1=====Branch_B2 cJU_NUMSUBEXPB = %d\n", (int)cJU_NUMSUBEXPB);
            for (int subexp = 0; subexp < cJU_NUMSUBEXPB; subexp++)
            {
                BITMAPB_t bitmap;       // portion for this subexpanse

                bitmap = JU_JBB_BITMAP(Pjbb, subexp);

//printf("\n=====2=====Branch_B2 subpop1[%d] bitmap = 0x%lx\n", subexp, bitmap);

                if (bitmap == 0)
                    continue;           /* empty subexpanse */

                Word_t subPop1 = j__udyCountBitsB(bitmap);
                assert(subPop1);

                digit           = subexp * cJU_BITSPERSUBEXPB;
                Word_t Pjp2Raw  = JU_JBB_PJP(Pjbb, subexp);
                Pjp_t  Pjp2     = P_JP(Pjp2Raw);
                assert(Pjp2 != (Pjp_t) NULL);

                for (offset = 0; bitmap != 0; bitmap >>= 1, digit++)
                {
                    if ((bitmap & 1) == 0)
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
//                  Probably many IMMEDs, so take a shortcut
                    if (ju_Type(Pjp2 + offset - 1) == 0) assert(0);
//#ifdef diag
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01)
                    {

 //printf("Immed Key = 0x%016lx\n", ju_IMM01Key(Pjp2 + offset - 1));

                        *PLeaf2++ = ju_IMM01Key(Pjp2 + offset - 1);
                        JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
//#endif  // diag

//printf("Line = %d\n", __LINE__);
#ifdef  JUDY1
                    Word_t pop1 = j__udyLeaf1orB1ToLeaf2(PLeaf2,      Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 2), Pjpm);
#else  // JUDYL
                    Word_t pop1 = j__udyLeaf1orB1ToLeaf2(PLeaf2, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 2), Pjpm);
                    Pjv += pop1;
#endif // JUDYL
                    PLeaf2 += pop1;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert((PLeaf2 - (uint16_t *)Pjllnew)  == cJU_LEAF2_MAXPOP1);
            JUDYLCODE(assert ((Pjv - JL_LEAF2VALUEAREA(Pjllnew, cJU_LEAF2_MAXPOP1)) == cJU_LEAF2_MAXPOP1););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF2);
            ju_SetLeafPop1(Pjp, cJU_LEAF2_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw2);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_B3:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        Pop1 = ju_BranchPop0(Pjp, 3) + 1;
        if (Pop1 > (cJU_LEAF3_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (3);
            digit = JU_DIGITATSTATE(Index, 3);
            goto BranchBKeep;
        }
//      BranchB3 to Leaf3
        assert(Pop1 == (cJU_LEAF3_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            PjllnewRaw3 = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm);
            if (PjllnewRaw3 == 0)
                return (-1);
            Pjll3_t Pjll3 = P_JLL3(PjllnewRaw3);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pjll3, cJU_LEAF3_MAXPOP1););
            PjbbRaw = ju_PntrInJp(Pjp);
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
                    Word_t pop1;
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == 0) assert(0);
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (3) - 2)
                    {
//                        JU_COPY3_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_DcdPop0(Pjp2 + offset - 1)));
                        JU_COPY3_LONG_TO_PINDEX(Pleaf, ju_IMM01Key(Pjp2 + offset - 1));
                        JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2 + offset - 1););
                        Pleaf += (3);   /* index size = level */
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf2ToLeaf3(Pleaf,      Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 3), Pjpm);
#else  // JUDYL
                    pop1 = j__udyLeaf2ToLeaf3(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 3), Pjpm);
                    Pjv += pop1;
#endif // JUDYL
                    Pleaf += (3) * pop1;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (3)) == (cJU_LEAF3_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF3VALUEAREA(Pjllnew, cJU_LEAF3_MAXPOP1)) ==
                       (cJU_LEAF3_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF3);
            ju_SetLeafPop1(Pjp, cJU_LEAF3_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw3);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_B4:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        Pop1 = ju_BranchPop0(Pjp, 4) + 1;
        if (Pop1 > (cJU_LEAF4_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((4) >= 2);
            level = (4);
            digit = JU_DIGITATSTATE(Index, 4);
            goto BranchBKeep;
        }
//      BranchB4 to Leaf4
        assert(Pop1 == (cJU_LEAF4_MAXPOP1));
        {
            uint32_t *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            PjllnewRaw4 = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm);
            if (PjllnewRaw4 == 0)
                return (-1);
            Pjll4_t Pjll4 = P_JLL4(PjllnewRawx);
            Pleaf = (uint32_t *)Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pjll4, cJU_LEAF4_MAXPOP1););
            PjbbRaw = ju_PntrInJp(Pjp);
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
                    Word_t pop1;
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == 0) assert(0);
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (4) - 2)
                    {
                        *Pleaf++ = ju_IMM01Key(Pjp2 + offset - 1);
                        JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf3ToLeaf4(Pleaf,      Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 4), Pjpm);
#else  // JUDYL
                    pop1 = j__udyLeaf3ToLeaf4(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 4), Pjpm);
                    Pjv += pop1;
#endif // JUDYL
                    Pleaf += pop1;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (4)) == (cJU_LEAF4_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF4VALUEAREA(Pjllnew, cJU_LEAF4_MAXPOP1)) ==
                       (cJU_LEAF4_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF4);
            ju_SetLeafPop1(Pjp, cJU_LEAF4_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw4);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_B5:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        Pop1 = ju_BranchPop0(Pjp, 5) + 1;
        if (Pop1 > (cJU_LEAF5_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((5) >= 2);
            level = (5);
            digit = JU_DIGITATSTATE(Index, 5);
            goto BranchBKeep;
        }
//      BranchB5 to Leaf5
        assert(Pop1 == (cJU_LEAF5_MAXPOP1));
        {
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            PjllnewRaw5 = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm);
            if (PjllnewRaw5 == 0)
                return (-1);
            Pjll5_t Pjll5 = P_JLL5(PjllnewRaw5);
            uint8_t *Pleaf5 = Pjll5->jl5_Leaf;
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, cJU_LEAF5_MAXPOP1););
            PjbbRaw = ju_PntrInJp(Pjp);
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
                    Word_t pop1;
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == 0) assert(0);
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_04)
                    {
                        JU_COPY5_LONG_TO_PINDEX(Pleaf, ju_IMM01Key(Pjp2 + offset - 1));
                        Pleaf += (5);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf4ToLeaf5(Pleaf,      Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 5), Pjpm);
#else  // JUDYL
                    pop1 = j__udyLeaf4ToLeaf5(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 5), Pjpm);
                    Pjv += pop1;
#endif // JUDYL
                    Pleaf += (5) * pop1;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (5)) == (cJU_LEAF5_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF5VALUEAREA(Pjllnew, cJU_LEAF5_MAXPOP1)) ==
                       (cJU_LEAF5_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF5);
            ju_SetLeafPop1(Pjp, cJU_LEAF5_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw5);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_B6:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        Pop1 = ju_BranchPop0(Pjp, 6) + 1;
        if (Pop1 > (cJU_LEAF6_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((6) >= 2);
            level = (6);
            digit = JU_DIGITATSTATE(Index, 6);
            goto BranchBKeep;
        }
//      BranchB6 to Leaf6
        assert(Pop1 == (cJU_LEAF6_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            PjllnewRaw6 = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm);
            if (PjllnewRaw6 == 0)
                return (-1);
            Pjll6_t Pjll6 = P_JLL6(PjllnewRaw6);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, cJU_LEAF6_MAXPOP1););
            PjbbRaw = ju_PntrInJp(Pjp);
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
                    Word_t pop1;
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == 0) assert(0);
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (6) - 2)
                    {
                        JU_COPY6_LONG_TO_PINDEX(Pleaf, ju_IMM01Key(Pjp2 + offset - 1));
                        Pleaf += (6);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf5ToLeaf6(Pleaf,      Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 6), Pjpm);
#else  // JUDYL
                    pop1 = j__udyLeaf5ToLeaf6(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 6), Pjpm);
                    Pjv += pop1;
#endif // JUDYL
                    Pleaf += (6) * pop1;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (6)) == (cJU_LEAF6_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF6VALUEAREA(Pjllnew, cJU_LEAF6_MAXPOP1)) ==
                       (cJU_LEAF6_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF6);
            ju_SetLeafPop1(Pjp, cJU_LEAF6_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw6);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_B7:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        Pop1 = ju_BranchPop0(Pjp, 7) + 1;
        if (Pop1 > (cJU_LEAF7_MAXPOP1)) /* hysteresis = 1 */
        {
            assert((7) >= 2);
            level = (7);
            digit = JU_DIGITATSTATE(Index, 7);
            goto BranchBKeep;
        }
//      BranchB7 to Leaf7
        assert(Pop1 == (cJU_LEAF7_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbbRaw;          /* BranchB to compress */
            Pjbb_t    Pjbb;
            Word_t    subexp;           /* current subexpanse number    */
            BITMAPB_t bitmap;           /* portion for this subexpanse  */
            Word_t    Pjp2Raw;          /* one subexpanses subarray     */
            Pjp_t     Pjp2;
            PjllnewRaw7 = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm);
            if (PjllnewRaw7 == 0)
                return (-1);
            Pjll7_t Pjll7 = P_JLL7(PjllnewRaw7);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, cJU_LEAF7_MAXPOP1););
            PjbbRaw = ju_PntrInJp(Pjp);
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
                    Word_t pop1;
                    if (!(bitmap & 1))
                        continue;       /* empty sub-subexpanse */
                    ++offset;           /* before any continue */
                    if (ju_Type(Pjp2 + offset - 1) == 0) assert(0);
                    if (ju_Type(Pjp2 + offset - 1) == cJU_JPIMMED_1_01 + (7) - 2)
                    {
                        JU_COPY7_LONG_TO_PINDEX(Pleaf, ju_IMM01Key(Pjp2 + offset - 1));
                        Pleaf += (7);   /* index size = level */
                        JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2 + offset - 1););
                        continue;       /* for-loop */
                    }
#ifdef  JUDY1
                    pop1 = j__udyLeaf6ToLeaf7(Pleaf,      Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 7), Pjpm);
#else  // JUDYL
                    pop1 = j__udyLeaf6ToLeaf7(Pleaf, Pjv, Pjp2 + offset - 1, JU_DIGITTOSTATE(digit, 7), Pjpm);
                    Pjv += pop1;
#endif // JUDYL
                    Pleaf += (7) * pop1;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (7)) == (cJU_LEAF7_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF7VALUEAREA(Pjllnew, cJU_LEAF7_MAXPOP1)) == (cJU_LEAF7_MAXPOP1)););
            j__udyFreeJBB(PjbbRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF7);
            ju_SetLeafPop1(Pjp, cJU_LEAF7_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw7);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
// A top-level BranchB is different and cannot use JU_BRANCHB():  Dont try to
// compress to a (LEAF8) leaf yet, but leave this for a later deletion
// (hysteresis > 0); and the next JP type depends on the system word size; so
// dont use JU_BRANCH_KEEP():
    case cJU_JPBRANCH_B8:
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
        Pjbb = P_JBB(ju_PntrInJp(Pjp));
        subexp = digit / cJU_BITSPERSUBEXPB;
        bitmap = JU_JBB_BITMAP(Pjbb, subexp);
        bitmask = JU_BITPOSMASKB(digit);
        assert(bitmap & bitmask);       // Index valid => digits bit is set.
        DBGCODE(parentJPtype = ju_Type(Pjp););
// Compute digits offset into the bitmap, with a fast method if all bits are
// set:
//        offset = j__udyCountBitsB(bitmap & JU_MASKLOWEREXC(bitmask));
          offset = ((bitmap == (cJU_FULLBITMAPB)) ?

                      digit % cJU_BITSPERSUBEXPB :
                      j__udyCountBitsB(bitmap & JU_MASKLOWEREXC(bitmask)));
        Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
        Pjp2 = P_JP(Pjp2Raw);
        assert(Pjp2 != (Pjp_t) NULL);   // valid subexpanse pointer.
// If not at a (deletable) JPIMMED_*_01, continue the walk (to descend through
// the BranchB):
        if (ju_Type(Pjp2 + offset) == 0) assert(0);
        if (ju_Type(Pjp2 + offset) != cJU_JPIMMED_1_01 + level - 2)
        {
            Pjp = Pjp2 + offset;
            break;
        }
// At JPIMMED_*_01:  Ensure the index is in the right expanse, then delete the
// Immed from the BranchB:
        assert(ju_IMM01Key(Pjp2 + offset) == JU_TrimToIMM01(Index));
// If only one index is left in the subexpanse, free the JP array:
        if ((numJPs = j__udyCountBitsB(bitmap)) == 1)
        {
            j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ 1, Pjpm);
            JU_JBB_PJP(Pjbb, subexp) = 0;
        }
// Shrink JP array in-place:
        else if (JU_BRANCHBJPDELINPLACE(numJPs))
        {
            assert(numJPs > 0);
            JU_DELETEINPLACE(Pjp2, numJPs, offset);
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
            JU_DELETECOPY(Pjpnew, Pjp2, numJPs, offset);
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
// digit, and Pop1 are in the context.
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
// else compressed to a leaf.  Variables level, Index, Pjp, and Pop1 are in the
// context.
//
// Note:  BranchU handling differs from BranchL and BranchB as described above.
// END OF MACROS, START OF CASES:
//
// Note:  Its no accident that the macro calls for these cases is nearly
// identical to the code for BranchLs, with the addition of cJU_JPNULL*
// parameters only needed for BranchUs.
    case cJU_JPBRANCH_U2:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        Pop1 = ju_BranchPop0(Pjp, 2) + 1;
        if (Pop1 > (cJU_LEAF2_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (2);                
            Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 2);
            break;                      /* descend to next level */
        }
        assert(Pop1 == (cJU_LEAF2_MAXPOP1));
        {
            uint16_t *Pleaf;
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjp_t     Pjp2 = P_JBU(ju_PntrInJp(Pjp))->jbu_jp;   // 1st Pjp_t
            Word_t    ldigit;           /* larger than uint8_t */
            PjllnewRaw2 = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm);
            if (PjllnewRaw2 == 0)
                return (-1);
            Pjll2_t Pjll2 = P_JLL2(PjllnewRaw2);
            Pleaf = (uint16_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf, cJU_LEAF2_MAXPOP1););

            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                if (ju_Type(Pjp2) == cJU_JPNULL1)
                    continue;

                if (ju_Type(Pjp2) == cJU_JPIMMED_1_01)
                {
                    *Pleaf++ = ju_IMM01Key(Pjp2);
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                Word_t pop1 = j__udyLeaf1orB1ToLeaf2(Pleaf,      Pjp2, JU_DIGITTOSTATE(ldigit, 2), Pjpm);
#else  // JUDYL
                Word_t pop1 = j__udyLeaf1orB1ToLeaf2(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 2), Pjpm);
                Pjv        += pop1;
#endif // JUDYL
                Pleaf      += pop1;
            }

//printf("\n=======After j__udyLeaf1orB1ToLeaf2+Pop inc %d, pOP1 = %d, cJU_LEAF2_MAXPOP1 = %d\n", POP1, pOP1, (int)cJU_LEAF2_MAXPOP1);

            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (2)) == (cJU_LEAF2_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF2VALUEAREA(Pjllnew, cJU_LEAF2_MAXPOP1)) ==
                       (cJU_LEAF2_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF2);
            ju_SetLeafPop1(Pjp, cJU_LEAF2_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw2);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_U3:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        Pop1 = ju_BranchPop0(Pjp, 3) + 1;
        if (Pop1 > (cJU_LEAF3_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (3);
            Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 3);
            break;                      /* descend to next level */
        }
        assert(Pop1 == (cJU_LEAF3_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjp_t     Pjp2 = P_JBU(ju_PntrInJp(Pjp))->jbu_jp;
            Word_t    ldigit;           /* larger than uint8_t */
            PjllnewRaw3 = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm);
            if (PjllnewRaw3 == 0)
                return (-1);
            Pjll3_t Pjll3 = P_JLL3(PjllnewRaw3);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pleaf, cJU_LEAF3_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                Word_t pop1;
                if (ju_Type(Pjp2) == (cJU_JPNULL2))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_02)
                {
                    JU_COPY3_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key(Pjp2)));
                    Pleaf += (3);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf2ToLeaf3(Pleaf,      Pjp2, JU_DIGITTOSTATE(ldigit, 3), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf2ToLeaf3(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 3), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (3) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (3)) == (cJU_LEAF3_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF3VALUEAREA(Pjllnew, cJU_LEAF3_MAXPOP1)) ==
                       (cJU_LEAF3_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF3);
            ju_SetLeafPop1(Pjp, cJU_LEAF3_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw3);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_U4:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        Pop1 = ju_BranchPop0(Pjp, 4) + 1;
        if (Pop1 > (cJU_LEAF4_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (4);
            Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 4);
            break;                      /* descend to next level */
        }
        assert(Pop1 == (cJU_LEAF4_MAXPOP1));
        {
            uint32_t *Pleaf;
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjp_t     Pjp2 = P_JBU(ju_PntrInJp(Pjp))->jbu_jp;
            Word_t    ldigit;           /* larger than uint8_t */
            PjllnewRaw4 = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm);
            if (PjllnewRaw4 == 0)
                return (-1);
            Pjll4_t Pjll4 = P_JLL4(PjllnewRaw4);
            Pleaf = (uint32_t *)Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, cJU_LEAF4_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                Word_t pop1;
                if (ju_Type(Pjp2) == (cJU_JPNULL3))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_03)
                {
                    *Pleaf++ = ju_IMM01Key(Pjp2);
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf3ToLeaf4(Pleaf,      Pjp2, JU_DIGITTOSTATE(ldigit, 4), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf3ToLeaf4(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 4), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (4)) == (cJU_LEAF4_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF4VALUEAREA(Pjllnew, cJU_LEAF4_MAXPOP1)) ==
                       (cJU_LEAF4_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF4);
            ju_SetLeafPop1(Pjp, cJU_LEAF4_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw4);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_U5:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        Pop1 = ju_BranchPop0(Pjp, 5) + 1;
        if (Pop1 > (cJU_LEAF5_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (5);
            Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 5);
            break;                      /* descend to next level */
        }
        assert(Pop1 == (cJU_LEAF5_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjp_t     Pjp2 = P_JBU(ju_PntrInJp(Pjp))->jbu_jp;
            Word_t    ldigit;           /* larger than uint8_t */
            PjllnewRaw5 = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm);
            if (PjllnewRaw5 == 0)
                return (-1);
            Pjll5_t Pjll5 = P_JLL5(PjllnewRaw5);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, cJU_LEAF5_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                Word_t pop1;
                if (ju_Type(Pjp2) == (cJU_JPNULL4))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_04)
                {
                    JU_COPY5_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key(Pjp2)));
                    Pleaf += (5);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf4ToLeaf5(Pleaf,      Pjp2, JU_DIGITTOSTATE(ldigit, 5), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf4ToLeaf5(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 5), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (5) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (5)) == (cJU_LEAF5_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF5VALUEAREA(Pjllnew, cJU_LEAF5_MAXPOP1)) ==
                       (cJU_LEAF5_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF5);
            ju_SetLeafPop1(Pjp, cJU_LEAF5_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw5);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_U6:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        Pop1 = ju_BranchPop0(Pjp, 6) + 1;
        if (Pop1 > (cJU_LEAF6_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (6);
            Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 6);
            break;                      /* descend to next level */
        }
        assert(Pop1 == (cJU_LEAF6_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjp_t     Pjp2 = P_JBU(ju_PntrInJp(Pjp))->jbu_jp;
            Word_t    ldigit;           /* larger than uint8_t */
            PjllnewRaw6 = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm);
            if (PjllnewRaw6 == 0)
                return (-1);
            Pjll6_t Pjll6 = P_JLL6(PjllnewRaw6);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, cJU_LEAF6_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                Word_t pop1;
                if (ju_Type(Pjp2) == (cJU_JPNULL5))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_05)
                {
                    JU_COPY6_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key(Pjp2)));
                    Pleaf += (6);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf5ToLeaf6(Pleaf,      Pjp2, JU_DIGITTOSTATE(ldigit, 6), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf5ToLeaf6(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 6), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (6) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (6)) == (cJU_LEAF6_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF6VALUEAREA(Pjllnew, cJU_LEAF6_MAXPOP1)) ==
                       (cJU_LEAF6_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF6);
            ju_SetLeafPop1(Pjp, cJU_LEAF6_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw6);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
    case cJU_JPBRANCH_U7:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        DBGCODE(parentJPtype = ju_Type(Pjp););
        Pop1 = ju_BranchPop0(Pjp, 7) + 1;
        if (Pop1 > (cJU_LEAF7_MAXPOP1)) /* hysteresis = 1 */
        {
            level = (7);
            Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
            Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, 7);
            break;                      /* descend to next level */
        }
        assert(Pop1 == (cJU_LEAF7_MAXPOP1));
        {
            uint8_t  *Pleaf;
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjp_t     Pjp2 = P_JBU(ju_PntrInJp(Pjp))->jbu_jp;
            Word_t    ldigit;           /* larger than uint8_t */
            PjllnewRaw7 = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm);
            if (PjllnewRaw7 == 0)
                return (-1);
            Pjll7_t Pjll7 = P_JLL7(PjllnewRaw7);
            Pleaf = (uint8_t *) Pjllnew;
            JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, cJU_LEAF7_MAXPOP1););
            for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++ldigit, ++Pjp2)
            {                           /* fast-process common types: */
                Word_t pop1;
                if (ju_Type(Pjp2) == (cJU_JPNULL6))
                    continue;
                if (ju_Type(Pjp2) == cJU_JPIMMED_1_06)
                {
                    JU_COPY7_LONG_TO_PINDEX(Pleaf, (Word_t)(ju_IMM01Key(Pjp2)));
                    Pleaf += (7);       /* index size = level */
                    JUDYLCODE(*Pjv++ = ju_ImmVal_01(Pjp2););
                    continue;           /* for-loop */
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf6ToLeaf7(Pleaf,      Pjp2, JU_DIGITTOSTATE(ldigit, 7), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf6ToLeaf7(Pleaf, Pjv, Pjp2, JU_DIGITTOSTATE(ldigit, 7), Pjpm);
                Pjv += pop1;
#endif // JUDYL
                Pleaf += (7) * pop1;
            }
            assert(((((Word_t)Pleaf) - ((Word_t)Pjllnew)) / (7)) == (cJU_LEAF7_MAXPOP1));
            JUDYLCODE(assert ((Pjv - JL_LEAF7VALUEAREA(Pjllnew, cJU_LEAF7_MAXPOP1)) ==
                       (cJU_LEAF7_MAXPOP1)););
            j__udyFreeJBU(PjbuRaw, Pjpm);       /* Pjp->jp_Type = (NewJPType);       */

            ju_SetJpType(Pjp, cJU_JPLEAF7);
            ju_SetLeafPop1(Pjp, cJU_LEAF7_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw7);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
    }
// A top-level BranchU is different and cannot use JU_BRANCHU():  Dont try to
// compress to a (LEAF8) leaf yet, but leave this for a later deletion
// (hysteresis > 0); just descend through the BranchU:
    case cJU_JPBRANCH_U8:
    {
        DBGCODE(parentJPtype = ju_Type(Pjp););
        level = cJU_ROOTSTATE;
//            Pjp   = P_JP(ju_PntrInJp(Pjp)) + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(Pjp));
        Pjp = Pjbu->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        break;
    }
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
// Variables ParentLevel, Pop1, PjllnewRaw, Pjllnew, Pjpm, and Index are in the
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
// subfield, but since a leaf maxPop1 is known to be <= 1 byte in size, the new
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
// Note:  Theres no way here to go from Leaf3 [Leaf7] to LEAF8 on a 32-bit
// [64-bit] system.  Thats handled in the main code, because its different in
// that a JPM is involved.
// Compress a Leaf* with Pop1 = 2, or a JPIMMED_*_02, into a JPIMMED_*_01:
//
// Copy whichever Index is NOT being deleted (and assert that the other one is
// found; Index must be valid).  This requires special handling of the Index
// bytes (and value area).  Variables Pjp, Index, offset, and Pleaf are in the
// context, offset is modified to the undeleted Index, and Pjp is modified
// including Jp_Addr0.
// Compress a Leaf* into a JPIMMED_*_0[2+]:
//
// This occurs as soon as its possible, with hysteresis = 0.  Variables Pop1,
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
// This variant compresses a Leaf* with Pop1 = 2 into a JPIMMED_*_01:
#endif // JUDYL
// See comments above about these:
//
// Note:  Here "23" means index size 2 or 3, and "47" means 4..7.
// Compress a Leaf* in place:
//
// Here hysteresis = 0 (no memory is wasted).  Variables Pop1, Pleaf, and
// offset, and for JudyL, Pjv, are in the context.
// Compress a Leaf* into a smaller memory object of the same JP type:
//
// Variables PjllnewRaw, Pjllnew, PleafPop1, Pjpm, PleafRaw, Pleaf, and offset
// are in the context.
// Overall common code for Leaf* deletion handling:
//
// See if the leaf can be:
// - (de)compressed to one a level higher (JU_LEAF_UPLEVEL()), or if not,
// - compressed to an Immediate JP (JU_LEAF_TOIMMED()), or if not,
// - shrunk in place (JU_LEAF_INPLACE()), or if none of those, then
// - shrink the leaf to a smaller chunk of memory (JU_LEAF_SHRINK()).
//
// Variables Pjp, Pop1, Index, and offset are in the context.
// The *Up parameters refer to a leaf one level up, if there is any.
// END OF MACROS, START OF CASES:
//
// (*) Leaf1 [[ => 1_15..08 ] => 1_07 => ... => 1_04 ] => 1_03 => 1_02 => 1_01
    case cJU_JPLEAF1:
    {
        assert(!ju_DcdNotMatchKey(Index, Pjp, 1));
        assert(ParentLevel > (1));
        Pop1 = ju_LeafPop1(Pjp);
        assert(((ParentLevel - 1) == (1)) || (Pop1 == (cJU_LEAF2_MAXPOP1)));
        if (((ParentLevel - 1) > (1)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF2_MAXPOP1)))
        { /* hysteresis = 1       */
            Word_t    D_cdP0;

            Word_t  Pjll2newRaw = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm);
            if (Pjll2newRaw  == 0)
                return (-1);
            Pjll_t  Pjll2new = P_JLL2(Pjll2newRaw2);
//printf("Line = %d\n", __LINE__);
#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA((uint16_t *) Pjll2new, cJU_LEAF2_MAXPOP1);
            (void)j__udyLeaf1orB1ToLeaf2((uint16_t *) Pjll2new, Pjv, Pjp, Index & cJU_DCDMASK(1), Pjpm);
#else  // JUDY1
            (void)j__udyLeaf1orB1ToLeaf2((uint16_t *) Pjll2new,      Pjp, Index & cJU_DCDMASK(1), Pjpm);
#endif // JUDY1
            D_cdP0 = (~cJU_MASKATSTATE((1) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF2);
            ju_SetLeafPop1(Pjp, cJU_LEAF2_MAXPOP1);
            ju_SetPntrInJp(Pjp, Pjll2newRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
//      else Not under a narrow pointer, can make an IMMED1?
        Word_t  Pjll1Raw = ju_PntrInJp(Pjp);
        Pjll1_t Pjll1    = P_JLL1(Pjll1Raw);

        offset = j__udySearchLeaf1(Pjll1, Pop1, Index, 1 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF1VALUEAREA(Pjll1, Pop1););
        assert(Pop1 > cJU_IMMED1_MAXPOP1);
        if ((Pop1 - 1) == cJU_IMMED1_MAXPOP1) 
        { /* hysteresis = 0 */
#ifdef  JUDYL
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(Pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif // JUDYL
            JU_DELETECOPY(ju_PImmed1(Pjp), Pjll1->jl1_Leaf, Pop1, offset);
            j__udyFreeJLL1(Pjll1Raw, Pop1, Pjpm);
#ifdef  JUDYL
            ju_SetPntrInJp(Pjp, PjvnewRaw);
#endif // JUDYL
            /* Pjp->jp_Type = (BaseJPType) - 2 + (MaxPop1); */
            ju_SetJpType(Pjp, cJU_JPIMMED_1_02 - 2 + cJU_IMMED1_MAXPOP1);
            return (1);
        }
//      else  No, therefore just shrink the Leaf1
        if (JU_LEAF1DELINPLACE(Pop1))
        { /* hysteresis = 0 */
            JU_DELETEINPLACE(Pjll1->jl1_Leaf, Pop1, offset);

// This is the place to update preamble

            JU_PADLEAF1(Pjll1->jl1_Leaf, Pop1 - 1);
#ifdef  JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif // JUDYL

#ifdef PCAS
//            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1

// entry Pjp and Pop1
UpdateLeafPop1AndReturnTrue:    // requires Pjp and Pop1 
            ju_SetLeafPop1(Pjp, Pop1 - 1);
#ifdef PCAS
            printf("ju_LeafPop1(Pjp) = 0x%lx, ju_DcdPop0(Pjp) = 0x%016lx\n", ju_LeafPop1(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

            return (1);
        }
        else                            // Shrink the Leaf1 size into another Alloc
        {
            Word_t Pjll1newRaw = j__udyAllocJLL1(Pop1 - 1, Pjpm);
            if (Pjll1newRaw == 0)
                return (-1);
            Pjll1_t Pjll1new = P_JLL1(Pjll1newRaw);
#ifdef  JUDYL
            Pjv_t Pjvnew = JL_LEAF1VALUEAREA(Pjll1new, Pop1 - 1);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);        // and Value area
#endif // JUDYL
            JU_DELETECOPY(Pjll1new->jl1_Leaf, Pjll1->jl1_Leaf, Pop1, offset);

// This is the place to update preamble

            JU_PADLEAF1(Pjll1new->jl1_Leaf, Pop1 - 1);
            j__udyFreeJLL1(Pjll1Raw, Pop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll1newRaw);
#ifdef PCAS
//            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
    }
    case cJU_JPLEAF2:
    {
        Word_t    PleafRaw;
        uint16_t *Pleaf;
        assert(!ju_DcdNotMatchKey(Index, Pjp, 2));
        assert(ParentLevel > (2));
        PleafRaw2 = ju_PntrInJp(Pjp);
        Pleaf = (uint16_t *) P_JLL2(PleafRaw2);
        Pop1 = ju_LeafPop1(Pjp);
        assert(((ParentLevel - 1) == (2)) || (Pop1 >= (cJU_LEAF3_MAXPOP1)));
        if (((ParentLevel - 1) > (2)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF3_MAXPOP1)))
        { /* hysteresis = 1       */
            Word_t    D_cdP0;
            PjllnewRaw3 = j__udyAllocJLL3(cJU_LEAF3_MAXPOP1, Pjpm);
            if (PjllnewRaw3 == 0)
                return (-1);
            Pjll3_t Pjll3 = P_JLL3(PjllnewRaw3);
#ifdef  JUDY1
            (void)j__udyLeaf2ToLeaf3((uint8_t *) Pjllnew,      Pjp, Index & cJU_DCDMASK(2), Pjpm);
#else  // JUDYL
            Pjv = JL_LEAF3VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF3_MAXPOP1);
            (void)j__udyLeaf2ToLeaf3((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(2), Pjpm);
#endif // JUDYL
            D_cdP0 = (~cJU_MASKATSTATE((2) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetLeafPop1(Pjp, cJU_LEAF3_MAXPOP1);
            ju_SetJpType(Pjp, cJU_JPLEAF3);
            ju_SetPntrInJp(Pjp, PjllnewRaw2);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf2(Pleaf, Pop1, Index, 2 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF2VALUEAREA(Pleaf, Pop1););
        assert(Pop1 > (cJU_IMMED2_MAXPOP1));
        if ((Pop1 - 1) == (cJU_IMMED2_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_PntrInJp(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY(ju_PImmed2(Pjp), Pleaf, Pop1, offset);
            ju_SetJpType(Pjp, (cJU_JPIMMED_2_02) - 1 + (cJU_IMMED2_MAXPOP1) - 1);
            j__udyFreeJLL2(PjllRaw, Pop1, Pjpm);
#endif  // JUDY1

#ifdef  JUDYL
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(Pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETECOPY(ju_PImmed2(Pjp), Pleaf, Pop1, offset);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
            j__udyFreeJLL2(PjllRaw, Pop1, Pjpm);

            ju_SetJpType(Pjp, (cJU_JPIMMED_2_02) - 2 + (cJU_IMMED2_MAXPOP1));
            ju_SetPntrInJp(Pjp, PjvnewRaw);
#endif // JUDYL
            return (1);
        }
        if (JU_LEAF2DELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, Pop1, offset);
            JU_PADLEAF2(Pleaf, Pop1 - 1);
#ifdef JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
        {
            Pjv_t Pjvnew;
            PjllnewRaw2 = j__udyAllocJLL2(Pop1 - 1, Pjpm);
            if (PjllnewRaw2 == 0)
                return (-1);
            Pjll2_t Pjll2 = P_JLL2(PjllnewRaw2);
            Pjvnew = JL_LEAF2VALUEAREA(Pjllnew, Pop1 - 1);
            JU_DELETECOPY((uint16_t *) Pjllnew, Pleaf, Pop1, offset);
            JU_PADLEAF2(Pjllnew, Pop1 - 1);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
            j__udyFreeJLL2(PleafRaw, Pop1, Pjpm);

            ju_SetPntrInJp(Pjp, PjllnewRaw2);
#endif // JUDYL
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
#ifdef  JUDY1
        PjllnewRaw2 = j__udyAllocJLL2(Pop1 - 1, Pjpm);
        if (PjllnewRaw2 == 0)
            return (-1);
        Pjllnew = P_JLL2(PjllnewRaw2);
        JU_DELETECOPY((uint16_t *) Pjllnew, Pleaf, Pop1, offset);
        JU_PADLEAF2(Pjllnew, Pop1 - 1);
        j__udyFreeJLL2(PleafRaw, Pop1, Pjpm);

        ju_SetPntrInJp(Pjp, PjllnewRaw);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
#endif // JUDY1
    }

    case cJU_JPLEAF3:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNotMatchKey(Index, Pjp, 3));
        assert(ParentLevel > (3));
        PleafRaw3 = ju_PntrInJp(Pjp);
        Pleaf = (uint8_t *) P_JLL3(PleafRaw3);
        Pop1 = ju_LeafPop1(Pjp);
        assert(((ParentLevel - 1) == (3)) || (Pop1 >= (cJU_LEAF4_MAXPOP1)));
        if (((ParentLevel - 1) > (3)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF4_MAXPOP1)))     
        { /* hysteresis = 1       */
            Word_t    D_cdP0;
            PjllnewRaw4 = j__udyAllocJLL4(cJU_LEAF4_MAXPOP1, Pjpm);
            if (PjllnewRaw4 == 0)
                return (-1);
            Pjll4_t Pjll4 = P_JLL4(PjllnewRaw4);
#ifdef  JUDY1
            (void)j__udyLeaf3ToLeaf4((uint32_t *)Pjllnew,      Pjp, Index & cJU_DCDMASK(3), Pjpm);
#else  // JUDYL
            Pjv = JL_LEAF4VALUEAREA((uint32_t *)Pjllnew, cJU_LEAF4_MAXPOP1);
            (void)j__udyLeaf3ToLeaf4((uint32_t *)Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(3), Pjpm);
#endif // JUDYL
            D_cdP0 = (~cJU_MASKATSTATE((3) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF4);
            ju_SetLeafPop1(Pjp, cJU_LEAF4_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf3(Pleaf, Pop1, Index, 3 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF3VALUEAREA(Pleaf, Pop1););
        assert(Pop1 > (cJU_IMMED3_MAXPOP1));
        if ((Pop1 - 1) == (cJU_IMMED3_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_PntrInJp(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD(ju_PImmed3(Pjp), Pleaf, Pop1, offset, 3);
            ju_SetJpType(Pjp, (cJU_JPIMMED_3_02) - 1 + (cJU_IMMED3_MAXPOP1) - 1);
            j__udyFreeJLL3(PjllRaw, Pop1, Pjpm);
#else  // JUDYL
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(Pop1 - 1, Pjpm)) == 0)
                return (-1);
            JUDYLCODE(Pjvnew = P_JV(PjvnewRaw););
            JU_DELETECOPY_ODD(ju_PImmed3(Pjp), Pleaf, Pop1, offset, 3);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
            j__udyFreeJLL3(PjllRaw, Pop1, Pjpm);

            ju_SetJpType(Pjp, (cJU_JPIMMED_3_02) - 2 + (cJU_IMMED3_MAXPOP1));
            ju_SetPntrInJp(Pjp, PjvnewRaw);
#endif // JUDYL
            return (1);
        }
//      else  still bigger than an IMMED
        if (JU_LEAF3DELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, Pop1, offset, 3);
#ifdef JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif  // JUDYL
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
//      else    must allocate less memory
        PjllnewRaw3 = j__udyAllocJLL3(Pop1 - 1, Pjpm);
        if (PjllnewRaw3 == 0)
            return (-1);
        Pjllnew3 = P_JLL3(PjllnewRaw3);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, Pop1, offset, 3);
#ifdef  JUDYL
        Pjv_t Pjvnew = JL_LEAF3VALUEAREA(Pjllnew, Pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif  // JUDYL
        j__udyFreeJLL3(PleafRaw, Pop1, Pjpm);

        ju_SetPntrInJp(Pjp, PjllnewRaw);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
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
        uint32_t *Pleaf;
        assert(!ju_DcdNotMatchKey(Index, Pjp, 4));
        assert(ParentLevel > (4));
        PleafRaw4 = ju_PntrInJp(Pjp);
        Pleaf = (uint32_t *)P_JLL4(PleafRaw4);
        Pop1 = ju_LeafPop1(Pjp);
        assert(((ParentLevel - 1) == (4)) || (Pop1 >= (cJU_LEAF5_MAXPOP1)));
        if (((ParentLevel - 1) > (4)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF5_MAXPOP1)))     
        { /* hysteresis = 1       */
            Word_t    D_cdP0;
            PjllnewRaw5 = j__udyAllocJLL5(cJU_LEAF5_MAXPOP1, Pjpm);
            if (PjllnewRaw5 == 0)
                return (-1);
            Pjll5_t Pjll5 = P_JLL5(PjllnewRaw5);
#ifdef  JUDY1
            (void)j__udyLeaf4ToLeaf5((uint8_t *) Pjllnew,      Pjp, Index & cJU_DCDMASK(4), Pjpm);
#else  // JUDYL
            Pjv = JL_LEAF5VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF5_MAXPOP1);
            (void)j__udyLeaf4ToLeaf5((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(4), Pjpm);
#endif // JUDYL
            D_cdP0 = (~cJU_MASKATSTATE((4) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF5);
            ju_SetLeafPop1(Pjp, cJU_LEAF5_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }

        offset = j__udySearchLeaf4(Pleaf, Pop1, Index, 4 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF4VALUEAREA(Pleaf, Pop1););
        assert(Pop1 > (cJU_IMMED4_MAXPOP1));    // Pop1 == 3


        if ((Pop1 - 1) == (cJU_IMMED4_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_PntrInJp(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY(ju_PImmed4(Pjp), Pleaf, Pop1, offset);
            ju_SetJpType(Pjp, (cJ1_JPIMMED_4_02) - 1 + (cJU_IMMED4_MAXPOP1) - 1);
            j__udyFreeJLL4(PjllRaw, Pop1, Pjpm);
#else  // JUDYL
            assert(cJU_IMMED4_MAXPOP1 == (3 - 1));
        
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(3 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETECOPY(ju_PImmed4(Pjp), Pleaf, 3, offset);
            JU_DELETECOPY(Pjvnew, Pjv, 3, offset);
            j__udyFreeJLL4(PjllRaw, 3, Pjpm);        /* Pjp->jp_PValue = PjvnewRaw;      */

            ju_SetJpType(Pjp, cJU_JPIMMED_4_02);
            ju_SetPntrInJp(Pjp, PjvnewRaw);      /* Pjp->jp_Type = (BaseJPType) - 2 + (MaxPop1); */
#endif // JUDYL
            return (1);
        }
        if (JU_LEAF4DELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf, Pop1, offset);
#ifdef JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif  // JUDYL
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
        PjllnewRaw4 = j__udyAllocJLL4(Pop1 - 1, Pjpm);
        if (PjllnewRaw4 == 0)
            return (-1);
        Pjllnew4 = P_JLL4(PjllnewRaw4);
        JU_DELETECOPY((uint32_t *)Pjllnew, Pleaf, Pop1, offset);
#ifdef JUDYL
        Pjv_t Pjvnew = JL_LEAF4VALUEAREA(Pjllnew, Pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif  // JUDYL
        j__udyFreeJLL4(PleafRaw, Pop1, Pjpm);

        ju_SetPntrInJp(Pjp, PjllnewRaw);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
    }
    case cJU_JPLEAF5:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNotMatchKey(Index, Pjp, 5));
        assert(ParentLevel > (5));
        PleafRaw5 = ju_PntrInJp(Pjp);
        Pleaf = (uint8_t *) P_JLL5(PleafRaw5);
        Pop1 = ju_LeafPop1(Pjp);
        assert(((ParentLevel - 1) == (5)) || (Pop1 >= (cJU_LEAF6_MAXPOP1)));
        if (((ParentLevel - 1) > (5)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF6_MAXPOP1)))
        { /* hysteresis = 1       */
            Word_t    D_cdP0;
            PjllnewRaw6 = j__udyAllocJLL6(cJU_LEAF6_MAXPOP1, Pjpm);
            if (PjllnewRaw6 == 0)
                return (-1);
            Pjll6_t Pjll6 = P_JLL6(PjllnewRawx);
#ifdef  JUDY1
            (void)j__udyLeaf5ToLeaf6((uint8_t *) Pjllnew,      Pjp, Index & cJU_DCDMASK(5), Pjpm);
#else  // JUDYL
            Pjv = JL_LEAF6VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF6_MAXPOP1);
            (void)j__udyLeaf5ToLeaf6((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(5), Pjpm);
#endif // JUDYL
            D_cdP0 = (~cJU_MASKATSTATE((5) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF6);
            ju_SetLeafPop1(Pjp, cJU_LEAF6_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf5(Pleaf, Pop1, Index, 5 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF5VALUEAREA(Pleaf, Pop1););
        assert(Pop1 > (cJU_IMMED5_MAXPOP1));
        if ((Pop1 - 1) == (cJU_IMMED5_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_PntrInJp(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD(ju_PImmed5(Pjp), Pleaf, Pop1, offset, 5);
            ju_SetJpType(Pjp, (cJ1_JPIMMED_5_02) - 1 + (cJU_IMMED5_MAXPOP1) - 1);
            j__udyFreeJLL5(PjllRaw, Pop1, Pjpm);
#else  // JUDYL
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                offset = j__udySearchLeaf5(Pleaf, 2, Index, 5 * 8);
                assert(offset >= 0);    /* Index must be valid */
                JU_COPY5_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 5]));
                D_cdP0 |= Index & cJU_DCDMASK(5);
                A_ddr = Pjv[offset ? 0 : 1];

                ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_5_01);
//                ju_SetPntrInJp(Pjp, A_ddr);
//                ju_SetDcdPop0(Pjp, D_cdP0);
//                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL5(PjllRaw, Pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
#endif // JUDYL
            return (1);
        }
        if (JU_LEAF5DELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, Pop1, offset, 5);
#ifdef JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif // JUDYL
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
//      else must allocate smaller Leaf
        PjllnewRaw5 = j__udyAllocJLL5(Pop1 - 1, Pjpm);
        if (PjllnewRaw5 == 0)
            return (-1);
        Pjllnew5 = P_JLL5(PjllnewRaw5);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, Pop1, offset, 5);
#ifdef JUDYL
        Pjv_t Pjvnew = JL_LEAF5VALUEAREA(Pjllnew, Pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif // JUDYL
        j__udyFreeJLL5(PleafRaw, Pop1, Pjpm);

        ju_SetPntrInJp(Pjp, PjllnewRaw);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
    }
    case cJU_JPLEAF6:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNotMatchKey(Index, Pjp, 6));
        assert(ParentLevel > (6));
        PleafRaw6= ju_PntrInJp(Pjp);
        Pleaf = (uint8_t *) P_JLL6(PleafRaw6);
        Pop1 = ju_LeafPop1(Pjp);
        assert(((ParentLevel - 1) == (6)) || (Pop1 >= (cJU_LEAF7_MAXPOP1)));
        if (((ParentLevel - 1) > (6)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF7_MAXPOP1)))     /* hysteresis = 1       */
        {
            Word_t    D_cdP0;
            PjllnewRaw7 = j__udyAllocJLL7(cJU_LEAF7_MAXPOP1, Pjpm);
            if (PjllnewRaw7  == 0)
                return (-1);
            Pjll7_t Pjll7 = P_JLL7(PjllnewRaw7);
#ifdef  JUDY1
            (void)j__udyLeaf6ToLeaf7((uint8_t *) Pjllnew,      Pjp, Index & cJU_DCDMASK(6), Pjpm);
#else  // JUDYL
            Pjv = JL_LEAF7VALUEAREA((uint8_t *) Pjllnew, cJU_LEAF7_MAXPOP1);
            (void)j__udyLeaf6ToLeaf7((uint8_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(6), Pjpm);
#endif // JUDYL
            D_cdP0 = (~cJU_MASKATSTATE((6) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF7);
            ju_SetLeafPop1(Pjp, cJU_LEAF7_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* delete from new leaf */
        }
        offset = j__udySearchLeaf6(Pleaf, Pop1, Index, 6 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF6VALUEAREA(Pleaf, Pop1););
        assert(Pop1 > (cJU_IMMED6_MAXPOP1));
        if ((Pop1 - 1) == (cJU_IMMED6_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_PntrInJp(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD(ju_PImmed6(Pjp), Pleaf, Pop1, offset, 6);
            ju_SetJpType(Pjp, (cJ1_JPIMMED_6_02) - 1 + (cJU_IMMED6_MAXPOP1) - 1);
            j__udyFreeJLL6(PjllRaw, Pop1, Pjpm);
#else  // JUDYL
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                offset = j__udySearchLeaf6(Pleaf, 2, Index, 6 * 8);
                assert(offset >= 0);    /* Index must be valid */
                JU_COPY6_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 6]));
                D_cdP0 |= Index & cJU_DCDMASK(6);
                A_ddr = Pjv[offset ? 0 : 1];

                ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_6_01);
//                ju_SetPntrInJp(Pjp, A_ddr);
//                ju_SetDcdPop0(Pjp, D_cdP0);
//                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL6(PjllRaw, Pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
#endif // JUDYL
            return (1);
        }
        if (JU_LEAF6DELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, Pop1, offset, 6);
#ifdef JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif // JUDYL
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
//      else  Must allocate smaller Leaf
        PjllnewRaw6 = j__udyAllocJLL6(Pop1 - 1, Pjpm);
        if (PjllnewRaw6 == 0)
            return (-1);
        Pjllnew = P_JLL6(PjllnewRaw6);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, Pop1, offset, 6);
#ifdef JUDYL
        Pjv_t Pjvnew = JL_LEAF6VALUEAREA(Pjllnew, Pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif // JUDYL
        j__udyFreeJLL6(PleafRaw, Pop1, Pjpm);

        ju_SetPntrInJp(Pjp, PjllnewRaw);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
    }

    case cJU_JPLEAF7:
    {
        Word_t    PleafRaw;
        uint8_t  *Pleaf;
        assert(!ju_DcdNotMatchKey(Index, Pjp, 7));
        assert(ParentLevel > (7));
        PleafRaw7 = ju_PntrInJp(Pjp);
        Pleaf = (uint8_t *) P_JLL7(PleafRaw7);
        Pop1 = ju_LeafPop1(Pjp); /* null. */ ;
        offset = j__udySearchLeaf7(Pleaf, Pop1, Index, 7 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JUDYLCODE(Pjv = JL_LEAF7VALUEAREA(Pleaf, Pop1););
        assert(Pop1 > (cJU_IMMED7_MAXPOP1));
        if ((Pop1 - 1) == (cJU_IMMED7_MAXPOP1)) /* hysteresis = 0 */
        {
            Word_t    PjllRaw = ju_PntrInJp(Pjp);
#ifdef  JUDY1
            JU_DELETECOPY_ODD(ju_PImmed7(Pjp), Pleaf, Pop1, offset, 7);
            ju_SetJpType(Pjp, (cJ1_JPIMMED_7_02) - 1 + (cJU_IMMED7_MAXPOP1) - 1);
            j__udyFreeJLL7(PjllRaw, Pop1, Pjpm);
#else  // JUDYL
            {
                Word_t    D_cdP0;
                Word_t    A_ddr = 0;
                offset = j__udySearchLeaf7(Pleaf, 2, Index, 7 * 8);
                assert(offset >= 0);    /* Index must be valid */
                JU_COPY7_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 7]));
                D_cdP0 |= Index & cJU_DCDMASK(7);
                A_ddr = Pjv[offset ? 0 : 1];

                ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_7_01);
//                ju_SetPntrInJp(Pjp, A_ddr);
//                ju_SetDcdPop0(Pjp, D_cdP0);
//                ju_SetJpType(Pjp, T_ype);
            }
            j__udyFreeJLL7(PjllRaw, Pop1, Pjpm);        /* Pjp->jp_Type = (Immed01JPType);        */
#endif // JUDYL
            return (1);
        }
//      else
        if (JU_LEAF7DELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE_ODD(Pleaf, Pop1, offset, 7);
#ifdef JUDYL
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif // JUDYL
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
        }
        PjllnewRaw7 = j__udyAllocJLL7(Pop1 - 1, Pjpm);
        if (PjllnewRaw7 == 0)
            return (-1);
        Pjllnew = P_JLL7(PjllnewRaw7);
        JU_DELETECOPY_ODD((uint8_t *) Pjllnew, Pleaf, Pop1, offset, 7);
#ifdef JUDYL
        Pjv_t Pjvnew = JL_LEAF7VALUEAREA(Pjllnew, Pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif // JUDYL
        j__udyFreeJLL7(PleafRaw, Pop1, Pjpm);

        ju_SetPntrInJp(Pjp, PjllnewRaw);
#ifdef PCAS
            printf("Line = %d\n", __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue; // requires Pjp and Pop1
    }
// ****************************************************************************
// BITMAP LEAF:
    case cJU_JPLEAF_B1U:
    {
        Word_t    PjlbRaw = ju_PntrInJp(Pjp);
        Pjlb_t    Pjlb    = P_JLB1(PjlbRaw); // pointer to bitmap part of the leaf.
#ifdef JUDYL
        Pjv_t     Pjvnew;
        Word_t    subexp;               // 1 of 8 subexpanses in bitmap.
        BITMAPL_t bitmap;               // for one subexpanse.
        BITMAPL_t bitmask;              // bit set for Indexs digit.
#endif // JUDYL
        assert(!ju_DcdNotMatchKey(Index, Pjp, 1));
        assert(ParentLevel > 1);
        assert(JU_BITMAPTESTL(P_JLB1(ju_PntrInJp(Pjp)), Index)); // valid Index:

        Pop1 = ju_LeafPop1(Pjp);
//printf("Del B1: Pop1 = %d, Parentlevel = %d, Line = %d\n", (int)Pop1, (int)ParentLevel, (int)__LINE__);
//        assert(Pop1 > cJU_IMMED1_MAXPOP1);
// Like a Leaf1, see if its under a narrow pointer and can become a Leaf2
// (hysteresis = 1):
//  This code may not work!!!!! (dlb) and probally not good for a large Leaf2
//        assert(((ParentLevel - 1) == (1)) || (Pop1 == (cJU_LEAF2_MAXPOP1)));
        if (((ParentLevel - 1) > (1)) /* under narrow pointer */  && (Pop1 == (cJU_LEAF2_MAXPOP1)))
        { /* hysteresis = 1       */
            PjllnewRaw2 = j__udyAllocJLL2(cJU_LEAF2_MAXPOP1, Pjpm);
            if (PjllnewRaw2 == 0)
                return (-1);
            Pjll2_t Pjll2 = P_JLL2(PjllnewRaw2);
           
//          Change this LeafB1 to a Leaf2
//printf("Del B1: Pop1 = %d, Parentlevel = %d, ToLeaf2, Line = %d\n", (int)Pop1, (int)ParentLevel, (int)__LINE__);
#ifdef  JUDY1
            (void)j__udyLeaf1orB1ToLeaf2((uint16_t *) Pjllnew,      Pjp, Index & cJU_DCDMASK(1), Pjpm);
#else  // JUDYL
            Pjv = JL_LEAF2VALUEAREA((uint16_t *) Pjllnew, cJU_LEAF2_MAXPOP1);
            (void)j__udyLeaf1orB1ToLeaf2((uint16_t *) Pjllnew, Pjv, Pjp, Index & cJU_DCDMASK(1), Pjpm);
#endif // JUDYL
            Word_t D_cdP0 = (~cJU_MASKATSTATE((1) + 1)) & ju_DcdPop0(Pjp);

            ju_SetDcdPop0(Pjp, D_cdP0);
            ju_SetJpType(Pjp, cJU_JPLEAF2);
            ju_SetLeafPop1(Pjp, cJU_LEAF2_MAXPOP1);
            ju_SetPntrInJp(Pjp, PjllnewRaw);
            goto ContinueDelWalk;       /* now go delete from new leaf2 */
        }
// Handle the special case, on Judy1, where a LeafB1 may go
// directly to a JPIMMED_1_15; as described in comments in Judy1.h and
// JudyIns.c.  Copy 1-byte indexes from old LeafB1 to the Immed:

#ifdef  JUDY1   // ==================================

//      If JUDY1 does not have a LEAF1, convert to IMMED_1_15
        if (cJU_IMMED1_MAXPOP1 >= cJU_LEAF1_MAXPOP1)  // if NO Leaf1 - compile out
        {
            if (Pop1 == (cJU_IMMED1_MAXPOP1 + 1))       // hysteresis = 0.
            {
//printf("Del B1: change to cJ1_JPIMMED_1_15, Pop1 = %d, cJU_IMMED1_MAXPOP1 = %d, Key = 0x%lx, Line = %d\n", (int)Pop1, (int)cJU_IMMED1_MAXPOP1, Index, (int)__LINE__);
                Word_t     ldigit;                      // larger than uint8_t.
                uint8_t   *Pleaf1new = ju_PImmed1(Pjp); // pointer to IMMEDs

                JU_BITMAPCLEARL(Pjlb, Index);           // unset Indexs bit.

// TBD:  This is very slow, there must be a better way:
//                                              256
                for (ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ldigit++)
                {
                    if (JU_BITMAPTESTL(Pjlb, ldigit))
                        *Pleaf1new++ = ldigit;          // only 15 of 256
                }
                j__udyFreeJLB1U(PjlbRaw, Pjpm);          // Free LeafB1
//#ifdef  JUDY1
                ju_SetJpType(Pjp, cJ1_JPIMMED_1_15);
//#else   //  JUDYL
//                ju_SetJpType(Pjp, cJ1_JPIMMED_1_08);
//#endif  //  JUDYL
                return (1);                             // done
            }
        }
        else  // Judy HAS a Leaf1
        {
//          Compress LeafB1 to a Leaf1:
            if (Pop1 == cJU_LEAF1_MAXPOP1)      // hysteresis = 1.
            {
////printf("Del B1: change to cJU_LEAF1_MAXPOP1, Pop1 = %d, Line = %d\n", (int)Pop1, (int)__LINE__);
                if (j__udyLeafB1ToLeaf1(Pjp, Pjpm) == -1)
                    return (-1);
//              jp_Type and Pointer and Free of B1 is done here
                goto ContinueDelWalk;           // delete Index in new Leaf1.
            }
        }
//printf("Del B1: Just Clear Bit, Pop1 = %d\n", (int)Pop1);
//      Just unset Indexs bit and continue
        JU_BITMAPCLEARL(P_JLB1(ju_PntrInJp(Pjp)), Index);

#ifdef PCAS
        printf("Del B1:=========Pop1 = %d ========Line = %d\n", (int)Pop1, __LINE__);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue;  // requires Pjp and Pop1

#endif  // JUDY1 ==================================




#ifdef  JUDYL   //  =================================
        if (Pop1 == cJU_LEAF1_MAXPOP1)      // hysteresis = 1.
        {
            if (j__udyLeafB1ToLeaf1(Pjp, Pjpm) == -1)
                return (-1);
//          jp_Type and Pointer and Free of B1 is done here
            goto ContinueDelWalk;           // delete Index in new Leaf1.
        }

// This is very different from Judy1 because of the need to manage the Value area:
// Get last byte to decode from Index, and pointer to bitmap leaf:

        digit = JU_DIGITATSTATE(Index, 1);
        Pjlb = P_JLB1(ju_PntrInJp(Pjp));

//      Prepare additional values:
        subexp = digit / cJU_BITSPERSUBEXPL;    // which subexpanse.
        bitmap = JU_JLB_BITMAP(Pjlb, subexp);   // subexps 64-bit map.
        PjvRaw = JL_JLB_PVALUE(Pjlb);           // corresponding values.
        Pjv = P_JV(PjvRaw);
        bitmask = JU_BITPOSMASKL(digit);        // mask for Index.

        assert(bitmap & bitmask);               // Index must be valid.

//printf("\n-----------LeafB1: bitmap = 0x%016lx, bitmask = 0x%016lx, LeafPop1 = 0x%lx, Pop1 = 0x%lx, digit = 0x%x, Index = 0x%016lx\n", bitmap, bitmask, ju_LeafPop1(Pjp), Pop1, (int)digit, Index);

//  Note: this should be for a uncompressed Value area
        Word_t pop1;
        if (bitmap == cJU_FULLBITMAPL)          // full bitmap, take shortcut:
        {
            pop1 = cJU_BITSPERSUBEXPL;
            offset = digit % cJU_BITSPERSUBEXPL;
        }
        else                            // compute subexpanse pop1 and value area offset:
        {
            pop1   = j__udyCountBitsL(bitmap);
            offset = j__udyCountBitsL(bitmap & (bitmask - 1));
        }
// Handle solitary Index remaining in this subexpanse:
        if (pop1 == 1)
        {
            j__udyLFreeJV(PjvRaw, 1, Pjpm);
            JL_JLB_PVALUE(Pjlb) = 0;            // mark subexpanse null Values
            JU_JLB_BITMAP(Pjlb, subexp) = 0;            // mark bitmap 0
#ifdef PCAS
        printf("\nDel B1: ju_LeafPop1(Pjp) = 0x%lx, ju_DcdPop0(Pjp) = 0x%016lx\n", ju_LeafPop1(Pjp), ju_DcdPop0(Pjp));
printf("Del B1: Line = %d, Pop1 = %d\n", __LINE__, (int)Pop1);
#endif  // PCAS
            goto UpdateLeafPop1AndReturnTrue;  // requires Pjp and Pop1
        }
//      else
// Shrink value area in place or move to a smaller value area:
        if (JL_LEAFVDELINPLACE(pop1))      // hysteresis = 0.
        {
            JU_DELETEINPLACE(Pjv, pop1, offset);
        }
        else    // allocate a smaller Value area
        {
            Word_t PjvnewRaw = j__udyLAllocJV(pop1 - 1, Pjpm);
            if (PjvnewRaw == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETECOPY(Pjvnew, Pjv, pop1, offset);
            j__udyLFreeJV(PjvRaw, pop1, Pjpm);
            JL_JLB_PVALUE(Pjlb) = PjvnewRaw;
        }
//        JU_JLB_BITMAP(Pjlb, subexp) ^= bitmask; // clear Indexs bit.
        JU_BITMAPCLEARL(Pjlb, Index);
#ifdef PCAS
        printf("Del B1: Line = %d, Pop1 = %d\n", __LINE__, (int)Pop1);
#endif  // PCAS
        goto UpdateLeafPop1AndReturnTrue;   // requires Pjp and Pop1

#endif  // JUDYL ==================================
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
        assert(!ju_DcdNotMatchKey(Index, Pjp, 2));
        assert(ParentLevel > 1);        // see above.

#ifdef PCAS
        printf("Del B1: cJ1_JPFULLPOPU1, ju_LeafPop1(Pjp) = 0x%lx, ju_DcdPop0(Pjp) = 0x%016lx\n", ju_LeafPop1(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

        Pop1 = ju_LeafPop1(Pjp);
        assert(Pop1 == 256);

        if ((PjlbRaw = j__udyAllocJLB1U(Pjpm)) == 0)
            return (-1);
        Pjlb = P_JLB1(PjlbRaw);
// Fully populate the leaf, then unset Key bit:
        for (subexp = 0; subexp < cJU_NUMSUBEXPL; ++subexp)
            JU_JLB_BITMAP(Pjlb, subexp) = cJU_FULLBITMAPL;

        JU_BITMAPCLEARL(Pjlb, Index);   // delete key

        ju_SetJpType(Pjp, cJU_JPLEAF_B1U);
//        ju_SetLeafPop1(Pjp, cJU_JPFULLPOPU1_POP0);
        ju_SetPntrInJp(Pjp, PjlbRaw);
        goto UpdateLeafPop1AndReturnTrue;   // requires Pjp and Pop1
    }
// ****************************************************************************
#endif // JUDY1
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
// Variables Pleaf, Pop1, and offset are in the context.
#ifdef JUDYL
// For JudyL the value area might need to be shrunk:
#endif // JUDYL
// Delete one Index from a larger Immed where no restructuring is required:
//
// Variables Pop1, Pjp, offset, and Index are in the context.
// END OF MACROS, START OF CASES:
// Single Index remains in Immed; convert JP to null:
    case cJU_JPIMMED_1_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U2));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL1);
        return (1);
    }
    case cJU_JPIMMED_2_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U3));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL2);
        return (1);
    }
    case cJU_JPIMMED_3_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U4));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL3);
        return (1);
    }
    case cJU_JPIMMED_4_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U5));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL4);
        return (1);
    }
    case cJU_JPIMMED_5_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U6));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL5);
        return (1);
    }
    case cJU_JPIMMED_6_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U7));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL6);
        return (1);
    }
    case cJU_JPIMMED_7_01:
    {
        assert(parentJPtype == (cJU_JPBRANCH_U8));
        assert(ju_IMM01Key(Pjp) == JU_TrimToIMM01(Index));
        ju_SetIMM01(Pjp, 0, 0, cJU_JPNULL7);
        return (1);
    }
// Multiple Indexes remain in the Immed JP; delete the specified Index:
    case cJU_JPIMMED_1_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (1));
        Pleaf = ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PntrInJp(Pjp);
        Pjv = P_JV(PjvRaw);
#endif // JUDYL
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            offset = (Pleaf[0] == JU_LEASTBYTES(Index, 1));     /* undeleted Ind */
            assert(Pleaf[offset ? 0 : 1] == JU_LEASTBYTES(Index, 1));
            D_cdP0 = (Index & cJU_DCDMASK(1)) | Pleaf[offset];
            JUDYLCODE(A_ddr = Pjv[offset];);

            ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_1_01);
//            ju_SetPntrInJp(Pjp, A_ddr);
//            ju_SetDcdPop0(Pjp, D_cdP0);
//            ju_SetJpType(Pjp, T_ype);
        }
#ifdef  JUDYL
        j__udyLFreeJV(PjvRaw, 2, Pjpm); /* Pjp->jp_Type = (NewJPType);   */
#endif // JUDYL
        return (1);
    }
    case cJU_JPIMMED_1_03:
    case cJU_JPIMMED_1_04:
    case cJU_JPIMMED_1_05:
    case cJU_JPIMMED_1_06:
    case cJU_JPIMMED_1_07:
    case cJU_JPIMMED_1_08:
#ifdef  JUDY1
    case cJ1_JPIMMED_1_09:
    case cJ1_JPIMMED_1_10:
    case cJ1_JPIMMED_1_11:
    case cJ1_JPIMMED_1_12:
    case cJ1_JPIMMED_1_13:
    case cJ1_JPIMMED_1_14:
    case cJ1_JPIMMED_1_15:
#endif // JUDY1
    {
        assert((ParentLevel - 1) == (1));
        uint8_t *Pleaf1 = ju_PImmed1(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PntrInJp(Pjp);
        Pjv = P_JV(PjvRaw);
#endif // JUDYL
        Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
        offset = j__udySearchImmed1(Pleaf1, Pop1, Index, 1 * 8);
        assert(offset >= 0);            /* Index must be valid */
#ifdef  JUDY1
        JU_DELETEINPLACE(Pleaf1, Pop1, offset);
#else  // JUDYL
        if (JL_LEAFVDELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf1, Pop1, offset);
            JU_DELETEINPLACE(Pjv, Pop1, offset);
        }
        else
        {
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(Pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETEINPLACE(Pleaf1, Pop1, offset);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
            j__udyLFreeJV(PjvRaw, Pop1, Pjpm); 
            ju_SetPntrInJp(Pjp, PjvnewRaw);
        }
#endif // JUDYL
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJU_JPIMMED_2_02:
    {
        uint16_t *Pleaf2;
        assert((ParentLevel - 1) == (2));
        Pleaf2 = ju_PImmed2(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PntrInJp(Pjp);
        Pjv = P_JV(PjvRaw);
#endif // JUDYL
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            offset = (Pleaf2[0] == JU_LEASTBYTES(Index, 2));     /* undeleted key */
            assert(Pleaf2[offset ? 0 : 1] == JU_LEASTBYTES(Index, 2));
            D_cdP0 = (Index & cJU_DCDMASK(2)) | Pleaf2[offset];
            JUDYLCODE(A_ddr = Pjv[offset];);
            ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_2_01);
        }                               /*  Pjp->jp_Type = (NewJPType);   */
#ifdef  JUDYL
        j__udyLFreeJV(PjvRaw, 2, Pjpm); /* Pjp->jp_Type = (NewJPType);   */
#endif // JUDYL
        return (1);
    }
    case cJU_JPIMMED_2_03:
    case cJU_JPIMMED_2_04:
#ifdef  JUDY1
    case cJ1_JPIMMED_2_05:
    case cJ1_JPIMMED_2_06:
    case cJ1_JPIMMED_2_07:
#endif // JUDY1
    {
        uint16_t *Pleaf2;
        assert((ParentLevel - 1) == (2));
        Pleaf2 = ju_PImmed2(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PntrInJp(Pjp);
        Pjv    = P_JV(PjvRaw);
#endif // JUDYL
        Pop1 = (ju_Type(Pjp)) - (cJU_JPIMMED_2_02) + 2;
        offset = j__udySearchLeaf2(Pleaf2, Pop1, Index, 2 * 8);
        assert(offset >= 0);            /* Index must be valid */
#ifdef  JUDY1
        JU_DELETEINPLACE(Pleaf2, Pop1, offset);
#else  // JUDYL
        if (JL_LEAFVDELINPLACE(Pop1))      /* hysteresis = 0 */
        {
            JU_DELETEINPLACE(Pleaf2, Pop1, offset);
            JU_DELETEINPLACE(Pjv, Pop1, offset);
        }
        else
        {
            Word_t    PjvnewRaw;
            Pjv_t     Pjvnew;
            if ((PjvnewRaw = j__udyLAllocJV(Pop1 - 1, Pjpm)) == 0)
                return (-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_DELETEINPLACE(Pleaf2, Pop1, offset);
            JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
            j__udyLFreeJV(PjvRaw, Pop1, Pjpm);  /*    Pjp->jp_PValue = PjvnewRaw;     */

            ju_SetPntrInJp(Pjp, PjvnewRaw);
        }                               /*   --(Pjp->jp_Type);       */
#endif // JUDYL
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJU_JPIMMED_3_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (3));
        Pleaf = ju_PImmed3(Pjp);
#ifdef JUDYL
        PjvRaw = ju_PntrInJp(Pjp);
        Pjv = P_JV(PjvRaw);
#endif // JUDYL
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            offset = j__udySearchLeaf3(Pleaf, 2, Index, 3 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY3_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 3]));
            D_cdP0 |= Index & cJU_DCDMASK(3);
#ifdef  JUDYL
            A_ddr = Pjv[offset ? 0 : 1];
#endif  // JUDYL
            ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_3_01);
        }
#ifdef  JUDYL
        j__udyLFreeJV(PjvRaw, 2, Pjpm); /* Pjp->jp_Type = (NewJPType);   */
#endif // JUDYL
        return (1);
    }
#ifdef  JUDY1
    case cJ1_JPIMMED_3_03:
    case cJ1_JPIMMED_3_04:
    case cJ1_JPIMMED_3_05:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (3));
        Pleaf = ju_PImmed3(Pjp);
        Pop1 = (ju_Type(Pjp)) - (cJU_JPIMMED_3_02) + 2;
        offset = j__udySearchLeaf3(Pleaf, Pop1, Index, 3 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JU_DELETEINPLACE_ODD(Pleaf, Pop1, offset, 3);  /*  --(Pjp->jp_Type);       */
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
#endif  // JUDY1

    case cJU_JPIMMED_4_02:
    {
        uint32_t *Pleaf4;
        Word_t    A_ddr = 0;
        assert((ParentLevel - 1) == (4));
        Pleaf4 = ju_PImmed4(Pjp);
        Pop1 = 2;
        offset = j__udySearchLeaf4(Pleaf4, Pop1, Index, 4 * 8);
        assert(offset >= 0);            /* Index must be valid */
        offset = (offset ? 0 : 1);      // offset is for remaining Key
        Word_t D_cdP0 = (Index & cJU_DCDMASK(4)) | Pleaf4[offset];
#ifdef  JUDYL
        PjvRaw = ju_PntrInJp(Pjp);
        Pjv    = P_JV(PjvRaw);
        A_ddr = Pjv[offset];
        j__udyLFreeJV(PjvRaw, Pop1, Pjpm);
#endif // JUDYL
        ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_4_01);
        return (1);
    }

#ifdef  JUDY1
    case cJ1_JPIMMED_4_03:
    {
        uint32_t *Pleaf;
        assert((ParentLevel - 1) == (4));
        Pleaf = ju_PImmed4(Pjp);
        Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
        offset = j__udySearchLeaf4(Pleaf, Pop1, Index, 4 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JU_DELETEINPLACE(Pleaf, Pop1, offset);
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJ1_JPIMMED_5_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (5));
        Pleaf = ju_PImmed5(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            offset = j__udySearchLeaf5(Pleaf, 2, Index, 5 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY5_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 5]));
            D_cdP0 |= Index & cJU_DCDMASK(5);
            ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_5_01);
        }                               /* Pjp->jp_Type = (NewJPType);   */
        return (1);
    }
    case cJ1_JPIMMED_5_03:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (5));
        Pleaf = ju_PImmed5(Pjp);
        Pop1 = (ju_Type(Pjp)) - (cJ1_JPIMMED_5_02) + 2;
        offset = j__udySearchLeaf5(Pleaf, Pop1, Index, 5 * 8);
        assert(offset >= 0);            /* Index must be valid */
        JU_DELETEINPLACE_ODD(Pleaf, Pop1, offset, 5); 
        ju_SetJpType(Pjp, ju_Type(Pjp) - 1);
        return (1);
    }
    case cJ1_JPIMMED_6_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (6));
        Pleaf = ju_PImmed6(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            offset = j__udySearchLeaf6(Pleaf, 2, Index, 6 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY6_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 6]));
            D_cdP0 |= Index & cJU_DCDMASK(6);
            ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_6_01);
        }
        return (1);
    }
    case cJ1_JPIMMED_7_02:
    {
        uint8_t  *Pleaf;
        assert((ParentLevel - 1) == (7));
        Pleaf = ju_PImmed7(Pjp);
        {
            Word_t    D_cdP0;
            Word_t    A_ddr = 0;
            offset = j__udySearchLeaf7(Pleaf, 2, Index, 7 * 8);
            assert(offset >= 0);        /* Index must be valid */
            JU_COPY7_PINDEX_TO_LONG(D_cdP0, &(Pleaf[offset ? 0 : 7]));
            D_cdP0 |= Index & cJU_DCDMASK(7);
            ju_SetIMM01(Pjp, A_ddr, D_cdP0, cJU_JPIMMED_7_01);
        }
        return (1);
    }
#endif // JUDY1
// ****************************************************************************
// INVALID JP TYPE:
    default:
    {
        JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
        return (-1);
    }                                   // switch
    }
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
            Word_t    DcdP0;
            DcdP0 = ju_DcdPop0(Pjp) - 1;        // decrement count.
            ju_SetDcdPop0(Pjp, DcdP0);
            break;
        }
        case 2:                        // collapse BranchL to single JP; see above:
        {
            Word_t    PjblRaw = ju_PntrInJp(Pjp);
            Pjbl_t    Pjbl = P_JBL(PjblRaw);
            *Pjp = Pjbl->jbl_jp[0];
            j__udyFreeJBL(PjblRaw, Pjpm);
            retcode = 1;
        }
        }
    }
    return (retcode);
}                                       // j__udyDelWalk()
#endif  // LATER

// ****************************************************************************
// J U D Y   1   U N S E T
// J U D Y   L   D E L
//
// Main entry point.  See the manual entry for details.
#ifdef  JUDY1
FUNCTION int
Judy1Unset(PPvoid_t PPArray,            // in which to delete.
           Word_t Index,                // to delete.
           PJError_t PJError            // optional, for returning error info.
          )
#else  // JUDYL
FUNCTION int
JudyLDel(PPvoid_t PPArray,              // in which to delete.
         Word_t Index,                  // to delete.
         PJError_t PJError              // optional, for returning error info.
        )
#endif                                  // JUDYL
{
    Word_t    Pop1;                     // population of leaf.
    int       offset;                   // at which to delete Index.
#ifdef  JUDY1
    int       retcode;                  // return code from Judy1Test().
#else  // JUDYL
    PPvoid_t  PPvalue;                  // pointer from JudyLGet().
#endif // JUDYL
// CHECK FOR NULL ARRAY POINTER (error by caller):
    if (PPArray == (PPvoid_t) NULL)
    {
        JU_SET_ERRNO(PJError, JU_ERRNO_NULLPPARRAY);
        return (JERRI);
    }


// CHECK IF INDEX IS INVALID:
//
// If so, theres nothing to do.  This saves a lot of time.  Pass through
// PJError, if any, from the "get" function. -- Get does not return PJError now
#ifdef  JUDY1
    if (j__udy1Test(*PPArray, Index, PJE0) == 0)        // Index not in Array - Done
        return (0);
#else  // JUDYL
    if (j__udyLGet (*PPArray, Index, PJE0) == 0)         // Index not in Array - Done
        return (0);
#endif // JUDYL

    printf("\nJudy1Unset and JudyLDel not implemented yet!\n");
    exit(1);

#ifdef LATER
// ****************************************************************************
// PROCESS TOP LEVEL (LEAF8) BRANCHES AND LEAVES:
// ****************************************************************************
// LEAF8 LEAF, OTHER SIZE:
//
// Shrink or convert the leaf as necessary.  Hysteresis = 0; none is possible.
    if (JU_LEAF8_POP0(*PPArray) < cJU_LEAF8_MAXPOP1)    // must be a LEAF8
    {
#ifdef JUDYL
        Pjv_t     Pjv;                  // current value area.
        Pjv_t     Pjvnew;               // value area in new leaf.
#endif // JUDYL
        Pjll8_t   Pjll8 = P_JLL8(*PPArray);     // first word of leaf.
        Pjll8_t   Pjll8new;             // replacement leaf.
        Pop1 = Pjll8->jl8_Population0 + 1;      // first word of leaf is pop0.
//      Delete single (last) Index from array:
        if (Pop1 == 1)
        {
            j__udyFreeJLL8(Pjll8, /* Pop1 = */ 1, (Pjpm_t) NULL);
            *PPArray = (Pvoid_t)NULL;
            return (1);
        }
// Locate Index in compressible leaf:
        offset = j__udySearchLeaf8(Pjll8->jl8_Leaf, Pop1, Index);
        assert(offset >= 0);            // Index must be valid.
#ifdef JUDYL
        Pjv = JL_LEAF8VALUEAREA(Pjll8, Pop1);
#endif // JUDYL

// Delete Index in-place:
//
// Note:  "Grow in place from Pop1 - 1" is the logical inverse of, "shrink in
// place from Pop1."  Also, Pjll8 points to the count word, so skip that for
// doing the deletion.
        if (JU_LEAF8DELINPLACE(Pop1))
        {
            JU_DELETEINPLACE(Pjll8->jl8_Leaf, Pop1, offset);
#ifdef JUDYL
//          also delete from value area:
            JU_DELETEINPLACE(Pjv, Pop1, offset);
#endif // JUDYL
            Pjll8->jl8_Population0--;   // decrement population in Leaf8
//          no jp_t yet to decrement
            return (1);                 // Done
        }
//      Allocate new leaf for use in either case below:
        Pjll8new = j__udyAllocJLL8(Pop1 - 1);
//        JU_CHECKALLOC(Pjll8_t, Pjll8new, JERRI);
        if (Pjll8new < (Pjll8_t) cBPW)   
        {                                               
            JU_SET_ERRNO(PJError, JU_ALLOC_ERRNO(Pjll8new));
            return(JERRI);
        }
//      Shrink to smaller LEAF8:
        Pjll8new->jl8_Population0 = (Pop1 - 1) - 1;
        JU_DELETECOPY(Pjll8new->jl8_Leaf, Pjll8->jl8_Leaf, Pop1, offset);
#ifdef JUDYL
        Pjvnew = JL_LEAF8VALUEAREA(Pjll8new, Pop1 - 1);
        JU_DELETECOPY(Pjvnew, Pjv, Pop1, offset);
#endif // JUDYL
        j__udyFreeJLL8(Pjll8, Pop1, (Pjpm_t) NULL);
////        *PPArray = (Pvoid_t)  Pjll8new | cJU_LEAF8);
        *PPArray = (Pvoid_t)Pjll8new;
        return (1);     // success 
    }
    else

// ****************************************************************************
// JRP BRANCH:
//
// Traverse through the JPM to do the deletion unless the population is small
// enough to convert immediately to a LEAF8.
    {
        Pjpm_t    Pjpm;
        Pjp_t     Pjp;                  // top-level JP to process.
        Word_t    digit;                // in a branch.
#ifdef JUDYL
        Pjv_t     Pjv;                  // to value area.
#endif // JUDYL
        Pjll8_t   Pjll8new;             // replacement leaf.
        Pjpm = P_JPM(*PPArray);         // top object in array (tree).
//        Pjp = &(Pjpm->jpm_JP);          // next object (first branch or leaf).
        Pjp = Pjpm->jpm_JP + 0;         // next object (first branch or leaf).

//        assert(((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_L)
//               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_B)
//               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_U));

// WALK THE TREE 
//
// Note:  Recursive code in j__udyDelWalk() knows how to collapse a lower-level
// BranchL containing a single JP into the parent JP as a narrow pointer, but
// the code here cant do that for a top-level BranchL.  The result can be
// PArray -> JPM -> BranchL containing a single JP.  This situation is
// unavoidable because a JPM cannot contain a narrow pointer; the BranchL is
// required in order to hold the top digit decoded, and it does not collapse to
// a LEAF8 until the population is low enough.
//
// TBD:  Should we add a topdigit field to JPMs so they can hold narrow
// pointers?
        if (j__udyDelWalk(Pjp, Index, cJU_ROOTSTATE, Pjpm) == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
            return (JERRI);
        }
        Pjpm->jpm_Pop0--;             // success; decrement total population.
        ju_SetDcdPop0(Pjp, ju_DcdPop0(Pjp) - 1);        // decrement DcdPop0

        if ((Pjpm->jpm_Pop0 + 1) != cJU_LEAF8_MAXPOP1)
        {
            return (1);
        }
// COMPRESS A BRANCH[LBU] TO A LEAF8:
//
        Pjll8new = j__udyAllocJLL8(cJU_LEAF8_MAXPOP1);
        PWord_t   PLeaf8new = Pjll8new->jl8_Leaf;
//        JU_CHECKALLOC(Pjll8_t, Pjll8new, JERRI);
        if (Pjll8new < (Pjll8_t) cBPW)   
        {                                               
            JU_SET_ERRNO(PJError, JU_ALLOC_ERRNO(Pjll8new));
            return(JERRI);
        }
// Plug leaf into root pointer and set population count:
////        *PPArray  = (Pvoid_t) ((Word_t) Pjll8new | cJU_LEAF8);
        *PPArray = (Pvoid_t)Pjll8new;
#ifdef JUDYL
        Pjv = JL_LEAF8VALUEAREA(Pjll8new, cJU_LEAF8_MAXPOP1);
#endif // JUDYL
//        *Pjll8new++ = cJU_LEAF8_MAXPOP1 - 1;     // set pop0.
        Pjll8new->jl8_Population0 = cJU_LEAF8_MAXPOP1 - 1;      // set pop0.

        switch (ju_Type(Pjp))
        {
        case cJU_JPLEAF7:
        {
            Word_t   PLeaf7Raw  = ju_PntrInJp(Pjp);
            uint8_t *Pjll7      = P_JLL7(PLeaf7Raw);

            (void) j__udyLeaf7ToLeaf8(
                        PLeaf8new,      // destination leaf.
#ifdef JUDYL
                        Pjv,            // destination value part of leaf.
#endif  // JUDYL
                        Pjp,            // 7-byte-index object from which to copy.
                        Index & cJU_DCDMASK(7), // most-significant byte, prefix to each Index.
                        Pjpm);  // for global accounting.
            break;
        }
// JPBRANCH_L:  Copy each JPs indexes to the new LEAF8 and free the old
// branch:
        case cJU_JPBRANCH_L8:
        {
            Word_t    PjblRaw = ju_PntrInJp(Pjp);
            Pjbl_t    Pjbl = P_JBL(PjblRaw);
            for (offset = 0; offset < Pjbl->jbl_NumJPs; ++offset)
            {
                Word_t pop1;
#ifdef  JUDY1
                pop1 = j__udyLeaf7ToLeaf8(PLeaf8new,      (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], cJU_BYTESPERWORD), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf7ToLeaf8(PLeaf8new, Pjv, (Pjbl->jbl_jp) + offset, JU_DIGITTOSTATE(Pjbl->jbl_Expanse[offset], cJU_BYTESPERWORD), Pjpm);
                Pjv += pop1;            // advance through values.
#endif // JUDYL
                PLeaf8new += pop1;      // advance through indexes.
            }
            j__udyFreeJBL(PjblRaw, Pjpm);
            break;                      // delete Index from new LEAF8.
        }
// JPBRANCH_B:  Copy each JPs indexes to the new LEAF8 and free the old
// branch, including each JP subarray:
        case cJU_JPBRANCH_B8:
        {
            Word_t    PjbbRaw = ju_PntrInJp(Pjp);
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
                    Word_t pop1;
                    if (!(bitmap & 1))  // skip empty sub-subexpanse.
                        continue;
#ifdef  JUDY1
                    pop1 = j__udyLeaf7ToLeaf8(PLeaf8new,      Pjp2 + offset, JU_DIGITTOSTATE(digit, cJU_BYTESPERWORD), Pjpm);
#else  // JUDYL
                    pop1 = j__udyLeaf7ToLeaf8(PLeaf8new, Pjv, Pjp2 + offset, JU_DIGITTOSTATE(digit, cJU_BYTESPERWORD), Pjpm);
#endif // JUDYL
                    PLeaf8new += pop1;  // advance through indexes.
#ifdef JUDYL
                    Pjv += pop1;        // advance through values.
#endif // JUDYL
                    ++offset;
                }
                j__udyFreeJBBJP(Pjp2Raw, /* pop1 = */ offset, Pjpm);
            }
            j__udyFreeJBB(PjbbRaw, Pjpm);
            break;                      // delete Index from new LEAF8.
        }                               // case cJU_JPBRANCH_B.
// JPBRANCH_U:  Copy each JPs indexes to the new LEAF8 and free the old
// branch:
        case cJU_JPBRANCH_U8:
        {
            Word_t    PjbuRaw = ju_PntrInJp(Pjp);
            Pjbu_t    Pjbu = P_JBU(PjbuRaw);
            Word_t    ldigit;           // larger than uint8_t.
            for (Pjp = Pjbu->jbu_jp, ldigit = 0; ldigit < cJU_BRANCHUNUMJPS; ++Pjp, ++ldigit)
            {
                Word_t pop1;
// Shortcuts, to save a little time for possibly big branches:
                if ((ju_Type(Pjp)) == cJU_JPNULLMAX)    // skip null JP.
                    continue;
// TBD:  Should the following shortcut also be used in BranchL and BranchB
// code?
                if ((ju_Type(Pjp)) == cJU_JPIMMED_7_01)
                {                       // single Immed:
                    Word_t pop1;
                    *PLeaf8new++ = JU_DIGITTOSTATE(ldigit, cJU_BYTESPERWORD) | ju_IMM01Key(Pjp);
#ifdef JUDYL
                    *Pjv++ = ju_ImmVal_01(Pjp);  // copy value area.
#endif // JUDYL
                    continue;
                }
#ifdef  JUDY1
                pop1 = j__udyLeaf7ToLeaf8(PLeaf8new,      Pjp, JU_DIGITTOSTATE(ldigit, cJU_BYTESPERWORD), Pjpm);
#else  // JUDYL
                pop1 = j__udyLeaf7ToLeaf8(PLeaf8new, Pjv, Pjp, JU_DIGITTOSTATE(ldigit, cJU_BYTESPERWORD), Pjpm);
                Pjv += pop1;            // advance through values.
#endif // JUDYL
                PLeaf8new += pop1;      // advance through indexes.
            }
            j__udyFreeJBU(PjbuRaw, Pjpm);
            break;                      // delete Index from new LEAF8.
        }                               // case cJU_JPBRANCH_U8.
        case cJU_JPLEAF6:
            printf("Found LEAF6 --- need to putin LEAF8\n");
            break;
        case cJU_JPLEAF5:
            printf("Found LEAF5 --- need to putin LEAF8\n");
            break;
        case cJU_JPLEAF4:
            printf("Found LEAF4 --- need to putin LEAF8\n");
            break;
        case cJU_JPLEAF3:
            printf("Found LEAF3 --- need to putin LEAF8\n");
            break;
        case cJU_JPLEAF2:
            printf("Found LEAF2 --- need to putin LEAF8\n");
            break;

// INVALID JP TYPE in jpm_t struct
        default:
            JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
            return (JERRI);
        }       // end switch
// FREE JPM (no longer needed):
        j__udyFreeJPM(Pjpm, (Pjpm_t) NULL);
        return (1);
    }
#endif  // LATER
}
 /*NOTREACHED*/                        // Judy1Unset() / JudyLDel()
