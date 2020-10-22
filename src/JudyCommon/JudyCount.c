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

// @(#) $Revision: 4.78 $ $Source: /judy/src/JudyCommon/JudyCount.c $
//
// Judy*Count() function for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.
//
// Compile with -DNOSMARTJBB, -DNOSEARCHUPJBU, and/or -DNOSMARTJLB to build a
// version with cache line optimizations deleted, for testing.
//
// Judy*Count() returns the "count of Indexes" (inclusive) between the two
// specified limits (Indexes).  This code is remarkably fast.  It traverses the
// "Judy array" data structure.
//
// This count code is the GENERIC untuned version (minimum code size).  It
// might be possible to tuned to a specific architecture to be faster.
// However, in real applications, with a modern machine, it is expected that
// the instruction times will be swamped by cache line fills.
// ****************************************************************************

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef  JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif  // JUDY1

#include "JudyPrivate1L.h"

#ifdef  TRACEJPC
#include "JudyPrintJP.c"
#endif  // TRACEJPC

// FORWARD DECLARATIONS (prototypes):
static  Word_t j__udy1LCountSM(Pjp_t Pjp, Word_t Index, Pjpm_t Pjpm);

// Each of Judy1 and JudyL get their own private (static) version of this
// function:

static  int j__udyCountLeafB1(Pjlb_t Pjlb, Word_t Index);

// TBD:  Should be made static for performance reasons?  And thus duplicated?
//
// Note:  There really are two different functions, but for convenience they
// are referred to here with a generic name.

#ifdef  JUDY1
#define j__udyJPPop1 j__udy1JPPop1
#else
#define j__udyJPPop1 j__udyLJPPop1
#endif  // JUDYL

Word_t j__udyJPPop1(      Pjp_t Pjp);

// LOCAL ERROR HANDLING:
//
// The Judy*Count() functions are unusual because they return 0 instead of JERR


// ****************************************************************************
// J U D Y   1   C O U N T
// J U D Y   L   C O U N T
//
// See the manual entry for details.
//
// This code is written recursively, at least at first, because thats much
// simpler; hope its fast enough.

