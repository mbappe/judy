#ifndef _JUDYPRIVATE1L_INCLUDED
#define	_JUDYPRIVATE1L_INCLUDED
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

// @(#) $Revision: 4.31 $ $Source: /judy/src/JudyCommon/JudyPrivate1L.h $

// ****************************************************************************
// Declare common cJU_* names for JP Types that occur in both Judy1 and JudyL,
// for use by code that ifdefs JUDY1 and JUDYL.  Only JP Types common to both
// Judy1 and JudyL are #defined here with equivalent cJU_* names.  JP Types
// unique to only Judy1 or JudyL are listed in comments, so the type lists
// match the Judy1.h and JudyL.h files.
//
// This file also defines cJU_* for other JP-related constants and functions
// that some shared JUDY1/JUDYL code finds handy.
//
// At least in principle this file should be included AFTER Judy1.h or JudyL.h.
//
// WARNING:  This file must be kept consistent with the enums in Judy1.h and
// JudyL.h.
//
// TBD:  You might think, why not define common cJU_* enums in, say,
// JudyPrivate.h, and then inherit them into superset enums in Judy1.h and
// JudyL.h?  The problem is that the enum lists for each class (cJ1_* and
// cJL_*) must be numerically "packed" into the correct order, for two reasons:
// (1) allow the compiler to generate "tight" switch statements with no wasted
// slots (although this is not very big), and (2) allow calculations using the
// enum values, although this is also not an issue if the calculations are only
// within each cJ*_JPIMMED_*_* class and the members are packed within the
// class.

#ifdef JUDY1

