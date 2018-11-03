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

// @(#) $Revision: 4.38 $ $Source: /judy/src/JudyCommon/JudyCascade.c $

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"

extern int j__udyCreateBranchL(Pjp_t, Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udyCreateBranchB(Pjp_t, Pjp_t, uint8_t *, Word_t, Pjpm_t);

//static const jbb_t StageJBBZero;	// zeroed versions of namesake struct.

// TBD:  There are multiple copies of (some of) these CopyWto3, Copy3toW,
// CopyWto7 and Copy7toW functions in Judy1Cascade.c, JudyLCascade.c, and
// JudyDecascade.c.  These static functions should probably be moved to a
// common place, made macros, or something to avoid having four copies.


// ****************************************************************************
// __ J U D Y   C O P Y   X   T O   W


FUNCTION static void j__udyCopy3toW(
	PWord_t	  PDest,
	uint8_t * PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY3_PINDEX_TO_LONG(*PDest, PSrc);
		PSrc	+= 3;
		PDest	+= 1;

	} while(--LeafIndexes);

} //j__udyCopy3toW()



FUNCTION static void j__udyCopy4toW(
	PWord_t	   PDest,
	uint32_t * PSrc,
	Word_t	   LeafIndexes)
{
	do { *PDest++ = *PSrc++;
	} while(--LeafIndexes);

} // j__udyCopy4toW()


FUNCTION static void j__udyCopy5toW(
	PWord_t	  PDest,
	uint8_t	* PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY5_PINDEX_TO_LONG(*PDest, PSrc);
		PSrc	+= 5;
		PDest	+= 1;

	} while(--LeafIndexes);

} // j__udyCopy5toW()


FUNCTION static void j__udyCopy6toW(
	PWord_t	  PDest,
	uint8_t	* PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY6_PINDEX_TO_LONG(*PDest, PSrc);
		PSrc	+= 6;
		PDest	+= 1;

	} while(--LeafIndexes);

} // j__udyCopy6toW()


FUNCTION static void j__udyCopy7toW(
	PWord_t	  PDest,
	uint8_t	* PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY7_PINDEX_TO_LONG(*PDest, PSrc);
		PSrc	+= 7;
		PDest	+= 1;

	} while(--LeafIndexes);

} // j__udyCopy7toW()



// ****************************************************************************
// __ J U D Y   C O P Y   W   T O   X


FUNCTION static void j__udyCopyWto3(
	uint8_t	* PDest,
	PWord_t	  PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY3_LONG_TO_PINDEX(PDest, *PSrc);
		PSrc	+= 1;
		PDest	+= 3;

	} while(--LeafIndexes);

} // j__udyCopyWto3()



FUNCTION static void j__udyCopyWto4(
	uint32_t *PDest32,
	PWord_t	  PSrc,
	Word_t	  LeafIndexes)
{
	do
	    *PDest32++ = *PSrc++;
	while(--LeafIndexes);

} // j__udyCopyWto4()


FUNCTION static void j__udyCopyWto5(
	uint8_t	* PDest,
	PWord_t	  PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY5_LONG_TO_PINDEX(PDest, *PSrc);
		PSrc	+= 1;
		PDest	+= 5;

	} while(--LeafIndexes);

} // j__udyCopyWto5()


FUNCTION static void j__udyCopyWto6(
	uint8_t	* PDest,
	PWord_t	  PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY6_LONG_TO_PINDEX(PDest, *PSrc);
		PSrc	+= 1;
		PDest	+= 6;

	} while(--LeafIndexes);

} // j__udyCopyWto6()


FUNCTION static void j__udyCopyWto7(
	uint8_t	* PDest,
	PWord_t	  PSrc,
	Word_t	  LeafIndexes)
{
	do
	{
		JU_COPY7_LONG_TO_PINDEX(PDest, *PSrc);
		PSrc	+= 1;
		PDest	+= 7;

	} while(--LeafIndexes);

} // j__udyCopyWto7()



// ****************************************************************************
// COMMON CODE (MACROS):
//
// Free objects in an array of valid JPs, StageJP[ExpCnt] == last one may
// include Immeds, which are ignored.

#define FREEALLEXIT(ExpCnt,StageJP,Pjpm)				\
	{								\
	    Word_t _expct = (ExpCnt);					\
	    while (_expct--) j__udyFreeSM(&((StageJP)[_expct]), Pjpm);  \
	    return(-1);                                                 \
	}

// Clear the array that keeps track of the number of JPs in a subexpanse:

#define ZEROJP(SubJPCount, NUMB)                                        \
	{								\
		for (int ii = 0; ii < (int)NUMB; ii++)                  \
                        SubJPCount[ii] = 0;                             \
	}

// ****************************************************************************
// __ J U D Y   S T A G E   J B B   T O   J B B
//
// Create a mallocd BranchB (jbb_t) from a staged BranchB while "splaying" a
// single old leaf.  Return 0 if out of memory, otherwise PjbbRaw.

static Word_t j__udyStageJBBtoJBB(
	Pjbb_t    PStageJBB,	// temp jbb_t on stack.
	Pjp_t     PjpArray,	// array of JPs to splayed new leaves.
	uint8_t * PSubCount,	// count of JPs for each subexpanse.
	Pjpm_t    Pjpm)		// the jpm_t for JudyAlloc*().
{
	Word_t    PjbbRaw;	// pointer to new bitmap branch.
	Pjbb_t    Pjbb;
	Word_t    subexp;

#ifdef PCAS
             printf("j__udyStageJBBtoJBB()\n");
#endif  // PCAS

// Get memory for new BranchB:

	PjbbRaw = j__udyAllocJBB(Pjpm);
	if (PjbbRaw == 0)
            return(0);
	Pjbb = P_JBB(PjbbRaw);

// Copy staged BranchB into just-allocated BranchB:

	*Pjbb = *PStageJBB;

// Allocate the JP subarrays (BJP) for the new BranchB:

	for (subexp = 0; subexp < cJU_NUMSUBEXPB; subexp++)
	{
	    Word_t PjpRaw;
	    Pjp_t  Pjp;
	    Word_t NumJP;       // number of JPs in each subexpanse.

#ifndef BMVALUE
	    if ((NumJP = PSubCount[subexp]) == 0)
                continue;	// empty.
#else /* BMVALUE */
	    NumJP = PSubCount[subexp];

#ifdef BBSEARCH
            printf("JBBtoJBB set sub = %ld to %lu count\n", subexp, NumJP);
            Pjbb->jbb_Pop1 |= NumJP << (subexp * 8);     // Sub pops in struct
#endif  // BBSEARCH

	    if (NumJP == 0)
                continue;	// empty.
#endif /* BMVALUE */

// Out of memory, back out previous allocations:

	    if ((PjpRaw = j__udyAllocJBBJP(NumJP, Pjpm)) == 0)
	    {
		while(subexp--)
		{
		    if ((NumJP = PSubCount[subexp]) == 0)
                        continue;

		    PjpRaw = JU_JBB_PJP(Pjbb, subexp);
		    j__udyFreeJBBJP(PjpRaw, NumJP, Pjpm);
		}
		j__udyFreeJBB(PjbbRaw, Pjpm);
		return(0);	// out of memory.
	    }
	    Pjp = P_JP(PjpRaw);

// Place the JP subarray pointer in the new BranchB, copy subarray JPs, and
// advance to the next subexpanse:

	    JU_JBB_PJP(Pjbb, subexp) = PjpRaw;
	    JU_COPYMEM(Pjp, PjpArray, NumJP);
	    PjpArray += NumJP;

	} // for each subexpanse.

// Change the PjpLeaf from Leaf to BranchB:


//      Done by caller NOW.
//	PjpLeaf->Jp_Addr0  = PjbbRaw;
//	PjpLeaf->jp_Type += cJU_JPBRANCH_B2 - cJU_JPLEAF2;  // Leaf to BranchB.

	return(PjbbRaw);

} // j__udyStageJBBtoJBB()