#ifdef  JUDY1
FUNCTION Word_t Judy1Count
#else
FUNCTION Word_t JudyLCount
#endif  // JUDYL
        (
        Pcvoid_t  PArray,       // JRP to first branch/leaf in SM.
        Word_t    Index1,       // starting Index.
        Word_t    Index2,       // ending Index.
        PJError_t PJError       // not used, because do not modify array
        )
{
        jpm_t     fakejpm = {0};// local temporary for small arrays.
        Pjpm_t    Pjpm;         // top JPM or local temporary for error info.
        jp_t      fakejp = {0}; // constructed for calling j__udy1LCountSM().
        Pjp_t     Pjp;          // JP to pass to j__udy1LCountSM().
        Word_t    pop1;         // total for the array.
        Word_t    pop1above1;   // indexes at or above Index1, inclusive.
        Word_t    pop1above2;   // indexes at or above Index2, exclusive.
        int       retcode;      // from Judy*First() calls.
#ifdef  JUDYL
        PPvoid_t  PPvalue;      // for JudyLFirst() calls.
#endif  // JUDYL

// CHECK FOR SHORTCUTS:
//
        if (PArray == (Pvoid_t) NULL)
            return(0);

#ifdef  TRACEJPC
#ifdef  JUDYL
//printf("\n---JudyLCount Index1 = 0x%lx, Index2 = 0x%lx\n", Index1, Index2);
#endif  // JUDYL

#ifdef  JUDY1
//printf("\n---Judy1Count Index1 = 0x%lx, Index2 = 0x%lx\n", Index1, Index2);
#endif  // JUDY1
#endif  // TRACEJPC

        if (Index1 > Index2)            // Swap Index1 with Index2
        {
            Index1 = Index1 ^ Index2;
            Index2 = Index1 ^ Index2;
            Index1 = Index1 ^ Index2;
        }

// If Index1 == Index2, simply check if the specified Index is set; pass
// through the return value from Judy1Test() or JudyLGet() with appropriate
// translations.
        if (Index1 == Index2)
        {
#ifdef  JUDY1
            retcode = Judy1Test(PArray, Index1, PJE0);
            if (retcode == 0)
                return((Word_t)0);
#else   // JUDYL
            PPvalue = JudyLGet(PArray, Index1, PJE0);
            if (PPvalue == (PPvoid_t) NULL)             // Index is not found.
                return((Word_t)0);
#endif  // JUDYL
            return((Word_t)1);                                  // single index is set.
        }


// Note:  Since even cJU_LEAF8 types require counting between two Indexes,
// prepare them here for common code below that calls j__udy1LCountSM(), rather
// than handling them even more specially here.
        if (JU_LEAF8_POP0(PArray) < cJU_LEAF8_MAXPOP1) // must be a LEAF8
        {
            Pjll8_t Pjll8  = P_JLL8(PArray);    // first word of leaf8.
            Pjpm           = & fakejpm;
            Pjp            = & fakejp;

            ju_SetPntrInJp(Pjp, (Word_t)Pjll8);

            ju_SetLeafPop1(Pjp, Pjpm->jpm_Pop0 + 1);
            ju_SetDcdPop0(Pjp, Pjpm->jpm_Pop0);
            ju_SetJpType(Pjp, cJU_JPLEAF8);

            Pjpm->jpm_Pop0 = Pjll8->jl8_Population0;    // from first word of leaf.
            pop1           = Pjpm->jpm_Pop0 + 1;
        }
        else
        {
            Pjpm = P_JPM(PArray);
            Pjp  = Pjpm->jpm_JP + 0;    // TEMP !!!! change below when right
            pop1 = Pjpm->jpm_Pop0 + 1;
        }

// COUNT POP1 ABOVE INDEX1, INCLUSIVE:

        if (Index1 == 0)        // shortcut, pop1above1 is entire population:
        {
            pop1above1 = pop1;
        }
        else                    // find first valid Index above Index1, if any:
        {
#ifdef  JUDY1
            retcode = Judy1First(PArray, &Index1, PJE0);
#endif  // JUDY1

#ifdef  JUDYL
            retcode = JudyLFirst(PArray, &Index1, PJE0) != (PPvoid_t)NULL;
#endif  // JUDYL
//          If no Index at or above Index1, done
            if (retcode == 0)
                return((Word_t)0);

            assert(retcode == 1);

#ifdef  DEBUG
#ifdef  JUDY1
            retcode = Judy1Test(PArray, Index1, PJE0);  // check on JudyFirst
#endif  // JUDY1

#ifdef  JUDYL
            PPvalue = JudyLGet(PArray, Index1, PJE0);   // check on JudyFirst
            retcode = (PPvalue != (PPvoid_t)NULL);      // found a next Index.
#endif  // JUDYL
            assert(retcode == 1);
#endif  // DEBUG

// If a first/next Index was found, call the counting motor starting with that
// valid Index, meaning the return should be positive
//          Return Population above Index1
            pop1above1 = j__udy1LCountSM(Pjp, Index1, Pjpm);
        }

// COUNT POP1 ABOVE INDEX2, EXCLUSIVE, AND RETURN THE DIFFERENCE:
//
// In principle, calculate the ordinal of each Index and take the difference,
// with caution about off-by-one errors due to the specified Indexes being set
// or unset.  In practice:
//
// - The ordinals computed here are inverse ordinals, that is, the populations
//   ABOVE the specified Indexes (Index1 inclusive, Index2 exclusive), so
//   subtract pop1above2 from pop1above1, rather than vice-versa.
//
// - Index1s result already includes a count for Index1 and/or Index2 if
//   either is set, so calculate pop1above2 exclusive of Index2.
//
// TBD:  If Index1 and Index2 fall in the same expanse in the top-state
// branch(es), would it be faster to walk the SM only once, to their divergence
// point, before calling j__udy1LCountSM() or equivalent?  Possibly a non-issue
// if a top-state pop1 becomes stored with each Judy1 array.  Also, consider
// whether the first call of j__udy1LCountSM() fills the cache, for common tree
// branches, for the second call.
//
// As for pop1above1, look for shortcuts for special cases when pop1above2 is
// zero.  Otherwise call the counting "motor".

            assert(pop1above1);         // just to be safe.

            if (Index2 == cJU_ALLONES)
                return(pop1above1);     // Index2 at limit.
            Index2++;

//          Find 1st Index >= Index2
//printf("\n ===========================- Count JudyFirst called with Key = 0x%016lx\n", Index2);
#ifdef  JUDY1
            retcode = Judy1First(PArray, &Index2, PJE0);
//printf(" ===========================- Count JudyFirst return with Key = 0x%016lx\n", Index2);
#else   // JUDYL
            retcode = (JudyLFirst(PArray, &Index2, PJE0) != (PPvoid_t) NULL);
#endif  // JUDYL
            if (retcode == 0)           // return all population >= Index1?
                return(pop1above1);     // NO Index above Index2.
            assert(retcode == 1);

#ifdef  DEBUG
//          Verify that JudyFirst did its job
#ifdef  JUDY1
            retcode = Judy1Test(PArray, Index2, PJE0);
#endif  // JUDY1

#ifdef  JUDYL
            PPvalue = JudyLGet(PArray, Index2, PJE0);
            retcode = (PPvalue != (PPvoid_t)NULL);
#endif  // JUDYL
            if (retcode != 1)
                printf("\n -- Count JudyFirst returned a bad Key = 0x%016lx, return = %d\n", Index2, retcode);
            assert(retcode == 1);
#endif  // DEBUG

// Just as for Index1, j__udy1LCountSM() cannot return 0

//printf("Begin2 motor, Index2 = %ld, jp_Type = %d, pop1 = %lu Line = %d\n", Index2, ju_Type(Pjp), pop1above2, (int)__LINE__);
            pop1above2 = j__udy1LCountSM(Pjp, Index2, Pjpm);
// printf("pop1above1 = %ld pop1above2 = %ld\n", pop1above1, pop1above2);
            return(pop1above1 - pop1above2);

} // Judy1Count() / JudyLCount()


