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
//extern Word_t j__udyBranchBfromParts(Pjp_t, uint8_t *, Word_t, Pjpm_t);

//static const jbb_t StageJBBZero;	// zeroed versions of namesake struct.

// TBD:  There are multiple copies of (some of) these CopyWto3, Copy3toW,
// CopyWto7 and Copy7toW functions in Judy1Cascade.c, JudyLCascade.c, and
// JudyDecascade.c.  These static functions should probably be moved to a
// common place, made macros, or something to avoid having four copies.


// ****************************************************************************
// __ J U D Y   C O P Y  L E A F ?   T O   W O R D

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
// __ J U D Y   C O P Y   W O R D  T O   L E A F ?

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
             int Totcnt =  PSubCount[0] + PSubCount[1] + PSubCount[2] + PSubCount[3];
//             double PerCent = (double)Totcnt / (double)
//printf("==================j__udyStageJBBtoJBB(), JPsubCnt0..3 is: %d %d %d %d = %d\n", 
//                     PSubCount[0],PSubCount[1],PSubCount[2],PSubCount[3], Totcnt);
#endif  // PCAS

// Get memory for new BranchB:

	PjbbRaw = j__udyAllocJBB(Pjpm);
	if (PjbbRaw == 0)
            return(0);
	Pjbb = P_JBB(PjbbRaw);

// Copy staged BranchB into just-allocated BranchB:

	*Pjbb = *PStageJBB;

//int jj = 0;
//for (int ii = 0; ii < 4; ii++)
//{
//     BITMAPB_t bmap;       // portion for this subexpanse
//     bmap = JU_JBB_BITMAP(Pjbb, ii);
//     jj += j__udyCountBitsB(bmap);
//}
//printf("===========jj = %d bits set after j__udyStageJBBtoJBB\n", jj);
//assert(jj == Pop1);

// Allocate the JP subarrays (BJP) for the new BranchB:

	for (subexp = 0; subexp < cJU_NUMSUBEXPB; subexp++)
	{
	    Word_t PjpRaw;
	    Pjp_t  Pjp;
	    Word_t NumJP;       // number of JPs in each subexpanse.

	    if ((NumJP = PSubCount[subexp]) == 0)
                continue;	// empty.

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
        Pjp_t      Pjp,
	uint16_t * Pjll2,	// array of 16-bit indexes.
#ifdef JUDYL
	Pjv_t      Pjv,		// array of associated values.
#endif  // JUDYL
	Word_t     LeafPop1,	// number of indexes/values.
	Pjpm_t     Pjpm)	// jpm_t for JudyAlloc*()/JudyFree*().
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
	Pjlb = P_JLB1(PjlbRaw);

#ifdef JUDYL
//      Note: Now JudyL bitmap leafs Value area are always UNCOMPRESSED
//      and therefore the Value area is indexed by least 8 bits of Key
        Pjv_t Pjvnew = JL_JLB_PVALUE(Pjlb);
#endif  // JUDYL

// Copy Leaf2 Keys (and Values) to LeafB1:
	for (offset = 0; offset < (int)LeafPop1; ++offset)
        {
            uint8_t key = Pjll2[offset];
	    JU_BITMAPSETL(Pjlb, key);
#ifdef JUDYL
            Pjvnew[key] = Pjv[offset];
#endif  // JUDYL
	}

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
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.

//	Word_t	   StageA    [cJU_LEAF4_MAXPOP1];

//	Temp staging for parts(Leaves) of newly splayed leaf
//	Word_t    *PLeafRaw  [cJU_LEAF2_MAXWORDS];      // Staging of Leaf2
	jp_t	   StageJP   [cJU_LEAF2_MAXPOP1];  // JPs of new leaves
	uint8_t	   StageExp  [cJU_LEAF2_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade2(), LeafPop1 = 0x%x, Index = 0x%016lx\n", (int)(cJU_LEAF2_MAXPOP1), Index);
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF2);
	assert(ju_LeafPop1(Pjp) == (cJU_LEAF2_MAXPOP1));

//	Get the address of the Leaf2
	Pjll2_t Pjll2 = P_JLL2(ju_PntrInJp(Pjp));

//      Copy Leaf2 to staging area
//        JU_COPYMEM((Word_t *)PLeaf, PLeaf, cJU_LEAF2_MAXWORDS);

