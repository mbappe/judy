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

// @(#) $Revision: 4.25 $ $Source: /judy/src/JudyCommon/JudyDecascade.c $
//
// "Decascade" support functions for JudyDel.c:  These functions convert
// smaller-index-size leaves to larger-index-size leaves, and also, bitmap
// leaves (LeafB1s) to Leaf1s, and some types of branches to smaller branches
// at the same index size.  Some "decascading" occurs explicitly in JudyDel.c,
// but rare or large subroutines appear as functions here, and the overhead to
// call them is negligible.
//
// Compile with one of -DJUDY1 or -DJUDYL.  Note:  Function names are converted
// to Judy1 or JudyL specific values by external #defines.

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#endif
#ifdef JUDYL
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"


// ****************************************************************************
// __ J U D Y   C O P Y   2   T O   3
//
// Copy one or more 2-byte Indexes to a series of 3-byte Indexes.

FUNCTION static void j__udyCopy2to3(
	uint8_t *  PDest,	// to where to copy 3-byte Indexes.
	uint16_t * PSrc,	// from where to copy 2-byte indexes.
	Word_t     Pop1,	// number of Indexes to copy.
	Word_t     MSByte)	// most-significant byte, prefix to each Index.
{
	assert(Pop1);

        do {
	    JU_COPY3_LONG_TO_PINDEX(PDest, MSByte | *PSrc);
            PSrc++;
	    PDest += 3;
        } while (--Pop1);

} // j__udyCopy2to3()



// ****************************************************************************
// __ J U D Y   C O P Y   3   T O   4
//
// Copy one or more 3-byte Indexes to a series of 4-byte Indexes.

FUNCTION static void j__udyCopy3to4(
	uint32_t * PDest,	// to where to copy 4-byte Indexes.
	uint8_t *  PSrc,	// from where to copy 3-byte indexes.
	Word_t     Pop1,	// number of Indexes to copy.
	Word_t     MSByte)	// most-significant byte, prefix to each Index.
{
	Word_t	   Temp;	// for building 4-byte Index.

	assert(Pop1);

        do {
	    JU_COPY3_PINDEX_TO_LONG(Temp, PSrc);
	    Temp |= MSByte;
	    PSrc += 3;
	    *PDest++ = Temp;		// truncates to uint32_t.
        } while (--Pop1);

} // j__udyCopy3to4()


// ****************************************************************************
// __ J U D Y   C O P Y   4   T O   5
//
// Copy one or more 4-byte Indexes to a series of 5-byte Indexes.

FUNCTION static void j__udyCopy4to5(
	uint8_t *  PDest,	// to where to copy 4-byte Indexes.
	uint32_t * PSrc,	// from where to copy 4-byte indexes.
	Word_t     Pop1,	// number of Indexes to copy.
	Word_t     MSByte)	// most-significant byte, prefix to each Index.
{
	Word_t	   Temp;	// for building 5-byte Index.

	assert(Pop1);

        do {
	    Temp = MSByte | *PSrc++;
	    JU_COPY5_LONG_TO_PINDEX(PDest, Temp);
	    PDest += 5;
        } while (--Pop1);

} // j__udyCopy4to5()


// ****************************************************************************
// __ J U D Y   C O P Y   5   T O   6
//
// Copy one or more 5-byte Indexes to a series of 6-byte Indexes.

FUNCTION static void j__udyCopy5to6(
	uint8_t * PDest,	// to where to copy 6-byte Indexes.
	uint8_t * PSrc,		// from where to copy 5-byte indexes.
	Word_t    Pop1,		// number of Indexes to copy.
	Word_t    MSByte)	// most-significant byte, prefix to each Index.
{
	Word_t	  Temp;		// for building 6-byte Index.

	assert(Pop1);

        do {
	    JU_COPY5_PINDEX_TO_LONG(Temp, PSrc);
	    Temp |= MSByte;
	    JU_COPY6_LONG_TO_PINDEX(PDest, Temp);
	    PSrc  += 5;
	    PDest += 6;
        } while (--Pop1);

} // j__udyCopy5to6()


// ****************************************************************************
// __ J U D Y   C O P Y   6   T O   7
//
// Copy one or more 6-byte Indexes to a series of 7-byte Indexes.

FUNCTION static void j__udyCopy6to7(
	uint8_t * PDest,	// to where to copy 6-byte Indexes.
	uint8_t * PSrc,		// from where to copy 5-byte indexes.
	Word_t    Pop1,		// number of Indexes to copy.
	Word_t    MSByte)	// most-significant byte, prefix to each Index.
{
	Word_t	  Temp;		// for building 6-byte Index.

	assert(Pop1);

        do {
	    JU_COPY6_PINDEX_TO_LONG(Temp, PSrc);
	    Temp |= MSByte;
	    JU_COPY7_LONG_TO_PINDEX(PDest, Temp);
	    PSrc  += 6;
	    PDest += 7;
        } while (--Pop1);

} // j__udyCopy6to7()




// ****************************************************************************
// __ J U D Y   C O P Y   7   T O   W
//
// Copy one or more 7-byte Indexes to a series of longs (words, always 8-byte).