// ****************************************************************************
// __ J U D Y 1 L   C O U N T   S M
//
// Given a pointer to a JP (with invalid jp_DcdPopO at cJU_ROOTSTATE), a known
// valid Index, and a Pjpm for returning error info, recursively visit a Judy
// array state machine (SM) and return the count of Indexes, including Index,
// through the end of the Judy array at this state or below.  In case of error
// or a count of 0 (should never happen), return C_JERR with appropriate
// JU_ERRNO in the Pjpm.
//
// Note:  This function is not told the current state because its encoded in
// the JP Type.
//
// Method:  To minimize cache line fills, while studying each branch, if Index
// resides above the midpoint of the branch (which often consists of multiple
// cache lines), ADD the populations at or above Index; otherwise, SUBTRACT
// from the population of the WHOLE branch (available from the JP) the
// populations at or above Index.  This is especially tricky for bitmap
// branches.
//
// Note:  Unlike, say, the Ins and Del walk routines, this function returns the
// same type of returns as Judy*Count(), so it can use *_SET_ERRNO*() macros
// the same way.

FUNCTION static Word_t j__udy1LCountSM(
        Pjp_t   Pjp,            // top of Judy (sub)SM.
        Word_t  Index,          // count at or above this Index.
        Pjpm_t  Pjpm)           // for returning error info.
{
        Pjbl_t  Pjbl;           // Pjp->Jp_Addr0 masked and cast to types:
        Pjbb_t  Pjbb;
        Pjbu_t  Pjbu;

        Word_t  digit;          // next digit to decode from Index.
        long    jpnum = 0;      // (quiet gcc) JP number in a branch (base 0).
        int     offset;         // index ordinal within a leaf, base 0.
        Word_t  pop1;           // total population of an expanse.
        Word_t  pop1above;      // to return.

#ifdef TRACEJPC
        JudyPrintJP(Pjp, "C", __LINE__, Index, Pjpm);
#endif  // TRACEJPC

// Common code to check Decode bits in a JP against the equivalent portion of
// Index; XOR together, then mask bits of interest; must be all 0:
//
// Note:  Why does this code only assert() compliance rather than actively
// checking for outliers?  Its because Index is supposed to be valid, hence
// always match any Dcd bits traversed.
//
// Note:  This assertion turns out to be always true for cState = 3 on 32-bit
// and 7 on 64-bit, but its harmless, probably removed by the compiler.

// Common code to prepare to handle a root-level or lower-level branch:
// Extract a state-dependent digit from Index in a "constant" way, obtain the
// total population for the branch in a state-dependent way, and then branch to
// common code for multiple cases:
//
// For root-level branches, the state is always cJU_ROOTSTATE, and the
// population is received in Pjpm->jpm_Pop0.
//
// Note:  The total population is only needed in cases where the common code
// "counts up" instead of down to minimize cache line fills.  However, its
// available cheaply, and its better to do it with a constant shift (constant
// state value) instead of a variable shift later "when needed".

//        return 1,.256, zero is converted to 256, without branch
//#define ju_LEAF_POP1(Pjp) (((int)((uint8_t)ju_LeafPop1(Pjp)) - 1) + 1)

        Word_t RawPntr = ju_PntrInJp(Pjp);
        switch (ju_Type(Pjp))
        {
// ----------------------------------------------------------------------------
// ROOT-STATE LEAF that starts with a Pop0 word; just count within the leaf:

        case cJU_JPLEAF8:
        {
            Pjll8_t Pjll8 = P_JLL8(RawPntr);    // first word of leaf.

            assert((Pjpm->jpm_Pop0) + 1 == Pjll8->jl8_Population0 + 1);  // sent correctly.

//          offset = j__udySearchLeaf8(Pjll8, Pjpm->jpm_Pop0 + 1, Index);
            offset = j__udySearchLeaf8(Pjll8, Pjll8->jl8_Population0 + 1, Index);
            return(Pjll8->jl8_Population0 + 1 - offset);        // INCLUSIVE of Index.
        }

// ----------------------------------------------------------------------------
// LINEAR BRANCH; count populations in JPs in the JBL ABOVE the next digit in
// Index, and recurse for the next digit in Index:
//
// Note:  There are no null JPs in a JBL; watch out for pop1 == 0.
//
// Note:  A JBL should always fit in one cache line => no need to count up
// versus down to save cache line fills.  (PREPB() sets pop1 for no reason.)

// Common code (state-independent) for all cases of linear branches:
        case cJU_JPBRANCH_L2:
        case cJU_JPBRANCH_L3:
        case cJU_JPBRANCH_L4:
        case cJU_JPBRANCH_L5:
        case cJU_JPBRANCH_L6:
        case cJU_JPBRANCH_L7:
        case cJU_JPBRANCH_L8:
        {
        int level = ju_Type(Pjp) - cJU_JPBRANCH_L2 + 2;
        digit     = JU_DIGITATSTATE(Index, level);
        pop1      = ju_BranchPop0(Pjp, level) + 1;

        Pjbl      = P_JBL(RawPntr);
        jpnum     = Pjbl->jbl_NumJPs;                   // above last JP.
        pop1above = 0;

        while (digit < (Pjbl->jbl_Expanse[--jpnum]))    // still ABOVE digit.
        {

//printf("\nBranchL loop Line = %d\n", (int)__LINE__);
            pop1 = j__udyJPPop1((Pjbl->jbl_jp) + jpnum);
            pop1above += pop1;
            assert(jpnum > 0);                          // should find digit.
        }
        assert(digit == (Pjbl->jbl_Expanse[jpnum]));    // should find digit.

//printf("\nBranchL Line = %d\n", (int)__LINE__);
        pop1 = j__udy1LCountSM((Pjbl->jbl_jp) + jpnum, Index, Pjpm);
        return(pop1above + pop1);
        }

// ----------------------------------------------------------------------------
// BITMAP BRANCH; count populations in JPs in the JBB ABOVE the next digit in
// Index, and recurse for the next digit in Index:

// Common code (state-independent) for all cases of bitmap branches:
        case cJU_JPBRANCH_B2:
        case cJU_JPBRANCH_B3:
        case cJU_JPBRANCH_B4:
        case cJU_JPBRANCH_B5:
        case cJU_JPBRANCH_B6:
        case cJU_JPBRANCH_B7:
        case cJU_JPBRANCH_B8:
        {
            long   subexp;      // for stepping through layer 1 (subexpanses).
            long   findsub;     // subexpanse containing   Index (digit).
            Word_t findbit;     // bit        representing Index (digit).
            Word_t lowermask;   // bits for indexes at or below Index.
            Word_t jpcount;     // JPs in a subexpanse.
            Word_t clbelow;     // cache lines below digits cache line.
            Word_t clabove;     // cache lines above digits cache line.

            int level = ju_Type(Pjp) - cJU_JPBRANCH_B2 + 2;
            digit = JU_DIGITATSTATE(Index, level);
            pop1      = ju_BranchPop0(Pjp, level) + 1;

            Pjbb      = P_JBB(RawPntr);
            findsub   = digit / cJU_BITSPERSUBEXPB;
            findbit   = digit % cJU_BITSPERSUBEXPB;
            lowermask = JU_MASKLOWERINC(JU_BITPOSMASKB(findbit));
            clbelow   = clabove = 0;    // initial/default => always downward.

            assert(JU_BITMAPTESTB(Pjbb, digit)); // digit must have a JP.
            assert(findsub < cJU_NUMSUBEXPB);    // falls in expected range.

#ifndef NOSMARTJBB  // enable to turn off smart code for comparison purposes.

// FIGURE OUT WHICH DIRECTION CAUSES FEWER CACHE LINE FILLS; adding the pop1s
// in JPs above Indexs JP, or subtracting the pop1s in JPs below Indexs JP.
//
// This is tricky because, while each set bit in the bitmap represents a JP,
// the JPs are scattered over cJU_NUMSUBEXPB subexpanses, each of which can
// contain JPs packed into multiple cache lines, and this code must visit every
// JP either BELOW or ABOVE the JP for Index.
//
// Number of cache lines required to hold a linear list of the given number of
// JPs, assuming the first JP is at the start of a cache line or the JPs in
// jpcount fit wholly within a single cache line, which is ensured by
// JudyMalloc():

#define CLPERJPS(jpcount) \
        ((((jpcount) * cJU_WORDSPERJP) + cJU_WORDSPERCL - 1) / cJU_WORDSPERCL)

// Count cache lines below/above for each subexpanse:

            for (subexp = 0; subexp < (int)cJU_NUMSUBEXPB; ++subexp)
            {
                jpcount = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, subexp));

// When at the subexpanse containing Index (digit), add cache lines
// below/above appropriately, excluding the cache line containing the JP for
// Index itself:

                if      (subexp <  findsub)  clbelow += CLPERJPS(jpcount);
                else if (subexp >  findsub)  clabove += CLPERJPS(jpcount);
                else // (subexp == findsub)
                {
                    Word_t clfind;      // cache line containing Index (digit).

                    clfind = CLPERJPS(j__udyCountBitsB(
                                    JU_JBB_BITMAP(Pjbb, subexp) & lowermask));

                    assert(clfind > 0);  // digit itself should have 1 CL.
                    clbelow += clfind - 1;
                    clabove += CLPERJPS(jpcount) - clfind;
                }
            }
#endif  // ! NOSMARTJBB

// COUNT POPULATION FOR A BITMAP BRANCH, in whichever direction should result
// in fewer cache line fills:
//
// Note:  If the remainder of Index is zero, pop1above is the pop1 of the
// entire expanse and theres no point in recursing to lower levels; but this
// should be so rare that its not worth checking for;
// Judy1Count()/JudyLCount() never even calls the motor for Index == 0 (all
// bytes).

// COUNT UPWARD, subtracting each "below or at" JPs pop1 from the whole
// expanses pop1:
//
// Note:  If this causes clbelow + 1 cache line fills including JPs cache
// line, thats OK; at worst this is the same as clabove.

            if (clbelow < clabove)
            {
                pop1above = pop1;               // subtract JPs at/below Index.

// Count JPs for which to accrue pop1s in this subexpanse:
//
// TBD:  If JU_JBB_BITMAP is cJU_FULLBITMAPB, dont bother counting.

                for (subexp = 0; subexp <= findsub; ++subexp)
                {
                    jpcount = j__udyCountBitsB((subexp < findsub) ?
                                      JU_JBB_BITMAP(Pjbb, subexp) :
                                      JU_JBB_BITMAP(Pjbb, subexp) & lowermask);

                    // should always find findbit:
                    assert((subexp < findsub) || jpcount);

// Subtract pop1s from JPs BELOW OR AT Index (digit):
//
// Note:  The pop1 for Indexs JP itself is partially added back later at a
// lower state.
//
// Note:  An empty subexpanse (jpcount == 0) is handled "for free".
//
// Note:  Must be null JP subexp pointer in empty subexpanse and non-empty in
// non-empty subexpanse:

                    for (jpnum = 0; jpnum < (long)jpcount; ++jpnum)
                    {
//printf("\nBranchB loop upper Line = %d\n", (int)__LINE__);
//                      pop1 = j__udyJPPop1(BMPJP(Pjbb, subexp, jpnum));
                        pop1 = j__udyJPPop1((P_JP(JU_JBB_PJP(Pjbb, subexp))) + jpnum);
                        pop1above -= pop1;
                    }
                    jpnum = jpcount - 1;        // make correct for digit.
                }
            }

// COUNT DOWNWARD, adding each "above" JPs pop1:

            else
            {
                long jpcountbf;                 // below findbit, inclusive.
                pop1above = 0;                  // add JPs above Index.
                jpcountbf = 0;                  // until subexp == findsub.

// Count JPs for which to accrue pop1s in this subexpanse:
//
// This is more complicated than counting upward because the scan of digits
// subexpanse must count ALL JPs, to know where to START counting down, and
// ALSO note the offset of digits JP to know where to STOP counting down.

                for (subexp = cJU_NUMSUBEXPB - 1; subexp >= findsub; --subexp)
                {
                    jpcount = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, subexp));

                    // should always find findbit:
                    assert((subexp > findsub) || jpcount);

                    if (! jpcount) continue;    // empty subexpanse, save time.

// Count JPs below digit, inclusive:

                    if (subexp == findsub)
                    {
                        jpcountbf = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, subexp)
                                                  & lowermask);
                    }

                    // should always find findbit:
                    assert((subexp > findsub) || jpcountbf);
                    assert(jpcount >= jpcountbf);       // proper relationship.