//        j__udyFreeJLL2(PLeafOrigRaw, cJU_LEAF2_MAXPOP1, Pjpm);

//	PLeaf = (uint16_t *)PLeafRaw;

//	And its Value area

#ifdef  JUDYL
//	Get the address of the Leaf and Value area
// Bug?       Pjv_t Pjv = JL_LEAF2VALUEAREA(Pjll2, cJU_LEAF2_MAXPOP1 - 1);
        Pjv_t Pjv = JL_LEAF2VALUEAREA(Pjll2, cJU_LEAF2_MAXPOP1);
#endif  // JUDYL

	CIndex = Pjll2->jl2_Leaf[0];
	StageJBB = (jbb_t){};          // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 2 byte index Leaf to 1 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF2_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ Pjll2->jl2_Leaf[End], 2))
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

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 2);

			if (Pop1 == 1)	// cJU_JPIMMED_1_01
			{
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(2)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_1_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_1_01);
#endif  // JUDYL

#ifdef  PCAS
printf("-----Set IMMED_1_01 in Cascade2, DcdP0 = 0x%016lx, 2nd Word = 0x%016lx\n", PjpJP->jp_Addr0, PjpJP->jp_Addr1);
#endif  // PCAS
			}
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
                                ju_SetPntrInJp(PjpJP, PjvnewRaw);
#endif  // JUDYL
//				Copy to JP as an immediate Leaf
//		                j__udyCopy2to1(PjpJP->jp_LIndex1, PLeaf + Start, Pop1);
				JU_COPYMEM(ju_PImmed1(PjpJP), Pjll2->jl2_Leaf + Start, Pop1);
//				Set Type, Population and Index size
//				PjpJP->jp_Type = cJU_JPIMMED_1_02 + Pop1 - 2;
                                ju_SetJpType(PjpJP, cJU_JPIMMED_1_02 - 2 + Pop1);
			}
#ifdef  JUDYL
			else if (Pop1 <= cJU_LEAF1_MAXPOP1) // still bigger
			{
#ifdef  PCAS
        printf("\n==================Cascade2 create Leaf1 = %lu, ", Pop1);
#endif  // PCAS
				Word_t  Pjll1Raw;	 // pointer to new leaf.
				Pjll1_t Pjll1;
//				Get a new Leaf
				Pjll1Raw = j__udyAllocJLL1(Pop1, Pjpm);
				if (Pjll1Raw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll1 = P_JLL1(Pjll1Raw);

//                              Put the LastKey in the Leaf1
                                Pjll1->jl1_LastKey = (Index & cJU_DCDMASK(1 + 1)) | 
                                        (CIndex & cJU_DCDMASK(1));

//				Copy Indexes to new Leaf
//		                j__udyCopy2to1(Pjll1, PLeaf2+Start, Pop1);
				JU_COPYMEM(Pjll1->jl1_Leaf, Pjll2->jl2_Leaf + Start, Pop1);

                                JU_PADLEAF1(Pjll1->jl1_Leaf, Pop1);

                                Word_t Counts = 0;
                                for (int loff = 0; loff < Pop1; loff++) 
                                {
                                    uint8_t digit8 = Pjll1->jl1_Leaf[loff];
                                    Counts += ((Word_t)1) << ((digit8 / 32) * 8);  // add 1
                                }
                                PjpJP->jp_subLeafPops = Counts;            // Init 8 SubExps

//				Copy to Values to new Leaf
				Pjv_t Pjvnew = JL_LEAF1VALUEAREA(Pjll1, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF1);
                                ju_SetPntrInJp(PjpJP, Pjll1Raw);
			}
#endif  // JUDYL
			else				// biggest
			{
//		cJU_JPLEAF_B1
				Word_t PjlbRaw;
#ifdef  PCAS
        printf("\n==================j__udyJLL2toJLB1 - B1Pop1 = %lu, ", Pop1);
#endif  // PCAS
				PjlbRaw = j__udyJLL2toJLB1(
                                                PjpJP,
						Pjll2->jl2_Leaf + Start,
#ifdef JUDYL
						Pjv + Start,
#endif
						Pop1, Pjpm);
				if (PjlbRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

//                                Put the LastKey in the LeafB1
//                                P_JLB1(PjlbRaw)->jlb_LastKey = (Index & cJU_DCDMASK(1 + 1)) | 
//                                        (CIndex & cJU_DCDMASK(1));
//                              Now Ddd is in the Branch place without Pop0
                                Word_t DcdP0 = (Index & cJU_DCDMASK(1 + 1)) | 
                                    (CIndex & cJU_DCDMASK(1));
                                ju_SetDcdPop0 (PjpJP, DcdP0);
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF_B1);
                                ju_SetPntrInJp(PjpJP, PjlbRaw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF2_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = Pjll2->jl2_Leaf[End];
			Start  = End;
		}
	}
//      Put all the Leaves below a Branch
//      This make it unnecessary to do Dcd Checking at the Leaf Level
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
		Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_L2);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B2);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
