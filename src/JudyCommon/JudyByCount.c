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

// @(#) $Revision: 4.28 $ $Source: /judy/src/JudyCommon/JudyByCount.c $
//
// Judy*ByCount() function for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.
//
// Compile with -DNOSMARTJBB, -DNOSMARTJBU, and/or -DNOSMARTJLB to build a
// version with cache line optimizations deleted, for testing.
//
// Judy*ByCount() is a conceptual although not literal inverse of Judy*Count().
// Judy*Count() takes a pair of Indexes, and allows finding the ordinal of a
// given Index (that is, its position in the list of valid indexes from the
// beginning) as a degenerate case, because in general the count between two
// Indexes, inclusive, is not always just the difference in their ordinals.
// However, it suffices for Judy*ByCount() to simply be an ordinal-to-Index
// mapper.
//
// Note:  Like Judy*Count(), this code must "count sideways" in branches, which
// can result in a lot of cache line fills.  However, unlike Judy*Count(), this
// code does not receive a specific Index, hence digit, where to start in each
// branch, so it cant accurately calculate cache line fills required in each
// direction.  The best it can do is an approximation based on the total
// population of the expanse (pop1 from Pjp) and the ordinal of the target
// Index (see SETOFFSET()) within the expanse.
//
// Compile with -DSMARTMETRICS to obtain global variables containing smart
// cache line metrics.  Note:  Dont turn this on simultaneously for this file
// and JudyCount.c because they export the same globals.
// ****************************************************************************

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"

// These are imported from JudyCount.c:
//
// TBD:  Should this be in common code?  Exported from a header file?

#ifdef JUDY1
extern	Word_t j__udy1JPPop1(const Pjp_t Pjp);
#define	j__udyJPPop1 j__udy1JPPop1
#else
extern	Word_t j__udyLJPPop1(const Pjp_t Pjp);
#define	j__udyJPPop1 j__udyLJPPop1
#endif

// Avoid duplicate symbols since this file is multi-compiled:

#ifdef SMARTMETRICS
#ifdef JUDY1
Word_t	jbb_upward   = 0;	// counts of directions taken:
Word_t	jbb_downward = 0;
Word_t	jbu_upward   = 0;
Word_t	jbu_downward = 0;
Word_t	jlb_upward   = 0;
Word_t	jlb_downward = 0;
#else
extern Word_t jbb_upward;
extern Word_t jbb_downward;
extern Word_t jbu_upward;
extern Word_t jbu_downward;
extern Word_t jlb_upward;
extern Word_t jlb_downward;
#endif
#endif


// ****************************************************************************
// J U D Y   1   B Y   C O U N T
// J U D Y   L   B Y   C O U N T
//
// See the manual entry.