#define	cJU_JRPNULL		cJ1_JRPNULL
#define	cJU_JPNULL1		cJ1_JPNULL1
#define	cJU_JPNULL2		cJ1_JPNULL2
#define	cJU_JPNULL3		cJ1_JPNULL3
#define	cJU_JPNULL4		cJ1_JPNULL4
#define	cJU_JPNULL5		cJ1_JPNULL5
#define	cJU_JPNULL6		cJ1_JPNULL6
#define	cJU_JPNULL7		cJ1_JPNULL7
#define	cJU_JPNULLMAX		cJ1_JPNULLMAX
#define	cJU_JPBRANCH1_L2	cJ1_JPBRANCH1_L2
#define	cJU_JPBRANCH_L2  	cJ1_JPBRANCH_L2  
#define	cJU_JPBRANCH1_L3	cJ1_JPBRANCH1_L3
#define	cJU_JPBRANCH_L3  	cJ1_JPBRANCH_L3  
#define	cJU_JPBRANCH1_L4	cJ1_JPBRANCH1_L4
#define	cJU_JPBRANCH_L4  	cJ1_JPBRANCH_L4  
#define	cJU_JPBRANCH1_L5	cJ1_JPBRANCH1_L5
#define	cJU_JPBRANCH_L5  	cJ1_JPBRANCH_L5  
#define	cJU_JPBRANCH1_L6	cJ1_JPBRANCH1_L6
#define	cJU_JPBRANCH_L6  	cJ1_JPBRANCH_L6  
#define	cJU_JPBRANCH1_L7	cJ1_JPBRANCH1_L7
#define	cJU_JPBRANCH_L7  	cJ1_JPBRANCH_L7  
#define	cJU_JPBRANCH1_L8	cJ1_JPBRANCH1_L8
#define	cJU_JPBRANCH_L8  	cJ1_JPBRANCH_L8  
#define	j__U_BranchBJPPopToWords j__1_BranchBJPPopToWords
#define	cJU_JPBRANCH_B2		cJ1_JPBRANCH_B2
#define	cJU_JPBRANCH_B2_p	cJ1_JPBRANCH_B2_p
#define	cJU_JPBRANCH_B3		cJ1_JPBRANCH_B3
#define	cJU_JPBRANCH_B3_p	cJ1_JPBRANCH_B3_p
#define	cJU_JPBRANCH_B4		cJ1_JPBRANCH_B4
#define	cJU_JPBRANCH_B4_p	cJ1_JPBRANCH_B4_p
#define	cJU_JPBRANCH_B5		cJ1_JPBRANCH_B5
#define	cJU_JPBRANCH_B5_p	cJ1_JPBRANCH_B5_p
#define	cJU_JPBRANCH_B6		cJ1_JPBRANCH_B6
#define	cJU_JPBRANCH_B6_p	cJ1_JPBRANCH_B6_p
#define	cJU_JPBRANCH_B7		cJ1_JPBRANCH_B7
#define	cJU_JPBRANCH_B7_p	cJ1_JPBRANCH_B7_p
#define	cJU_JPBRANCH_B8		cJ1_JPBRANCH_B8
#define	cJU_JPBRANCH_B8_p	cJ1_JPBRANCH_B8_p
#define	cJU_JPBRANCH_U2		cJ1_JPBRANCH_U2
#define	cJU_JPBRANCH_U2_p	cJ1_JPBRANCH_U2_p
#define	cJU_JPBRANCH_U3		cJ1_JPBRANCH_U3
#define	cJU_JPBRANCH_U3_p	cJ1_JPBRANCH_U3_p
#define	cJU_JPBRANCH_U4		cJ1_JPBRANCH_U4
#define	cJU_JPBRANCH_U4_p	cJ1_JPBRANCH_U4_p
#define	cJU_JPBRANCH_U5		cJ1_JPBRANCH_U5
#define	cJU_JPBRANCH_U5_p	cJ1_JPBRANCH_U5_p
#define	cJU_JPBRANCH_U6		cJ1_JPBRANCH_U6
#define	cJU_JPBRANCH_U6_p	cJ1_JPBRANCH_U6_p
#define	cJU_JPBRANCH_U7		cJ1_JPBRANCH_U7
#define	cJU_JPBRANCH_U7_p	cJ1_JPBRANCH_U7_p
#define	cJU_JPBRANCH_U8		cJ1_JPBRANCH_U8
#define	cJU_JPBRANCH_U8_p	cJ1_JPBRANCH_U8_p
#define	cJU_JPLEAF2		cJ1_JPLEAF2
#define	cJU_JPLEAF3		cJ1_JPLEAF3
#define	cJU_JPLEAF4		cJ1_JPLEAF4
#define	cJU_JPLEAF5		cJ1_JPLEAF5
#define	cJU_JPLEAF6		cJ1_JPLEAF6
#define	cJU_JPLEAF7		cJ1_JPLEAF7
#define	cJU_JPLEAF8		cJ1_JPLEAF8
#define	cJU_JPLEAF_B1U		cJ1_JPLEAF_B1U
//				cJ1_JPFULLPOPU1
#define	cJU_JPIMMED_1_01	cJ1_JPIMMED_1_01
#define	cJU_JPIMMED_2_01	cJ1_JPIMMED_2_01
#define	cJU_JPIMMED_3_01	cJ1_JPIMMED_3_01
#define	cJU_JPIMMED_4_01	cJ1_JPIMMED_4_01
#define	cJU_JPIMMED_5_01	cJ1_JPIMMED_5_01
#define	cJU_JPIMMED_6_01	cJ1_JPIMMED_6_01
#define	cJU_JPIMMED_7_01	cJ1_JPIMMED_7_01
#define	cJU_JPIMMED_1_02	cJ1_JPIMMED_1_02
#define	cJU_JPIMMED_1_03	cJ1_JPIMMED_1_03
#define	cJU_JPIMMED_1_04	cJ1_JPIMMED_1_04
#define	cJU_JPIMMED_1_05	cJ1_JPIMMED_1_05
#define	cJU_JPIMMED_1_06	cJ1_JPIMMED_1_06
#define	cJU_JPIMMED_1_07	cJ1_JPIMMED_1_07
#define	cJU_JPIMMED_1_08	cJ1_JPIMMED_1_08
//				cJ1_JPIMMED_1_09
//				cJ1_JPIMMED_1_10
//				cJ1_JPIMMED_1_11
//				cJ1_JPIMMED_1_12
//				cJ1_JPIMMED_1_13
//				cJ1_JPIMMED_1_14
//				cJ1_JPIMMED_1_15
#define	cJU_JPIMMED_2_02	cJ1_JPIMMED_2_02
#define	cJU_JPIMMED_2_03	cJ1_JPIMMED_2_03
#define	cJU_JPIMMED_2_04	cJ1_JPIMMED_2_04
//				cJ1_JPIMMED_2_05
//				cJ1_JPIMMED_2_06
//				cJ1_JPIMMED_2_07
#define	cJU_JPIMMED_3_02	cJ1_JPIMMED_3_02
//				cJ1_JPIMMED_3_03
//				cJ1_JPIMMED_3_04
//				cJ1_JPIMMED_3_05
#define	cJU_JPIMMED_4_02	cJ1_JPIMMED_4_02
//				cJ1_JPIMMED_4_03
//				cJ1_JPIMMED_5_02
//				cJ1_JPIMMED_6_02
//				cJ1_JPIMMED_7_02
#define	cJU_JPIMMED_CAP		cJ1_JPIMMED_CAP