FUNCTION static void j__udyCopy7toW(
	PWord_t   PDest,	// to where to copy full-word Indexes.
	uint8_t * PSrc,		// from where to copy 7-byte indexes.
	Word_t    Pop1,		// number of Indexes to copy.
	Word_t    MSByte)	// most-significant byte, prefix to each Index.
{
	assert(Pop1);

        do {
	    JU_COPY7_PINDEX_TO_LONG(*PDest, PSrc);
	    *PDest++ |= MSByte;
	    PSrc     += 7;
        } while (--Pop1);

} // j__udyCopy7toW()



// ****************************************************************************
// __ J U D Y   B R A N C H   B   T O   B R A N C H   L
//
// When a BranchB shrinks to have few enough JPs, call this function to convert
// it to a BranchL.  Return 1 for success, or -1 for failure (with details in
// Pjpm).

FUNCTION int j__udyBranchBToBranchL(
	Pjp_t	Pjp,		// points to BranchB to shrink.
	Pjpm_t	Pjpm)		// for global accounting.
{
	Word_t	PjbbRaw;	// old BranchB to shrink.
	Pjbb_t	Pjbb;
	Word_t	PjblRaw;	// new BranchL to create.
	Pjbl_t	Pjbl;
	Word_t	Digit;		// in BranchB.
	Word_t  NumJPs;		// non-null JPs in BranchB.
	uint8_t Expanse[cJU_BRANCHLMAXJPS];	// for building jbl_Expanse[].
	Pjp_t	Pjpjbl;		// current JP in BranchL.
	Word_t  SubExp;		// in BranchB.

	assert(ju_Type(Pjp) >= cJU_JPBRANCH_B2);
	assert(ju_Type(Pjp) <= cJU_JPBRANCH_B);

//        PjbbRaw	= Pjp->Jp_Addr0;
        PjbbRaw = ju_PntrInJp(Pjp);
	Pjbb	= P_JBB(PjbbRaw);

// Copy 1-byte subexpanse digits from BranchB to temporary buffer for BranchL,
// for each bit set in the BranchB:
//
// TBD:  The following supports variable-sized linear branches, but they are no
// longer variable; this could be simplified to save the copying.
//
// TBD:  Since cJU_BRANCHLMAXJP == 7 now, and cJU_BRANCHUNUMJPS == 256, the
// following might be inefficient; is there a faster way to do it?  At least
// skip wholly empty subexpanses?

	for (NumJPs = Digit = 0; Digit < cJU_BRANCHUNUMJPS; ++Digit)
	{
	    if (JU_BITMAPTESTB(Pjbb, Digit))
	    {
		Expanse[NumJPs++] = Digit;
		assert(NumJPs <= cJU_BRANCHLMAXJPS);	// required of caller.
	    }
	}

// Allocate and populate the BranchL:

	if ((PjblRaw = j__udyAllocJBL(Pjpm)) == 0) return(-1);
	Pjbl = P_JBL(PjblRaw);

	JU_COPYMEM(Pjbl->jbl_Expanse, Expanse, NumJPs);

	Pjbl->jbl_NumJPs = NumJPs;

// Copy JPs from each BranchB subexpanse subarray:

	Pjpjbl = P_JP(Pjbl->jbl_jp);	// start at first JP in array.

	for (SubExp = 0; SubExp < cJU_NUMSUBEXPB; ++SubExp)
	{
	    Word_t PjpRaw = JU_JBB_PJP(Pjbb, SubExp);	// current Pjp.
	    Pjp_t Pjp;

	    if (PjpRaw == 0) continue;  // skip empty subexpanse.
	    Pjp = P_JP(PjpRaw);

	    NumJPs = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, SubExp));
	    assert(NumJPs);
	    JU_COPYMEM(Pjpjbl, Pjp, NumJPs);	 // one subarray at a time.

	    Pjpjbl += NumJPs;
	    j__udyFreeJBBJP(PjpRaw, NumJPs, Pjpm);	// subarray.
	}
	j__udyFreeJBB(PjbbRaw, Pjpm);		// BranchB itself.

// Finish up:  Calculate new JP type (same index size = level in new class),
// and tie new BranchB into parent JP:

// printf("\n============================ calculate Type by cJU_JPBRANCH_L - cJU_JPBRANCH_B = %d\n", (int)(cJU_JPBRANCH_L - cJU_JPBRANCH_B));
//        Pjp->jp_Type += cJU_JPBRANCH_L - cJU_JPBRANCH_B;
        ju_SetJpType(Pjp, ju_Type(Pjp) + cJU_JPBRANCH_L - cJU_JPBRANCH_B);
//        Pjp->Jp_Addr0  = PjblRaw;
	ju_SetPntrInJp(Pjp, PjblRaw);

	return(1);

} // j__udyBranchBToBranchL()


#ifdef notdef
// ****************************************************************************
// __ J U D Y   B R A N C H   U   T O   B R A N C H   B
//
// When a BranchU shrinks to need little enough memory, call this function to
// convert it to a BranchB to save memory (at the cost of some speed).  Return
// 1 for success, or -1 for failure (with details in Pjpm).
//
// TBD:  Fill out if/when needed.  Not currently used in JudyDel.c for reasons
// explained there.