//      Add more Dcd bits because jp_t is at a lower level
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(2) | (cJU_LEAF2_MAXPOP1 - 2));
	return(1);

} // j__udyCascade2()

// ****************************************************************************
// __ J U D Y   C A S C A D E 3
//
// Return *Pjp as a (compressed) cJU_LEAF2, cJU_BRANCH_L3, cJU_BRANCH_B3.

FUNCTION int j__udyCascade3(
	Pjp_t	   Pjp,
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF3_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF3_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF3_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade3(), LeafPop1 = 0x%lx, Index = 0x%016lx\n", ju_LeafPop1(Pjp), Index);
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF3);
	assert(ju_LeafPop1(Pjp) == (cJU_LEAF3_MAXPOP1));

//	Get the address of the Leaf
	Pjll3_t Pjll3 = P_JLL3(ju_PntrInJp(Pjp));

//	Extract leaf to Word_t and insert-sort Index into it
	j__udyCopy3toW(StageA, Pjll3->jl3_Leaf, cJU_LEAF3_MAXPOP1);

#ifdef  JUDYL
//	Get the address of the Leaf and Value area
	Pjv_t Pjv = JL_LEAF3VALUEAREA(Pjll3, cJU_LEAF3_MAXPOP1);
#endif  // JUDYL

	CIndex = StageA[0];
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

			Pjp_t  PjpJP   = StageJP + ExpCnt;
			Word_t Pop1    = End - Start;
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
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(3)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_2_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_2_01);
#endif  // JUDYL

#ifdef  PCAS
printf("-----Set IMMED_2_01 in Cascade3, DcdP0 = 0x%016lx, 2nd Word = 0x%016lx\n", PjpJP->jp_Addr0, PjpJP->jp_Addr1);
#endif  // PCAS
			}
			else if (Pop1 <= cJU_IMMED2_MAXPOP1)
			{
//		cJL_JPIMMED_2_02..3:  JudyL 64
//		cJ1_JPIMMED_2_02..7:  Judy1 64
#ifdef JUDYL
//				Alloc is 1st in case of malloc fail

//				Allocate Value area for Immediate Leaf
				Word_t PjvnewRaw = j__udyLAllocJV(Pop1, Pjpm);
				if (PjvnewRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjv_t  Pjvnew = P_JV(PjvnewRaw);

//				Copy to Values to Value Leaf
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
                                ju_SetPntrInJp(PjpJP, PjvnewRaw);
#endif  // JUDYL
//				Copy to Index to JP as an immediate Leaf
//		                j__udyCopyWto2(PjpJP->jp_LIndex2, StageA+Start, Pop1);
				JU_COPYMEM(ju_PImmed2(PjpJP), StageA + Start, Pop1);

//				Set Type, Population and Index size
                                ju_SetJpType(PjpJP, cJU_JPIMMED_2_02 + Pop1 - 2);
			}
			else	// Make a linear leaf2
			{
//		cJU_JPLEAF2
				Word_t Pjll2Raw = j__udyAllocJLL2(Pop1, Pjpm);
				if (Pjll2Raw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll2_t Pjll2 = P_JLL2(Pjll2Raw);

//                              Put the LastKey in the Leaf2
                                Pjll2->jl2_LastKey = (Index & cJU_DCDMASK(2 + 1)) | 
                                        (CIndex & cJU_DCDMASK(2));

//				Copy least 2 bytes per Index of Leaf to new Leaf
//		                j__udyCopyWto2(Pjll, StageA+Start, Pop1);
				JU_COPYMEM(Pjll2->jl2_Leaf, StageA + Start, Pop1);

                                JU_PADLEAF2(Pjll2->jl2_Leaf, Pop1);

                                Word_t Counts = 0;
                                for (int loff = 0; loff < Pop1; loff++) 
                                {
                                    uint16_t digit16 = Pjll2->jl2_Leaf[loff] >> (16 - 3);
                                    Counts += ((Word_t)1) << (digit16 * 8);  // add 1
                                }
                                PjpJP->jp_subLeafPops = Counts;            // Init 8 SubExps

//     printf("In3, Leaf2 = %u %u %u %u %u %u %u %u\n", (uint8_t)(Counts >> 56),(uint8_t)(Counts >> 48),(uint8_t)(Counts >> 40),(uint8_t)(Counts >> 32),(uint8_t)(Counts >> 24),(uint8_t)(Counts >> 16),(uint8_t)(Counts >> 8),(uint8_t)(Counts >> 0));

#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjv_t Pjvnew = JL_LEAF2VALUEAREA(Pjll2, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF2);
                                ju_SetPntrInJp(PjpJP, Pjll2Raw);
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
//      Put all the Leaves below a BranchL or BranchB:
//      This make it unnecessary to do Dcd Checking at the Leaf Level
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
		Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);
            ju_SetJpType(Pjp, cJU_JPBRANCH_L3);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B3);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
//      Add more Dcd bits because jp_t is at a lower level
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(3) | (cJU_LEAF3_MAXPOP1 - 2));
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
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF4_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF4_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF4_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

	assert(ju_Type(Pjp) == cJU_JPLEAF4);
	assert(ju_LeafPop1(Pjp) == (cJU_LEAF4_MAXPOP1));