// ****************************************************************************
// __ J U D Y   J L L 2   T O   J L B 1
//
// Create a LeafB1 (jlb_t = JLB1) from a Leaf2 (2-byte Indexes and for JudyL,
// Word_t Values).  Return NULL if out of memory, else a pointer to the new
// LeafB1.
//
// NOTE:  Caller must release the Leaf2 that was passed in.

FUNCTION static Word_t j__udyJLL2toJLB1(
	uint16_t * Pjll,	// array of 16-bit indexes.

#ifdef JUDYL
	Pjv_t      Pjv,		// array of associated values.
#endif  // JUDYL

	Word_t     LeafPop1,	// number of indexes/values.
	Pjpm_t    Pjpm)	// jpm_t for JudyAlloc*()/JudyFree*().
{
	Word_t     PjlbRaw;
	Pjlb_t     Pjlb;
	int	   offset;
#ifdef  JUDYL
        int	   subexp;
#endif  // JUDYL

#ifdef  PCAS
        printf("j__udyJLL2toJLB1() -- Pop1 = %d\n", (int)LeafPop1);
#endif  // PCAS

//      Allocate the LeafB1:
	if ((PjlbRaw = j__udyAllocJLB1(Pjpm)) == 0)
            return(0);
	Pjlb = P_JLB(PjlbRaw);

// Copy Leaf2 indexes to LeafB1:

	for (offset = 0; offset < (int)LeafPop1; ++offset)
	    JU_BITMAPSETL(Pjlb, Pjll[offset]);

#ifdef JUDYL

// Build LeafVs from bitmap:

	for (subexp = 0; subexp < (int)cJU_NUMSUBEXPL; ++subexp)
	{
	    struct _POINTER_VALUES
	    {
		Word_t pv_Pop1;		// size of value area.
		Word_t pv_Pjv;		// raw pointer to value area.
	    } pv[cJU_NUMSUBEXPL];

// Get the population of the subexpanse, and if any, allocate a LeafV:

	    pv[subexp].pv_Pop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, subexp));

#ifndef BMVALUE
	    if (pv[subexp].pv_Pop1)
	    {
		Pjv_t Pjvnew;
#else /* BMVALUE */
	    if (pv[subexp].pv_Pop1 == 0)
                continue;               // skip subexpanse
#endif /* BMVALUE */

#ifndef BMVALUE
// TBD:  There is an opportunity to put pop == 1 value in pointer:
#else /* BMVALUE */
	    if (pv[subexp].pv_Pop1 == 1)
            {
		JL_JLB_PVALUE(Pjlb, subexp) = *Pjv;             // place Value
                Pjv++;
                continue;               // on to next subexpanse
            }
            {
		Pjv_t Pjvnew;
#endif /* BMVALUE */

		pv[subexp].pv_Pjv = j__udyLAllocJV(pv[subexp].pv_Pop1, Pjpm);

// Upon out of memory, free all previously allocated:

		if (pv[subexp].pv_Pjv == 0)
		{
		    while(subexp--)
		    {
			if (pv[subexp].pv_Pop1)
			{
			    j__udyLFreeJV(pv[subexp].pv_Pjv, pv[subexp].pv_Pop1,
					  Pjpm);
			}
		    }
		    j__udyFreeJLB1(PjlbRaw, Pjpm);
		    return(0);
		}

		Pjvnew = P_JV(pv[subexp].pv_Pjv);
		JU_COPYMEM(Pjvnew, Pjv, pv[subexp].pv_Pop1);
		Pjv += pv[subexp].pv_Pop1;	// advance value pointer.

// Place raw pointer to value array in bitmap subexpanse:

		JL_JLB_PVALUE(Pjlb, subexp) = pv[subexp].pv_Pjv;

            } // a  populated subexpanse > 1.
	} // each subexpanse.

#endif // JUDYL

	return(PjlbRaw);	// pointer to LeafB1.

} // j__udyJLL2toJLB1()


// ****************************************************************************
// __ J U D Y   C A S C A D E 2
//
// Entry PLeaf of size LeafPop1 is either compressed or splayed with pointer
// returned in Pjp.  Entry Levels sizeof(Word_t) down to level 2.
//
// Splay or compress the 2-byte Index Leaf that Pjp point to.  Return *Pjp as a
// (compressed) cJU_LEAFB1 or a cJU_BRANCH_*2

FUNCTION int j__udyCascade2(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	uint16_t * PLeaf;	// pointer to leaf, explicit type.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.
#ifdef  JUDYL
        Pjv_t	   Pjv;	        // value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	Word_t    *PLeafRaw  [cJU_LEAF2_MAXWORDS];      // Staging of Leaf2
	jp_t	   StageJP   [cJU_LEAF2_MAXPOP1];  // JPs of new leaves
	uint8_t	   StageExp  [cJU_LEAF2_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

	assert(ju_Type(Pjp) == cJU_JPLEAF2);
	assert((ju_DcdPop0(Pjp) & 0xFFFF) == (cJU_LEAF2_MAXPOP1-1));

#ifdef  PCAS
        printf("\n==================Cascade2(), jp_t = %d\n", (int)sizeof(jp_t));
#endif  // PCAS

//	Get the address of the Leaf2
	Word_t  PLeafOrigRaw = ju_BaLPntr(Pjp);
	Word_t *PLeafOrig    = (Word_t *)P_JLL(PLeafOrigRaw);

	PLeaf = (uint16_t *)PLeafRaw;

//      Copy Leaf2 to staging area
        JU_COPYMEM((Word_t *)PLeafRaw, PLeafOrig, cJU_LEAF2_MAXWORDS);

        j__udyFreeJLL2(PLeafOrigRaw, cJU_LEAF2_MAXPOP1, Pjpm);

	PLeaf = (uint16_t *)PLeafRaw;

//	And its Value area

#ifdef  JUDYL
	Pjv = JL_LEAF2VALUEAREA(PLeaf, cJU_LEAF2_MAXPOP1);
#endif  // JUDYL

//  If Leaf is in 1 expanse -- just compress it to a Bitmap Leaf

	CIndex = PLeaf[0];
	if (!JU_DIGITATSTATE(CIndex ^ PLeaf[cJU_LEAF2_MAXPOP1-1], 2))
	{
//	cJU_JPLEAF_B1
                Word_t DcdP0;
		Word_t PjlbRaw;
#ifdef  PCAS
        printf("\n==================j__udyJLL2toJLB1(0)\n");
#endif  // PCAS
		PjlbRaw = j__udyJLL2toJLB1(PLeaf,
#ifdef JUDYL
				     Pjv,
#endif
				     cJU_LEAF2_MAXPOP1, Pjpm);
		if (PjlbRaw == 0)
                    return(-1);  // out of memory

//		Merge in another Dcd byte because compressing
		DcdP0 = (CIndex & cJU_DCDMASK(1)) | ju_DcdPop0(Pjp);

// not tested     JU_JPSETADT(Pjp, PjlbRaw, DcdP0, cJU_JPLEAF_B1);

                ju_SetDcdPop0(Pjp, DcdP0);
                ju_SetLeafPop0(Pjp, cJU_LEAF2_MAXPOP1 - 1); 
                ju_SetJpType(Pjp, cJU_JPLEAF_B1);
                ju_SetBaLPntr(Pjp, PjlbRaw);
               
		return(1);
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 2 byte index Leaf to 1 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF2_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ PLeaf[End], 2))
		   )
		{
//			Build a leaf below the previous expanse
//
			Pjp_t  PjpJP	= StageJP + ExpCnt;
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, 2);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);
#ifdef SUBEXPCOUNTS
			StageJBB.jbb_subPop1[subexp] += Pop1; // pop of subexpanse