#ifdef JUDY1
FUNCTION int Judy1ByCount
#else
FUNCTION PPvoid_t JudyLByCount
#endif
        (
	Pcvoid_t  PArray,	// root pointer to first branch/leaf in SM.
	Word_t	  Count,	// ordinal of Index to find, 1..MAX.
	Word_t *  PIndex,	// to return found Index.
	PJError_t PJError	// optional, for returning error info.
        )
{
	Word_t	  Count0;	// Count, base-0, to match pop0.
	Word_t	  state;	// current state in SM.
	Word_t	  pop1;		// of current branch or leaf, or of expanse.
	Word_t	  pop1lower;	// pop1 of expanses (JPs) below that for Count.
	Word_t	  digit;	// current word in branch.
	Word_t	  jpcount;	// JPs in a BranchB subexpanse.
        Word_t    RawPntr;      // current Raw pointer in jp_t
	long	  jpnum;	// JP number in a branch (base 0).
	long	  subexp;	// for stepping through layer 1 (subexpanses).
	int	  offset;	// index ordinal within a leaf, base 0.

	Pjp_t	  Pjp;		// current JP in branch.

// CHECK FOR EMPTY ARRAY OR NULL PINDEX:

	if (PArray == (Pvoid_t) NULL) JU_RET_NOTFOUND;

	if (PIndex == (PWord_t) NULL)
	{
	    JU_SET_ERRNO(PJError, JU_ERRNO_NULLPINDEX);
	    JUDY1CODE(return(JERRI );)
	    JUDYLCODE(return(PPJERR);)
	}

// Convert Count to Count0; assume special case of Count = 0 maps to ~0, as
// desired, to represent the last index in a full array:
//
// Note:  Think of Count0 as a reliable "number of Indexes below the target."

	Count0 = Count - 1;
	assert((Count || Count0 == ~0));  // ensure CPU is sane about 0 - 1.
	pop1lower = 0;

	if (JU_LEAF8_POP0(PArray) < cJU_LEAF8_MAXPOP1) // must be a LEAF8
	{
	    Pjll8_t Pjll8 = P_JLL8(PArray);		// first word of leaf.

	    if (Count0 > Pjll8->jl8_Population0) JU_RET_NOTFOUND;	// too high.

//	    *PIndex = Pjll8[Count];			// Index, base 1.
	    *PIndex = Pjll8->jl8_Leaf[Count0];		// Index, base 1.

	    JU_RET_FOUND_LEAF8(Pjll8, Pjll8->jl8_Population0 + 1, Count0);
	}
	else
	{
	    Pjpm_t Pjpm = P_JPM(PArray);

	    if (Count0 > (Pjpm->jpm_Pop0)) JU_RET_NOTFOUND;	// too high.

//	    Pjp  = &(Pjpm->jpm_JP);
	    Pjp  = Pjpm->jpm_JP + 0;
	    pop1 =  (Pjpm->jpm_Pop0) + 1;

	    goto SMByCount;
	}

// COMMON CODE:
//
// Prepare to handle a root-level or lower-level branch:  Save the current
// state, obtain the total population for the branch in a state-dependent way,
// and then branch to common code for multiple cases.
//
// For root-level branches, the state is always cJU_ROOTSTATE, and the array
// population must already be set in pop1; it is not available in jp_DcdPopO.
//
// Note:  The total population is only needed in cases where the common code
// "counts down" instead of up to minimize cache line fills.  However, its
// available cheaply, and its better to do it with a constant shift (constant
// state value) instead of a variable shift later "when needed".

#define	PREPB_ROOT(Next)	\
	state = cJU_ROOTSTATE;	\
	goto Next

// Use PREP_BRANCH_DCD() to first copy the Dcd bytes to *PIndex if there are any
// (only if state < cJU_ROOTSTATE - 1):

#define	PREP_BRANCH_DCD(Pjp,cState,Next)			        \
/* _SET_BRANCH_DCD(*PIndex, Pjp, cState);               */              \
        *PIndex = JU_MERGE_DCD_KEY(*PIndex, ju_DcdPop0(Pjp), cState);   \
	PREPB((Pjp), cState, Next)

#define	PREPB(Pjp,cState,Next)	\
	state = (cState);	\
	pop1  = JU_JPBRANCH_POP0(Pjp, (cState)) + 1; \
	goto Next

// Calculate whether the ordinal of an Index within a given expanse falls in
// the lower or upper half of the expanses population, taking care with
// unsigned math and boundary conditions:
//
// Note:  Assume the ordinal falls within the expanses population, that is,
// 0 < (Count - Pop1lower) <= Pop1exp (assuming infinite math).
//
// Note:  If the ordinal is the middle element, it doesnt matter whether
// LOWERHALF() is TRUE or FALSE.

#define	LOWERHALF(Count0,Pop1lower,Pop1exp) \
	(((Count0) - (Pop1lower)) < ((Pop1exp) / 2))

// Calculate the (signed) offset within a leaf to the desired ordinal (Count -
// Pop1lower; offset is one less), and optionally ensure its in range:

#define	SETOFFSET(Offset,Count0,Pop1lower,Pjp)	\
	(Offset) = (Count0) - (Pop1lower);	\
	assert((Offset) >= 0);			\
	assert((Offset) <= (ju_LeafPop1(Pjp) - 1))

// Variations for immediate indexes, with and without pop1-specific assertions:

#define	SETOFFSET_IMM_CK(Offset,Count0,Pop1lower,cPop1)	\
	(Offset) = (Count0) - (Pop1lower);		\
	assert((Offset) >= 0);				\
	assert((Offset) <  (cPop1))

//#define	SETOFFSET_IMM(Offset,Count0,Pop1lower) (Offset) = (Count0) - (Pop1lower)


// STATE MACHINE -- TRAVERSE TREE:
//
// In branches, look for the expanse (digit), if any, where the total pop1
// below or at that expanse would meet or exceed Count, meaning the Index must
// be in this expanse.

SMByCount:			// return here for next branch/leaf.

        RawPntr = ju_PntrInJp(Pjp);
	switch (ju_Type(Pjp))
	{


// ----------------------------------------------------------------------------
// LINEAR BRANCH; count populations in JPs in the JBL upwards until finding the
// expanse (digit) containing Count, and "recurse".
//
// Note:  There are no null JPs in a JBL; watch out for pop1 == 0.
//
// Note:  A JBL should always fit in one cache line => no need to count up
// versus down to save cache line fills.
//
// TBD:  The previous is no longer true.  Consider enhancing this code to count
// up/down, but it can wait for a later tuning phase.  In the meantime, PREPB()
// sets pop1 for the whole array, but that value is not used here.  001215:
// Maybe its true again?

	case cJU_JPBRANCH_L2:  PREP_BRANCH_DCD(Pjp, 2, BranchL);
	case cJU_JPBRANCH_L3:  PREP_BRANCH_DCD(Pjp, 3, BranchL);
	case cJU_JPBRANCH_L4:  PREP_BRANCH_DCD(Pjp, 4, BranchL);
	case cJU_JPBRANCH_L5:  PREP_BRANCH_DCD(Pjp, 5, BranchL);
	case cJU_JPBRANCH_L6:  PREP_BRANCH_DCD(Pjp, 6, BranchL);
	case cJU_JPBRANCH_L7:  PREP_BRANCH_DCD(Pjp, 7, BranchL);
        case cJU_JPBRANCH_L8:  PREPB_ROOT(	 BranchL);
	{
	    Pjbl_t Pjbl;

// Common code (state-independent) for all cases of linear branches:

BranchL:
	    Pjbl = P_JBL(RawPntr);

	    for (jpnum = 0; jpnum < (Pjbl->jbl_NumJPs); ++jpnum)
	    {
	        if ((pop1 = j__udyJPPop1((Pjbl->jbl_jp) + jpnum)) == cJU_ALLONES)
	        {
		    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
                    assert(0);
		    JUDY1CODE(return(JERRI );)
		    JUDYLCODE(return(PPJERR);)
	        }
	        assert(pop1 != 0);

// Warning:  pop1lower and pop1 are unsigned, so do not subtract 1 and compare
// >=, but instead use the following expression:

	        if (pop1lower + pop1 > Count0)	 // Index is in this expanse.
	        {
		    JU_SETDIGIT(*PIndex, Pjbl->jbl_Expanse[jpnum], state);
		    Pjp = (Pjbl->jbl_jp) + jpnum;
		    goto SMByCount;			// look under this expanse.
	        }

	        pop1lower += pop1;			// add this JPs pop1.
	    }

	    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);  // should never get here.
            assert(0);
	    JUDY1CODE(return(JERRI );)
	    JUDYLCODE(return(PPJERR);)

	} // case cJU_JPBRANCH_L


// ----------------------------------------------------------------------------
// BITMAP BRANCH; count populations in JPs in the JBB upwards or downwards
// until finding the expanse (digit) containing Count, and "recurse".
//
// Note:  There are no null JPs in a JBB; watch out for pop1 == 0.

	case cJU_JPBRANCH_B2:  PREP_BRANCH_DCD(Pjp, 2, BranchB);
	case cJU_JPBRANCH_B3:  PREP_BRANCH_DCD(Pjp, 3, BranchB);
	case cJU_JPBRANCH_B4:  PREP_BRANCH_DCD(Pjp, 4, BranchB);
	case cJU_JPBRANCH_B5:  PREP_BRANCH_DCD(Pjp, 5, BranchB);
	case cJU_JPBRANCH_B6:  PREP_BRANCH_DCD(Pjp, 6, BranchB);
//	case cJU_JPBRANCH_B7:  PREPB(	 Pjp, 7, BranchB);
	case cJU_JPBRANCH_B7:  PREP_BRANCH_DCD(Pjp, 7, BranchB);
	case cJU_JPBRANCH_B8:   PREPB_ROOT(	 BranchB);
	{
	    Pjbb_t Pjbb;

// Common code (state-independent) for all cases of bitmap branches:

BranchB:
	    Pjbb = P_JBB(RawPntr);

// Shorthand for one subexpanse in a bitmap and for one JP in a bitmap branch:
//
// Note: BMPJP0 exists separately to support assertions.

#define	BMPJP0(Subexp)	     (P_JP(JU_JBB_PJP(Pjbb, Subexp)))
#define	BMPJP(Subexp,JPnum)  (BMPJP0(Subexp) + (JPnum))


// Common code for descending through a JP:
//
// Determine the digit for the expanse and save it in *PIndex; then "recurse".

#define	JBB_FOUNDEXPANSE			\
	{					\
	    JU_BITMAPDIGITB(digit, subexp, JU_JBB_BITMAP(Pjbb,subexp), jpnum); \
	    JU_SETDIGIT(*PIndex, digit, state);	\
	    Pjp = BMPJP(subexp, jpnum);		\
	    goto SMByCount;			\
	}


#ifndef NOSMARTJBB  // enable to turn off smart code for comparison purposes.

// FIGURE OUT WHICH DIRECTION CAUSES FEWER CACHE LINE FILLS; adding the pop1s
// in JPs upwards, or subtracting the pop1s in JPs downwards:
//
// See header comments about limitations of this for Judy*ByCount().

#endif

// COUNT UPWARD, adding each "below" JPs pop1:

#ifndef NOSMARTJBB  // enable to turn off smart code for comparison purposes.

	    if (LOWERHALF(Count0, pop1lower, pop1))
	    {
#endif
#ifdef SMARTMETRICS
		++jbb_upward;
#endif
		for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
		{
		    if ((jpcount = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb,subexp)))
		     && (BMPJP0(subexp) == (Pjp_t) NULL))
		    {
			JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);  // null ptr.
                        assert(0);
			JUDY1CODE(return(JERRI );)
			JUDYLCODE(return(PPJERR);)
		    }

// Note:  An empty subexpanse (jpcount == 0) is handled "for free":

		    for (jpnum = 0; jpnum < jpcount; ++jpnum)
		    {
			if ((pop1 = j__udyJPPop1(BMPJP(subexp, jpnum)))
			  == cJU_ALLONES)
			{
			    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
                            assert(0);
			    JUDY1CODE(return(JERRI );)
			    JUDYLCODE(return(PPJERR);)
			}
			assert(pop1 != 0);

// Warning:  pop1lower and pop1 are unsigned, see earlier comment:

			if ((pop1lower + pop1) > Count0)
			    JBB_FOUNDEXPANSE;	// Index is in this expanse.

			pop1lower += pop1;	// add this JPs pop1.
		    }
		}