#ifdef  PCAS
        printf("\n==================Cascade4(), LeafPop1 = 0x%lx, Index = 0x%016lx\n", ju_LeafPop1(Pjp), Index);
#endif  // PCAS

//	Get the address of the Leaf
	Pjll4_t Pjll4 = P_JLL4(ju_PntrInJp(Pjp));

//	Extract 4 byte index Leaf to Word_t
	j__udyCopy4toW(StageA, Pjll4->jl4_Leaf, cJU_LEAF4_MAXPOP1);

#ifdef  JUDYL
//	Get the address of the Leaf and Value area
	Pjv_t Pjv = JL_LEAF4VALUEAREA(Pjll4, cJU_LEAF4_MAXPOP1);
#endif  // JUDYL

	CIndex = StageA[0];
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
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(4)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_3_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_3_01);
#endif  // JUDYL

#ifdef  PCAS
//printf("-----Set IMMED_3_01 in Cascade4, index = 0x%016lx, 2nd Word = 0x%016lx, CIndex = 0x%016lx\n", DcdP0, PjpJP->jp_Addr1, CIndex);
#endif  // PCAS
			}
			else if (Pop1 <= cJU_IMMED3_MAXPOP1)
			{
//		cJL_JPIMMED_3_02   :  JudyL 64
//		cJ1_JPIMMED_3_02..5:  Judy1 64

#ifdef JUDYL
//				Alloc is 1st in case of malloc fail
				Word_t PjvnewRaw = j__udyLAllocJV(Pop1, Pjpm);
				if (PjvnewRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjv_t  Pjvnew = P_JV(PjvnewRaw);

//				Copy to Values to Value Leaf
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
//				PjpJP->jp_PValue = PjvnewRaw;
                                ju_SetPntrInJp(PjpJP, PjvnewRaw);
#endif  // JUDYL
				j__udyCopyWto3(ju_PImmed3(PjpJP), StageA + Start, Pop1);
//				Set type, population and Index size
                                ju_SetJpType(PjpJP, cJU_JPIMMED_3_02 - 2 + Pop1);
			}
			else
			{
//		cJU_JPLEAF3
				Word_t Pjll3Raw = j__udyAllocJLL3(Pop1, Pjpm);
				if (Pjll3Raw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll3_t Pjll3 = P_JLL3(Pjll3Raw);

//                              Put the LastKey in the Leaf3
                                Pjll3->jl3_LastKey = (Index & cJU_DCDMASK(3 + 1)) | 
                                        (CIndex & cJU_DCDMASK(3));

//				Copy Indexes to new Leaf
				j__udyCopyWto3(Pjll3->jl3_Leaf, StageA + Start, Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
				Pjv_t Pjvnew = JL_LEAF3VALUEAREA(Pjll3, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF3);
                                ju_SetPntrInJp(PjpJP, Pjll3Raw);
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
//      Put all the Leaves just below (one Level) a BranchL or BranchB:
//      This make it unnecessary to do Dcd Checking at the Leaf Level
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
		Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);
            ju_SetJpType(Pjp, cJU_JPBRANCH_L4);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B4);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
//      Add more Dcd bits because jp_t is at a lower level
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(4) | (cJU_LEAF4_MAXPOP1 - 2));
	return(1);

} // j__udyCascade4()

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
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF5_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF5_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF5_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade5(), LeafPop1 = 0x%lx, Index = 0x%016lx\n", ju_LeafPop1(Pjp), Index);
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF5);