#endif
//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 2);
			if (Pop1 == 1)	// cJU_JPIMMED_1_01
			{
	                    Word_t DcdP0;
	                    DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(1)) | CIndex;
#ifdef  JUDY1
//                          JU_JPSETADT(PjpJP, 0, DcdP0, cJ1_JPIMMED_1_01);
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_1_01);
#else   // JUDYL
//                          JU_JPSETADT(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_1_01);
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_1_01);
#endif  // JUDYL
			}
//#ifdef  JUDY1
			else if (Pop1 <= cJU_IMMED1_MAXPOP1) // bigger > 1
			{
//		cJL_JPIMMED_1_02..7:  JudyL 
//		cJ1_JPIMMED_1_02..15: Judy1 
#ifdef JUDYL
				Word_t PjvnewRaw;	// value area of leaf.
				Pjv_t  Pjvnew;

//				Allocate Value area for Immediate Leaf
				PjvnewRaw = j__udyLAllocJV(Pop1, Pjpm);
				if (PjvnewRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjvnew = P_JV(PjvnewRaw);

//				Copy to Values to Value Leaf
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
//				PjpJP->jp_PValue = PjvnewRaw;
                                ju_SetPjvPntr(PjpJP, PjvnewRaw);
#endif  // JUDYL
//				Copy to JP as an immediate Leaf
//		                j__udyCopy2to1(PjpJP->jp_LIndex1, PLeaf + Start, Pop1);
				JU_COPYMEM(ju_PImmed1(PjpJP), PLeaf + Start, Pop1);
//				Set Type, Population and Index size
//				PjpJP->jp_Type = cJU_JPIMMED_1_02 + Pop1 - 2;
                                ju_SetJpType(PjpJP, cJU_JPIMMED_1_02 + Pop1 - 2);
			}
//#endif  //  JUDY1

///////////#ifdef  JUDYL
			else if (Pop1 <= cJU_LEAF1_MAXPOP1) // still bigger
			{
                                Word_t  DcdP0;
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

//				Get a new Leaf
				PjllRaw = j__udyAllocJLL1(Pop1, Pjpm);
				if (PjllRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll = P_JLL(PjllRaw);
#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjvnew = JL_LEAF1VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif
//				Copy Indexes to new Leaf
//		                j__udyCopy2to1(Pjll, PLeaf+Start, Pop1);
				JU_COPYMEM((uint8_t *)Pjll, PLeaf+Start, Pop1);

                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(2))
                                                |
                                        (CIndex & cJU_DCDMASK(2-1));
//                                                |
//                                        (Pop1 - 1);

// testing                                JU_JPSETADT(PjpJP, PjllRaw, DcdP0, cJU_JPLEAF1);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);
                                ju_SetJpType(PjpJP, cJU_JPLEAF1);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
			}
//////////////#endif  // JUDYL
			else				// biggest
			{
//		cJU_JPLEAF_B1
                                Word_t  DcdP0;
				Word_t PjlbRaw;

#ifdef  PCAS
        printf("\n==================j__udyJLL2toJLB1(1)\n");
#endif  // PCAS
				PjlbRaw = j__udyJLL2toJLB1(
						PLeaf + Start,
#ifdef JUDYL
						Pjv + Start,
#endif
						Pop1, Pjpm);
				if (PjlbRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(2))
                                                |
                                        (CIndex & cJU_DCDMASK(2-1));
//                                                |
//                                        (Pop1 - 1);
//                                JU_JPSETADT(PjpJP, PjlbRaw, DcdP0, cJU_JPLEAF_B1);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);        //Warning, must be after DcdP0
                                ju_SetJpType(PjpJP, cJU_JPLEAF_B1);
                                ju_SetBaLPntr(PjpJP, PjlbRaw);

			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF2_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = PLeaf[End];
			Start  = End;
		}
	}

//      Now put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L2;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L2);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B2;
            ju_SetJpType(Pjp, cJU_JPBRANCH_B2);
//            Pjp->Jp_Addr0  = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

} // j__udyCascade2()


// ****************************************************************************
// __ J U D Y   C A S C A D E 3
//
// Return *Pjp as a (compressed) cJU_LEAF2, cJU_BRANCH_L3, cJU_BRANCH_B3.

FUNCTION int j__udyCascade3(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	uint8_t  * PLeaf;	// pointer to leaf, explicit type.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.
#ifdef  JUDYL
        Pjv_t	   Pjv; 	// value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF3_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF3_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF3_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

	assert(ju_Type(Pjp) == cJU_JPLEAF3);
	assert((ju_DcdPop0(Pjp) & 0xFFFFFF) == (cJU_LEAF3_MAXPOP1-1));

#ifdef  PCAS
        printf("\n==================Cascade3()\n");
#endif  // PCAS

//	Get the address of the Leaf
	PLeaf = (uint8_t *) P_JLL(ju_BaLPntr(Pjp));

//	Extract leaf to Word_t and insert-sort Index into it
	j__udyCopy3toW(StageA, PLeaf, cJU_LEAF3_MAXPOP1);

//	Get the address of the Leaf and Value area
#ifdef  JUDYL
	Pjv = JL_LEAF3VALUEAREA(PLeaf, cJU_LEAF3_MAXPOP1);
#endif  // JUDYL

//  If Leaf is in 1 expanse -- just compress it (compare 1st, last & Index)

	CIndex = StageA[0];
//printf("Index = 0x%lx != last = 0x%lx\n", CIndex, StageA[cJU_LEAF3_MAXPOP1-1]);
	if (!JU_DIGITATSTATE(CIndex ^ StageA[cJU_LEAF3_MAXPOP1-1], 3))
	{
                Word_t DcdP0;
		Word_t PjllRaw;	 // pointer to new leaf.
		Pjll_t Pjll;
#ifdef  JUDYL
		Pjv_t  Pjvnew;	 // value area of new leaf.
#endif  // JUDYL

//		Alloc a 2 byte Index Leaf
		PjllRaw	= j__udyAllocJLL2(cJU_LEAF3_MAXPOP1, Pjpm);
		if (PjllRaw == 0)
                    return(-1);  // out of memory

		Pjll = P_JLL(PjllRaw);

//		Copy just 2 bytes Indexes to new Leaf
//		j__udyCopyWto2(Pjll, StageA, cJU_LEAF3_MAXPOP1);
		JU_COPYMEM((uint16_t *) Pjll, StageA, cJU_LEAF3_MAXPOP1);
#ifdef JUDYL
//		Copy Value area into new Leaf
		Pjvnew = JL_LEAF2VALUEAREA(Pjll, cJU_LEAF3_MAXPOP1);
		JU_COPYMEM(Pjvnew, Pjv, cJU_LEAF3_MAXPOP1);
#endif  // JUDYL
//		Form new JP, Pop0 field is unchanged
//		Add in another Dcd byte because compressing
                DcdP0 = (CIndex & cJU_DCDMASK(2)) | ju_DcdPop0(Pjp);

//                JU_JPSETADT(Pjp, PjllRaw, DcdP0, cJU_JPLEAF2);

                ju_SetDcdPop0(Pjp, DcdP0);
                ju_SetJpType(Pjp, cJU_JPLEAF2);
                ju_SetBaLPntr(Pjp, PjllRaw);
//                ju_SetLeafPop0(PjpJP, );

		return(1); // Success
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 3 byte index Leaf to 2 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF3_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ StageA[End], 3))
		   )
		{
//			Build a leaf below the previous expanse

			Pjp_t  PjpJP	= StageJP + ExpCnt;
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, 3);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 3);

			if (Pop1 == 1)	// cJU_JPIMMED_2_01
			{
	                    Word_t DcdP0;
	                    DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(2)) | CIndex;