#ifndef NOSMARTJBB  // enable to turn off smart code for comparison purposes.
	    }


// COUNT DOWNWARD, subtracting each "above" JPs pop1 from the whole expanses
// pop1:

	    else
	    {
#ifdef SMARTMETRICS
		++jbb_downward;
#endif
		pop1lower += pop1;		// add whole branch to start.

		for (subexp = cJU_NUMSUBEXPB - 1; subexp >= 0; --subexp)
		{
		    if ((jpcount = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, subexp)))
		     && (BMPJP0(subexp) == (Pjp_t) NULL))
		    {
			JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);  // null ptr.
                        assert(0);
			JUDY1CODE(return(JERRI );)
			JUDYLCODE(return(PPJERR);)
		    }

// Note:  An empty subexpanse (jpcount == 0) is handled "for free":

		    for (jpnum = jpcount - 1; jpnum >= 0; --jpnum)
		    {
			if ((pop1 = j__udyJPPop1(BMPJP(subexp, jpnum)))
			  == cJU_ALLONES)
			{
			    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
                            assert(0);
			    JUDY1CODE(return(JERRI );)
			    JUDYLCODE(return(PPJERR);)
			}
			assert(pop1 != 0);

// Warning:  pop1lower and pop1 are unsigned, see earlier comment:

			pop1lower -= pop1;

// Beware unsigned math problems:

			if ((pop1lower == 0) || (pop1lower - 1 < Count0))
			    JBB_FOUNDEXPANSE;	// Index is in this expanse.
		    }
		}
	    }