#else // JUDYL ****************************************************************

#define	cJU_JRPNULL		cJL_JRPNULL
#define	cJU_JPNULL1		cJL_JPNULL1
#define	cJU_JPNULL2		cJL_JPNULL2
#define	cJU_JPNULL3		cJL_JPNULL3
#define	cJU_JPNULL4		cJL_JPNULL4
#define	cJU_JPNULL5		cJL_JPNULL5
#define	cJU_JPNULL6		cJL_JPNULL6
#define	cJU_JPNULL7		cJL_JPNULL7
#define	cJU_JPNULLMAX		cJL_JPNULLMAX
#define	cJU_JPBRANCH1_L2	cJL_JPBRANCH1_L2
#define	cJU_JPBRANCH_L2  	cJL_JPBRANCH_L2  
#define	cJU_JPBRANCH1_L3	cJL_JPBRANCH1_L3
#define	cJU_JPBRANCH_L3  	cJL_JPBRANCH_L3  
#define	cJU_JPBRANCH1_L4	cJL_JPBRANCH1_L4
#define	cJU_JPBRANCH_L4  	cJL_JPBRANCH_L4  
#define	cJU_JPBRANCH1_L5	cJL_JPBRANCH1_L5
#define	cJU_JPBRANCH_L5  	cJL_JPBRANCH_L5  
#define	cJU_JPBRANCH1_L6	cJL_JPBRANCH1_L6
#define	cJU_JPBRANCH_L6  	cJL_JPBRANCH_L6  
#define	cJU_JPBRANCH1_L7	cJL_JPBRANCH1_L7
#define	cJU_JPBRANCH_L7  	cJL_JPBRANCH_L7  
#define	cJU_JPBRANCH1_L8	cJL_JPBRANCH1_L8
#define	cJU_JPBRANCH_L8  	cJL_JPBRANCH_L8  
#define	j__U_BranchBJPPopToWords j__L_BranchBJPPopToWords
#define	cJU_JPBRANCH_B2		cJL_JPBRANCH_B2
#define	cJU_JPBRANCH_B2_p	cJL_JPBRANCH_B2_p
#define	cJU_JPBRANCH_B3		cJL_JPBRANCH_B3
#define	cJU_JPBRANCH_B3_p	cJL_JPBRANCH_B3_p
#define	cJU_JPBRANCH_B4		cJL_JPBRANCH_B4
#define	cJU_JPBRANCH_B4_p	cJL_JPBRANCH_B4_p
#define	cJU_JPBRANCH_B5		cJL_JPBRANCH_B5
#define	cJU_JPBRANCH_B5_p	cJL_JPBRANCH_B5_p
#define	cJU_JPBRANCH_B6		cJL_JPBRANCH_B6
#define	cJU_JPBRANCH_B6_p	cJL_JPBRANCH_B6_p
#define	cJU_JPBRANCH_B7		cJL_JPBRANCH_B7
#define	cJU_JPBRANCH_B7_p	cJL_JPBRANCH_B7_p
#define	cJU_JPBRANCH_B8		cJL_JPBRANCH_B8
#define	cJU_JPBRANCH_B8_p	cJL_JPBRANCH_B8_p
#define	cJU_JPBRANCH_U2		cJL_JPBRANCH_U2
#define	cJU_JPBRANCH_U2_p	cJL_JPBRANCH_U2_p
#define	cJU_JPBRANCH_U3		cJL_JPBRANCH_U3
#define	cJU_JPBRANCH_U3_p	cJL_JPBRANCH_U3_p
#define	cJU_JPBRANCH_U4		cJL_JPBRANCH_U4
#define	cJU_JPBRANCH_U4_p	cJL_JPBRANCH_U4_p
#define	cJU_JPBRANCH_U5		cJL_JPBRANCH_U5
#define	cJU_JPBRANCH_U5_p	cJL_JPBRANCH_U5_p
#define	cJU_JPBRANCH_U6		cJL_JPBRANCH_U6
#define	cJU_JPBRANCH_U6_p	cJL_JPBRANCH_U6_p
#define	cJU_JPBRANCH_U7		cJL_JPBRANCH_U7
#define	cJU_JPBRANCH_U7_p	cJL_JPBRANCH_U7_p
#define	cJU_JPBRANCH_U8		cJL_JPBRANCH_U8
#define	cJU_JPBRANCH_U8_p	cJL_JPBRANCH_U8_p
#define	cJU_JPLEAF1		cJL_JPLEAF1
#define	cJU_JPLEAF2		cJL_JPLEAF2
#define	cJU_JPLEAF3		cJL_JPLEAF3
#define	cJU_JPLEAF4		cJL_JPLEAF4
#define	cJU_JPLEAF5		cJL_JPLEAF5
#define	cJU_JPLEAF6		cJL_JPLEAF6
#define	cJU_JPLEAF7		cJL_JPLEAF7
#define	cJU_JPLEAF8		cJL_JPLEAF8
#define	cJU_JPLEAF_B1U		cJL_JPLEAF_B1U
#define cJU_JLEAFMAX            cJL_JLEAFMAX
#define	cJU_JPIMMED_1_01	cJL_JPIMMED_1_01
#define	cJU_JPIMMED_2_01	cJL_JPIMMED_2_01
#define	cJU_JPIMMED_3_01	cJL_JPIMMED_3_01
#define	cJU_JPIMMED_4_01	cJL_JPIMMED_4_01
#define	cJU_JPIMMED_5_01	cJL_JPIMMED_5_01
#define	cJU_JPIMMED_6_01	cJL_JPIMMED_6_01
#define	cJU_JPIMMED_7_01	cJL_JPIMMED_7_01
#define	cJU_JPIMMED_1_02	cJL_JPIMMED_1_02
#define	cJU_JPIMMED_1_03	cJL_JPIMMED_1_03
#define	cJU_JPIMMED_1_04	cJL_JPIMMED_1_04
#define	cJU_JPIMMED_1_05	cJL_JPIMMED_1_05
#define	cJU_JPIMMED_1_06	cJL_JPIMMED_1_06
#define	cJU_JPIMMED_1_07	cJL_JPIMMED_1_07
#define	cJU_JPIMMED_1_08	cJL_JPIMMED_1_08
#define	cJU_JPIMMED_2_02	cJL_JPIMMED_2_02
#define	cJU_JPIMMED_2_03	cJL_JPIMMED_2_03
#define	cJU_JPIMMED_2_04	cJL_JPIMMED_2_04
#define	cJU_JPIMMED_3_02	cJL_JPIMMED_3_02
#define	cJU_JPIMMED_4_02	cJL_JPIMMED_4_02
#define	cJU_JPIMMED_CAP		cJL_JPIMMED_CAP