#ifdef  JUDY1
//                          JU_JPSETADT(PjpJP, 0, DcdP0, cJ1_JPIMMED_2_01);
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_2_01);
#else   // JUDYL
//                          JU_JPSETADT(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_2_01);
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_2_01);
#endif  // JUDYL
			}
			else if (Pop1 <= cJU_IMMED2_MAXPOP1)
			{
//		cJ1_JPIMMED_2_02..3:  Judy1 32
//		cJL_JPIMMED_2_02..3:  JudyL 64
//		cJ1_JPIMMED_2_02..7:  Judy1 64
#ifdef JUDYL
//				Alloc is 1st in case of malloc fail
				Word_t PjvnewRaw;  // value area of new leaf.
				Pjv_t  Pjvnew;

//				Allocate Value area for Immediate Leaf
				PjvnewRaw = j__udyLAllocJV(Pop1, Pjpm);
				if (PjvnewRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjvnew = P_JV(PjvnewRaw);

//				Copy to Values to Value Leaf
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
//				PjpJP->jp_PValue = PjvnewRaw;
                                ju_SetPjvPntr(PjpJP, PjvnewRaw);
#endif  // JUDYL
//				Copy to Index to JP as an immediate Leaf
//		                j__udyCopyWto2(PjpJP->jp_LIndex2, StageA+Start, Pop1);
				JU_COPYMEM(ju_PImmed2(PjpJP), StageA + Start, Pop1);

//				Set Type, Population and Index size
//				PjpJP->jp_Type = cJU_JPIMMED_2_02 + Pop1 - 2;
                                ju_SetJpType(PjpJP, cJU_JPIMMED_2_02 + Pop1 - 2);
			}

			else	// Make a linear leaf2
			{
//		cJU_JPLEAF2
                                Word_t  DcdP0;
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

				PjllRaw = j__udyAllocJLL2(Pop1, Pjpm);
				if (PjllRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll = P_JLL(PjllRaw);
#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjvnew = JL_LEAF2VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif
//				Copy least 2 bytes per Index of Leaf to new Leaf
//		                j__udyCopyWto2(Pjll, StageA+Start, Pop1);
				JU_COPYMEM((uint16_t *) Pjll, StageA+Start,
					   Pop1);

                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(3))
                                                |
                                        (CIndex & cJU_DCDMASK(3-1));
//                                                |
//                                        (Pop1 - 1);

//                                JU_JPSETADT(PjpJP, PjllRaw, DcdP0, cJU_JPLEAF2);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);
                                ju_SetJpType(PjpJP, cJU_JPLEAF2);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF3_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = StageA[End];
			Start  = End;
		}
	}

//      Now put all the Leaves below a BranchL or BranchB:
//        printf("ExpCnt = %lu\n", ExpCnt);
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L3;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L3);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B3;
            ju_SetJpType(Pjp, cJU_JPBRANCH_B3);
//            Pjp->Jp_Addr0 = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

} // j__udyCascade3()



// ****************************************************************************
// __ J U D Y   C A S C A D E 4
//
// Cascade from a cJU_JPLEAF4 to one of the following:
//  1. if leaf is in 1 expanse:
//        compress it into a JPLEAF3
//  2. if leaf contains multiple expanses:
//        create linear or bitmap branch containing
//        each new expanse is either a:
//               JPIMMED_3_01  branch
//               JPIMMED_3_02  branch
//               JPLEAF3

FUNCTION int j__udyCascade4(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	uint32_t * PLeaf;	// pointer to leaf, explicit type.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.
#ifdef  JUDYL
        Pjv_t	   Pjv; 	// value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF4_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF4_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF4_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

	assert(ju_Type(Pjp) == cJU_JPLEAF4);
	assert((ju_DcdPop0(Pjp) & 0xFFFFFFFF) == (cJU_LEAF4_MAXPOP1-1));

#ifdef  PCAS
        printf("\n==================Cascade4()\n");
#endif  // PCAS

//	Get the address of the Leaf
	PLeaf = (uint32_t *) P_JLL(ju_BaLPntr(Pjp));

//	Extract 4 byte index Leaf to Word_t
	j__udyCopy4toW(StageA, PLeaf, cJU_LEAF4_MAXPOP1);

//	Get the address of the Leaf and Value area
#ifdef  JUDYL
	Pjv = JL_LEAF4VALUEAREA(PLeaf, cJU_LEAF4_MAXPOP1);
#endif  // JUDYL

//  If Leaf is in 1 expanse -- just compress it (compare 1st, last & Index)

	CIndex = StageA[0];
	if (!JU_DIGITATSTATE(CIndex ^ StageA[cJU_LEAF4_MAXPOP1-1], 4))
	{
                Word_t DcdP0;
		Word_t PjllRaw;	 // pointer to new leaf.
		Pjll_t Pjll;
#ifdef  JUDYL
                Pjv_t  Pjvnew;	 // value area of new Leaf.
#endif  // JUDYL

//		Alloc a 3 byte Index Leaf
		PjllRaw = j__udyAllocJLL3(cJU_LEAF4_MAXPOP1, Pjpm);
		if (PjllRaw == 0)
                    return(-1);  // out of memory

		Pjll = P_JLL(PjllRaw);

//		Copy Index area into new Leaf
		j__udyCopyWto3((uint8_t *) Pjll, StageA, cJU_LEAF4_MAXPOP1);
#ifdef JUDYL
//		Copy Value area into new Leaf
		Pjvnew = JL_LEAF3VALUEAREA(Pjll, cJU_LEAF4_MAXPOP1);
		JU_COPYMEM(Pjvnew, Pjv, cJU_LEAF4_MAXPOP1);
#endif
	        DcdP0 = ju_DcdPop0(Pjp) | (CIndex & cJU_DCDMASK(3));
//                JU_JPSETADT(Pjp, PjllRaw, DcdP0, cJU_JPLEAF3);

                ju_SetDcdPop0(Pjp, DcdP0);
                ju_SetJpType(Pjp, cJU_JPLEAF3);
                ju_SetBaLPntr(Pjp, PjllRaw);
//                ju_SetLeafPop0(PjpJP, );

		return(1);
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 4 byte index Leaf to 3 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF4_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ StageA[End], 4))
		   )
		{
//			Build a leaf below the previous expanse

			Pjp_t  PjpJP	= StageJP + ExpCnt;
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, 4);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 4);

			if (Pop1 == 1)	// cJU_JPIMMED_3_01
			{
	                    Word_t DcdP0;
	                    DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(3)) | CIndex;