// Add pop1s from JPs ABOVE Index (digit):

                    // no null JP subexp pointers:

                    for (jpnum = jpcount - 1; jpnum >= jpcountbf; --jpnum)
                    {
//printf("\nBranchB loop lower Line = %d\n", (int)__LINE__);
//                      pop1 = j__udyJPPop1(BMPJP(Pjbb, subexp, jpnum));
                        pop1 = j__udyJPPop1((P_JP(JU_JBB_PJP(Pjbb, subexp))) + jpnum);
                        pop1above += pop1;
                    }
                    // jpnum is now correct for digit.
                }
            } // else.

// Return the net population ABOVE the digits JP at this state (in this JBB)
// plus the population AT OR ABOVE Index in the SM under the digits JP:

//printf("\nBranchB Line = %d\n", (int)__LINE__);
//          pop1 = j__udy1LCountSM(BMPJP(Pjbb, findsub, jpnum), Index, Pjpm);
            pop1 = j__udy1LCountSM(((P_JP(JU_JBB_PJP(Pjbb, findsub))) + jpnum), Index, Pjpm);
            return(pop1above + pop1);

        } // case.


// ----------------------------------------------------------------------------
// UNCOMPRESSED BRANCH; count populations in JPs in the JBU ABOVE the next
// digit in Index, and recurse for the next digit in Index:
//
// Note:  If the remainder of Index is zero, pop1above is the pop1 of the
// entire expanse and theres no point in recursing to lower levels; but this
// should be so rare that its not worth checking for;
// Judy1Count()/JudyLCount() never even calls the motor for Index == 0 (all
// bytes).