#endif // JUDYL


// ****************************************************************************
// cJU*_ other than JP types:

#ifdef JUDY1

#define	cJU_LEAF8_MAXPOP1	cJ1_LEAF8_MAXPOP1
#define	cJU_LEAF1_MAXPOP1	cJ1_LEAF1_MAXPOP1
#define	cJU_LEAF2_MAXPOP1	cJ1_LEAF2_MAXPOP1
#define cJU_LEAF2_MAXWORDS      cJ1_LEAF2_MAXWORDS 
#define	cJU_LEAF3_MAXPOP1	cJ1_LEAF3_MAXPOP1
#define	cJU_LEAF4_MAXPOP1	cJ1_LEAF4_MAXPOP1
#define	cJU_LEAF5_MAXPOP1	cJ1_LEAF5_MAXPOP1
#define	cJU_LEAF6_MAXPOP1	cJ1_LEAF6_MAXPOP1
#define	cJU_LEAF7_MAXPOP1	cJ1_LEAF7_MAXPOP1
#define cJU_JPMAXIMMED1         cJ1_JPMAXIMMED1
#define	cJU_JPMAXBRANCH         cJ1_JPMAXBRANCH
#define	cJU_JPMAXBRANCH_noDCD	cJ1_JPMAXBRANCH_noDCD
#define cJU_JLEAFMAX            cJ1_JLEAFMAX
#define	cJU_IMMED1_MAXPOP1	cJ1_IMMED1_MAXPOP1
#define	cJU_IMMED2_MAXPOP1	cJ1_IMMED2_MAXPOP1
#define	cJU_IMMED3_MAXPOP1	cJ1_IMMED3_MAXPOP1
#define	cJU_IMMED4_MAXPOP1	cJ1_IMMED4_MAXPOP1
#define	cJU_IMMED5_MAXPOP1	cJ1_IMMED5_MAXPOP1
#define	cJU_IMMED6_MAXPOP1	cJ1_IMMED6_MAXPOP1
#define	cJU_IMMED7_MAXPOP1	cJ1_IMMED7_MAXPOP1