FUNCTION int j__udyBranchUToBranchB(
	Pjp_t	Pjp,		// points to BranchU to shrink.
	Pjpm_t	Pjpm)		// for global accounting.
{
	assert(FALSE);
	return(1);
}
#endif // notdef

// ****************************************************************************
// __ J U D Y   L E A F   B 1   T O   L E A F   1
//
// Shrink a bitmap leaf (cJU_LEAFB1) to linear leaf (cJU_JPLEAF1).
// Return 1 for success, or -1 for failure (with details in Pjpm).
//
// Note:  This function is different than the other JudyLeaf*ToLeaf*()
// functions because it receives a Pjp, not just a leaf, and handles its own
// allocation and free, in order to allow the caller to continue with a LeafB1
// if allocation fails.

FUNCTION int j__udyLeafB1ToLeaf1(
	Pjp_t	  Pjp,		// points to LeafB1 to shrink.
	Pjpm_t	  Pjpm)		// for global accounting.
{
	assert(ju_Type(Pjp) == cJU_JPLEAF_B1);
	assert(((ju_DcdPop0(Pjp) & 0xFF) + 1) == cJU_LEAF1_MAXPOP1);
        assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));

#ifdef  PCAS
printf("Dec: j__udyLeafB1ToLeaf1, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

//      Allocate JPLEAF1 and prepare pointers:
	Word_t Pjll1Raw = j__udyAllocJLL1(cJU_LEAF1_MAXPOP1, Pjpm);
	if (Pjll1Raw == 0)
	    return(-1);

	Pjll1_t	 Pjll1 = P_JLL1(Pjll1Raw);
        uint8_t *Pleaf1 = Pjll1->jl1_Leaf;

#ifdef JUDYL
	Pjv_t  PjvNew = JL_LEAF1VALUEAREA(Pjll1, cJL_LEAF1_MAXPOP1);
#endif // JUDYL

	Word_t PjlbRaw	= ju_PntrInJp(Pjp);
	Pjlb_t Pjlb     = P_JLB(PjlbRaw);

//      Copy 1-byte indexes from old LeafB1 to new Leaf1:
	for (int digit = 0; digit < cJU_BRANCHUNUMJPS; ++digit)
        {
	    if (JU_BITMAPTESTL(Pjlb, digit))
		*Pleaf1++ = (uint8_t)digit;
        }
#ifdef JUDYL
//      Copy all old-LeafB1 Value areas from value subarrays to new Leaf1:
	Word_t pop1;
	for (Word_t SubExp = 0; SubExp < cJU_NUMSUBEXPL; ++SubExp)
	{
	    Word_t PjvRaw = JL_JLB_PVALUE(Pjlb, SubExp);
	    Pjv_t Pjv     = P_JV(PjvRaw);

	    if (Pjv == (Pjv_t) NULL) continue;	// skip empty subarray.

	    pop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, SubExp));  // subarray.
	    assert(pop1);

	    JU_COPYMEM(PjvNew, Pjv, pop1);		// copy value areas.
	    j__udyLFreeJV(PjvRaw, pop1, Pjpm);
	    PjvNew += pop1;				// advance through new.
	}
//      number of Keys == number of Values
//printf("----JudyL Pjv pop1 = %d, cJU_LEAF1_MAXPOP1 = %d\n", (int)pop1, (int)cJU_LEAF1_MAXPOP1);
//        assert(pop1 == cJU_LEAF1_MAXPOP1); only max of 64
#endif // JUDYL

// Finish up:  Free the old LeafB1 and plug the new Leaf1 into the JP:
// Note:  jp_DcdPopO and jp_LeafPop0 does NOT change here.
	j__udyFreeJLB1(PjlbRaw, Pjpm);
        ju_SetPntrInJp(Pjp, Pjll1Raw);                  // new ^
        ju_SetJpType(Pjp, cJU_JPLEAF1);                 // new jp_Type

	return(1);

} // j__udyLeafB1ToLeaf1()

// ****************************************************************************
// __ J U D Y   L E A F   1   T O   L E A F   2
//
// Copy 1-byte Indexes from a LeafB1 or Leaf1 to 2-byte Indexes in a Leaf2.
// Pjp MUST be one of:  cJU_JPLEAF_B1, cJU_JPLEAF1, or cJU_JPIMMED_1_*.
// Return number of Indexes copied.
//
// TBD:  In this and all following functions, the caller should already be able
// to compute the Pop1 return value, so why return it?