#ifdef  JUDY1
//                          JU_JPSETADT(PjpJP, 0, DcdP0, cJ1_JPIMMED_3_01);
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_3_01);
#else   // JUDYL
//                          JU_JPSETADT(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_3_01);
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_3_01);
#endif  // JUDYL
			}
			else if (Pop1 <= cJU_IMMED3_MAXPOP1)
			{
//		cJ1_JPIMMED_3_02   :  Judy1 32
//		cJL_JPIMMED_3_02   :  JudyL 64
//		cJ1_JPIMMED_3_02..5:  Judy1 64

#ifdef JUDYL
//				Alloc is 1st in case of malloc fail
				Word_t PjvnewRaw;  // value area of new leaf.
				Pjv_t  Pjvnew;

//				Allocate Value area for Immediate Leaf
				PjvnewRaw = j__udyLAllocJV(Pop1, Pjpm);
				if (PjvnewRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjvnew = P_JV(PjvnewRaw);

//				Copy to Values to Value Leaf
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
//				PjpJP->jp_PValue = PjvnewRaw;
                                ju_SetPjvPntr(PjpJP, PjvnewRaw);
#endif  // JUDYL
				j__udyCopyWto3(ju_PImmed1(PjpJP), StageA + Start, Pop1);
//				Set type, population and Index size
//                                PjpJP->jp_Type = cJU_JPIMMED_3_02 + Pop1 - 2;
                                ju_SetJpType(PjpJP, cJU_JPIMMED_3_02 + Pop1 - 2);
			}
			else
			{
//		cJU_JPLEAF3
                                Word_t  DcdP0;
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew;	 // value area of new leaf.
#endif  // JUDYL

				PjllRaw = j__udyAllocJLL3(Pop1, Pjpm);
				if (PjllRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll = P_JLL(PjllRaw);

//				Copy Indexes to new Leaf
				j__udyCopyWto3((uint8_t *) Pjll, StageA + Start, Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjvnew = JL_LEAF3VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL
                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(4))
                                                |
                                        (CIndex & cJU_DCDMASK(4-1));
//                                                |
//                                        (Pop1 - 1);

//                                JU_JPSETADT(PjpJP, PjllRaw, DcdP0, cJU_JPLEAF3);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);
                                ju_SetJpType(PjpJP, cJU_JPLEAF3);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF4_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = StageA[End];
			Start  = End;
		}
	}

//      Now put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L4;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L4);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B4;
            ju_SetJpType(Pjp, cJU_JPBRANCH_B4);
//            Pjp->Jp_Addr0 = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

}  // j__udyCascade4()


// ****************************************************************************
// __ J U D Y   C A S C A D E 5
//
// Cascade from a cJU_JPLEAF5 to one of the following:
//  1. if leaf is in 1 expanse:
//        compress it into a JPLEAF4
//  2. if leaf contains multiple expanses:
//        create linear or bitmap branch containing
//        each new expanse is either a:
//               JPIMMED_4_01  branch
//               JPLEAF4

FUNCTION int j__udyCascade5(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	uint8_t  * PLeaf;	// pointer to leaf, explicit type.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.
#ifdef  JUDYL
         Pjv_t	   Pjv; 	// value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF5_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF5_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF5_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade5()\n");
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF5);
	assert((ju_DcdPop0(Pjp) & 0xFFFFFFFFFF) == (cJU_LEAF5_MAXPOP1-1));

//	Get the address of the Leaf
	PLeaf = (uint8_t *) P_JLL(ju_BaLPntr(Pjp));

//	Extract 5 byte index Leaf to Word_t
	j__udyCopy5toW(StageA, PLeaf, cJU_LEAF5_MAXPOP1);

//	Get the address of the Leaf and Value area
#ifdef  JUDYL
	Pjv = JL_LEAF5VALUEAREA(PLeaf, cJU_LEAF5_MAXPOP1);
#endif  // JUDYL

//  If Leaf is in 1 expanse -- just compress it (compare 1st, last & Index)

	CIndex = StageA[0];
	if (!JU_DIGITATSTATE(CIndex ^ StageA[cJU_LEAF5_MAXPOP1-1], 5))
	{
                Word_t DcdP0;
		Word_t PjllRaw;	 // pointer to new leaf.
		Pjll_t Pjll;
#ifdef  JUDYL
                Pjv_t  Pjvnew;	 // value area of new leaf.
#endif  // JUDYL

//		Alloc a 4 byte Index Leaf
		PjllRaw = j__udyAllocJLL4(cJU_LEAF5_MAXPOP1, Pjpm);
		if (PjllRaw == 0)
                    return(-1);  // out of memory

		Pjll = P_JLL(PjllRaw);

//		Copy Index area into new Leaf
		j__udyCopyWto4((uint32_t *)Pjll, StageA, cJU_LEAF5_MAXPOP1);
#ifdef JUDYL
//		Copy Value area into new Leaf
		Pjvnew = JL_LEAF4VALUEAREA(Pjll, cJU_LEAF5_MAXPOP1);
		JU_COPYMEM(Pjvnew, Pjv, cJU_LEAF5_MAXPOP1);
#endif

	        DcdP0 = ju_DcdPop0(Pjp) | (CIndex & cJU_DCDMASK(4));
//                JU_JPSETADT(Pjp, PjllRaw, DcdP0, cJU_JPLEAF4);
                ju_SetBaLPntr(Pjp, PjllRaw);
                ju_SetDcdPop0(Pjp, DcdP0);
                ju_SetJpType(Pjp, cJU_JPLEAF4);
//                ju_SetLeafPop0(Pjp, );

		return(1);
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 5 byte index Leaf to 4 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF5_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ StageA[End], 5))
		   )
		{
//			Build a leaf below the previous expanse

			Pjp_t  PjpJP	= StageJP + ExpCnt;
//                        *PjpJP          = (jp_t){};
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, 5);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 5);

			if (Pop1 == 1)	// cJU_JPIMMED_4_01
			{
	                    Word_t DcdP0;
	                    DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(4)) | CIndex;
#ifdef  JUDY1
//                          JU_JPSETADT(PjpJP, 0, DcdP0, cJ1_JPIMMED_4_01);
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_4_01);
#else   // JUDYL
//                          JU_JPSETADT(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_4_01);
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_4_01);
#endif  // JUDYL
			}