#define	JU_LEAF1POPTOWORDS(Pop1)	J1_LEAF1POPTOWORDS(Pop1)
#define	JU_LEAF2POPTOWORDS(Pop1)	J1_LEAF2POPTOWORDS(Pop1)
#define	JU_LEAF3POPTOWORDS(Pop1)	J1_LEAF3POPTOWORDS(Pop1)
#define	JU_LEAF4POPTOWORDS(Pop1)	J1_LEAF4POPTOWORDS(Pop1)
#define	JU_LEAF5POPTOWORDS(Pop1)	J1_LEAF5POPTOWORDS(Pop1)
#define	JU_LEAF6POPTOWORDS(Pop1)	J1_LEAF6POPTOWORDS(Pop1)
#define	JU_LEAF7POPTOWORDS(Pop1)	J1_LEAF7POPTOWORDS(Pop1)
#define	JU_LEAF8POPTOWORDS(Pop1)	J1_LEAF8POPTOWORDS(Pop1)

#define	JU_LEAF1GROWINPLACE(Pop1)	J1_LEAF1GROWINPLACE(Pop1)
#define	JU_LEAF2GROWINPLACE(Pop1)	J1_LEAF2GROWINPLACE(Pop1)
#define	JU_LEAF3GROWINPLACE(Pop1)	J1_LEAF3GROWINPLACE(Pop1)
#define	JU_LEAF4GROWINPLACE(Pop1)	J1_LEAF4GROWINPLACE(Pop1)
#define	JU_LEAF5GROWINPLACE(Pop1)	J1_LEAF5GROWINPLACE(Pop1)
#define	JU_LEAF6GROWINPLACE(Pop1)	J1_LEAF6GROWINPLACE(Pop1)
#define	JU_LEAF7GROWINPLACE(Pop1)	J1_LEAF7GROWINPLACE(Pop1)
#define	JU_LEAF8GROWINPLACE(Pop1)	J1_LEAF8GROWINPLACE(Pop1)