// Common code (state-independent) for all cases of uncompressed branches:

        case cJU_JPBRANCH_U2:
        case cJU_JPBRANCH_U3:
        case cJU_JPBRANCH_U4:
        case cJU_JPBRANCH_U5:
        case cJU_JPBRANCH_U6:
        case cJU_JPBRANCH_U7:
        case cJU_JPBRANCH_U8:
        {
            int level = ju_Type(Pjp) - cJU_JPBRANCH_U2 + 2;
            digit     = JU_DIGITATSTATE(Index, level);
            pop1above = ju_BranchPop0(Pjp, level) + 1;
            Pjbu      = P_JBU(RawPntr);

#ifndef noBigU
            jpnum     = digit / 8;                      // 0..31 loops
            for (int idx = 0; idx < jpnum; idx++)       // fast loop
                pop1above -= Pjbu->jbu_sums8[idx];
//          Do remaining 0..7 jp_t up to including digit
            jpnum *= 8;                                 // 0..7 loops left
#endif  // noBigU

            for ( ; jpnum <= digit; jpnum++)            // slow 
                 pop1above -= j__udyJPPop1(Pjbu->jbu_jp + jpnum);
            pop1 = j__udy1LCountSM(Pjbu->jbu_jp + digit, Index, Pjpm);
            return(pop1above + pop1);
        }