#ifndef noIMMED37
#ifdef JUDY1
			else if (Pop1 <= cJ1_IMMED4_MAXPOP1)
			{
//		cJ1_JPIMMED_4_02..3: Judy1 64

//                              Copy to Index to JP as an immediate Leaf
				j__udyCopyWto4(ju_PImmed4(PjpJP),
					       StageA + Start, Pop1);

//                              Set pointer, type, population and Index size
//                                PjpJP->jp_Type = cJ1_JPIMMED_4_02 + Pop1 - 2;
                                ju_SetJpType(PjpJP, cJ1_JPIMMED_4_02 + Pop1 - 2);
			}
#endif  // JUDY1
#endif  // ! noIMMED37
			else
			{
//		cJU_JPLEAF4
                                Word_t  DcdP0;
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

//				Get a new Leaf
				PjllRaw = j__udyAllocJLL4(Pop1, Pjpm);
				if (PjllRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll = P_JLL(PjllRaw);

//				Copy Indexes to new Leaf
				j__udyCopyWto4((uint32_t *)Pjll, StageA + Start,
					       Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjvnew = JL_LEAF4VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL

                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(5))
                                                |
                                        (CIndex & cJU_DCDMASK(5-1));
//                                                |
//                                        (Pop1 - 1);

//                                JU_JPSETADT(PjpJP, PjllRaw, DcdP0, cJU_JPLEAF4);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);        // should be after ^
                                ju_SetJpType(PjpJP, cJU_JPLEAF4);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF5_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = StageA[End];
			Start  = End;
		}
	}

//      Now put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L5;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L5);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B5;
            ju_SetJpType(Pjp, cJU_JPBRANCH_B5);
//            Pjp->Jp_Addr0 = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

}  // j__udyCascade5()


// ****************************************************************************
// __ J U D Y   C A S C A D E 6
//
// Cascade from a cJU_JPLEAF6 to one of the following:
//  1. if leaf is in 1 expanse:
//        compress it into a JPLEAF5
//  2. if leaf contains multiple expanses:
//        create linear or bitmap branch containing
//        each new expanse is either a:
//               JPIMMED_5_01 ... JPIMMED_5_03  branch
//               JPIMMED_5_01  branch
//               JPLEAF5

FUNCTION int j__udyCascade6(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	uint8_t  * PLeaf;	// pointer to leaf, explicit type.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.
#ifdef  JUDYL
        Pjv_t	   Pjv;	        // value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF6_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF6_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF6_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade6()\n");
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF6);
	assert((ju_DcdPop0(Pjp) & 0xFFFFFFFFFFFF) == (cJU_LEAF6_MAXPOP1-1));

//	Get the address of the Leaf
	PLeaf = (uint8_t *) P_JLL(ju_BaLPntr(Pjp));

//	Extract 6 byte index Leaf to Word_t
	j__udyCopy6toW(StageA, PLeaf, cJU_LEAF6_MAXPOP1);

//	Get the address of the Leaf and Value area
#ifdef  JUDYL
	Pjv = JL_LEAF6VALUEAREA(PLeaf, cJU_LEAF6_MAXPOP1);
#endif  // JUDYL

//  If Leaf is in 1 expanse -- just compress it (compare 1st, last & Index)

	CIndex = StageA[0];
	if (!JU_DIGITATSTATE(CIndex ^ StageA[cJU_LEAF6_MAXPOP1-1], 6))
	{
                Word_t DcdP0;
		Word_t PjllRaw;	 // pointer to new leaf.
		Pjll_t Pjll;
#ifdef  JUDYL
                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

//		Alloc a 5 byte Index Leaf
		PjllRaw = j__udyAllocJLL5(cJU_LEAF6_MAXPOP1, Pjpm);
		if (PjllRaw == 0)
                    return(-1);  // out of memory

		Pjll = P_JLL(PjllRaw);

//		Copy Index area into new Leaf
		j__udyCopyWto5((uint8_t *) Pjll, StageA, cJU_LEAF6_MAXPOP1);
#ifdef JUDYL
//		Copy Value area into new Leaf
		Pjvnew = JL_LEAF5VALUEAREA(Pjll, cJU_LEAF6_MAXPOP1);
		JU_COPYMEM(Pjvnew, Pjv, cJU_LEAF6_MAXPOP1);
#endif  // JUDYL
	        DcdP0 = ju_DcdPop0(Pjp) | (CIndex & cJU_DCDMASK(5));
//                JU_JPSETADT(Pjp, PjllRaw, DcdP0, cJU_JPLEAF5);

                ju_SetDcdPop0(Pjp, DcdP0);
                ju_SetJpType(Pjp, cJU_JPLEAF5);
                ju_SetBaLPntr(Pjp, PjllRaw);
//                ju_SetLeafPop0(PjpJP, );

		return(1);
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 6 byte index Leaf to 5 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF6_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ StageA[End], 6))
		   )
		{
//			Build a leaf below the previous expanse

			Pjp_t  PjpJP	= StageJP + ExpCnt;
//                        *PjpJP          = (jp_t){};
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, 6);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 6);

			if (Pop1 == 1)	// cJU_JPIMMED_5_01
			{
	                    Word_t DcdP0;
	                    DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(5)) | CIndex;
#ifdef  JUDY1
//                          JU_JPSETADT(PjpJP, 0, DcdP0, cJ1_JPIMMED_5_01);
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_5_01);
#else   // JUDYL
//                          JU_JPSETADT(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_5_01);
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_5_01);
#endif  // JUDYL
			}
#ifdef JUDY1
			else if (Pop1 <= cJ1_IMMED5_MAXPOP1)
			{
//		cJ1_JPIMMED_5_02..3: Judy1 64

//                              Copy to Index to JP as an immediate Leaf
				j__udyCopyWto5(ju_PImmed1(PjpJP),
					       StageA + Start, Pop1);

//                              Set pointer, type, population and Index size
//                                PjpJP->jp_Type = cJ1_JPIMMED_5_02 + Pop1 - 2;
                                ju_SetJpType(PjpJP, cJ1_JPIMMED_5_02 + Pop1 - 2);
			}
#endif  // JUDY1
			else
			{
//		cJU_JPLEAF5
                                Word_t  DcdP0;
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

//				Get a new Leaf
				PjllRaw = j__udyAllocJLL5(Pop1, Pjpm);
				if (PjllRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll = P_JLL(PjllRaw);

//				Copy Indexes to new Leaf
				j__udyCopyWto5((uint8_t *) Pjll, StageA + Start,
					       Pop1);

//				Copy to Values to new Leaf
#ifdef JUDYL
				Pjvnew = JL_LEAF5VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL

                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(6))
                                                |
                                        (CIndex & cJU_DCDMASK(6-1));
//                                                |
//                                        (Pop1 - 1);

//                                JU_JPSETADT(PjpJP, PjllRaw, DcdP0, cJU_JPLEAF5);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);
                                ju_SetJpType(PjpJP, cJU_JPLEAF5);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF6_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = StageA[End];
			Start  = End;
		}
	}

//      Now put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L6;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L6);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B6;
            ju_SetJpType(Pjp, cJU_JPBRANCH_B6);
//            Pjp->Jp_Addr0 = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

}  // j__udyCascade6()