#define	j__udyPartstoBranchL	j__udy1PartstoBranchL
#define	j__udyPartstoBranchB	j__udy1PartstoBranchB
#define	j__udyConvertBranchBtoU	j__udy1ConvertBranchBtoU
#define	j__udyConvertBranchLtoU	j__udy1ConvertBranchLtoU
#define	j__udyCascade1		j__udy1Cascade1
#define	j__udyCascade2		j__udy1Cascade2
#define	j__udyCascade3		j__udy1Cascade3
#define	j__udyCascade4		j__udy1Cascade4
#define	j__udyCascade5		j__udy1Cascade5
#define	j__udyCascade6		j__udy1Cascade6
#define	j__udyCascade7		j__udy1Cascade7
#define	j__udyCascade8		j__udy1Cascade8
#define	j__udyInsertBranchL	j__udy1InsertBranchL

#define	j__udyBranchBToBranchL	j__udy1BranchBToBranchL
#define	j__udyLeafB1ToLeaf1	j__udy1LeafB1ToLeaf1
#define	j__udyLeaf1orB1ToLeaf2	j__udy1Leaf1orB1ToLeaf2
#define	j__udyLeaf2ToLeaf3	j__udy1Leaf2ToLeaf3
#define	j__udyLeaf3ToLeaf4	j__udy1Leaf3ToLeaf4
#define	j__udyLeaf4ToLeaf5	j__udy1Leaf4ToLeaf5
#define	j__udyLeaf5ToLeaf6	j__udy1Leaf5ToLeaf6
#define	j__udyLeaf6ToLeaf7	j__udy1Leaf6ToLeaf7
#define	j__udyLeaf7ToLeaf8	j__udy1Leaf7ToLeaf8

#define	jpm_t			j1pm_t
#define	Pjpm_t			Pj1pm_t

#define	jlb_t			j1lb_t
#define	Pjlb_t			Pj1lb_t

#define cJU_WORDSPERLEAFB1U     cJ1_WORDSPERLEAFB1U     
#define	JU_JLB_BITMAP		J1_JLB_BITMAP

#define	j__udyAllocJPM		j__udy1AllocJ1PM
#define	j__udyAllocJBL		j__udy1AllocJBL
#define	j__udyAllocJBB		j__udy1AllocJBB
#define	j__udyAllocJBBJP	j__udy1AllocJBBJP
#define	j__udyAllocJBU		j__udy1AllocJBU
#define	j__udyAllocJLL1		j__udy1AllocJLL1
#define	j__udyAllocJLL2		j__udy1AllocJLL2
#define	j__udyAllocJLL3		j__udy1AllocJLL3
#define	j__udyAllocJLL4		j__udy1AllocJLL4
#define	j__udyAllocJLL5		j__udy1AllocJLL5
#define	j__udyAllocJLL6		j__udy1AllocJLL6
#define	j__udyAllocJLL7		j__udy1AllocJLL7
#define	j__udyAllocJLL8		j__udy1AllocJLL8
#define	j__udyAllocJLB1U	j__udy1AllocJLB1U
#define	j__udyFreeJPM		j__udy1FreeJ1PM
#define	j__udyFreeJBL		j__udy1FreeJBL
#define	j__udyFreeJBB		j__udy1FreeJBB
#define	j__udyFreeJBBJP		j__udy1FreeJBBJP
#define	j__udyFreeJBU		j__udy1FreeJBU
#define	j__udyFreeJLL1		j__udy1FreeJLL1
#define	j__udyFreeJLL2		j__udy1FreeJLL2
#define	j__udyFreeJLL3		j__udy1FreeJLL3
#define	j__udyFreeJLL4		j__udy1FreeJLL4
#define	j__udyFreeJLL5		j__udy1FreeJLL5
#define	j__udyFreeJLL6		j__udy1FreeJLL6
#define	j__udyFreeJLL7		j__udy1FreeJLL7
#define	j__udyFreeJLL8		j__udy1FreeJLL8
#define	j__udyFreeJLB1U		j__udy1FreeJLB1U
#define	j__udyFreeSM		j__udy1FreeSM

#define	j__uMaxWords		j__u1MaxWords

#else // JUDYL ****************************************************************