//	Get the address of the Leaf
	Pjll5_t Pjll5 = P_JLL5(ju_PntrInJp(Pjp));

//	Extract 5 byte index Leaf to Word_t
	j__udyCopy5toW(StageA, Pjll5->jl5_Leaf, cJU_LEAF5_MAXPOP1);

//	Get the address of the Leaf and Value area
#ifdef  JUDYL
        Pjv_t	   Pjv = JL_LEAF5VALUEAREA(Pjll5, cJU_LEAF5_MAXPOP1);
#endif  // JUDYL

	CIndex = StageA[0];
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
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(5)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_4_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_4_01);
#endif  // JUDYL

#ifdef  PCAS
printf("-----Set IMMED_4_01 in Cascade5, DcdP0 = 0x%016lx, 2nd Word = 0x%016lx\n", PjpJP->jp_Addr0, PjpJP->jp_Addr1);
#endif  // PCAS
			}
			else if (Pop1 <= cJU_IMMED4_MAXPOP1)
			{
//		cJL_JPIMMED_4_02   :  JudyL 64
//		cJ1_JPIMMED_4_02..5:  Judy1 64
#ifdef JUDYL
                                assert(Pop1 == 2);
//				Alloc is 1st in case of malloc fail
				Word_t PjvnewRaw = j__udyLAllocJV(Pop1, Pjpm);
				if (PjvnewRaw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjv_t  Pjvnew = P_JV(PjvnewRaw);

//				Copy to Values to Value Leaf
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
//				PjpJP->jp_PValue = PjvnewRaw;
                                ju_SetPntrInJp(PjpJP, PjvnewRaw);
#endif  // JUDYL
				j__udyCopyWto4(ju_PImmed4(PjpJP), StageA + Start, Pop1);
//				Set type, population and Index size
                                ju_SetJpType(PjpJP, cJU_JPIMMED_4_02 - 2 + Pop1);
			}
			else
			{
//		cJU_JPLEAF4
//				Get a new Leaf
				Word_t Pjll4Raw = j__udyAllocJLL4(Pop1, Pjpm);
				if (Pjll4Raw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll4_t Pjll4 = P_JLL4(Pjll4Raw);

//                              Put the LastKey in the Leaf4
                                Pjll4->jl4_LastKey = (Index & cJU_DCDMASK(4 + 1)) | 
                                        (CIndex & cJU_DCDMASK(4));

//				Copy Indexes to new Leaf
				j__udyCopyWto4(Pjll4->jl4_Leaf, StageA + Start, Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
		                Pjv_t  Pjvnew = JL_LEAF4VALUEAREA(Pjll4, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF4);
                                ju_SetPntrInJp(PjpJP, Pjll4Raw);
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
//      Put all the Leaves below a BranchL or BranchB:
//      This make it unnecessary to do Dcd Checking at the Leaf Level
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
		Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);
            ju_SetJpType(Pjp, cJU_JPBRANCH_L5);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B5);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
//      Add more Dcd bits because jp_t is at a lower level
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(5) | (cJU_LEAF5_MAXPOP1 - 2));
	return(1);

} // j__udyCascade5()

// ****************************************************************************
// __ J U D Y   C A S C A D E 6
//
// Cascade from a cJU_JPLEAF6 to one of the following:
//  1. if leaf is in 1 expanse:
//        compress it into a JPLEAF5
//  2. if leaf contains multiple expanses:
//        create linearor bitmap branch containing
//        each new expanse is either a:
//               JPIMMED_5_01 ... JPIMMED_5_03  branch
//               JPIMMED_5_01  branch
//               JPLEAF5