#endif // NOSMARTJBB

	    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);  // should never get here.
            assert(0);
	    JUDY1CODE(return(JERRI );)
	    JUDYLCODE(return(PPJERR);)

	} // case cJU_JPBRANCH_B


// ----------------------------------------------------------------------------
// UNCOMPRESSED BRANCH; count populations in JPs in the JBU upwards or
// downwards until finding the expanse (digit) containing Count, and "recurse".

	case cJU_JPBRANCH_U2:  PREP_BRANCH_DCD(Pjp, 2, BranchU);
	case cJU_JPBRANCH_U3:  PREP_BRANCH_DCD(Pjp, 3, BranchU);
	case cJU_JPBRANCH_U4:  PREP_BRANCH_DCD(Pjp, 4, BranchU);
	case cJU_JPBRANCH_U5:  PREP_BRANCH_DCD(Pjp, 5, BranchU);
	case cJU_JPBRANCH_U6:  PREP_BRANCH_DCD(Pjp, 6, BranchU);
//	case cJU_JPBRANCH_U7:  PREPB(	 Pjp, 7, BranchU);
	case cJU_JPBRANCH_U7:  PREP_BRANCH_DCD(Pjp, 7, BranchU);
	case cJU_JPBRANCH_U8:   PREPB_ROOT(	 BranchU);
	{
	    Pjbu_t Pjbu;

// Common code (state-independent) for all cases of uncompressed branches:

BranchU:
	    Pjbu = P_JBU(RawPntr);

// Common code for descending through a JP:
//
// Save the digit for the expanse in *PIndex, then "recurse".

#define	JBU_FOUNDEXPANSE			\
	{					\
	    JU_SETDIGIT(*PIndex, jpnum, state);	\
	    Pjp = (Pjbu->jbu_jp) + jpnum;	\
	    goto SMByCount;			\
	}


#ifndef NOSMARTJBU  // enable to turn off smart code for comparison purposes.

// FIGURE OUT WHICH DIRECTION CAUSES FEWER CACHE LINE FILLS; adding the pop1s
// in JPs upwards, or subtracting the pop1s in JPs downwards:
//
// See header comments about limitations of this for Judy*ByCount().

#endif

// COUNT UPWARD, simply adding the pop1 of each JP:

#ifndef NOSMARTJBU  // enable to turn off smart code for comparison purposes.

	    if (LOWERHALF(Count0, pop1lower, pop1))
	    {
#endif
#ifdef SMARTMETRICS
		++jbu_upward;
#endif

		for (jpnum = 0; jpnum < cJU_BRANCHUNUMJPS; ++jpnum)
		{
		    // shortcut, save a function call:
//		    if ((Pjbu->jbu_jp[jpnum].jp_Type) <= cJU_JPNULLMAX)
		    if (ju_Type(Pjbu->jbu_jp + jpnum) <= cJU_JPNULLMAX)
			continue;

		    if ((pop1 = j__udyJPPop1((Pjbu->jbu_jp) + jpnum))
		     == cJU_ALLONES)
		    {
			JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
                            assert(0);
			JUDY1CODE(return(JERRI );)
			JUDYLCODE(return(PPJERR);)
		    }
		    assert(pop1 != 0);

// Warning:  pop1lower and pop1 are unsigned, see earlier comment:

		    if (pop1lower + pop1 > Count0)
			JBU_FOUNDEXPANSE;	// Index is in this expanse.

		    pop1lower += pop1;		// add this JPs pop1.
		}
#ifndef NOSMARTJBU  // enable to turn off smart code for comparison purposes.
	    }


// COUNT DOWNWARD, subtracting the pop1 of each JP above from the whole
// expanses pop1:

	    else
	    {
#ifdef SMARTMETRICS
		++jbu_downward;
#endif
		pop1lower += pop1;		// add whole branch to start.

		for (jpnum = cJU_BRANCHUNUMJPS - 1; jpnum >= 0; --jpnum)
		{
		    // shortcut, save a function call:
//		    if ((Pjbu->jbu_jp[jpnum].jp_Type) <= cJU_JPNULLMAX)
		    if (ju_Type(Pjbu->jbu_jp + jpnum) <= cJU_JPNULLMAX)
			continue;

		    if ((pop1 = j__udyJPPop1(Pjbu->jbu_jp + jpnum))
		     == cJU_ALLONES)
		    {
			JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
                            assert(0);
			JUDY1CODE(return(JERRI );)
			JUDYLCODE(return(PPJERR);)
		    }
		    assert(pop1 != 0);

// Warning:  pop1lower and pop1 are unsigned, see earlier comment:

		    pop1lower -= pop1;

// Beware unsigned math problems:

		    if ((pop1lower == 0) || (pop1lower - 1 < Count0))
			JBU_FOUNDEXPANSE;	// Index is in this expanse.
		}
	    }
#endif // NOSMARTJBU

	    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);  // should never get here.
                            assert(0);
	    JUDY1CODE(return(JERRI );)
	    JUDYLCODE(return(PPJERR);)

	} // case cJU_JPBRANCH_U