#define	cJU_LEAF8_MAXPOP1	cJL_LEAF8_MAXPOP1
#define	cJU_LEAF1_MAXPOP1	cJL_LEAF1_MAXPOP1
#define	cJU_LEAF2_MAXPOP1	cJL_LEAF2_MAXPOP1
#define cJU_LEAF2_MAXWORDS      cJL_LEAF2_MAXWORDS 
#define	cJU_LEAF3_MAXPOP1	cJL_LEAF3_MAXPOP1
#define	cJU_LEAF4_MAXPOP1	cJL_LEAF4_MAXPOP1
#define	cJU_LEAF5_MAXPOP1	cJL_LEAF5_MAXPOP1
#define	cJU_LEAF6_MAXPOP1	cJL_LEAF6_MAXPOP1
#define	cJU_LEAF7_MAXPOP1	cJL_LEAF7_MAXPOP1
#define cJU_JPMAXIMMED1         cJL_JPMAXIMMED1
#define	cJU_JPMAXBRANCH         cJL_JPMAXBRANCH
#define	cJU_JPMAXBRANCH_noDCD	cJL_JPMAXBRANCH_noDCD
#define	cJU_IMMED1_MAXPOP1	cJL_IMMED1_MAXPOP1
#define	cJU_IMMED2_MAXPOP1	cJL_IMMED2_MAXPOP1
#define	cJU_IMMED3_MAXPOP1	cJL_IMMED3_MAXPOP1
#define	cJU_IMMED4_MAXPOP1	cJL_IMMED4_MAXPOP1
#define	cJU_IMMED5_MAXPOP1	cJL_IMMED5_MAXPOP1
#define	cJU_IMMED6_MAXPOP1	cJL_IMMED6_MAXPOP1
#define	cJU_IMMED7_MAXPOP1	cJL_IMMED7_MAXPOP1

#define	JU_LEAF1POPTOWORDS(Pop1)	JL_LEAF1POPTOWORDS(Pop1)
#define	JU_LEAF2POPTOWORDS(Pop1)	JL_LEAF2POPTOWORDS(Pop1)
#define	JU_LEAF3POPTOWORDS(Pop1)	JL_LEAF3POPTOWORDS(Pop1)
#define	JU_LEAF4POPTOWORDS(Pop1)	JL_LEAF4POPTOWORDS(Pop1)
#define	JU_LEAF5POPTOWORDS(Pop1)	JL_LEAF5POPTOWORDS(Pop1)
#define	JU_LEAF6POPTOWORDS(Pop1)	JL_LEAF6POPTOWORDS(Pop1)
#define	JU_LEAF7POPTOWORDS(Pop1)	JL_LEAF7POPTOWORDS(Pop1)
#define	JU_LEAF8POPTOWORDS(Pop1)	JL_LEAF8POPTOWORDS(Pop1)

#define	JU_LEAF1GROWINPLACE(Pop1)	JL_LEAF1GROWINPLACE(Pop1)
#define	JU_LEAF2GROWINPLACE(Pop1)	JL_LEAF2GROWINPLACE(Pop1)
#define	JU_LEAF3GROWINPLACE(Pop1)	JL_LEAF3GROWINPLACE(Pop1)
#define	JU_LEAF4GROWINPLACE(Pop1)	JL_LEAF4GROWINPLACE(Pop1)
#define	JU_LEAF5GROWINPLACE(Pop1)	JL_LEAF5GROWINPLACE(Pop1)
#define	JU_LEAF6GROWINPLACE(Pop1)	JL_LEAF6GROWINPLACE(Pop1)
#define	JU_LEAF7GROWINPLACE(Pop1)	JL_LEAF7GROWINPLACE(Pop1)
#define	JU_LEAF8GROWINPLACE(Pop1)	JL_LEAF8GROWINPLACE(Pop1)