FUNCTION int j__udyCascade6(
	Pjp_t	   Pjp,
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF6_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF6_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF6_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade6(), LeafPop1 = 0x%lx, Index = 0x%016lx\n", ju_LeafPop1(Pjp), Index);
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF6);
	assert(ju_LeafPop1(Pjp) == (cJU_LEAF6_MAXPOP1));

//	Get the address of the Leaf
	Pjll6_t Pjll6 = P_JLL6(ju_PntrInJp(Pjp));

//	Extract 6 byte index Leaf to Word_t
	j__udyCopy6toW(StageA, Pjll6->jl6_Leaf, cJU_LEAF6_MAXPOP1);

#ifdef  JUDYL
//	Get the address of the Leaf and Value area
        Pjv_t Pjv = JL_LEAF6VALUEAREA(Pjll6, cJU_LEAF6_MAXPOP1);
#endif  // JUDYL

	CIndex = StageA[0];
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
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(6)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_5_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_5_01);
#endif  // JUDYL

#ifdef  PCAS
printf("-----Set IMMED_5_01 in Cascade6, DcdP0 = 0x%016lx, 2nd Word = 0x%016lx\n", PjpJP->jp_Addr0, PjpJP->jp_Addr1);
#endif  // PCAS
			}
#ifdef JUDY1
			else if (Pop1 <= cJ1_IMMED5_MAXPOP1)
			{
//		cJ1_JPIMMED_5_02..3: Judy1 64

//                              Copy to Index to JP as an immediate Leaf
				j__udyCopyWto5(ju_PImmed5(PjpJP), StageA + Start, Pop1);

//                              Set pointer, type, population and Index size
                                ju_SetJpType(PjpJP, cJ1_JPIMMED_5_02 + Pop1 - 2);
			}
#endif  // JUDY1
			else
			{
//		cJU_JPLEAF5
				Word_t  Pjll5Raw;	 // pointer to new leaf.
				Pjll5_t Pjll5;

//				Get a new Leaf
				Pjll5Raw = j__udyAllocJLL5(Pop1, Pjpm);
				if (Pjll5Raw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll5 = P_JLL5(Pjll5Raw);

//                              Put the LastKey in the Leaf5
                                Pjll5->jl5_LastKey = (Index & cJU_DCDMASK(5 + 1)) | 
                                    (CIndex & cJU_DCDMASK(5));

//				Copy Indexes to new Leaf
				j__udyCopyWto5(Pjll5->jl5_Leaf, StageA + Start, Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
		                Pjv_t Pjvnew = JL_LEAF5VALUEAREA(Pjll5, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF5);
                                ju_SetPntrInJp(PjpJP, Pjll5Raw);
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
//      Put all the Leaves below a BranchL or BranchB:
//      This make it unnecessary to do Dcd Checking at the Leaf Level
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
		Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);
            ju_SetJpType(Pjp, cJU_JPBRANCH_L6);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B6);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
//      Add more Dcd bits because jp_t is at a lower level
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(6) | (cJU_LEAF6_MAXPOP1 - 2));
	return(1);

} // j__udyCascade6()

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
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t     CIndex;	// current Index word.

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	   StageJP   [cJU_LEAF7_MAXPOP1];  // JPs of new leaves
	Word_t	   StageA    [cJU_LEAF7_MAXPOP1];
	uint8_t	   StageExp  [cJU_LEAF7_MAXPOP1];  // Expanses of new leaves
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade7(), LeafPop1 = 0x%x, Index = 0x%016lx\n", (int)ju_LeafPop1(Pjp), Index);
#endif  // PCAS

	assert(ju_Type(Pjp) == cJU_JPLEAF7);
	assert(ju_LeafPop1(Pjp) == (cJU_LEAF7_MAXPOP1));

//	Get the address of the Leaf
	Pjll7_t Pjll7 = P_JLL7(ju_PntrInJp(Pjp));

//	Extract 7 byte index Leaf to Word_t
	j__udyCopy7toW(StageA, Pjll7->jl7_Leaf, cJU_LEAF7_MAXPOP1);

#ifdef  JUDYL
//	Get the address of the Leaf and Value area
        Pjv_t Pjv = JL_LEAF7VALUEAREA(Pjll7, cJU_LEAF7_MAXPOP1);