// ****************************************************************************
// __ J U D Y   C A S C A D E 7
//
// Cascade from a cJU_JPLEAF7 to one of the following:
//  1. if leaf is in 1 expanse:
//        compress it into a JPLEAF6
//  2. if leaf contains multiple expanses:
//        create linear or bitmap branch containing
//        each new expanse is either a:
//               JPIMMED_6_01 ... JPIMMED_6_02  branch
//               JPIMMED_6_01  branch
//               JPLEAF6

FUNCTION int j__udyCascade7(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	uint8_t  * PLeaf;	// pointer to leaf, explicit type.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.
#ifdef  JUDYL
        Pjv_t	   Pjv; 	// value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF7_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF7_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF7_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

	assert(ju_Type(Pjp) == cJU_JPLEAF7);
	assert(ju_DcdPop0(Pjp) == (cJU_LEAF7_MAXPOP1-1));

#ifdef  PCAS
        printf("\n==================Cascade7()\n");
#endif  // PCAS

//	Get the address of the Leaf
	PLeaf = (uint8_t *) P_JLL(ju_BaLPntr(Pjp));

//	Extract 7 byte index Leaf to Word_t
	j__udyCopy7toW(StageA, PLeaf, cJU_LEAF7_MAXPOP1);

//	Get the address of the Leaf and Value area
#ifdef  JUDYL
	Pjv = JL_LEAF7VALUEAREA(PLeaf, cJU_LEAF7_MAXPOP1);
#endif  // JUDYL

//  If Leaf is in 1 expanse -- just compress it (compare 1st, last & Index)

	CIndex = StageA[0];
	if (!JU_DIGITATSTATE(CIndex ^ StageA[cJU_LEAF7_MAXPOP1-1], 7))
	{
                Word_t DcdP0;
		Word_t PjllRaw;	 // pointer to new leaf.
		Pjll_t Pjll;
#ifdef  JUDYL
                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

//		Alloc a 6 byte Index Leaf
		PjllRaw = j__udyAllocJLL6(cJU_LEAF7_MAXPOP1, Pjpm);
		if (PjllRaw == 0)
                    return(-1);  // out of memory

		Pjll = P_JLL(PjllRaw);

//		Copy Index area into new Leaf
		j__udyCopyWto6((uint8_t *) Pjll, StageA, cJU_LEAF7_MAXPOP1);
#ifdef JUDYL
//		Copy Value area into new Leaf
		Pjvnew = JL_LEAF6VALUEAREA(Pjll, cJU_LEAF7_MAXPOP1);
		JU_COPYMEM(Pjvnew, Pjv, cJU_LEAF7_MAXPOP1);
#endif

	        DcdP0 = ju_DcdPop0(Pjp) | (CIndex & cJU_DCDMASK(6));
//                JU_JPSETADT(Pjp, PjllRaw, DcdP0, cJU_JPLEAF6);

                ju_SetDcdPop0(Pjp, DcdP0);
                ju_SetJpType(Pjp, cJU_JPLEAF6);
                ju_SetBaLPntr(Pjp, PjllRaw);
//                ju_SetLeafPop0(PjpJP, );

		return(1);
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 7 byte index Leaf to 6 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF7_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ StageA[End], 7))
		   )
		{
//			Build a leaf below the previous expanse

			Pjp_t  PjpJP	= StageJP + ExpCnt;
//                        *PjpJP          = (jp_t){};
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, 7);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);
#ifdef SUBEXPCOUNTS
			StageJBB.jbb_subPop1[subexp] += Pop1; // pop of subexpanse
#endif
//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 7);

			if (Pop1 == 1)	// cJU_JPIMMED_6_01
			{
	                    Word_t DcdP0;
	                    DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(6)) | CIndex;
#ifdef  JUDY1
//                          JU_JPSETADT(PjpJP, 0, DcdP0, cJ1_JPIMMED_6_01);
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_6_01);
#else   // JUDYL
//                          JU_JPSETADT(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_6_01);
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_6_01);
#endif  // JUDYL
			}
#ifndef noIMMED37
#ifdef JUDY1
			else if (Pop1 == cJ1_IMMED6_MAXPOP1)
			{
//		cJ1_JPIMMED_6_02:    Judy1 64

//                              Copy to Index to JP as an immediate Leaf
				j__udyCopyWto6(ju_PImmed1(PjpJP),
					       StageA + Start, 2);

//                              Set pointer, type, population and Index size
//                                PjpJP->jp_Type = cJ1_JPIMMED_6_02;
                                ju_SetJpType(PjpJP, cJ1_JPIMMED_6_02);
			}
#endif  // JUDY1
#endif // ! noIMMED37
			else
			{
//		cJU_JPLEAF6
                                Word_t  DcdP0;
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL

//				Get a new Leaf
				PjllRaw = j__udyAllocJLL6(Pop1, Pjpm);
				if (PjllRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);
				Pjll = P_JLL(PjllRaw);

//				Copy Indexes to new Leaf
				j__udyCopyWto6((uint8_t *) Pjll, StageA + Start,
					       Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjvnew = JL_LEAF6VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif

                                DcdP0 = (ju_DcdPop0(Pjp) & cJU_DCDMASK(7))
                                                |
                                        (CIndex & cJU_DCDMASK(7-1));
//                                                |
//                                        (Pop1 - 1);

//                                JU_JPSETADT(PjpJP, PjllRaw, DcdP0, cJU_JPLEAF6);

                                ju_SetDcdPop0(PjpJP, DcdP0);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);
                                ju_SetJpType(PjpJP, cJU_JPLEAF6);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF7_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = StageA[End];
			Start  = End;
		}
	}

//      Now put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L7;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L7);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B7;
            ju_SetJpType(Pjp, cJU_JPBRANCH_B7);
//            Pjp->Jp_Addr0 = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

}  // j__udyCascade7()



// ****************************************************************************
// __ J U D Y   C A S C A D E   L
//
// (Compressed) cJU_LEAF3[7], cJ1_JPBRANCH_L.
//
// Cascade from a LEAFW (under Pjp) to one of the following:
//  1. if LEAFW is in 1 expanse:
//        create linear branch with a JPLEAF3[7] under it
//  2. LEAFW contains multiple expanses:
//        create linear or bitmap branch containing new expanses
//        each new expanse is either a: 32   64
//               JPIMMED_3_01  branch    Y    N
//               JPIMMED_7_01  branch    N    Y
//               JPLEAF3                 Y    N
//               JPLEAF7                 N    Y