// ----------------------------------------------------------------------------
// LINEAR LEAF:
//
// Return the Index at the proper ordinal (see SETOFFSET()) in the leaf.  First
// copy Dcd bytes, if there are any (only if state < cJU_ROOTSTATE - 1), to
// *PIndex.
//
// Note:  The preceding branch traversal code MIGHT set pop1 for this expanse
// (linear leaf) as a side-effect, but dont depend on that (for JUDYL, which
// is the only cases that need it anyway).

#ifdef JUDYL
	case cJU_JPLEAF1:
        {
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 1);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll1->jl1_LastKey, 1);
	    offset  = Count0 - pop1lower;
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll1->jl1_Leaf[offset], 1);
//	    JU_SETDIGIT1(*PIndex, Pjll1->jl1_Leaf[offset]);
	    pop1 = ju_LeafPop1(Pjp);
	    JU_RET_FOUND_LEAF1(Pjll1, pop1, offset);
        }
#endif  //JUDYL

	case cJU_JPLEAF2:
        {
	    Pjll2_t Pjll2 = P_JLL2(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 2);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll2->jl2_LastKey, 2);
#ifdef JUDYL
	    pop1 = ju_LeafPop1(Pjp);
#endif  //JUDYL
	    offset  = Count0 - pop1lower;
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll2->jl2_Leaf[offset], 2); //  *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(2))) | Pjll2->jl2_Leaf[offset];
	    JU_RET_FOUND_LEAF2(Pjll2, pop1, offset);
        }

	case cJU_JPLEAF3:
	{
	    Pjll3_t Pjll3 = P_JLL3(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 3);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll3->jl3_LastKey, 3);