#endif  // JUDYL

	CIndex = StageA[0];
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

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, 7);

			if (Pop1 == 1)	// cJU_JPIMMED_6_01
			{
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(7)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_6_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_6_01);
#endif  // JUDYL

#ifdef  PCAS
printf("-----Set IMMED_6_01 in Cascade7, DcdP0 = 0x%016lx, 2nd Word = 0x%016lx\n", PjpJP->jp_Addr0, PjpJP->jp_Addr1);
#endif  // PCAS
			}
#ifdef JUDY1
			else if (Pop1 == cJ1_IMMED6_MAXPOP1)
			{
//		cJ1_JPIMMED_6_02:    Judy1 64

//                              Copy to Index to JP as an immediate Leaf
				j__udyCopyWto6(ju_PImmed6(PjpJP), StageA + Start, 2);

//                              Set pointer, type, population and Index size
                                ju_SetJpType(PjpJP, cJ1_JPIMMED_6_02);
			}
#endif  // JUDY1
			else
			{
//		cJU_JPLEAF6
//				Get a new Leaf
				Word_t Pjll6Raw = j__udyAllocJLL6(Pop1, Pjpm);
				if (Pjll6Raw == 0)
					FREEALLEXIT(ExpCnt, StageJP, Pjpm);

				Pjll6_t Pjll6 = P_JLL6(Pjll6Raw);

//                              Put the LastKey in the Leaf6
                                Pjll6->jl6_LastKey = (Index & cJU_DCDMASK(6 + 1)) | 
                                    (CIndex & cJU_DCDMASK(6));

//				Copy Indexes to new Leaf
				j__udyCopyWto6(Pjll6->jl6_Leaf, StageA + Start, Pop1);
#ifdef JUDYL
//				Copy to Values to new Leaf
		                Pjv_t  Pjvnew = JL_LEAF6VALUEAREA(Pjll6, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif  // JUDYL
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF6);
                                ju_SetPntrInJp(PjpJP, Pjll6Raw);
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
//      Put all the Leaves below a BranchL or BranchB:
//      This make it unnecessary to do Dcd Checking at the Leaf Level
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
		Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);
            ju_SetJpType(Pjp, cJU_JPBRANCH_L7);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B7);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
//      Add more Dcd bits because jp_t is at a lower level
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(7) | (cJU_LEAF7_MAXPOP1 - 2));
	return(1);

} // j__udyCascade7()

// ****************************************************************************
// __ J U D Y   C A S C A D E 8
//
// (Compressed) cJU_LEAF3[7], cJ1_JPBRANCH_L.
//
// Cascade from a LEAF8 (under Pjp) to one of the following:
//  1. if LEAF8 is in 1 expanse:
//        create linear branch with a JPLEAF3[7] under it
//  2. LEAF8 contains multiple expanses:
//        create linear or bitmap branch containing new expanses
//        each new expanse is either a: 32   64
//               JPIMMED_7_01  branch    N    Y
//               JPLEAF7                 N    Y