FUNCTION int j__udyCascadeL(
	Pjp_t	   Pjp,
	Pjpm_t	   Pjpm)
{
	Pjllw_t	   Pjllw;	// leaf to work on.
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t	   CIndex;	// current Index word.
#ifdef  JUDYL
        Pjv_t	   Pjv; 	// value area of leaf.
#endif  // JUDYL

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	StageJP [cJU_LEAFW_MAXPOP1];
	uint8_t	StageExp[cJU_LEAFW_MAXPOP1];
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================CascadeL()\n");
#endif  // PCAS

//	Get the address of the Leaf (in root structure)
	Pjllw = P_JLLW(ju_BaLPntr(Pjp));

////printf("In CascadeL Pjllw = 0x%lx, Type = %d\n", (Word_t)Pjllw, ju_Type(Pjp));

//	assert(Pjllw[0] == (cJU_LEAFW_MAXPOP1 - 1));
	assert(Pjllw->jlw_Population0 == (cJU_LEAFW_MAXPOP1 - 1));

//	Get pointer to Value area of old Leaf
#ifdef  JUDYL
	Pjv = JL_LEAFWVALUEAREA(Pjllw, cJU_LEAFW_MAXPOP1);
#endif  // JUDYL

//      Now point to Key area, !!! Crap should struct
////	Pjllw++;

// If Leaf is in 1 expanse -- first compress it (compare 1st, last & Index):

////	CIndex = Pjllw[0];	// also used far below
        CIndex = Pjllw->jlw_Leaf[0];
	if (!JU_DIGITATSTATE(CIndex ^ Pjllw->jlw_Leaf[cJU_LEAFW_MAXPOP1 - 1], cJU_ROOTSTATE))
	{
		Word_t PjllRaw;		// pointer to new leaf.
		Pjll_t Pjll;
#ifdef  JUDYL
                Pjv_t  Pjvnew; 		// value area of new leaf.
#endif  // JUDYL

//		Get the common expanse to all elements in Leaf
		StageExp[0] = JU_DIGITATSTATE(CIndex, cJU_ROOTSTATE);

//		Alloc a 3[7] byte Index Leaf
		PjllRaw	= j__udyAllocJLL7(cJU_LEAFW_MAXPOP1, Pjpm);
		if (PjllRaw == 0)
                    return(-1);  // out of memory

		Pjll = P_JLL(PjllRaw);

//		Copy LEAFW to a cJU_JPLEAF7
		j__udyCopyWto7((uint8_t *) Pjll, Pjllw->jlw_Leaf, cJU_LEAFW_MAXPOP1);
#ifdef JUDYL
//		Get the Value area of new Leaf
		Pjvnew = JL_LEAF7VALUEAREA(Pjll, cJU_LEAFW_MAXPOP1);
		JU_COPYMEM(Pjvnew, Pjv, cJU_LEAFW_MAXPOP1);
#endif  // JUDYL


//		Following not needed because cJU_DCDMASK(3[7]) is == 0
//////		StageJP[0].jp_DcdPopO	|= (CIndex & cJU_DCDMASK(3[7]));
                Pjp_t Pjpt = StageJP + 0;
                ju_SetBaLPntr(Pjpt, PjllRaw);
                ju_SetDcdPop0(Pjpt, cJU_LEAFW_MAXPOP1 - 1);
                ju_SetLeafPop0(Pjpt, cJU_LEAFW_MAXPOP1 - 1);
//                JU_JPSETADT(&(StageJP[0]), PjllRaw, cJU_LEAFW_MAXPOP1-1, cJU_JPLEAF7);
                ju_SetJpType(Pjpt, cJU_JPLEAF7);
//		Create a 1 element Linear branch
		if (j__udyCreateBranchL(Pjp, StageJP, StageExp, 1, Pjpm) == -1)
		    return(-1);

//		Change the type of callers JP
//                Pjp->jp_Type = cJU_JPBRANCH_L;
                ju_SetJpType(Pjp, cJU_JPBRANCH_L);

		return(1);
	}

//  Else in 2+ expanses, splay Leaf into smaller leaves at higher compression

//	StageJBB = StageJBBZero;       // zero staged bitmap branch
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 4[8] byte Index Leaf to 3[7] byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAFW_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ Pjllw->jlw_Leaf[End], cJU_ROOTSTATE))
		   )
		{
//			Build a leaf below the previous expanse

			Pjp_t  PjpJP	= StageJP + ExpCnt;
//                        *PjpJP          = (jp_t){};
			Word_t Pop1	= End - Start;
			Word_t expanse = JU_DIGITATSTATE(CIndex, cJU_ROOTSTATE);
			Word_t subexp  = expanse / cJU_BITSPERSUBEXPB;
//
//                      set the bit that is the current expanse
			JU_JBB_BITMAP(&StageJBB, subexp) |= JU_BITPOSMASKB(expanse);
#ifdef SUBEXPCOUNTS
			StageJBB.jbb_subPop1[subexp] += Pop1; // pop of subexpanse
#endif
//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex,
							   cJU_ROOTSTATE);

			if (Pop1 == 1)	// cJU_JPIMMED_3[7]_01
			{
#ifdef  JUDY1
//                        JU_JPSETADT(PjpJP, 0, CIndex, cJ1_JPIMMED_7_01);
                          ju_SetIMM01(PjpJP, 0, CIndex, cJ1_JPIMMED_7_01);
#else   // JUDYL
//                        JU_JPSETADT(PjpJP, Pjv[Start], CIndex, cJL_JPIMMED_7_01);
                          ju_SetIMM01(PjpJP, Pjv[Start], CIndex, cJL_JPIMMED_7_01);
#endif  // JUDYL

			}
#ifdef JUDY1
			else if (Pop1 <= cJ1_IMMED7_MAXPOP1)
			{
//		cJ1_JPIMMED_7_02   :  Judy1 64
//                              Copy to JP as an immediate Leaf
//			     j__udyCopyWto7(ju_PImmed1(PjpJP), Pjllw+Start, 2);
			     j__udyCopyWto7(ju_PImmed1(PjpJP), Pjllw->jlw_Leaf+Start, 2);
//                             PjpJP->jp_Type = cJ1_JPIMMED_7_02;
                            ju_SetJpType(PjpJP, cJ1_JPIMMED_7_02);
			}
#endif  // JUDY1

			else // Linear Leaf JPLEAF7
			{
//		cJU_JPLEAF7
				Word_t PjllRaw;	 // pointer to new leaf.
				Pjll_t Pjll;
#ifdef  JUDYL
		                Pjv_t  Pjvnew; 	 // value area of new leaf.
#endif  // JUDYL
				PjllRaw = j__udyAllocJLL7(Pop1, Pjpm);
				if (PjllRaw == 0)
                                    return(-1);
				Pjll = P_JLL(PjllRaw);

//				j__udyCopyWto7((uint8_t *) Pjll, Pjllw + Start, Pop1);
				j__udyCopyWto7((uint8_t *) Pjll, Pjllw->jlw_Leaf + Start, Pop1);
#ifdef JUDYL
				Pjvnew = JL_LEAF7VALUEAREA(Pjll, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif // JUDYL
                                ju_SetDcdPop0(PjpJP, Pop1 - 1);
                                ju_SetLeafPop0(PjpJP, Pop1 - 1);
                                ju_SetBaLPntr(PjpJP, PjllRaw);
//                                JU_JPSETADT(PjpJP, PjllRaw, Pop1 - 1, cJU_JPLEAF7);
                                ju_SetJpType(PjpJP, cJU_JPLEAF7);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAFW_MAXPOP1)
                            break;

//			New Expanse, Start and Count
//			CIndex = Pjllw[End];
			CIndex = Pjllw->jlw_Leaf[End];
			Start  = End;
		}
	}

// Now put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_L;
            ju_SetJpType(Pjp, cJU_JPBRANCH_L);
	}
	else
	{
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//            Pjp->jp_Type = cJU_JPBRANCH_B;  // cJU_LEAFW is out of sequence
            ju_SetJpType(Pjp, cJU_JPBRANCH_B);
//            Pjp->Jp_Addr0 = PjbbRaw;
            ju_SetBaLPntr(Pjp, PjbbRaw);
	}
	return(1);

} // j__udyCascadeL()