#ifdef JUDYL
	    pop1 = ju_LeafPop1(Pjp);
#endif  //JUDYL
	    offset  = Count0 - pop1lower;
	    Word_t lsb = Pjll3->jl3_Leaf[offset];
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, lsb, 3); //    *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(3))) | lsb;
	    JU_RET_FOUND_LEAF3(Pjll3, pop1, offset);
	}

	case cJU_JPLEAF4:
        {
	    Pjll4_t Pjll4 = P_JLL4(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 4);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll4->jl4_LastKey, 4);
#ifdef JUDYL
	    pop1 = ju_LeafPop1(Pjp);
#endif  //JUDYL
	    offset  = Count0 - pop1lower;
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll4->jl4_Leaf[offset], 4); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(4))) | Pjll4->jl4_Leaf[offset];
	    JU_RET_FOUND_LEAF4(Pjll4, pop1, offset);
        }

	case cJU_JPLEAF5:
	{
	    Pjll5_t Pjll5 = P_JLL5(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 5);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll5->jl5_LastKey, 5);
#ifdef JUDYL
	    pop1 = ju_LeafPop1(Pjp);
#endif  //JUDYL
	    offset  = Count0 - pop1lower;
	    Word_t lsb = Pjll5->jl5_Leaf[offset];
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, lsb, 5); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(5))) | lsb;
	    JU_RET_FOUND_LEAF5(Pjll5, pop1, offset);
	}

	case cJU_JPLEAF6:
	{
	    Pjll6_t Pjll6 = P_JLL6(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 6);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll6->jl6_LastKey, 6);
#ifdef JUDYL
	    pop1 = ju_LeafPop1(Pjp);
#endif  //JUDYL
	    offset  = Count0 - pop1lower;
	    Word_t lsb = Pjll6->jl6_Leaf[offset];
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, lsb, 6); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(6))) | lsb;
	    JU_RET_FOUND_LEAF6(Pjll6, pop1, offset);
	}

	case cJU_JPLEAF7:
	{
	    Pjll7_t Pjll7 = P_JLL7(RawPntr);
//	    JU_SETLEAFDCD(*PIndex, Pjp, 7);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjll7->jl7_LastKey, 7);
#ifdef JUDYL
	    pop1 = ju_LeafPop1(Pjp);
#endif  //JUDYL
	    offset  = Count0 - pop1lower;
	    Word_t lsb = Pjll7->jl7_Leaf[offset];
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, lsb, 7); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(7))) | lsb;
	    JU_RET_FOUND_LEAF7(Pjll7, pop1, offset);
	}


// ----------------------------------------------------------------------------
// BITMAP LEAF:
//
// Return the Index at the proper ordinal (see SETOFFSET()) in the leaf by
// counting bits.  First copy Dcd bytes (always present since state 1 <
// cJU_ROOTSTATE) to *PIndex.
//
// Note:  The preceding branch traversal code MIGHT set pop1 for this expanse
// (bitmap leaf) as a side-effect, but dont depend on that.

	case cJU_JPLEAF_B1U:
	{
	    Pjlb_t Pjlb = P_JLB1(RawPntr);
	    *PIndex = JU_MERGE_DCD_KEY(*PIndex, Pjlb->jlb_LastKey, 1); // JU_SETLEAFDCD(*PIndex, Pjp, 1);
	    pop1 = ju_LeafPop1(Pjp);

// COUNT UPWARD, adding the pop1 of each subexpanse:
//
// The entire bitmap should fit in one cache line, but still try to save some
// CPU time by counting the fewest possible number of subexpanses from the
// bitmap.
//
// See header comments about limitations of this for Judy*ByCount().

#ifndef NOSMARTJLB  // enable to turn off smart code for comparison purposes.

	    if (LOWERHALF(Count0, pop1lower, pop1))
	    {
#endif
#ifdef SMARTMETRICS
		++jlb_upward;
#endif
		for (subexp = 0; subexp < cJU_NUMSUBEXPL; ++subexp)
		{
		    pop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, subexp));

// Warning:  pop1lower and pop1 are unsigned, see earlier comment:

		    if (pop1lower + pop1 > Count0)
			goto LeafB1;		// Index is in this subexpanse.

		    pop1lower += pop1;		// add this subexpanses pop1.
		}
#ifndef NOSMARTJLB  // enable to turn off smart code for comparison purposes.
	    }