FUNCTION int j__udyCascade8(
	Pjp_t	   Pjp,
        Word_t     Index,
	Pjpm_t	   Pjpm)
{
	Word_t	   End, Start;	// temporaries.
	Word_t	   ExpCnt;	// count of expanses of splay.
	Word_t	   CIndex;	// current Index word.

//	Temp staging for parts(Leaves) of newly splayed leaf
	jp_t	StageJP [cJU_LEAF8_MAXPOP1];
	uint8_t	StageExp[cJU_LEAF8_MAXPOP1];
	uint8_t	   SubJPCount[cJU_NUMSUBEXPB];     // JPs in each subexpanse
	jbb_t      StageJBB;                       // staged bitmap branch

#ifdef  PCAS
        printf("\n==================Cascade8(), Pop1 = %ld, Index = 0x%016lx\n", Pjpm->jpm_Pop0 + 1, Index);
#endif  // PCAS

	assert(Pjpm->jpm_Pop0 == (cJU_LEAF8_MAXPOP1-1));

//	Get the address of the Leaf (in root structure)
	Pjll8_t	 Pjll8 = P_JLL8(ju_PntrInJp(Pjp));

#ifdef  JUDYL
//	Get the address of the Leaf and Value area
	Pjv_t Pjv = JL_LEAF8VALUEAREA(Pjll8, cJU_LEAF8_MAXPOP1);
#endif  // JUDYL

        CIndex = Pjll8->jl8_Leaf[0];    // 1st Index
	StageJBB = (jbb_t){};           // zero staged bitmap branch
	ZEROJP(SubJPCount, cJU_NUMSUBEXPB);

//	Splay the 8 byte Index Leaf to 7 byte Index Leaves
	for (ExpCnt = Start = 0, End = 1; ; End++)
	{
//		Check if new expanse or last one
		if (	(End == cJU_LEAF8_MAXPOP1)
				||
			(JU_DIGITATSTATE(CIndex ^ Pjll8->jl8_Leaf[End], cJU_ROOTSTATE))
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

//                      count number of expanses in each subexpanse
			SubJPCount[subexp]++;

//			Save byte expanse of leaf
			StageExp[ExpCnt] = JU_DIGITATSTATE(CIndex, cJU_ROOTSTATE);

			if (Pop1 == 1)	// cJU_JPIMMED_7_01
			{
	                    Word_t DcdP0 = (Index & cJU_DCDMASK(8)) | CIndex;
#ifdef  JUDY1
                            ju_SetIMM01(PjpJP, 0, DcdP0, cJ1_JPIMMED_7_01);
#else   // JUDYL
                            ju_SetIMM01(PjpJP, Pjv[Start], DcdP0, cJL_JPIMMED_7_01);
#endif  // JUDYL

#ifdef  PCAS
printf("-----Set IMMED_7_01 in Cascade8, DcdP0 = 0x%016lx, 2nd Word = 0x%016lx\n", PjpJP->jp_Addr0, PjpJP->jp_Addr1);
#endif  // PCAS
			}
#ifdef JUDY1
			else if (Pop1 == cJ1_IMMED7_MAXPOP1)
			{
//		cJ1_JPIMMED_7_02   :  Judy1 64
//                          Copy to JP as an immediate Leaf
			    j__udyCopyWto7(ju_PImmed7(PjpJP), Pjll8->jl8_Leaf + Start, 2);
                            ju_SetJpType(PjpJP, cJ1_JPIMMED_7_02);
			}
#endif  // JUDY1
			else // Linear Leaf JPLEAF7
			{
//		cJU_JPLEAF7
				Word_t Pjll7Raw = j__udyAllocJLL7(Pop1, Pjpm);
				if (Pjll7Raw == 0)
                                    return(-1);

				Pjll7_t Pjll7 = P_JLL7(Pjll7Raw);

//                              Put the LastKey in the Leaf7
                                Pjll7->jl7_LastKey = (Index & cJU_DCDMASK(7 + 1)) | 
                                    (CIndex & cJU_DCDMASK(7));

				j__udyCopyWto7(Pjll7->jl7_Leaf, Pjll8->jl8_Leaf + Start, Pop1);
#ifdef JUDYL
		                Pjv_t  Pjvnew = JL_LEAF7VALUEAREA(Pjll7, Pop1);
				JU_COPYMEM(Pjvnew, Pjv + Start, Pop1);
#endif // JUDYL
                                ju_SetLeafPop1(PjpJP, Pop1);
                                ju_SetJpType  (PjpJP, cJU_JPLEAF7);
                                ju_SetPntrInJp(PjpJP, Pjll7Raw);
			}
			ExpCnt++;
//                      Done?
			if (End == cJU_LEAF8_MAXPOP1)
                            break;

//			New Expanse, Start and Count
			CIndex = Pjll8->jl8_Leaf[End];
			Start  = End;
		}
	}

//      Put all the Leaves below a BranchL or BranchB:
	if (ExpCnt <= cJU_BRANCHLMAXJPS) // put the Leaves below a BranchL
	{
	    if (j__udyCreateBranchL(Pjp, StageJP, StageExp, ExpCnt,
			Pjpm) == -1) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_L8);
	}
	else
	{
            StageJBB.jbb_NumJPs = ExpCnt;
	    Word_t PjbbRaw = j__udyStageJBBtoJBB(&StageJBB, StageJP, SubJPCount, Pjpm);
	    if (PjbbRaw == 0) FREEALLEXIT(ExpCnt, StageJP, Pjpm);

            ju_SetJpType(Pjp, cJU_JPBRANCH_B8);
            ju_SetPntrInJp(Pjp, PjbbRaw);
	}
        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(8) | (cJU_LEAF8_MAXPOP1 - 2));
	return(1);

} // j__udyCascade8()