FUNCTION Word_t  j__udyLeaf1orB1ToLeaf2(
	uint16_t * PLeaf2,	// destination uint16_t * Index portion of leaf.
#ifdef JUDYL
	Pjv_t	   Pjv2,	// destination value part of leaf.
#endif
	Pjp_t	   Pjp,		// 1-byte-index object from which to copy.
	Word_t     MSByte,	// most-significant byte, prefix to each Index.
	Pjpm_t	   Pjpm)	// for global accounting.
{
	Word_t	   Pop1;	// number of Keys in leaf[1B] or IMMED_1_xx.
#ifdef JUDYL
        Word_t     Pjv1Raw;	// source object value area.
        Pjv_t	   Pjv1;
#endif  // JUDYL

	switch (ju_Type(Pjp))
	{
// JPLEAF_B1:
	case cJU_JPLEAF_B1:
	{
//            printf("B1: ju_DcdPop0 0xFF) + 1) = %lu, cJU_LEAF2_MAXPOP1 = %d\n", (ju_DcdPop0(Pjp) & 0xFF) + 1, (int)cJU_LEAF2_MAXPOP1);
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));

            Word_t PjlbRaw = ju_PntrInJp(Pjp);
            Pjlb_t Pjlb    = P_JLB(PjlbRaw);

// Copy 1-byte indexes from old LeafB1 to new Leaf2, including splicing in
// the missing MSByte needed in the Leaf2:

//	    for (Word_t digit = 0, Pop1 = 0; digit < cJU_BRANCHUNUMJPS; digit++)
	    Pop1 = 0; 
	    for (Word_t digit = 0; digit < 256; digit++)
            {
		if (JU_BITMAPTESTL(Pjlb, digit)) 
                {
////                    printf("================Digit = 0x%lx, Pop1 = %d\n", digit, (int)Pop1);
                    *PLeaf2++ = MSByte | digit;
                    Pop1++;
                }
            }
//            printf("Pop1 = %ld\n", Pop1);
//            printf("ju_LeafPop1 = %d, B1: ret: Pop1 = %d, ju_DcdPop1 = %d\n", (int)ju_LeafPop0(Pjp) + 1, (int)Pop1, (int)(ju_DcdPop0(Pjp) & 0xFF) + 1);
            assert(Pop1 == (ju_DcdPop0(Pjp) & 0xFF) + 1);

#ifdef JUDYL
// Copy all old-LeafB1 value areas from value subarrays to new Leaf2:

	    for (int subexp = 0; subexp < cJU_NUMSUBEXPL; subexp++)
	    {
	        Word_t pop1;
		Pjv1Raw = JL_JLB_PVALUE(Pjlb, subexp);
		if (Pjv1Raw == 0) continue;	        // skip empty.
		Pjv1 = P_JV(Pjv1Raw);

		pop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, subexp));
		assert(pop1);

		JU_COPYMEM(Pjv2, Pjv1, pop1);	        // copy value areas.
		Pjv2 += pop1;			        // advance through new.
		j__udyLFreeJV(Pjv1Raw, pop1, Pjpm);
	    }
#endif // JUDYL
	    j__udyFreeJLB1(PjlbRaw, Pjpm);  // LeafB1 itself.
	    return(Pop1);
	} // case cJU_JPLEAF_B1

// JPLEAF1:
	case cJU_JPLEAF1:
	{
//            printf("L1: ju_DcdPop0 0xFF) + 1) = %lu, cJU_LEAF2_MAXPOP1 = %d\n", (ju_DcdPop0(Pjp) & 0xFF) + 1, (int)cJU_LEAF2_MAXPOP1);
	    Word_t   Pjll1Raw = ju_PntrInJp(Pjp);
	    Pjll1_t  Pjll1    = P_JLL1(Pjll1Raw);

//          if Leaf1 to Leaf2, Population is unchanged
	    Pop1 = ju_LeafPop0(Pjp) + 1;

// Copy all Index bytes including splicing in missing MSByte needed in Leaf2
// (plus, for JudyL, value areas):

	    JUDYLCODE(Pjv1 = JL_LEAF1VALUEAREA(Pjll1, Pop1);)
	    for (int offset = 0; offset < Pop1; offset++)
	    {
		PLeaf2[offset] = MSByte | Pjll1->jl1_Leaf[offset];
		JUDYLCODE(Pjv2[offset] = Pjv1[offset];)
	    }
	    j__udyFreeJLL1(Pjll1Raw, Pop1, Pjpm);
	    return(Pop1);
	}

// JPIMMED_1_01:
//      Note: This case is done before the call to this routine to save time
	case cJU_JPIMMED_1_01:
	{
//printf("--------Immed Key = 0x%016lx\n", ju_IMM01Key(Pjp));
	    PLeaf2[0] = ju_IMM01Key(Pjp);	// see above.
	    JUDYLCODE(Pjv2[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}

// JPIMMED_1_0[2+]:

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
#endif
	{
//          population of IMMEDs is from the jp_Type
	    Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
//            printf("IMMED_1_%02ld: ju_DcdPop0 0xFF) + 1) = %lu, cJU_LEAF2_MAXPOP1 = %d\n", Pop1, (ju_DcdPop0(Pjp) & 0xFF) + 1, (int)cJU_LEAF2_MAXPOP1);
#ifdef JUDYL
	    Pjv1Raw = ju_PntrInJp(Pjp);
            Pjv1    = P_JV(Pjv1Raw);
#endif  // JUDYL
	    for (int offset = 0; offset < Pop1; offset++)
	    {
		PLeaf2[offset] = MSByte | ju_PImmed1(Pjp)[offset];
#ifdef JUDYL
		Pjv2  [offset] = Pjv1[offset];
#endif  // JUDYL
	    }
#ifdef JUDYL
	    j__udyLFreeJV(Pjv1Raw, Pop1, Pjpm);
#endif  // JUDYL
	    return(Pop1);
	}
#ifdef  JUDY1
	case cJ1_JPFULLPOPU1:   // cant happen, B1
        {
            printf("cJ1_JPFULLPOPU1: ju_DcdPop0 & 0xFF) + 1) = %lu, cJU_LEAF2_MAXPOP1 = %d\n", (ju_DcdPop0(Pjp) & 0xFF) + 1, (int)cJU_LEAF2_MAXPOP1);
            assert(FALSE);
            return(Pop1);
        }
#endif

// UNEXPECTED CASES, including JPNULL1, should be handled by caller:

	default: 
            assert(FALSE); 
            break;

	} // switch
        return (0);

//printf("\n===============++++++++++++++++++++++++++++++++++===FAILED jpType = %d\n", ju_Type(Pjp));

} // j__udyLeaf1orB1ToLeaf2()