#ifdef  JUDYL
        case cJU_JPLEAF1:
        {
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf1(Pjll1, pop1, Index, 1 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }
#endif  // JUDYL

        case cJU_JPLEAF2:
        {
            Pjll2_t Pjll2 = P_JLL2(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf2(Pjll2, pop1, Index, 2 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPLEAF3:
        {
            Pjll3_t Pjll3 = P_JLL3(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf3(Pjll3, pop1, Index, 3 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPLEAF4:
        {
            Pjll4_t Pjll4 = P_JLL4(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf4(Pjll4, pop1, Index, 4 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPLEAF5:
        {
            Pjll5_t Pjll5 = P_JLL5(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf5(Pjll5, pop1, Index, 5 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPLEAF6:
        {
            Pjll6_t Pjll6 = P_JLL6(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf6(Pjll6, pop1, Index, 6 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPLEAF7:
        {
            Pjll7_t Pjll7 = P_JLL7(RawPntr);
            pop1 = ju_LeafPop1(Pjp);
            offset = j__udySearchLeaf7(Pjll7, pop1, Index, 7 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

// ----------------------------------------------------------------------------
// BITMAP LEAF; searcju_LeafPop1(Pjp)h the leaf for Index:
//
// Since the bitmap describes Indexes digitally rather than linearly, this is
// not really a search, but just a count.

        case cJU_JPLEAF_B1U:
        {
            Pjlb_t   Pjlb   = P_JLB1(RawPntr);
            pop1   = ju_LeafPop1(Pjp);
#ifdef  JUDYL
            if (pop1 == 0)      // really 256 in JudyL
                pop1 = 256;
//          Note: B1 cannot exceed Population of 255 in Judy1
#endif  // JUDYL
//                                                   64
            Word_t Bitmask = ((Word_t)1 << (Index % cbPW)) - 1;
            int    subexp  = (uint8_t)Index / cbPW;     // 64 bits / subexp
            int    offset  = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, subexp) & Bitmask);

//          Add offset from previous subexpanses (bitmaps) 0..3
            for (int idx = 0; idx < subexp; idx++)
                offset += j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, idx));

            return(pop1 - offset);
        }

#ifdef  JUDY1
// ----------------------------------------------------------------------------
// FULL POPULATION:
//
// Return the count of Indexes AT OR ABOVE Index, which is the total population
// of the expanse (a constant) less the value of the undecoded digit remaining
// in Index (its base-0 offset in the expanse), which yields an inclusive count
// above.
//
// TBD:  This only supports a 1-byte full expanse.  Should this extract a
// stored value for pop0 and possibly more LSBs of Index, to handle larger full
// expanses?

        case cJ1_JPFULLPOPU1:
        {
//printf("\ncJ1_JPFULLPOPU1, Index = 0x%lx\n", Index);
            return(256 - ((uint8_t)Index));
        }
#endif  // JUDY1


// ----------------------------------------------------------------------------
// IMMEDIATE:

        case cJU_JPIMMED_1_01:
        case cJU_JPIMMED_2_01:
        case cJU_JPIMMED_3_01:
        case cJU_JPIMMED_4_01:
        case cJU_JPIMMED_5_01:
        case cJU_JPIMMED_6_01:
        case cJU_JPIMMED_7_01:
        return(1);

        case cJU_JPIMMED_1_02:
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
#endif  // JUDY1
        {
            pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
            offset = j__udySearchImmed1(ju_PImmed1(Pjp), pop1, Index, 1 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPIMMED_2_02:
        case cJU_JPIMMED_2_03:
        case cJU_JPIMMED_2_04:
#ifdef  JUDY1
        case cJ1_JPIMMED_2_05:
        case cJ1_JPIMMED_2_06:
        case cJ1_JPIMMED_2_07:
#endif  // JUDY1
        {
            pop1 = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;
            offset = j__udySearchImmed2(ju_PImmed2(Pjp), pop1, Index, 2 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPIMMED_3_02:
#ifdef  JUDY1
        case cJ1_JPIMMED_3_03:
        case cJ1_JPIMMED_3_04:
        case cJ1_JPIMMED_3_05:
#endif  // JUDY1
        {
            pop1   = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
            offset = j__udySearchImmed3(ju_PImmed3(Pjp), pop1, Index, 3 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

        case cJU_JPIMMED_4_02:
#ifdef  JUDY1
        case cJ1_JPIMMED_4_03:
#endif  // JUDY1
        {
            pop1   = ju_Type(Pjp) - cJU_JPIMMED_4_02 + 2;
            offset = j__udySearchImmed4(ju_PImmed4(Pjp), pop1, Index, 4 * 8);
            assert(offset >= 0);
            return(pop1 - offset);
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_6_02:
        case cJ1_JPIMMED_7_02:
        {
// printf("\n Count---- Index = 0x%016lx\n", Index);
            offset = j__udy1SearchImm02(Pjp, Index);

//            printf("--Count cJ1_JPIMMED_%d_02 offset = %d\n", ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 5, offset);
            assert(offset >= 0);
            return(2 - offset);
        }
#endif  // JUDY1

// ----------------------------------------------------------------------------
// OTHER CASES:

        default:
        {
//printf("\nCount jp_Type = %d Line = %d\n", ju_Type(Pjp), (int)__LINE__);
            JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
            assert(0);
            return(0);
        }
        } // switch on JP type

        /*NOTREACHED*/

} // j__udy1LCountSM()


// ****************************************************************************
// J U D Y   C O U N T   L E A F   B 1
//
// This is a private analog of the j__udySearchLeaf*() functions for counting
// in bitmap 1-byte leaves.  Since a bitmap leaf describes Indexes digitally
// rather than linearly, this is not really a search, but just a count of the
// valid Indexes == set bits below or including Index, which should be valid.
// Return the "offset" of Index in Pjll;
// Note:  The source code for this function looks identical for both Judy1 and
// JudyL, but the JU_JLB_BITMAP macro varies.

FUNCTION static int j__udyCountLeafB1(
        Pjlb_t  Pjlb,           // bitmap leaf
        Word_t  Index)          // to which to count.
{
        Word_t  digit   = Index & cJU_MASKATSTATE(1);
        int     findsub = digit / cJU_BITSPERSUBEXPL;
        int     findbit = digit % cJU_BITSPERSUBEXPL;
        int     count;          // in leaf through Index.
        int     subexp;         // for stepping through subexpanses.

//      Get count for previous subexpanses (bitmaps)
        for (count = subexp = 0; subexp < findsub; subexp++)
                count += j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, subexp));

//      And add in last partial count matching Key
        count += j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, findsub)
            & JU_MASKLOWERINC(JU_BITPOSMASKL(findbit)));
        return(count);
} // j__udyCountLeafB1()


// ****************************************************************************
// J U D Y   J P   P O P 1
//
// This function takes any type of JP other than a root-level JP (cJU_LEAF8* or
// cJU_JPBRANCH* with no number suffix) and extracts the Pop1 from it.  In some
// sense this is a wrapper around the JU_JP*_POP0 macros.  Why write it as a
// function instead of a complex macro containing a trinary?  (See version
// Judy1.h version 4.17.)  We think its cheaper to call a function containing
// a switch statement with "constant" cases than to do the variable
// calculations in a trinary.
//
// For invalid JP Types return cJU_ALLONES.  Note that this is an impossibly
// high Pop1 for any JP below a top level branch.

FUNCTION Word_t j__udyJPPop1(
        Pjp_t Pjp)              // JP to count.
{
        switch (ju_Type(Pjp))
        {
        case cJU_JPNULL1:
        case cJU_JPNULL2:
        case cJU_JPNULL3:
        case cJU_JPNULL4:
        case cJU_JPNULL5:
        case cJU_JPNULL6:
        case cJU_JPNULL7:
            return(0);

        case cJU_JPBRANCH_L2:
        case cJU_JPBRANCH_B2:
        case cJU_JPBRANCH_U2:
            return(ju_BranchPop0(Pjp, 2) + 1);

        case cJU_JPBRANCH_L3:
        case cJU_JPBRANCH_B3:
        case cJU_JPBRANCH_U3:
            return(ju_BranchPop0(Pjp, 3) + 1);

        case cJU_JPBRANCH_L4:
        case cJU_JPBRANCH_B4:
        case cJU_JPBRANCH_U4:
            return(ju_BranchPop0(Pjp, 4) + 1);

        case cJU_JPBRANCH_L5:
        case cJU_JPBRANCH_B5:
        case cJU_JPBRANCH_U5:
            return(ju_BranchPop0(Pjp, 5) + 1);

        case cJU_JPBRANCH_L6:
        case cJU_JPBRANCH_B6:
        case cJU_JPBRANCH_U6:
            return(ju_BranchPop0(Pjp, 6) + 1);

        case cJU_JPBRANCH_L7:
        case cJU_JPBRANCH_B7:
        case cJU_JPBRANCH_U7:
            return(ju_BranchPop0(Pjp, 7) + 1);

#ifdef  JUDYL
        case cJU_JPLEAF1:
#endif  // JUDYL
        case cJU_JPLEAF2:
        case cJU_JPLEAF3:
        case cJU_JPLEAF4:
        case cJU_JPLEAF5:
        case cJU_JPLEAF6:
        case cJU_JPLEAF7:
             return(ju_LeafPop1(Pjp));

        case cJU_JPLEAF_B1U:
        {
            int pop1 = ju_LeafPop1(Pjp);
#ifdef  JUDYL
            if (pop1 == 0) pop1 = 256;
#endif  // JUDYL
             return(pop1);
        }

#ifdef  JUDY1
        case cJ1_JPFULLPOPU1:
           return(cJU_JPFULLPOPU1_POP0 + 1);
#endif  // JUDY1
        case cJU_JPIMMED_1_01:
        case cJU_JPIMMED_2_01:
        case cJU_JPIMMED_3_01:
        case cJU_JPIMMED_4_01:
        case cJU_JPIMMED_5_01:
        case cJU_JPIMMED_6_01:
        case cJU_JPIMMED_7_01:
            return(1);

        case cJU_JPIMMED_1_02:  return(2);
        case cJU_JPIMMED_1_03:  return(3);
        case cJU_JPIMMED_1_04:  return(4);
        case cJU_JPIMMED_1_05:  return(5);
        case cJU_JPIMMED_1_06:  return(6);
        case cJU_JPIMMED_1_07:  return(7);
        case cJU_JPIMMED_1_08:  return(8);
#ifdef  JUDY1
        case cJ1_JPIMMED_1_09:  return(9);
        case cJ1_JPIMMED_1_10:  return(10);
        case cJ1_JPIMMED_1_11:  return(11);
        case cJ1_JPIMMED_1_12:  return(12);
        case cJ1_JPIMMED_1_13:  return(13);
        case cJ1_JPIMMED_1_14:  return(14);
        case cJ1_JPIMMED_1_15:  return(15);
#endif  // JUDY1
        case cJU_JPIMMED_2_02:  return(2);
        case cJU_JPIMMED_2_03:  return(3);
        case cJU_JPIMMED_2_04:  return(4);
#ifdef  JUDY1
        case cJ1_JPIMMED_2_05:  return(5);
        case cJ1_JPIMMED_2_06:  return(6);
        case cJ1_JPIMMED_2_07:  return(7);
#endif  // JUDY1

#ifdef  JUDY1
#endif  // JUDY1

        case cJU_JPIMMED_3_02:
        case cJU_JPIMMED_4_02:
            return(2);

#ifdef  JUDY1
        case cJ1_JPIMMED_3_03:
        case cJ1_JPIMMED_4_03:
            return(3);

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_6_02:
        case cJ1_JPIMMED_7_02:
            return(2);
#endif  // JUDY1

        default:
//printf("in j__udyJPPop1 jp_Type = %d Line = %d\n", ju_Type(Pjp), (int)__LINE__);
            assert(FALSE);
            exit(-1);
        }
        /*NOTREACHED*/

} // j__udyJPPop1()