#define	j__udyPartstoBranchL	j__udyLPartstoBranchL
#define	j__udyPartstoBranchB	j__udyLPartstoBranchB
#define	j__udyConvertBranchBtoU	j__udyLConvertBranchBtoU
#define	j__udyConvertBranchLtoU	j__udyLConvertBranchLtoU
#define	j__udyCascade1		j__udyLCascade1
#define	j__udyCascade2		j__udyLCascade2
#define	j__udyCascade3		j__udyLCascade3
#define	j__udyCascade4		j__udyLCascade4
#define	j__udyCascade5		j__udyLCascade5
#define	j__udyCascade6		j__udyLCascade6
#define	j__udyCascade7		j__udyLCascade7
#define	j__udyCascade8		j__udyLCascade8
#define	j__udyInsertBranchL	j__udyLInsertBranchL

#define	j__udyBranchBToBranchL	j__udyLBranchBToBranchL
#define	j__udyLeafB1ToLeaf1	j__udyLLeafB1ToLeaf1
#define	j__udyLeaf1orB1ToLeaf2	j__udyLLeaf1orB1ToLeaf2
#define	j__udyLeaf2ToLeaf3	j__udyLLeaf2ToLeaf3
#define	j__udyLeaf3ToLeaf4	j__udyLLeaf3ToLeaf4
#define	j__udyLeaf4ToLeaf5	j__udyLLeaf4ToLeaf5
#define	j__udyLeaf5ToLeaf6	j__udyLLeaf5ToLeaf6
#define	j__udyLeaf6ToLeaf7	j__udyLLeaf6ToLeaf7
#define	j__udyLeaf7ToLeaf8	j__udyLLeaf7ToLeaf8

#define	jpm_t			jLpm_t
#define	Pjpm_t			PjLpm_t

#define	jlb_t			jLlb_t
#define	Pjlb_t			PjLlb_t

#define cJU_WORDSPERLEAFB1U     cJL_WORDSPERLEAFB1U
#define	JU_JLB_BITMAP		JL_JLB_BITMAP

#define	j__udyAllocJPM		j__udyLAllocJLPM
#define	j__udyAllocJBL		j__udyLAllocJBL
#define	j__udyAllocJBB		j__udyLAllocJBB
#define	j__udyAllocJBBJP	j__udyLAllocJBBJP
#define	j__udyAllocJBU		j__udyLAllocJBU
#define	j__udyAllocJLL1		j__udyLAllocJLL1
#define	j__udyAllocJLL2		j__udyLAllocJLL2
#define	j__udyAllocJLL3		j__udyLAllocJLL3
#define	j__udyAllocJLL4		j__udyLAllocJLL4
#define	j__udyAllocJLL5		j__udyLAllocJLL5
#define	j__udyAllocJLL6		j__udyLAllocJLL6
#define	j__udyAllocJLL7		j__udyLAllocJLL7
#define	j__udyAllocJLL8		j__udyLAllocJLL8
#define	j__udyAllocJLB1U	j__udyLAllocJLB1U
//				j__udyLAllocJV
#define	j__udyFreeJPM		j__udyLFreeJLPM
#define	j__udyFreeJBL		j__udyLFreeJBL
#define	j__udyFreeJBB		j__udyLFreeJBB
#define	j__udyFreeJBBJP		j__udyLFreeJBBJP
#define	j__udyFreeJBU		j__udyLFreeJBU
#define	j__udyFreeJLL1		j__udyLFreeJLL1
#define	j__udyFreeJLL2		j__udyLFreeJLL2
#define	j__udyFreeJLL3		j__udyLFreeJLL3
#define	j__udyFreeJLL4		j__udyLFreeJLL4
#define	j__udyFreeJLL5		j__udyLFreeJLL5
#define	j__udyFreeJLL6		j__udyLFreeJLL6
#define	j__udyFreeJLL7		j__udyLFreeJLL7
#define	j__udyFreeJLL8		j__udyLFreeJLL8
#define	j__udyFreeJLB1U		j__udyLFreeJLB1U
#define	j__udyFreeSM		j__udyLFreeSM
//				j__udyLFreeJV

#define	j__uMaxWords		j__uLMaxWords

#endif // JUDYL

#endif // _JUDYPRIVATE1L_INCLUDED