// *****************************************************************************
// __ J U D Y   L E A F   2   T O   L E A F   3
//
// Copy 2-byte Indexes from a Leaf2 to 3-byte Indexes in a Leaf3.
// Pjp MUST be one of:  cJU_JPLEAF2 or cJU_JPIMMED_2_*.
// Return number of Indexes copied.
//
// Note:  By the time this function is called to compress a level-3 branch to a
// Leaf3, the branch has no narrow pointers under it, meaning only level-2
// objects are below it and must be handled here.

FUNCTION Word_t  j__udyLeaf2ToLeaf3(
	uint8_t * PLeaf3,	// destination "uint24_t *" Index part of leaf.
#ifdef JUDYL
	Pjv_t	  Pjv3,		// destination value part of leaf.
#endif
	Pjp_t	  Pjp,		// 2-byte-index object from which to copy.
	Word_t    MSByte,	// most-significant byte, prefix to each Index.
	Pjpm_t	  Pjpm)		// for global accounting.
{
	Word_t	  Pop1;		// Indexes in leaf.
#ifdef  JUDYL
	Word_t  Pjv2Raw;	// source object value area.
        Pjv_t   Pjv2;
#endif

#ifdef  PCAS
printf("j__udyLeaf2ToLeaf3, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

	switch (ju_Type(Pjp))
	{
// JPLEAF2:
	case cJU_JPLEAF2:
	{
	    uint16_t * PLeaf2 = (uint16_t *) P_JLL(ju_PntrInJp(Pjp));

	    Pop1 = ju_LeafPop0(Pjp) + 1;
	    j__udyCopy2to3(PLeaf3, PLeaf2, Pop1, MSByte);
#ifdef JUDYL
	    Pjv2 = JL_LEAF2VALUEAREA(PLeaf2, Pop1);
	    JU_COPYMEM(Pjv3, Pjv2, Pop1);
#endif
            j__udyFreeJLL2(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    return(Pop1);
	}


// JPIMMED_2_01:
//
// Note:  jp_DcdPopO has 3 [7] bytes of Index (all but most significant byte),
// so the "assignment" to PLeaf3[] is exact [truncates] and MSByte is not
// needed.

	case cJU_JPIMMED_2_01:
	{
	    JU_COPY3_LONG_TO_PINDEX(PLeaf3, ju_IMM01Key(Pjp));	// see above.
//            JUDYLCODE(Pjv3[0] = Pjp->jp_PValue;)
	    JUDYLCODE(Pjv3[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}


// JPIMMED_2_0[2+]:

	case cJU_JPIMMED_2_02:
	case cJU_JPIMMED_2_03:
	case cJU_JPIMMED_2_04:
#ifdef  JUDY1
	case cJ1_JPIMMED_2_05:
	case cJ1_JPIMMED_2_06:
	case cJ1_JPIMMED_2_07:
#endif
	{
	    uint16_t * PLeaf2 = ju_PImmed2(Pjp);

	    Pop1 = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2; assert(Pop1);
	    j__udyCopy2to3(PLeaf3, PLeaf2, Pop1, MSByte);
#ifdef JUDYL
            Pjv2Raw = ju_PntrInJp(Pjp);
	    Pjv2    = P_JV(Pjv2Raw);
	    JU_COPYMEM(Pjv3, Pjv2, Pop1);
	    j__udyLFreeJV(Pjv2Raw, Pop1, Pjpm);
#endif
	    return(Pop1);
	}


// UNEXPECTED CASES, including JPNULL2, should be handled by caller:

	default: 
            assert(FALSE); 
            break;

	} // switch
	return (0);

} // j__udyLeaf2ToLeaf3()



// ****************************************************************************
// __ J U D Y   L E A F   3   T O   L E A F   4
//
// Copy 3-byte Indexes from a Leaf3 to 4-byte Indexes in a Leaf4.
// Pjp MUST be one of:  cJU_JPLEAF3 or cJU_JPIMMED_3_*.
// Return number of Indexes copied.
//
// Note:  By the time this function is called to compress a level-4 branch to a
// Leaf4, the branch has no narrow pointers under it, meaning only level-3
// objects are below it and must be handled here.

FUNCTION Word_t  j__udyLeaf3ToLeaf4(
	uint32_t * PLeaf4,	// destination uint32_t * Index part of leaf.
#ifdef JUDYL
	Pjv_t	   Pjv4,	// destination value part of leaf.
#endif
	Pjp_t	   Pjp,		// 3-byte-index object from which to copy.
	Word_t     MSByte,	// most-significant byte, prefix to each Index.
	Pjpm_t	   Pjpm)	// for global accounting.
{
	Word_t	   Pop1;	// Indexes in leaf.
JUDYLCODE(Word_t   Pjv3Raw;)	// source object value area.
JUDYLCODE(Pjv_t	   Pjv3;)

#ifdef  PCAS
printf("j__udyLeaf3ToLeaf4, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

	switch (ju_Type(Pjp))
	{
// JPLEAF3:
	case cJU_JPLEAF3:
	{
            uint8_t * PLeaf3 = (uint8_t *) P_JLL(ju_PntrInJp(Pjp));

	    Pop1 = ju_LeafPop0(Pjp) + 1;
	    j__udyCopy3to4(PLeaf4, (uint8_t *) PLeaf3, Pop1, MSByte);
#ifdef JUDYL
	    Pjv3 = JL_LEAF3VALUEAREA(PLeaf3, Pop1);
	    JU_COPYMEM(Pjv4, Pjv3, Pop1);
#endif
	    j__udyFreeJLL3(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    return(Pop1);
	}


// JPIMMED_3_01:
//
// Note:  jp_DcdPopO has 7 bytes of Index (all but most significant byte), so
// the assignment to PLeaf4[] truncates and MSByte is not needed.

	case cJU_JPIMMED_3_01:
	{
	    PLeaf4[0] = ju_IMM01Key(Pjp);	// see above.
	    JUDYLCODE(Pjv4[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}


// JPIMMED_3_0[2+]:

	case cJU_JPIMMED_3_02:
#ifdef JUDY1
	case cJ1_JPIMMED_3_03:
	case cJ1_JPIMMED_3_04:
	case cJ1_JPIMMED_3_05:
#endif
	{
	    uint8_t * PLeaf3 = ju_PImmed3(Pjp);

	    JUDY1CODE(Pop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;)
	    JUDYLCODE(Pop1 = 2;)

	    j__udyCopy3to4(PLeaf4, PLeaf3, Pop1, MSByte);
#ifdef JUDYL
	    Pjv3Raw = ju_PntrInJp(Pjp);
	    Pjv3    = P_JV(Pjv3Raw);
	    JU_COPYMEM(Pjv4, Pjv3, Pop1);
	    j__udyLFreeJV(Pjv3Raw, Pop1, Pjpm);
#endif
	    return(Pop1);
	}
//      UNEXPECTED CASES, including JPNULL3, should be handled by caller:
	default: 
            assert(FALSE); 
            break;

	} // switch
        return (0);

} // j__udyLeaf3ToLeaf4()


// Note:  In all following j__udyLeaf*ToLeaf*() functions, JPIMMED_*_0[2+]
// cases exist for Judy1 (&& 64-bit) only.  JudyL has no equivalent Immeds.


// *****************************************************************************
// __ J U D Y   L E A F   4   T O   L E A F   5
//
// Copy 4-byte Indexes from a Leaf4 to 5-byte Indexes in a Leaf5.
// Pjp MUST be one of:  cJU_JPLEAF4 or cJU_JPIMMED_4_*.
// Return number of Indexes copied.
//
// Note:  By the time this function is called to compress a level-5 branch to a
// Leaf5, the branch has no narrow pointers under it, meaning only level-4
// objects are below it and must be handled here.

FUNCTION Word_t  j__udyLeaf4ToLeaf5(
	uint8_t * PLeaf5,	// destination "uint40_t *" Index part of leaf.
#ifdef JUDYL
	Pjv_t	  Pjv5,		// destination value part of leaf.
#endif
	Pjp_t	  Pjp,		// 4-byte-index object from which to copy.
	Word_t    MSByte,	// most-significant byte, prefix to each Index.
	Pjpm_t	  Pjpm)		// for global accounting.
{
	Word_t	  Pop1;		// Indexes in leaf.
JUDYLCODE(Pjv_t	  Pjv4;)	// source object value area.

#ifdef  PCAS
printf("j__udyLeaf4ToLeaf5, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

	switch (ju_Type(Pjp))
	{
// JPLEAF4:
	case cJU_JPLEAF4:
	{
	    uint32_t * PLeaf4 = (uint32_t *) P_JLL(ju_PntrInJp(Pjp));

	    Pop1 = ju_LeafPop0(Pjp) + 1;
            assert(Pop1);
	    j__udyCopy4to5(PLeaf5, PLeaf4, Pop1, MSByte);
#ifdef JUDYL
	    Pjv4 = JL_LEAF4VALUEAREA(PLeaf4, Pop1);
	    JU_COPYMEM(Pjv5, Pjv4, Pop1);
#endif
	    j__udyFreeJLL4(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    return(Pop1);
	}


// JPIMMED_4_01:
//
// Note:  jp_DcdPopO has 7 bytes of Index (all but most significant byte), so
// the assignment to PLeaf5[] truncates and MSByte is not needed.

	case cJU_JPIMMED_4_01:
	{
	    JU_COPY5_LONG_TO_PINDEX(PLeaf5, ju_IMM01Key(Pjp));	// see above.
//            JUDYLCODE(Pjv5[0] = Pjp->jp_PValue;)
	    JUDYLCODE(Pjv5[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}

// JPIMMED_4_0[4+]:

	case cJU_JPIMMED_4_02:
#ifdef JUDY1
	case cJ1_JPIMMED_4_03:
#endif  // JUDY1
	{
	    uint32_t * PLeaf4 = ju_PImmed4(Pjp);

	    Pop1 = ju_Type(Pjp) - cJU_JPIMMED_4_02 + 2;
	    j__udyCopy4to5(PLeaf5, PLeaf4, Pop1, MSByte);
#ifdef JUDYL
            assert(Pop1 == 2);
	    Word_t Pjv4Raw = ju_PntrInJp(Pjp);
	    Pjv4    = P_JV(Pjv4Raw);
	    JU_COPYMEM(Pjv5, Pjv4, 2);
	    j__udyLFreeJV(Pjv4Raw, 2, Pjpm);
#endif  // JUDYL
	    return(Pop1);
	}


// UNEXPECTED CASES, including JPNULL4, should be handled by caller:

	default: 
            assert(FALSE); 
            break;

	} // switch
	return (0);

} // j__udyLeaf4ToLeaf5()


// ****************************************************************************
// __ J U D Y   L E A F   5   T O   L E A F   6
//
// Copy 5-byte Indexes from a Leaf5 to 6-byte Indexes in a Leaf6.
// Pjp MUST be one of:  cJU_JPLEAF5 or cJU_JPIMMED_5_*.
// Return number of Indexes copied.
//
// Note:  By the time this function is called to compress a level-6 branch to a
// Leaf6, the branch has no narrow pointers under it, meaning only level-5
// objects are below it and must be handled here.

FUNCTION Word_t  j__udyLeaf5ToLeaf6(
	uint8_t * PLeaf6,	// destination uint8_t * Index part of leaf.
#ifdef JUDYL
	Pjv_t	  Pjv6,		// destination value part of leaf.
#endif
	Pjp_t	  Pjp,		// 5-byte-index object from which to copy.
	Word_t    MSByte,	// most-significant byte, prefix to each Index.
	Pjpm_t	  Pjpm)		// for global accounting.
{
	Word_t	  Pop1;		// Indexes in leaf.
JUDYLCODE(Pjv_t	  Pjv5;)	// source object value area.

#ifdef  PCAS
printf("j__udyLeaf5ToLeaf6, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

	switch (ju_Type(Pjp))
	{
// JPLEAF5:
	case cJU_JPLEAF5:
	{
	    uint8_t * PLeaf5 = (uint8_t *) P_JLL(ju_PntrInJp(Pjp));

	    Pop1 = ju_LeafPop0(Pjp) + 1;
	    j__udyCopy5to6(PLeaf6, PLeaf5, Pop1, MSByte);
#ifdef JUDYL
	    Pjv5 = JL_LEAF5VALUEAREA(PLeaf5, Pop1);
	    JU_COPYMEM(Pjv6, Pjv5, Pop1);
#endif
	    j__udyFreeJLL5(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    return(Pop1);
	}


// JPIMMED_5_01:
//
// Note:  jp_DcdPopO has 7 bytes of Index (all but most significant byte), so
// the assignment to PLeaf6[] truncates and MSByte is not needed.

	case cJU_JPIMMED_5_01:
	{
	    JU_COPY6_LONG_TO_PINDEX(PLeaf6, ju_IMM01Key(Pjp));	// see above.
//            JUDYLCODE(Pjv6[0] = Pjp->jp_PValue;)
	    JUDYLCODE(Pjv6[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}


#ifdef JUDY1

// JPIMMED_5_0[2+]:

	case cJ1_JPIMMED_5_02:
	case cJ1_JPIMMED_5_03:
	{
//            uint8_t * PLeaf5 = (uint8_t *) (Pjp->jp_1Index1);
	    uint8_t * PLeaf5 = ju_PImmed5(Pjp);

	    Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 2;
	    j__udyCopy5to6(PLeaf6, PLeaf5, Pop1, MSByte);
	    return(Pop1);
	}
#endif // JUDY1


// UNEXPECTED CASES, including JPNULL5, should be handled by caller:

	default: 
            assert(FALSE); 
            break;

	} // switch
	return(0);

} // j__udyLeaf5ToLeaf6()


// *****************************************************************************
// __ J U D Y   L E A F   6   T O   L E A F   7
//
// Copy 6-byte Indexes from a Leaf2 to 7-byte Indexes in a Leaf7.
// Pjp MUST be one of:  cJU_JPLEAF6 or cJU_JPIMMED_6_*.
// Return number of Indexes copied.
//
// Note:  By the time this function is called to compress a level-7 branch to a
// Leaf7, the branch has no narrow pointers under it, meaning only level-6
// objects are below it and must be handled here.

FUNCTION Word_t  j__udyLeaf6ToLeaf7(
	uint8_t * PLeaf7,	// destination "uint24_t *" Index part of leaf.
#ifdef JUDYL
	Pjv_t	  Pjv7,		// destination value part of leaf.
#endif
	Pjp_t	  Pjp,		// 6-byte-index object from which to copy.
	Word_t    MSByte,	// most-significant byte, prefix to each Index.
	Pjpm_t	  Pjpm)		// for global accounting.
{
	Word_t	  Pop1;		// Indexes in leaf.
JUDYLCODE(Pjv_t	  Pjv6;)	// source object value area.

#ifdef  PCAS
printf("j__udyLeaf6ToLeaf7, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

	switch (ju_Type(Pjp))
	{
// JPLEAF6:
	case cJU_JPLEAF6:
	{
	    uint8_t * PLeaf6 = (uint8_t *) P_JLL(ju_PntrInJp(Pjp));

	    Pop1 = ju_LeafPop0(Pjp) + 1;
	    j__udyCopy6to7(PLeaf7, PLeaf6, Pop1, MSByte);
#ifdef JUDYL
	    Pjv6 = JL_LEAF6VALUEAREA(PLeaf6, Pop1);
	    JU_COPYMEM(Pjv7, Pjv6, Pop1);
#endif
	    j__udyFreeJLL6(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    return(Pop1);
	}


// JPIMMED_6_01:
//
// Note:  jp_DcdPopO has 7 bytes of Index (all but most significant byte), so
// the "assignment" to PLeaf7[] is exact and MSByte is not needed.

	case cJU_JPIMMED_6_01:
	{
	    JU_COPY7_LONG_TO_PINDEX(PLeaf7, ju_IMM01Key(Pjp));	// see above.
//            JUDYLCODE(Pjv7[0] = Pjp->jp_PValue;)
	    JUDYLCODE(Pjv7[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}


#ifdef JUDY1

// JPIMMED_6_02:

	case cJ1_JPIMMED_6_02:
	{
//            uint8_t * PLeaf6 = (uint8_t *) (Pjp->jp_1Index1);
	    uint8_t * PLeaf6 = ju_PImmed6(Pjp);

	    j__udyCopy6to7(PLeaf7, PLeaf6, /* Pop1 = */ 2, MSByte);
	    return(2);
	}
#endif // JUDY1


// UNEXPECTED CASES, including JPNULL6, should be handled by caller:

	default: 
            assert(FALSE); 
            break;

	} // switch
	return(0);

} // j__udyLeaf6ToLeaf7()





// ****************************************************************************
// __ J U D Y   L E A F   7   T O   L E A F   W
//
// Copy 7-byte Indexes from a Leaf7 to 8-byte Indexes in a LeafW.
// Pjp MUST be one of:  cJU_JPLEAF7 or cJU_JPIMMED_7_*.
// Return number of Indexes copied.
//
// Note:  By the time this function is called to compress a level-L branch to a
// LeafW, the branch has no narrow pointers under it, meaning only level-7
// objects are below it and must be handled here.

FUNCTION Word_t  j__udyLeaf7ToLeafW(
	PWord_t	PWordW,	        // destination Index part of leaf.
#ifdef JUDYL
	Pjv_t	Pjv,		// destination value part of leaf.
#endif
	Pjp_t	Pjp,		// 7-byte-index object from which to copy.
	Word_t	MSByte,		// most-significant byte, prefix to each Index.
	Pjpm_t	Pjpm)		// for global accounting.
{
	Word_t	Pop1;		// Indexes in leaf.
JUDYLCODE(Pjv_t	Pjv7;)		// source object value area.

#ifdef  PCAS
printf("j__udyLeaf7ToLeafW, Pop0 = 0x%lx, DcdPop0 = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp));
#endif  // PCAS

	switch (ju_Type(Pjp))
	{
// JPLEAF7:
	case cJU_JPLEAF7:
	{
	    uint8_t * PLeaf7 = (uint8_t *) P_JLL(ju_PntrInJp(Pjp));

	    Pop1 = ju_LeafPop0(Pjp) + 1;
	    j__udyCopy7toW(PWordW, PLeaf7, Pop1, MSByte);
#ifdef JUDYL
	    Pjv7 = JL_LEAF7VALUEAREA(PLeaf7, Pop1);
	    JU_COPYMEM(Pjv, Pjv7, Pop1);
#endif
	    j__udyFreeJLL7(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    return(Pop1);
	}


// JPIMMED_7_01:
//
// Note:  jp_DcdPopO has 7 bytes of Index (all but most significant byte), and
// MSByte must be ord in.

	case cJU_JPIMMED_7_01:
	{
//	      Pjllw[0] = MSByte | JU_JPDCDPOP0(Pjp);		// see above.
	    PWordW[0] = MSByte | ju_IMM01Key(Pjp);		// see above.
//            JUDYLCODE(Pjv[0] = Pjp->jp_PValue;)
	    JUDYLCODE(Pjv[0] = ju_ImmVal_01(Pjp);)
	    return(1);
	}


#ifdef JUDY1

// JPIMMED_7_02:

	case cJ1_JPIMMED_7_02:
	{
//            uint8_t * PLeaf7 = (uint8_t *) (Pjp->jp_1Index1);
	    uint8_t * PLeaf7 = ju_PImmed7(Pjp);

	    j__udyCopy7toW(PWordW, PLeaf7, /* Pop1 = */ 2, MSByte);
	    return(2);
	}
#endif


// UNEXPECTED CASES, including JPNULL7, should be handled by caller:

	default: 
            assert(FALSE); 
            break;

	} // switch
	return(0);

} // j__udyLeaf7ToLeafW()