// COUNT DOWNWARD, subtracting each "above" subexpanses pop1 from the whole
// expanses pop1:

	    else
	    {
#ifdef SMARTMETRICS
		++jlb_downward;
#endif
		pop1lower += pop1;		// add whole leaf to start.

		for (subexp = cJU_NUMSUBEXPL - 1; subexp >= 0; --subexp)
		{
		    pop1lower -= j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, subexp));

// Beware unsigned math problems:

		    if ((pop1lower == 0) || (pop1lower - 1 < Count0))
			goto LeafB1;		// Index is in this subexpanse.
		}
	    }
#endif // NOSMARTJLB

	    JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);  // should never get here.
                            assert(0);
	    JUDY1CODE(return(JERRI );)
	    JUDYLCODE(return(PPJERR);)


// RETURN INDEX FOUND:
//
// Come here with subexp set to the correct subexpanse, and pop1lower set to
// the sum for all lower expanses and subexpanses in the Judy tree.  Calculate
// and save in *PIndex the digit corresponding to the ordinal in this
// subexpanse.

LeafB1:
            offset = Count0 - pop1lower;        // SETOFFSET(offset, Count0, pop1lower, Pjp);
	    JU_BITMAPDIGITL(digit, subexp, JU_JLB_BITMAP(Pjlb, subexp), offset);
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, digit, 1); // JU_SETDIGIT1(*PIndex, digit);
	    JU_RET_FOUND_LEAF_B1U(Pjlb, subexp, offset);
//	    == return((PPvoid_t) (P_JV(JL_JLB_PVALUE(Pjlb, subexp)) + offset))

	} // case cJU_JPLEAF_B1


#ifdef JUDY1
// ----------------------------------------------------------------------------
// FULL POPULATION:
//
// Copy Dcd bytes (always present since state 1 < cJU_ROOTSTATE) to *PIndex,
// then set the appropriate digit for the ordinal (see SETOFFSET()) in the leaf
// as the LSB in *PIndex.

	case cJ1_JPFULLPOPU1:

            *PIndex = JU_MERGE_DCD_KEY(*PIndex, ju_DcdPop0(Pjp), 1); // JU_SETBRANCHDCD(*PIndex, Pjp, 1);
            offset = Count0 - pop1lower;        // SETOFFSET(offset, Count0, pop1lower, Pjp);
	    assert(offset >= 0);
	    assert(offset <= cJU_JPFULLPOPU1_POP0);
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, offset, 1); // JU_SETDIGIT1(*PIndex, offset);
            return(1);  // JU_RET_FOUND_FULLPOPU1;
#endif  // JUDY1


// ----------------------------------------------------------------------------
// IMMEDIATE:
//
// Locate the Index with the proper ordinal (see SETOFFSET()) in the Immediate,
// depending on leaf Index Size and pop1.  Note:  There are no Dcd bytes in an
// Immediate JP, but in a cJU_JPIMMED_*_01 JP, the field holds the least bytes
// of the immediate Index.

#define	SET_01(cState)  JU_SETDIGITS(*PIndex, ju_IMM01Key(Pjp), cState)

	case cJU_JPIMMED_1_01: SET_01(1); goto Imm_01;
	case cJU_JPIMMED_2_01: SET_01(2); goto Imm_01;
	case cJU_JPIMMED_3_01: SET_01(3); goto Imm_01;
	case cJU_JPIMMED_4_01: SET_01(4); goto Imm_01;
	case cJU_JPIMMED_5_01: SET_01(5); goto Imm_01;
	case cJU_JPIMMED_6_01: SET_01(6); goto Imm_01;
	case cJU_JPIMMED_7_01: SET_01(7); goto Imm_01;

Imm_01:
	    DBGCODE(SETOFFSET_IMM_CK(offset, Count0, pop1lower, 1);)
	    JU_RET_FOUND_IMM_01(Pjp);

// Shorthand for where to find start of Index bytes array:
//
// Optional code to check the remaining ordinal (see SETOFFSET_IMM()) against
// the Index Size of the Immediate:

// #ifndef DEBUG				// simple placeholder:
// #define	IMM(cPop1,Next) 
// 	goto Next
// #else					// extra pop1-specific checking:
// #define	IMM(cPop1,Next)						
// 	SETOFFSET_IMM_CK(offset, Count0, pop1lower, cPop1);	
// 	goto Next
// #endif

	case cJU_JPIMMED_1_02:          // IMM( 2, Imm1);
	case cJU_JPIMMED_1_03:          // IMM( 3, Imm1);
	case cJU_JPIMMED_1_04:          // IMM( 4, Imm1);
	case cJU_JPIMMED_1_05:          // IMM( 5, Imm1);
	case cJU_JPIMMED_1_06:          // IMM( 6, Imm1);
	case cJU_JPIMMED_1_07:          // IMM( 7, Imm1);
	case cJU_JPIMMED_1_08:          // IMM( 8, Imm1);

#ifdef  JUDY1
	case cJ1_JPIMMED_1_09:          // IMM( 9, Imm1);
	case cJ1_JPIMMED_1_10:          // IMM(10, Imm1);
	case cJ1_JPIMMED_1_11:          // IMM(11, Imm1);
	case cJ1_JPIMMED_1_12:          // IMM(12, Imm1);
	case cJ1_JPIMMED_1_13:          // IMM(13, Imm1);
	case cJ1_JPIMMED_1_14:          // IMM(14, Imm1);
	case cJ1_JPIMMED_1_15:          // IMM(15, Imm1);
#endif

// Imm1:	    
            offset = Count0 - pop1lower;        //Imm1:	    SETOFFSET_IMM(offset, Count0, pop1lower);
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, ju_PImmed1(Pjp)[offset], 1); // JU_SETDIGIT1(*PIndex, ju_PImmed1(Pjp)[offset]);
	    JU_RET_FOUND_IMM(Pjp, offset);

	case cJU_JPIMMED_2_02:          // IMM(2, Imm2);
	case cJU_JPIMMED_2_03:          // IMM(3, Imm2);
	case cJU_JPIMMED_2_04:          // IMM(4, Imm2);

#ifdef  JUDY1
	case cJ1_JPIMMED_2_05:          // IMM(5, Imm2);
	case cJ1_JPIMMED_2_06:          // IMM(6, Imm2);
	case cJ1_JPIMMED_2_07:          // IMM(7, Imm2);
#endif

// Imm2:	    
            offset = Count0 - pop1lower; //Imm2:	    SETOFFSET_IMM(offset, Count0, pop1lower);
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, ju_PImmed2(Pjp)[offset], 1);    //  *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(2))) | ju_PImmed2(Pjp)[offset];
	    JU_RET_FOUND_IMM(Pjp, offset);

	case cJU_JPIMMED_3_02:          // IMM(2, Imm3);

#ifdef  JUDY1
	case cJ1_JPIMMED_3_03:          // IMM(3, Imm3);
//	case cJ1_JPIMMED_3_04:          // IMM(4, Imm3);
//	case cJ1_JPIMMED_3_05:          // IMM(5, Imm3);
#endif

// Imm3:
	{
            offset = Count0 - pop1lower; //	    SETOFFSET_IMM(offset, Count0, pop1lower);
	    Word_t lsb;
	    JU_COPY3_PINDEX_TO_LONG(lsb, ju_PImmed3(Pjp) + offset);
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, lsb, 3); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(3))) | lsb;
	    JU_RET_FOUND_IMM(Pjp, offset);
	}

	case cJU_JPIMMED_4_02:          // IMM(2, Imm4);
#ifdef  JUDY1
	case cJ1_JPIMMED_4_03:          // IMM(3, Imm4);
#endif  // JUDYL
        {
           
// Imm4:
            offset = Count0 - pop1lower; //  SETOFFSET_IMM(offset, Count0, pop1lower); 
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, ju_PImmed4(Pjp)[offset], 4); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(4))) | ju_PImmed4(Pjp)[offset];
	    JU_RET_FOUND_IMM(Pjp, offset);
        }
#ifdef  JUDY1
	case cJ1_JPIMMED_5_02:          // IMM(2, Imm5);
	case cJ1_JPIMMED_6_02:          // IMM(2, Imm6);
	case cJ1_JPIMMED_7_02:          // IMM(2, Imm7);
	{
            int level = ju_Type(Pjp) - cJ1_JPIMMED_2_02 + 2;
	    offset = Count0 - pop1lower; //	    SETOFFSET_IMM(offset, Count0, pop1lower);
            Word_t Dcd;
//	        lsb = ju_PImmed7(Pjp)[offset];
            if (offset == 0)
                Dcd = ju_IMM01Key(Pjp);
            else
                Dcd = ju_IMM02Key(Pjp);
            *PIndex = JU_MERGE_DCD_KEY(*PIndex, Dcd, level); // *PIndex = (*PIndex & (~JU_LEASTBYTESMASK(7))) | lsb;
//	    JU_RET_FOUND_IMM(Pjp, offset);
            return(1);
	}
#endif // JUDY1


// ----------------------------------------------------------------------------
// UNEXPECTED JP TYPES:

	default: 
            JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
            assert(0);
	    JUDY1CODE(return(JERRI );)
	    JUDYLCODE(return(PPJERR);)

	} // SMByCount switch.

	/*NOTREACHED*/

} // Judy1ByCount() / JudyLByCount()
