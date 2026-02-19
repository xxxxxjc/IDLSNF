/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2017, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TLibCommon/Debug.h"

#include <cmath>
#include <algorithm>
using namespace std;

//自定义变量引用

extern int CTUIndex;
extern int CurrentPOC;
extern int CUComCount;
extern int CUDepth[85];
extern int CUPartSize[85];
extern int CUResetPart;
extern int CUTargetMode[85];
extern double Capacity;//统计嵌入容量   ---emd把Capacity的类型改成了double
extern int isorg;
extern int PUcategeory[4][8];//统计8*8的类型总数 2N*2N N*N  N*2N  2N*N
extern int FPUcategeory[4][8];//统计第一个P帧8*8的类型
extern int judgeMode;

int ChangeFlag = 0;


//by lzh
extern int EMD_16_CUTargetMode[16];
extern int CUnum_16 ;
extern int EMD_32_CUTargetMode[4];  
extern int CUnum_32 ;
extern int EMD_64_CUTargetMode[1];  
extern int CUnum_64 ;
extern int EMD_8_CUTargetMode[64];  
extern int CUnum_8 ;
extern int TOTAL_8;
extern int TOTAL_16;
extern int TOTAL_32;
extern int TOTAL_64;
int find_flag = 0; //用来嵌信息的
int EMD_SUM = 0; // 判断目前的EMD SUM
int aim_bit =0; //EMD待嵌入信息

extern int CurrentPOC;




// 计算两点之间的距离

int manhattan(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

int custom_dist(int px, int py, int x, int y)
{
    int D;

    if (x == px || y == py)
        D = 0;     // 只差一维，优先级高
    else
        D = 1;     // 两维都不同，优先级低

    return D * 12 + manhattan(px, py, x, y);
}
void replace_by_custom_1(int* p, int* q,int x1, int y1){
        *p = x1;
        *q = y1;
}

void replace_by_custom_2(int* p, int* q,
    int x1, int y1,
    int x2, int y2)
{
    int px = *p, py = *q;

    int F1 = custom_dist(px, py, x1, y1);
    int F2 = custom_dist(px, py, x2, y2);

    if (F1 <= F2) {
        *p = x1;
        *q = y1;
    }
    else {
        *p = x2;
        *q = y2;
    }
}
void replace_by_custom_3(int* p, int* q,
    int x1, int y1,
    int x2, int y2,
    int x3, int y3)
{
    int px = *p, py = *q;

    int bestX = x1, bestY = y1;
    int bestF = custom_dist(px, py, x1, y1);

    int F2 = custom_dist(px, py, x2, y2);
    if (F2 < bestF) {
        bestF = F2;
        bestX = x2;
        bestY = y2;
    }

    int F3 = custom_dist(px, py, x3, y3);
    if (F3 < bestF) {
        bestF = F3;
        bestX = x3;
        bestY = y3;
    }

    *p = bestX;
    *q = bestY;
}
int R1[] = { 0, 1, 2, 3, 4, 5, 6, 7, 11, 17, 23, 28, 29 };
int R1_len = sizeof(R1) / sizeof(R1[0]);

// 定义一个结构体表示坐标
typedef struct {
    int x;
    int y;
} Coordinate;

// 定义8×8的五个列表,对应论文五种颜色
Coordinate list8_1[] = { {0,0},{2,0},{0,3},{2,3} };
Coordinate list8_2[] = { {0, 1}, {0, 2},{1, 0}, {1, 1}, {1, 2}, {1, 3} };
Coordinate list8_3[] = { {2,1},{2,2} };
Coordinate list8_4[] = { {3,0},{3,1},{3,2},{3,3} };

// 定义每个列表的长度
int len8_1 = sizeof(list8_1) / sizeof(list8_1[0]);
int len8_2 = sizeof(list8_2) / sizeof(list8_2[0]);
int len8_3 = sizeof(list8_3) / sizeof(list8_3[0]);
int len8_4 = sizeof(list8_4) / sizeof(list8_4[0]);

// 定义16×16的九个列表,对应论文九种颜色
Coordinate list16_1[] = { {0,0},{0,5},{5,0},{5,5} };
Coordinate list16_2[] = { {0,1},{0,2},{0,3}, {0,4},{1,0},{1,1},{1,2},{1,3},{1,4},{1,0},{2,1},{2,2},{2,3},{2,4},
{3,0},{3,1},{3,2},{3,3},{3,4},{4,0}, {4, 1},{4,2}, {4,3},{4,4} };
Coordinate list16_3[] = { {5,1},{5,2},{5,3},{5,4} };
Coordinate list16_4[] = { {6,0},{6,1},{6,2},{6,3},{6,4} };
Coordinate list16_5[] = { {1,5},{2,5},{3,5},{4,5} };
Coordinate list16_6[] = { {0,6},{1,6},{2,6},{3,6},{4,6} };
Coordinate list16_7[] = { {5,6} };
Coordinate list16_8[] = { {6,5} };
Coordinate list16_9[] = { {6,6} };

int len16_1 = sizeof(list16_1) / sizeof(list16_1[0]);
int len16_2 = sizeof(list16_2) / sizeof(list16_2[0]);
int len16_3 = sizeof(list16_3) / sizeof(list16_3[0]);
int len16_4 = sizeof(list16_4) / sizeof(list16_4[0]);
int len16_5 = sizeof(list16_5) / sizeof(list16_5[0]);
int len16_6 = sizeof(list16_6) / sizeof(list16_6[0]);
int len16_7 = sizeof(list16_7) / sizeof(list16_7[0]);
int len16_8 = sizeof(list16_8) / sizeof(list16_8[0]);
int len16_9 = sizeof(list16_9) / sizeof(list16_9[0]);
// 判断坐标是否在列表中
int isInList(Coordinate* list, int len, Coordinate point) {
    for (int i = 0; i < len; i++) {
        if (list[i].x == point.x && list[i].y == point.y) {
            return 1;  // 找到匹配的坐标，返回1
        }
    }
    return 0;  // 没有找到匹配的坐标，返回0
}
typedef struct {
    int value;  // 唯一值
    int x;      // 对应 j
    int y;      // 对应 i
} MapEntry;

MapEntry unique_map[] = {
    { 8, 3, 1 },
    { 9, 4, 1 },
    { 10, 5, 1 },
    { 12, 1, 2 },
    { 13, 2, 2 },
    { 14, 3, 2 },
    { 15, 4, 2 },
    { 16, 5, 2 },
    { 18, 1, 3 },
    { 19, 2, 3 },
    { 20, 3, 3 },
    { 21, 4, 3 },
    { 22, 5, 3 },
    { 24, 1, 4 },
    { 25, 2, 4 },
    { 26, 3, 4 },
    { 27, 4, 4 },
    { 30, 2, 5 },
    { 31, 3, 5 }
};
//----------------------------------xjc定义函数：结束----------------------------

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
 \param    uhTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 \param    chromaFormat  chroma format
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormat)
{
  Int i;

  m_uhTotalDepth   = uhTotalDepth + 1;
  m_ppcBestCU      = new TComDataCU*[m_uhTotalDepth-1];
  m_ppcTempCU      = new TComDataCU*[m_uhTotalDepth-1];

  m_ppcPredYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcPredYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcOrigYuv     = new TComYuv*[m_uhTotalDepth-1];

  UInt uiNumPartitions;
  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    uiNumPartitions = 1<<( ( m_uhTotalDepth - i - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> i;
    UInt uiHeight = uiMaxHeight >> i;

    m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
    m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );

    m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcOrigYuv    [i] = new TComYuv; m_ppcOrigYuv    [i]->create(uiWidth, uiHeight, chromaFormat);
  }

  m_bEncodeDQP                     = false;
  m_stillToCodeChromaQpOffsetFlag  = false;
  m_cuChromaQpOffsetIdxPlus1       = 0;
  m_bFastDeltaQP                   = false;

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster( m_uhTotalDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );

  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
}

Void TEncCu::destroy()
{
  Int i;

  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    if(m_ppcBestCU[i])
    {
      m_ppcBestCU[i]->destroy();      delete m_ppcBestCU[i];      m_ppcBestCU[i] = NULL;
    }
    if(m_ppcTempCU[i])
    {
      m_ppcTempCU[i]->destroy();      delete m_ppcTempCU[i];      m_ppcTempCU[i] = NULL;
    }
    if(m_ppcPredYuvBest[i])
    {
      m_ppcPredYuvBest[i]->destroy(); delete m_ppcPredYuvBest[i]; m_ppcPredYuvBest[i] = NULL;
    }
    if(m_ppcResiYuvBest[i])
    {
      m_ppcResiYuvBest[i]->destroy(); delete m_ppcResiYuvBest[i]; m_ppcResiYuvBest[i] = NULL;
    }
    if(m_ppcRecoYuvBest[i])
    {
      m_ppcRecoYuvBest[i]->destroy(); delete m_ppcRecoYuvBest[i]; m_ppcRecoYuvBest[i] = NULL;
    }
    if(m_ppcPredYuvTemp[i])
    {
      m_ppcPredYuvTemp[i]->destroy(); delete m_ppcPredYuvTemp[i]; m_ppcPredYuvTemp[i] = NULL;
    }
    if(m_ppcResiYuvTemp[i])
    {
      m_ppcResiYuvTemp[i]->destroy(); delete m_ppcResiYuvTemp[i]; m_ppcResiYuvTemp[i] = NULL;
    }
    if(m_ppcRecoYuvTemp[i])
    {
      m_ppcRecoYuvTemp[i]->destroy(); delete m_ppcRecoYuvTemp[i]; m_ppcRecoYuvTemp[i] = NULL;
    }
    if(m_ppcOrigYuv[i])
    {
      m_ppcOrigYuv[i]->destroy();     delete m_ppcOrigYuv[i];     m_ppcOrigYuv[i] = NULL;
    }
  }
  if(m_ppcBestCU)
  {
    delete [] m_ppcBestCU;
    m_ppcBestCU = NULL;
  }
  if(m_ppcTempCU)
  {
    delete [] m_ppcTempCU;
    m_ppcTempCU = NULL;
  }

  if(m_ppcPredYuvBest)
  {
    delete [] m_ppcPredYuvBest;
    m_ppcPredYuvBest = NULL;
  }
  if(m_ppcResiYuvBest)
  {
    delete [] m_ppcResiYuvBest;
    m_ppcResiYuvBest = NULL;
  }
  if(m_ppcRecoYuvBest)
  {
    delete [] m_ppcRecoYuvBest;
    m_ppcRecoYuvBest = NULL;
  }
  if(m_ppcPredYuvTemp)
  {
    delete [] m_ppcPredYuvTemp;
    m_ppcPredYuvTemp = NULL;
  }
  if(m_ppcResiYuvTemp)
  {
    delete [] m_ppcResiYuvTemp;
    m_ppcResiYuvTemp = NULL;
  }
  if(m_ppcRecoYuvTemp)
  {
    delete [] m_ppcRecoYuvTemp;
    m_ppcRecoYuvTemp = NULL;
  }
  if(m_ppcOrigYuv)
  {
    delete [] m_ppcOrigYuv;
    m_ppcOrigYuv = NULL;
  }
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init( TEncTop* pcEncTop )
{
  m_pcEncCfg           = pcEncTop;
  m_pcPredSearch       = pcEncTop->getPredSearch();
  m_pcTrQuant          = pcEncTop->getTrQuant();
  m_pcRdCost           = pcEncTop->getRdCost();

  m_pcEntropyCoder     = pcEncTop->getEntropyCoder();
  m_pcBinCABAC         = pcEncTop->getBinCABAC();

  m_pppcRDSbacCoder    = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder  = pcEncTop->getRDGoOnSbacCoder();

  m_pcRateCtrl         = pcEncTop->getRateCtrl();
  m_lumaQPOffset       = 0;
  initLumaDeltaQpLUT();
}


void countC(int sizes[85])//------------------------------yyyyyyy
{
	  if (CUDepth[0] != 0)//被划分 CUTargetMode[0] = 255;
	  {
		  for (int j = 1; j <= 64; j = j + 21)
		  {
			  if (CUDepth[j] == 11)
			  {//32*32
				  PUcategeory[1][sizes[j]]++;
				  if(CurrentPOC==1){
					  FPUcategeory[1][sizes[j]]++;
				  }
			  }else{
				   for (int i = 1+j; i <= 16+j; i = i + 5)//判断2，7，12，17四个CU
				  {
					  if (CUDepth[i] == 22)//划分成了16x16
					  {
						  PUcategeory[2][sizes[i]]++;
						  if(CurrentPOC==1){
							  FPUcategeory[2][sizes[i]]++;
						  }
					  }else{//8*8
						   for(int k=1;k<5;k++){
							  PUcategeory[3][sizes[i+k]]++;
							  if(CurrentPOC==1){
								  FPUcategeory[3][sizes[i+k]]++;
							  }
						   }
					  }
				   }
			  }
		  }
	  }else{//64*64
		  PUcategeory[0][sizes[0]]++;
		  if(CurrentPOC==1){
			  FPUcategeory[0][sizes[0]]++;
		  }
	  }

}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** 
 \param  pCtu pointer of CU data class
 */
Void TEncCu::compressCtu( TComDataCU* pCtu )////------------------------------yyyyyyy
{
  // initialize CU data
  m_ppcBestCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );
  m_ppcTempCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );

    //累计CTUIndex   ---by emd
  int i = 0;
  int j = 0;
  int n = 0;
  int ii=0;
  int randnum=0;


  // ljd 菱形编码定义全局变量--结束------------------------------






  for(int jj = 0; jj<16;jj++)
       {
	   EMD_16_CUTargetMode[jj] =  111 ;// 清零
	   }
  for(int jj = 0; jj<4;jj++)
       {
	   EMD_32_CUTargetMode[jj] =  111 ;// 清零
	   }
  for(int jj = 0; jj<64;jj++)
       {
	   EMD_8_CUTargetMode[jj] =  111 ;// 清零
	   }
  EMD_64_CUTargetMode[0] = 0;
  CUnum_16=0;// 清零
  CUnum_8=0;
  CUnum_32=0;
  CUnum_64=0;


  //初始化
  if (pCtu->getSlice()->getSliceType() != I_SLICE)
  {
	  CUComCount = 0;
	//  CTUIndex=pCtu->getCtuRsAddr()+1;
	   CurrentPOC = pCtu->getSlice()->getPOC();
	//  printf("---------------CTUIndex=%d",pCtu->getSlice()->getPOC());
	//  printf("---------------Fram %2d,CTUIndex=%d,CtuRsAddr=%d\n", currentPOC, CTUIndex, pCtu->getCtuRsAddr());
	  //初始化存储的数组
	  for (int i = 0; i < 85; i++)
	  {
		  CUPartSize[i] = 0;
		  CUDepth[i] = 3;
		  CUTargetMode[i] = 1;//默认都为1+++7
	  }
	}

  //第一次压缩，标志位为0
  CUResetPart = 0;
  ChangeFlag = 0;
  // analysis of CU
  DEBUG_STRING_NEW(sDebug)


	  	  ///////// test zzz begin  ///////
	  /*printf("CUPartSize:\n");
	  for (int i=0;i<85;i++)
	  {

		  if((i+1)%5==0)
			  printf("\n");
		  printf("%5d",CUPartSize[i]);
	  }

	  printf("CUTargetMode:\n");
	  for (int i=0;i<85;i++)
	  {

		  if((i+1)%5==0)
			  printf("\n");
		  printf("%5d",CUTargetMode[i]);
	  }*/
	  ///////// test zzz end  ///////
  




  xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 DEBUG_STRING_PASS_INTO(sDebug) );

  //CTU信息分析  
  srand((unsigned)time(NULL));

  if (CUComCount > 84 && pCtu->getSlice()->getSliceType() != I_SLICE)
  {




      // 复制emd代码--开始 *************

      if (CUDepth[0] != 0)//被划分 CUTargetMode[0] = 255; 判断是不是64
      {
          CUTargetMode[0] = 255;
          for (j = 1; j <= 64; j = j + 21)
          {
              if (CUDepth[j] == 11) //index是85个PU模式，十位是本来应该depth，个位是本身的level。       划分为32X32
              {
                  CUTargetMode[j] = CUPartSize[j]; //将PU6当成PU1来修改
                  EMD_32_CUTargetMode[CUnum_32] = j;
                  CUnum_32++;
                  TOTAL_32++;
              }
              else
              {//被划分，TargetMode改为255,然后看内部CU
                  CUTargetMode[j] = 255;
                  for (i = 1 + j; i <= 16 + j; i = i + 5)//判断2，7，12，17四个CU
                  {
                      if (CUDepth[i] == 22)   //        划分成了16x16
                      {
                          CUTargetMode[i] = CUPartSize[i]; //将PU6当成PU1来修改
                          EMD_16_CUTargetMode[CUnum_16] = i;
                          CUnum_16++;
                          TOTAL_16++;
                      }
                      else//划分成了8x8x4   TargetMode改为255  Question1
                      {
                          TOTAL_8 += 4;
                          CUTargetMode[i] = 255;
                          for (n = 1; n <= 4; n++)
                          {
                              CUTargetMode[i + n] = CUPartSize[i + n];//是不是应该加上这句话？ --刘金豆
                              //cout<<"CUPartSize[i + n]="<<CUPartSize[i + n]<<endl;//--ljd

                              EMD_8_CUTargetMode[CUnum_8] = i + n;
                              CUnum_8++;

                          }

                      }

                  }
              }
          }
      }
      else//64没有划分
      {
          CUTargetMode[0] = CUPartSize[0];//Level3
          CUnum_64 = 1;
          TOTAL_64++;
          //printf(">>>>>>>>>>>>TargetMode 0 ==  %d  ----\n",  CUTargetMode[0]);
      }




      /*改进16×16映射关系*/
      if (CUnum_16 > 2 && 1 ) // 16X16开关
      {
          //// 0-6分别对应2Nx2N,2NxN,Nx2N,2NxnU,2NxnD,nLx2N,nRx2N
		  ChangeFlag = 1;
          int mapping1[7] = { 0,1,2,4,5,6,7 };//HEVC自己的PU-数字mapping关系一一映射
          int mapping_rule1[7] = { 0,0,2,-2,-2,-1,-1 };//{0,1,4,2,3,5,6} 我自己的矩阵的PU-数字mapping关系一一映射
          int mapping2[7] = { 0,1,4,2,3,5,6 };
          int mapping_rule2[7] = { 0,0,-2,2,2,1,1 }; //{0,1,2,4,5,6,7}再改回HEVC原始的映射
          for (ii = 0; ii + 1 < CUnum_16; ii = ii + 2) //ii指的是16x16  的target
          {
              if (Capacity < 999999999) {
                  int p, q, d;
                  p = CUTargetMode[EMD_16_CUTargetMode[ii]];
                  q = CUTargetMode[EMD_16_CUTargetMode[ii + 1]];
                  for (i = 0; i < 7; i++)
                  {
                      if (p == mapping1[i])
                      {
                          p = p + mapping_rule1[i];
                          break;
                      }
                  }
                  for (i = 0; i < 7; i++)
                  {
                      if (q == mapping1[i])
                      {
                          q = q + mapping_rule1[i];
                          break;
                      }
                  }
                  d = rand() % 32;
                  int flag = 1;
                  Capacity += 5;
                  int matrix[7][7];
                  int row_start_values[7] = { 0, 5, 11, 17, 23, 28, 1 };

                  // 填充矩阵
                  for (int i = 0; i < 7; i++) {
                      // 计算列的起始值
                      int start_value = row_start_values[i];

                      for (int j = 0; j < 7; j++) {
                          matrix[i][j] = start_value;

                          // 每列递增 +1，遇到 32 归零
                          start_value++;
                          if (start_value >= 32) {
                              start_value = 0;
                          }
                      }
                  }
                  int R1[] = { 0, 1, 2, 3, 4, 5, 6, 7, 11, 17, 23, 28, 29 };
                  int R1_len = sizeof(R1) / sizeof(R1[0]);
                  int is_in_R1 = 0;

                  for (int i = 0; i < R1_len; i++) {
                      if (d == R1[i]) {
                          is_in_R1 = 1;
                          break;
                      }
                  }
                  int xs[3], ys[3];
                  int found = 0;flag = 0;
                  Coordinate point = { p, q };
                  if (isInList(list16_1, len16_1, point)) {
                      for (int i = 0; i < 7; i++) {          // i = 行 = y
                          for (int j = 0; j < 7; j++) {      // j = 列 = x
                              if (matrix[i][j] == d) {
                                  // 记录坐标 (x=j, y=i)
                                  xs[found] = j;
                                  ys[found] = i;
                                  found++;
                                  if (found == 3) break; // 最多 3 个
                              }
                          }
                          if (found == 3) break;
                      }
                      // 根据找到的数量调用不同函数
                      if (found == 1) {
                          // 一个坐标
                          replace_by_custom_1(&p, &q, xs[0], ys[0]);
                      }
                      if (found == 2) {
                          // 两个坐标
                          replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                      }
                      else if (found == 3) {
                          // 三个坐标
                          replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                      }
                  }
                  else if (isInList(list16_2, len16_2, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 5; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 5) {
                                  j_start = 1;j_end = 4;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 5;                 // j = 0~5
                              }

                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;   
                                  }
                              }
                              if (found) break;
                          }
                      }


                  }
                  else if (isInList(list16_3, len16_3, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 5; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 5) {
                                  j_start = 1;j_end = 6;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 6;                 // j = 0~5
                              }

                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                      }
                  }
                  else if (isInList(list8_4, len8_4, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 5; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 5) {
                                  j_start = 6;j_end = 6;                 // j = 1~4
                              }
                              else {
                                  j_start = 5;j_end = 6;                 // j = 0~5
                              }

                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                          if (!found) {
                              for (int i = 0; i < 7; i++) {          // i = 行 = y
                                  for (int j = 0; j < 5; j++) {      // j = 列 = x
                                      if (matrix[i][j] == d) {
                                          p = j;q = i;found = 1;
                                          break;
                                      }
                                  }
                                  if (found) break;
                              }
                          }
                      }
                  }
                  else if (isInList(list16_5, len16_5, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 6; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 5) {
                                  j_start = 1;j_end = 4;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 5;                 // j = 0~5
                              }

                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                      }

                  }
                  else if (isInList(list16_6, len16_6, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 5; i <= 6; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 5) {
                                  j_start = 1;j_end = 4;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 5;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                          if (!found) {
                              for (int i = 0; i < 5; i++) {          // i = 行 = y
                                  for (int j = 0; j < 7; j++) {      // j = 列 = x
                                      if (matrix[i][j] == d) {
                                          p = j;q = i;found = 1;
                                          break;
                                      }
                                  }
                                  if (found) break;
                              }
                          }
                      }

                  }
                  else if (isInList(list16_7, len16_7, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 5; i <= 6; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 5) {
                                  j_start = 1;j_end = 6;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 6;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                          if (!found) {
                              for (int i = 0; i < 5; i++) {          // i = 行 = y
                                  for (int j = 0; j < 7; j++) {      // j = 列 = x
                                      if (matrix[i][j] == d) {
                                          p = j;q = i;found = 1;
                                          break;
                                      }
                                  }
                                  if (found) break;
                              }
                          }
                      }
                  }
                  else if (isInList(list16_8, len16_8, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 6; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 5) {
                                  j_start = 6;j_end = 6;                 // j = 1~4
                              }
                              else {
                                  j_start = 5;j_end = 6;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                          if (!found) {
                              for (int i = 0; i < 7; i++) {          // i = 行 = y
                                  for (int j = 0; j < 5; j++) {      // j = 列 = x
                                      if (matrix[i][j] == d) {
                                          p = j;q = i;found = 1;
                                          break;
                                      }
                                  }
                                  if (found) break;
                              }
                          }
                      }

                  }
                  else if (isInList(list16_9, len16_9, point)) {
                      if (is_in_R1) {
                          for (int i = 0; i < 7; i++) {          // i = 行 = y
                              for (int j = 0; j < 7; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 5; i <= 6; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 5) {
                                  j_start = 6;j_end = 6;                 // j = 1~4
                              }
                              else {
                                  j_start = 5;j_end = 6;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                          if (!found) {
                              for (int i = 0; i < 7; i++) {          // i = 行 = y
                                  for (int j = 0; j < 7; j++) {      // j = 列 = x
                                      if (matrix[i][j] == d) {
                                          p = j;q = i;found = 1;
                                          break;
                                      }
                                  }
                                  if (found) break;
                              }
                          }
                      }
                  }


                  for (i = 0; i < 7; i++) //重新映射到HEVC默认的PU划分模式与数字的对应关系
                  {
                      if (p == mapping2[i])
                      {
                          p = p + mapping_rule2[i];
                          break;
                      }
                  }

                  for (i = 0; i < 7; i++) //重新映射到HEVC默认的PU划分模式与数字的对应关系
                  {
                      if (q == mapping2[i])
                      {
                          q = q + mapping_rule2[i];
                          break;
                      }
                  }
                  CUTargetMode[EMD_16_CUTargetMode[ii]] = p;
                  CUTargetMode[EMD_16_CUTargetMode[ii + 1]] = q;
              }//将PU划分模式映射为我自己的PU模式与数字的对应关系

          }

      }



      if (CUnum_8 > 2 && 1 ) //龟壳+矩阵 8X8开关
      {
          ChangeFlag = 1;
          for (ii = 0; ii + 1 < CUnum_8; ii = ii + 2) //ii指的是8X8  的target
          {
              if (Capacity < 999999999) {
                  int p, q, d;
                  p = CUTargetMode[EMD_8_CUTargetMode[ii]];
                  q = CUTargetMode[EMD_8_CUTargetMode[ii + 1]];
                  int flag = 0;
                  randnum = rand() % 8;
                  d = randnum;
                  Capacity += 3;
                  //注意 这里因为代码中建立的是矩阵 论文里为了表述清楚改成了二维坐标系
                  //因此对应的坐标x,y代码和论文中正好相反 但是其中的数值没有变化 看的时候要注意
                  int matrix[4][4];
                  int row, col;
                  int startValue = 0;
                  for (col = 0; col < 4; col++) {
                      startValue = col * 3; // Increment start value for each column
                      for (row = 0; row < 4; row++) {
                          matrix[col][row] = (startValue + row) % 8; // Modulo 8 for wrap around
                      }
                  }
                  int R2[] = { 0, 1, 2, 3, 4, 6 };
                  int R2_len = sizeof(R2) / sizeof(R2[0]);
                  int is_in_R2 = 0;
                  int xs[3], ys[3];
                  int found = 0;flag = 0;
                  for (int i = 0; i < R2_len; i++) {
                      if (d == R2[i]) {
                          is_in_R2 = 1;
                          break;
                      }
                  }
                  Coordinate point = { p, q };
                  if (isInList(list8_1, len8_1, point)) {

                      for (int i = 0; i < 4; i++) {          // i = 行 = y
                          for (int j = 0; j < 4; j++) {      // j = 列 = x
                              if (matrix[i][j] == d) {
                                  // 记录坐标 (x=j, y=i)
                                  xs[found] = j;
                                  ys[found] = i;
                                  found++;
                                  if (found == 3) break; // 最多 3 个
                              }
                          }
                          if (found == 3) break;
                      }
                      if (found == 1) {
                          // 一个坐标
                          replace_by_custom_1(&p, &q, xs[0], ys[0]);
                      }
                      if (found == 2) {
                          // 两个坐标
                          replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                      }
                      else if (found == 3) {
                          // 三个坐标
                          replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                      }

                  }
                  else if (isInList(list8_2, len8_2, point)) {
                      if (is_in_R2) {
                          for (int i = 0; i < 4; i++) {          // i = 行 = y
                              for (int j = 0; j < 4; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 2; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 2) {
                                  j_start = 1;j_end = 2;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 3;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                      }
                  }
                  else if (isInList(list8_3, len8_3, point)) {
                      if (is_in_R2) {
                          for (int i = 0; i < 4; i++) {          // i = 行 = y
                              for (int j = 0; j < 4; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 0; i <= 3; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 0 || i == 2) {
                                  j_start = 1;j_end = 2;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 3;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                      }

                  }
                  else if (isInList(list8_4, len8_4, point)) {
                      if (is_in_R2) {
                          for (int i = 0; i < 4; i++) {          // i = 行 = y
                              for (int j = 0; j < 4; j++) {      // j = 列 = x
                                  if (matrix[i][j] == d) {
                                      // 记录坐标 (x=j, y=i)
                                      xs[found] = j;
                                      ys[found] = i;
                                      found++;
                                      if (found == 3) break; // 最多 3 个
                                  }
                              }
                              if (found == 3) break;
                          }
                          if (found == 2) {
                              // 两个坐标
                              replace_by_custom_2(&p, &q, xs[0], ys[0], xs[1], ys[1]);
                          }
                          else if (found == 3) {
                              // 三个坐标
                              replace_by_custom_3(&p, &q, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]);
                          }
                      }
                      else {
                          for (int i = 2; i <= 3; i++) {        // i 最大为 5
                              int j_start, j_end;
                              if (i == 2) {
                                  j_start = 1;j_end = 2;                 // j = 1~4
                              }
                              else {
                                  j_start = 0;j_end = 3;                 // j = 0~5
                              }
                              for (int j = j_start; j <= j_end; j++) {
                                  if (matrix[i][j] == d) {
                                      p = j;q = i;found = 1;
                                      break;
                                  }
                              }
                              if (found) break;
                          }
                          if (!found) {
                              for (int i = 0; i < 2; i++) {          // i = 行 = y
                                  for (int j = 0; j < 4; j++) {      // j = 列 = x
                                      if (matrix[i][j] == d) {
                                          p = j;q = i;found = 1;
                                          break;
                                      }
                                  }
                                  if (found) break;
                              }
                          }
                      }

                  }

                  CUTargetMode[EMD_8_CUTargetMode[ii]] = p;
                  CUTargetMode[EMD_8_CUTargetMode[ii + 1]] = q;

              }
              // ① 得到当前连续两个8×8 CU 的 PU模式,也就是论文中的 ps和ps+1 ，对应代码中是：puMode1和puMode2

              //cout<<"CUTargetMode[EMD_8_CUTargetMode[ii]]="<<CUTargetMode[EMD_8_CUTargetMode[ii]]<<endl;
              
          }






      }
  }









  
 
  if( CUComCount>84 && pCtu->getSlice()->getSliceType() != I_SLICE){
	  if(ChangeFlag==1){
		  isorg = 0;
		  countC(CUTargetMode);
	  }else{
		  isorg = 1;
		  countC(CUPartSize);
	  }
  }
 
  if (1 && CUComCount>84 && pCtu->getSlice()->getSliceType() != I_SLICE && ChangeFlag==1)//如果CUPartSize[84]==255说明该CTU不完整，不可用
  {													
	  //第二次压缩当前CTU
	  m_ppcBestCU[0]->initCtu(pCtu->getPic(), pCtu->getCtuRsAddr());
	  m_ppcTempCU[0]->initCtu(pCtu->getPic(), pCtu->getCtuRsAddr());

	  CUResetPart = 1;//第二次压缩，需要选择
	  CUComCount = 0;
	  xCompressCU(m_ppcBestCU[0], m_ppcTempCU[0], 0 DEBUG_STRING_PASS_INTO(sDebug));
  }


  DEBUG_STRING_OUTPUT(std::cout, sDebug)

#if ADAPTIVE_QP_SELECTION
  if( m_pcEncCfg->getUseAdaptQpSelect() )
  {
    if(pCtu->getSlice()->getSliceType()!=I_SLICE) //IIII
    {
      xCtuCollectARLStats( pCtu );
    }
  }
#endif
}
/** \param  pCtu  pointer of CU data class
 */
Void TEncCu::encodeCtu ( TComDataCU* pCtu )
{
  if ( pCtu->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  if ( pCtu->getSlice()->getUseChromaQpAdj() )
  {
    setCodeChromaQpAdjFlag(true);
  }

  // Encode CU data
  xEncodeCU( pCtu, 0, 0 );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncCu::initLumaDeltaQpLUT()
{
  const LumaLevelToDeltaQPMapping &mapping=m_pcEncCfg->getLumaLevelToDeltaQPMapping();

  if ( !mapping.isEnabled() )
  {
    return;
  }

  // map the sparse LumaLevelToDeltaQPMapping.mapping to a fully populated linear table.

  Int         lastDeltaQPValue=0;
  std::size_t nextSparseIndex=0;
  for(Int index=0; index<LUMA_LEVEL_TO_DQP_LUT_MAXSIZE; index++)
  {
    while (nextSparseIndex < mapping.mapping.size() && index>=mapping.mapping[nextSparseIndex].first)
    {
      lastDeltaQPValue=mapping.mapping[nextSparseIndex].second;
      nextSparseIndex++;
    }
    m_lumaLevelToDeltaQPLUT[index]=lastDeltaQPValue;
  }
}

Int TEncCu::calculateLumaDQP(TComDataCU *pCU, const UInt absPartIdx, const TComYuv * pOrgYuv)
{
  const Pel *pY = pOrgYuv->getAddr(COMPONENT_Y, absPartIdx);
  const Int stride  = pOrgYuv->getStride(COMPONENT_Y);
  Int width = pCU->getWidth(absPartIdx);
  Int height = pCU->getHeight(absPartIdx);
  Double avg = 0;

  // limit the block by picture size
  const TComSPS* pSPS = pCU->getSlice()->getSPS();
  if ( pCU->getCUPelX() + width > pSPS->getPicWidthInLumaSamples() )
  {
    width = pSPS->getPicWidthInLumaSamples() - pCU->getCUPelX();
  }
  if ( pCU->getCUPelY() + height > pSPS->getPicHeightInLumaSamples() )
  {
    height = pSPS->getPicHeightInLumaSamples() - pCU->getCUPelY();
  }

  // Get QP offset derived from Luma level
  if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().mode == LUMALVL_TO_DQP_AVG_METHOD )
  {
    // Use avg method
    Int sum = 0;
    for (Int y = 0; y < height; y++)
    {
      for (Int x = 0; x < width; x++)
      {
        sum += pY[x];
      }
      pY += stride;
    }
    avg = (Double)sum/(width*height);
  }
  else
  {
    // Use maximum luma value
    Int maxVal = 0;
    for (Int y = 0; y < height; y++)
    {
      for (Int x = 0; x < width; x++)
      {
        if (pY[x] > maxVal)
        {
          maxVal = pY[x];
        }
      }
      pY += stride;
    }
    // use a percentage of the maxVal
    avg = (Double)maxVal * m_pcEncCfg->getLumaLevelToDeltaQPMapping().maxMethodWeight;
  }

  Int lumaIdx = Clip3<Int>(0, Int(LUMA_LEVEL_TO_DQP_LUT_MAXSIZE)-1, Int(avg+0.5) );
  Int QP = m_lumaLevelToDeltaQPLUT[lumaIdx];
  return QP;
}

//! Derive small set of test modes for AMP encoder speed-up
#if AMP_ENC_SPEEDUP
#if AMP_MRG
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
#else
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver)
#endif
{
  if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
  {
    bTestAMP_Hor = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
  {
    bTestAMP_Ver = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->getMergeFlag(0) == false && pcBestCU->isSkipped(0) == false )
  {
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

#if AMP_MRG
  //! Utilizing the partition size of parent PU
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( eParentPartSize == NUMBER_OF_PART_SIZES ) //! if parent is intra
  {
    if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
    {
      bTestMergeAMP_Hor = true;
    }
    else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
    {
      bTestMergeAMP_Ver = true;
    }
  }

  if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->isSkipped(0) == false )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( pcBestCU->getWidth(0) == 64 )
  {
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }
#else
  //! Utilizing the partition size of parent PU
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  {
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

  if ( eParentPartSize == SIZE_2Nx2N )
  {
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }
#endif
}
#endif


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Compress a CU block recursively with enabling sub-CTU-level delta QP
 *  - for loop of QP value to compress the current CU with all possible QP
*/
#if AMP_ENC_SPEEDUP
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, const UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug_), PartSize eParentPartSize )//------------------------------yyyyyyy
#else
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, const UInt uiDepth )
#endif
{
	int TempCount = 0;
  TComPic* pcPic = rpcBestCU->getPic();
  DEBUG_STRING_NEW(sDebug)
  const TComPPS &pps=*(rpcTempCU->getSlice()->getPPS());
  const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
  
  // These are only used if getFastDeltaQp() is true
  const UInt fastDeltaQPCuMaxSize    = Clip3(sps.getMaxCUHeight()>>sps.getLog2DiffMaxMinCodingBlockSize(), sps.getMaxCUHeight(), 32u);

  // get Original YUV data from picture
  m_ppcOrigYuv[uiDepth]->copyFromPicYuv( pcPic->getPicYuvOrg(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu() );

  // variable for Cbf fast mode PU decision
  Bool    doNotBlockPu = true;
  Bool    earlyDetectionSkipMode = false;

  const UInt uiLPelX   = rpcBestCU->getCUPelX();
  const UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
  const UInt uiTPelY   = rpcBestCU->getCUPelY();
  const UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;
  const UInt uiWidth   = rpcBestCU->getWidth(0);

  Int iBaseQP = xComputeQP( rpcBestCU, uiDepth );
  Int iMinQP;
  Int iMaxQP;
  Bool isAddLowestQP = false;

  const UInt numberValidComponents = rpcBestCU->getPic()->getNumberValidComponents();

  //计算次数-------------------------------------------------------------------------

  if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)//PocIndex == 6
  {
	  CUComCount++;
	  TempCount = CUComCount-1;
	 // printf("\tCTUIndex=%d\tCUComCount=%d\n", CTUIndex, CUComCount);
	  CUDepth[TempCount] = 10 * uiDepth;

  }


  if( uiDepth <= pps.getMaxCuDQPDepth() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else
  {
    iMinQP = rpcTempCU->getQP(0);
    iMaxQP = rpcTempCU->getQP(0);
  }

  if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() )
  {
    if ( uiDepth <= pps.getMaxCuDQPDepth() )
    {
      // keep using the same m_QP_LUMA_OFFSET in the same CTU
      m_lumaQPOffset = calculateLumaDQP(rpcTempCU, 0, m_ppcOrigYuv[uiDepth]);
    }
    iMinQP = iBaseQP - m_lumaQPOffset;
    iMaxQP = iMinQP; // force encode choose the modified QO
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  // transquant-bypass (TQB) processing loop variable initialisation ---

  const Int lowestQP = iMinQP; // For TQB, use this QP which is the lowest non TQB QP tested (rather than QP'=0) - that way delta QPs are smaller, and TQB can be tested at all CU levels.

  if ( (pps.getTransquantBypassEnabledFlag()) )
  {
    isAddLowestQP = true; // mark that the first iteration is to cost TQB mode.
    iMinQP = iMinQP - 1;  // increase loop variable range by 1, to allow testing of TQB mode along with other QPs
    if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
    {
      iMaxQP = iMinQP;
    }
  }

  TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());

  const Bool bBoundary = !( uiRPelX < sps.getPicWidthInLumaSamples() && uiBPelY < sps.getPicHeightInLumaSamples() );

  if ( !bBoundary )
  {
    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
      const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP);

      if (bIsLosslessMode)
      {
        iQP = lowestQP;
      }
      if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && uiDepth <= pps.getMaxCuDQPDepth() )
      {
        getSliceEncoder()->updateLambda(pcSlice, iQP);
      }

      m_cuChromaQpOffsetIdxPlus1 = 0;
      if (pcSlice->getUseChromaQpAdj())
      {
        /* Pre-estimation of chroma QP based on input block activity may be performed
         * here, using for example m_ppcOrigYuv[uiDepth] */
        /* To exercise the current code, the index used for adjustment is based on
         * block position
         */
        Int lgMinCuSize = sps.getLog2MinCodingBlockSize() +
                          std::max<Int>(0, sps.getLog2DiffMaxMinCodingBlockSize()-Int(pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth()));
        m_cuChromaQpOffsetIdxPlus1 = ((uiLPelX >> lgMinCuSize) + (uiTPelY >> lgMinCuSize)) % (pps.getPpsRangeExtension().getChromaQpOffsetListLen() + 1);
      }

      rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

      // do inter modes, SKIP and 2Nx2N
      if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
      {
		// 2Nx2N
		if (CUResetPart == 0 ||CUTargetMode[TempCount] == 0){
			if(m_pcEncCfg->getUseEarlySkipDetection())
			{
			  xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
			  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );//by Competition for inter_2Nx2N
			}
			// SKIP
			xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode );//by Merge for inter_2Nx2N
			rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

			if(!m_pcEncCfg->getUseEarlySkipDetection())
			{
			  // 2Nx2N, NxN
			  xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
			  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
			  if(m_pcEncCfg->getUseCbfFastMode())
			  {
				doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
			  }
			}
		}
      }
	  
      if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
      {
        iQP = iMinQP;
      }
    }

    if(!earlyDetectionSkipMode)
    {
      for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
      {
        const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP); // If lossless, then iQP is irrelevant for subsequent modules.

        if (bIsLosslessMode)
        {
          iQP = lowestQP;
        }

        rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

        // do inter modes, NxN, 2NxN, and Nx2N
        if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
        {
          // 2Nx2N, NxN

          if(!( (rpcBestCU->getWidth(0)==8) && (rpcBestCU->getHeight(0)==8) ))
          {
            if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() && doNotBlockPu)
            {
				if (CUResetPart == 0 || CUTargetMode[TempCount] == 3){
				  xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );
				  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				}
            }
          }

          if(doNotBlockPu)
          {
			  if(CUResetPart == 0||CUTargetMode[TempCount] == 2){
				xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N DEBUG_STRING_PASS_INTO(sDebug)  );
				rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
				{
				  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
				}
			  }
          }
          if(doNotBlockPu)
          {
			  if(CUResetPart == 0||CUTargetMode[TempCount] == 1){
				xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN DEBUG_STRING_PASS_INTO(sDebug)  );
				rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
				{
				  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
				}
			  }
          }

          //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
          if(sps.getUseAMP() && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() )
          {
#if AMP_ENC_SPEEDUP
            Bool bTestAMP_Hor = false, bTestAMP_Ver = false;

#if AMP_MRG
            Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

            deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);
#else
            deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver);
#endif

            //! Do horizontal AMP
            if ( bTestAMP_Hor || (CUResetPart == 1 && CUTargetMode[TempCount]>3))
            {
              if(doNotBlockPu)
              {
				  if(CUResetPart == 0||CUTargetMode[TempCount] == 4){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug) );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
					if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
					{
					  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
					}
				  }
              }
              if(doNotBlockPu)
              {
				  if(CUResetPart == 0||CUTargetMode[TempCount] == 5){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug) );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
					if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
					{
					  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
					}
				  }
              }
            }
#if AMP_MRG
            else if ( bTestMergeAMP_Hor|| (CUResetPart == 1 && CUTargetMode[TempCount]>3) )
            {
              if(doNotBlockPu)
              {
				  if(CUResetPart == 0||CUTargetMode[TempCount] == 4){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug), true );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
					if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
					{
					  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
					}
				  }
              }
              if(doNotBlockPu)
              {
				  if(CUResetPart == 0||CUTargetMode[TempCount] == 5){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug), true );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
					if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
					{
					  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
					}
				  }
              }
            }
#endif

            //! Do horizontal AMP
            if ( bTestAMP_Ver || (CUResetPart == 1 && CUTargetMode[TempCount]>3))
            {
              if(doNotBlockPu)
              {
				   if(CUResetPart == 0||CUTargetMode[TempCount] == 6){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug) );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
					if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
					{
					  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
					}
				   }
              }
              if(doNotBlockPu)
              {
				   if(CUResetPart == 0||CUTargetMode[TempCount] == 7){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug) );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				  }
			  }
            }
#if AMP_MRG
            else if ( bTestMergeAMP_Ver || (CUResetPart == 1 && CUTargetMode[TempCount]>3))
            {
              if(doNotBlockPu)
              {
				  if(CUResetPart == 0||CUTargetMode[TempCount] == 6){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug), true );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
					if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
					{
					  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
					}
				  }
              }
              if(doNotBlockPu)
              {
				  if(CUResetPart == 0||CUTargetMode[TempCount] == 7){
					xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug), true );
					rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				  }
              }
            }
#endif

#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

#endif
          }
        }

        // do normal intra modes
        // speedup for inter frames
        if((rpcBestCU->getSlice()->getSliceType() == I_SLICE)                                        ||
            ((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (
              (rpcBestCU->getCbf( 0, COMPONENT_Y  ) != 0)                                            ||
             ((rpcBestCU->getCbf( 0, COMPONENT_Cb ) != 0) && (numberValidComponents > COMPONENT_Cb)) ||
             ((rpcBestCU->getCbf( 0, COMPONENT_Cr ) != 0) && (numberValidComponents > COMPONENT_Cr))  // avoid very complex intra if it is unlikely
            )))
        {
			if(CUResetPart==0){
			  xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
			  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
			  if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() )
			  {
				if( rpcTempCU->getWidth(0) > ( 1 << sps.getQuadtreeTULog2MinSize() ) )
				{
				  xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );
				  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				}
			  }
			}
        }
		if(rpcBestCU->getSlice()->getSliceType() != I_SLICE && CUResetPart!=0){
				if(CUTargetMode[TempCount]==SIZE_2Nx2N){
					xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
					 rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
				}
				if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() )
				  {
					if( rpcTempCU->getWidth(0) > ( 1 << sps.getQuadtreeTULog2MinSize() ) )
					{
						if(CUTargetMode[TempCount]==SIZE_NxN){
					  xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );
					  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );}
					}
				  }
			}
        // test PCM
        if(sps.getUsePCM()
          && rpcTempCU->getWidth(0) <= (1<<sps.getPCMLog2MaxSize())
          && rpcTempCU->getWidth(0) >= (1<<sps.getPCMLog2MinSize()) )
        {
          UInt uiRawBits = getTotalBits(rpcBestCU->getWidth(0), rpcBestCU->getHeight(0), rpcBestCU->getPic()->getChromaFormat(), sps.getBitDepths().recon);
          UInt uiBestBits = rpcBestCU->getTotalBits();
          if((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(uiRawBits, 0)))
          {
            xCheckIntraPCM (rpcBestCU, rpcTempCU);
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
          }
        }

        if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
        {
          iQP = iMinQP;
        }
      }
    }

    if( rpcBestCU->getTotalCost()!=MAX_DOUBLE )
    {
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeSplitFlag( rpcBestCU, 0, uiDepth, true );
      rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
      rpcBestCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );
      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
    }
  }

  // copy original YUV samples to PCM buffer
  if( rpcBestCU->getTotalCost()!=MAX_DOUBLE && rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[uiDepth]);
  }

  if( uiDepth == pps.getMaxCuDQPDepth() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else if( uiDepth < pps.getMaxCuDQPDepth() )
  {
    iMinQP = iBaseQP;
    iMaxQP = iBaseQP;
  }
  else
  {
    const Int iStartQP = rpcTempCU->getQP(0);
    iMinQP = iStartQP;
    iMaxQP = iStartQP;
  }

  if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() )
  {
    iMinQP = iBaseQP - m_lumaQPOffset;
    iMaxQP = iMinQP;
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
  {
    iMaxQP = iMinQP; // If all TUs are forced into using transquant bypass, do not loop here.
  }

  const Bool bSubBranch = bBoundary || !( m_pcEncCfg->getUseEarlyCU() && rpcBestCU->getTotalCost()!=MAX_DOUBLE && rpcBestCU->isSkipped(0) );

  if( bSubBranch && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() && (!getFastDeltaQp() || uiWidth > fastDeltaQPCuMaxSize || bBoundary))
  {
    // further split
    Double splitTotalCost = 0;

    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
      const Bool bIsLosslessMode = false; // False at this level. Next level down may set it to true.

      rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

      UChar       uhNextDepth         = uiDepth+1;
      TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
      TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];
      DEBUG_STRING_NEW(sTempDebug)

      for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
      {
        pcSubBestPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.
        pcSubTempPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.

        if( ( pcSubBestPartCU->getCUPelX() < sps.getPicWidthInLumaSamples() ) && ( pcSubBestPartCU->getCUPelY() < sps.getPicHeightInLumaSamples() ) )
        {
          if ( 0 == uiPartUnitIdx) //initialize RD with previous depth buffer
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
          }
          else
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
          }

#if AMP_ENC_SPEEDUP
          DEBUG_STRING_NEW(sChild)
          if ( !(rpcBestCU->getTotalCost()!=MAX_DOUBLE && rpcBestCU->isInter(0)) )
          {
            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), NUMBER_OF_PART_SIZES );
          }
          else
          {

            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), rpcBestCU->getPartitionSize(0) );
          }
          DEBUG_STRING_APPEND(sTempDebug, sChild)
#else
          xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth );
#endif

          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
          xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );
          if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && pps.getMaxCuDQPDepth() >= 1 )
          {
            splitTotalCost += pcSubBestPartCU->getTotalCost();
          }
        }
        else
        {
          pcSubBestPartCU->copyToPic( uhNextDepth );
          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );
        }
      }

      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
      if( !bBoundary )
      {
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag( rpcTempCU, 0, uiDepth, true );
        if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && pps.getMaxCuDQPDepth() >= 1 )
        {
          Int splitBits = m_pcEntropyCoder->getNumberOfWrittenBits();
          Double splitBitCost = m_pcRdCost->calcRdCost( splitBits, 0 );
          splitTotalCost += splitBitCost;
        }

        rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      }

      if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && pps.getMaxCuDQPDepth() >= 1 )
      {
        rpcTempCU->getTotalCost() = splitTotalCost;
      }
      else
      {
        rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
      }

      if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
      {
        Bool hasResidual = false;
        for( UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx ++)
        {
          if( (     rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Y)
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cb) && (numberValidComponents > COMPONENT_Cb))
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cr) && (numberValidComponents > COMPONENT_Cr)) ) )
          {
            hasResidual = true;
            break;
          }
        }

        if ( hasResidual )
        {
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQP( rpcTempCU, 0, false );
          rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
          rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
          rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

          Bool foundNonZeroCbf = false;
          rpcTempCU->setQPSubCUs( rpcTempCU->getRefQP( 0 ), 0, uiDepth, foundNonZeroCbf );
          assert( foundNonZeroCbf );
        }
        else
        {
          rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
        }
      }

      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

      // If the configuration being tested exceeds the maximum number of bytes for a slice / slice-segment, then
      // a proper RD evaluation cannot be performed. Therefore, termination of the
      // slice/slice-segment must be made prior to this CTU.
      // This can be achieved by forcing the decision to be that of the rpcTempCU.
      // The exception is each slice / slice-segment must have at least one CTU.
      if (rpcBestCU->getTotalCost()!=MAX_DOUBLE)
      {
        const Bool isEndOfSlice        =    pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES
                                         && ((pcSlice->getSliceBits()+rpcBestCU->getTotalBits())>pcSlice->getSliceArgument()<<3)
                                         && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceCurStartCtuTsAddr())
                                         && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
        const Bool isEndOfSliceSegment =    pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES
                                         && ((pcSlice->getSliceSegmentBits()+rpcBestCU->getTotalBits()) > pcSlice->getSliceSegmentArgument()<<3)
                                         && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
                                             // Do not need to check slice condition for slice-segment since a slice-segment is a subset of a slice.
        if(isEndOfSlice||isEndOfSliceSegment)
        {
          rpcBestCU->getTotalCost()=MAX_DOUBLE;
        }
      }
	  if (CUResetPart == 0)//如果Reset标志位为False，那么是第一次执行，需要运行。如果Reset标志为True，且该CU有指定的划分类型，那么不需要继续运行。如果没有，需要运行。
	  {
		  //printf("------RD compare current larger prediction with sub---------\n");
		  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false)); // RD compare current larger prediction with sub partitioned prediction.
	  }
	  else if (CUTargetMode[TempCount] == 255)
	  {
		  //printf("ResetCheckBestMode---------%d", TempCount);
		  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false));
		  //printf("done\n");
	  }
	  else
	  {
		  //printf("No Need To Check%d\n", TempCount);
	  }
      //xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false) ); // RD compare current larger prediction
     //  if(( CUResetPart == 0))	  printf("%d============Nx2N ==============%d %d %d\n", rpcBestCU->getWidth(0),rpcBestCU->getPartitionSize(0),TempCount,CUTargetMode[TempCount]);                                                                                                                                                // with sub partitioned prediction.
    }
  }

  //可以在下面保存CU最终结果
	if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)//PocIndex==6
	{
		CUDepth[TempCount] += (int)rpcBestCU->getDepth(0);
		//  printf("============TempCount==============%d \n", TempCount);
		CUPartSize[TempCount] = rpcBestCU->getPartitionSize(0);
		//cout<<"CUPartSize["<<TempCount<<"]="<<CUPartSize[TempCount]<<endl;
		//if(( CUResetPart != 0))	  printf("%d============Nx2N ==============%d %d %d\n", rpcBestCU->getWidth(0),rpcBestCU->getPartitionSize(0),TempCount,CUTargetMode[TempCount]);
	}

	//*****************



	//*****************






  DEBUG_STRING_APPEND(sDebug_, sDebug);

  rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

  xCopyYuv2Pic( rpcBestCU->getPic(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu(), uiDepth, uiDepth );   // Copy Yuv data to picture Yuv
  if (bBoundary)
  {
    return;
  }

  // Assert if Best prediction mode is NONE
  // Selected mode's RD-cost must be not MAX_DOUBLE.
  
  //if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
//	  cout<<"rpcBestCU->getPartitionSize ( 0 ) "<<rpcBestCU->getPartitionSize ( 0 )<<endl;  //test:ljd

  assert( rpcBestCU->getPartitionSize ( 0 ) != NUMBER_OF_PART_SIZES       );
  assert( rpcBestCU->getPredictionMode( 0 ) != NUMBER_OF_PREDICTION_MODES );
  assert( rpcBestCU->getTotalCost     (   ) != MAX_DOUBLE                 );
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::finishCU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());

  //Calculate end address
  const Int  currentCTUTsAddr = pcPic->getPicSym()->getCtuRsToTsAddrMap(pcCU->getCtuRsAddr());
  const Bool isLastSubCUOfCtu = pcCU->isLastSubCUOfCtu(uiAbsPartIdx);
  if ( isLastSubCUOfCtu )
  {
    // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
    // i.e. when the slice segment CurEnd CTU address is the current CTU address+1.
    if (pcSlice->getSliceSegmentCurEndCtuTsAddr() != currentCTUTsAddr+1)
    {
      m_pcEntropyCoder->encodeTerminatingBit( 0 );
    }
  }
}

/** Compute QP for each CU
 * \param pcCU Target CU
 * \param uiDepth CU depth
 * \returns quantization parameter
 */
Int TEncCu::xComputeQP( TComDataCU* pcCU, UInt uiDepth )
{
  Int iBaseQp = pcCU->getSlice()->getSliceQp();
  Int iQpOffset = 0;
  if ( m_pcEncCfg->getUseAdaptiveQP() )
  {
    TEncPic* pcEPic = dynamic_cast<TEncPic*>( pcCU->getPic() );
    UInt uiAQDepth = min( uiDepth, pcEPic->getMaxAQDepth()-1 );
    TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer( uiAQDepth );
    UInt uiAQUPosX = pcCU->getCUPelX() / pcAQLayer->getAQPartWidth();
    UInt uiAQUPosY = pcCU->getCUPelY() / pcAQLayer->getAQPartHeight();
    UInt uiAQUStride = pcAQLayer->getAQPartStride();
    TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

    Double dMaxQScale = pow(2.0, m_pcEncCfg->getQPAdaptationRange()/6.0);
    Double dAvgAct = pcAQLayer->getAvgActivity();
    Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
    Double dNormAct = (dMaxQScale*dCUAct + dAvgAct) / (dCUAct + dMaxQScale*dAvgAct);
    Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
    iQpOffset = Int(floor( dQpOffset + 0.49999 ));
  }

  return Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQp+iQpOffset );
}

/** encode a CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::xEncodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
        TComPic   *const pcPic   = pcCU->getPic();
        TComSlice *const pcSlice = pcCU->getSlice();
  const TComSPS   &sps =*(pcSlice->getSPS());
  const TComPPS   &pps =*(pcSlice->getPPS());

  const UInt maxCUWidth  = sps.getMaxCUWidth();
  const UInt maxCUHeight = sps.getMaxCUHeight();

        Bool bBoundary = false;
        UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  const UInt uiRPelX   = uiLPelX + (maxCUWidth>>uiDepth)  - 1;
        UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  const UInt uiBPelY   = uiTPelY + (maxCUHeight>>uiDepth) - 1;

  if( ( uiRPelX < sps.getPicWidthInLumaSamples() ) && ( uiBPelY < sps.getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartitionsInCtu() >> (uiDepth<<1) )>>2;
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      setdQPFlag(true);
    }

    if( uiDepth == pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
    {
      setCodeChromaQpAdjFlag(true);
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xEncodeCU( pcCU, uiAbsPartIdx, uiDepth+1 );
      }
    }
    return;
  }

  if( uiDepth <= pps.getMaxCuDQPDepth() && pps.getUseDQP())
  {
    setdQPFlag(true);
  }

  if( uiDepth <= pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
  {
    setCodeChromaQpAdjFlag(true);
  }

  if (pps.getTransquantBypassEnabledFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx );
  }

  if( !pcSlice->isIntra() )
  {
    m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
  }

  if( pcCU->isSkipped( uiAbsPartIdx ) )
  {
    m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx );
    finishCU(pcCU,uiAbsPartIdx);
    return;
  }

  m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );
  m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyCoder->encodeIPCMInfo( pcCU, uiAbsPartIdx );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      // Encode slice finish
      finishCU(pcCU,uiAbsPartIdx);
      return;
    }
  }

  // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyCoder->encodePredInfo( pcCU, uiAbsPartIdx );

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdj = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, codeChromaQpAdj );
  setCodeChromaQpAdjFlag( codeChromaQpAdj );
  setdQPFlag( bCodeDQP );

  // --- write terminating bit ---
  finishCU(pcCU,uiAbsPartIdx);
}

Int xCalcHADs8x8_ISlice(Pel *piOrg, Int iStrideOrg)
{
  Int k, i, j, jj;
  Int diff[64], m1[8][8], m2[8][8], m3[8][8], iSumHad = 0;

  for( k = 0; k < 64; k += 8 )
  {
    diff[k+0] = piOrg[0] ;
    diff[k+1] = piOrg[1] ;
    diff[k+2] = piOrg[2] ;
    diff[k+3] = piOrg[3] ;
    diff[k+4] = piOrg[4] ;
    diff[k+5] = piOrg[5] ;
    diff[k+6] = piOrg[6] ;
    diff[k+7] = piOrg[7] ;

    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];

    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];

    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }

  //vertical
  for (i=0; i < 8; i++)
  {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];

    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];

    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      iSumHad += abs(m2[i][j]);
    }
  }
  iSumHad -= abs(m2[0][0]);
  iSumHad =(iSumHad+2)>>2;
  return(iSumHad);
}

Int  TEncCu::updateCtuDataISlice(TComDataCU* pCtu, Int width, Int height)
{
  Int  xBl, yBl;
  const Int iBlkSize = 8;

  Pel* pOrgInit   = pCtu->getPic()->getPicYuvOrg()->getAddr(COMPONENT_Y, pCtu->getCtuRsAddr(), 0);
  Int  iStrideOrig = pCtu->getPic()->getPicYuvOrg()->getStride(COMPONENT_Y);
  Pel  *pOrg;

  Int iSumHad = 0;
  for ( yBl=0; (yBl+iBlkSize)<=height; yBl+= iBlkSize)
  {
    for ( xBl=0; (xBl+iBlkSize)<=width; xBl+= iBlkSize)
    {
      pOrg = pOrgInit + iStrideOrig*yBl + xBl;
      iSumHad += xCalcHADs8x8_ISlice(pOrg, iStrideOrig);
    }
  }
  return(iSumHad);
}

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \param earlyDetectionSkipMode
 */
Void TEncCu::xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode )
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
  if(getFastDeltaQp())
  {
    return;   // never check merge in fast deltaqp mode
  }
  TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0;
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

  for( UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );

  Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
  for( UInt ui = 0; ui < numValidMergeCand; ++ui )
  {
    mergeCandBuffer[ui] = 0;
  }

  Bool bestIsSkip = false;

  UInt iteration;
  if ( rpcTempCU->isLosslessCoded(0))
  {
    iteration = 1;
  }
  else
  {
    iteration = 2;
  }
  DEBUG_STRING_NEW(bestStr)

  for( UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual )
  {
    for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
    {
      if(!(uiNoResidual==1 && mergeCandBuffer[uiMergeCand]==1))
      {
        if( !(bestIsSkip && uiNoResidual == 0) )
        {
          DEBUG_STRING_NEW(tmpStr)
          // set MC parameters
          rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setCUTransquantBypassSubParts( bTransquantBypassFlag, 0, uhDepth );
          rpcTempCU->setChromaQpAdjSubParts( bTransquantBypassFlag ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );
          rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level

          // do MC
          m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
          // estimate residual and encode everything
          m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                     m_ppcOrigYuv    [uhDepth],
                                                     m_ppcPredYuvTemp[uhDepth],
                                                     m_ppcResiYuvTemp[uhDepth],
                                                     m_ppcResiYuvBest[uhDepth],
                                                     m_ppcRecoYuvTemp[uhDepth],
                                                     (uiNoResidual != 0) DEBUG_STRING_PASS_INTO(tmpStr) );

#if DEBUG_STRING
          DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

          if ((uiNoResidual == 0) && (rpcTempCU->getQtRootCbf(0) == 0))
          {
            // If no residual when allowing for one, then set mark to not try case where residual is forced to 0
            mergeCandBuffer[uiMergeCand] = 1;
          }

          Int orgQP = rpcTempCU->getQP( 0 );
          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr));

          rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag );

          if( m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip )
          {
            bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
          }
        }
      }
    }

    if(uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
    {
      if(rpcBestCU->getQtRootCbf( 0 ) == 0)
      {
        if( rpcBestCU->getMergeFlag( 0 ))
        {
          *earlyDetectionSkipMode = true;
        }
        else if(m_pcEncCfg->getMotionEstimationSearchMethod() != MESEARCH_SELECTIVE)
        {
          Int absoulte_MV=0;
          for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
          {
            if ( rpcBestCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
            {
              TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList( uiRefListIdx ));
              Int iHor = pcCUMvField->getMvd( 0 ).getAbsHor();
              Int iVer = pcCUMvField->getMvd( 0 ).getAbsVer();
              absoulte_MV+=iHor+iVer;
            }
          }

          if(absoulte_MV == 0)
          {
            *earlyDetectionSkipMode = true;
          }
        }
      }
    }
  }
  DEBUG_STRING_APPEND(sDebug, bestStr)
}


#if AMP_MRG
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize )
#endif
{
  DEBUG_STRING_NEW(sTest)

  if(getFastDeltaQp())
  {
    const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
    const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight()>>(sps.getLog2DiffMaxMinCodingBlockSize()), sps.getMaxCUHeight(), 32u);
    if(ePartSize != SIZE_2Nx2N || rpcTempCU->getWidth( 0 ) > fastDeltaQPCuMaxSize)
    {
      return; // only check necessary 2Nx2N Inter in fast deltaqp mode
    }
  }

  // prior to this, rpcTempCU will have just been reset using rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
  UChar uhDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setPartSizeSubParts  ( ePartSize,  0, uhDepth );
  rpcTempCU->setPredModeSubParts  ( MODE_INTER, 0, uhDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );

#if AMP_MRG
  rpcTempCU->setMergeAMP (true);
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] DEBUG_STRING_PASS_INTO(sTest), false, bUseMRG );
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] );
#endif

#if AMP_MRG
  if ( !rpcTempCU->getMergeAMP() )
  {
    return;
  }
#endif

  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false DEBUG_STRING_PASS_INTO(sTest) );
  rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

#if DEBUG_STRING
  DebugInterPredResiReco(sTest, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}

Void TEncCu::xCheckRDCostIntra( TComDataCU *&rpcBestCU,
                                TComDataCU *&rpcTempCU,
                                PartSize     eSize
                                DEBUG_STRING_FN_DECLARE(sDebug) )
{
  DEBUG_STRING_NEW(sTest)

  if(getFastDeltaQp())
  {
    const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
    const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight()>>(sps.getLog2DiffMaxMinCodingBlockSize()), sps.getMaxCUHeight(), 32u);
    if(rpcTempCU->getWidth( 0 ) > fastDeltaQPCuMaxSize)
    {
      return; // only check necessary 2Nx2N Intra in fast deltaqp mode
    }
  }

  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );

  rpcTempCU->setPartSizeSubParts( eSize, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth );

  Pel resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];

  m_pcPredSearch->estIntraPredLumaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest) );

  m_ppcRecoYuvTemp[uiDepth]->copyToPicComponent(COMPONENT_Y, rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getCtuRsAddr(), rpcTempCU->getZorderIdxInCtu() );

  if (rpcBestCU->getPic()->getChromaFormat()!=CHROMA_400)
  {
    m_pcPredSearch->estIntraPredChromaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest) );
  }

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }

  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodePredInfo( rpcTempCU, 0 );
  m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true );

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdjFlag = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( rpcTempCU, 0, uiDepth, bCodeDQP, codeChromaQpAdjFlag );
  setCodeChromaQpAdjFlag( codeChromaQpAdjFlag );
  setdQPFlag( bCodeDQP );

  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );

  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}


/** Check R-D costs for a CU with PCM mode.
 * \param rpcBestCU pointer to best mode CU data structure
 * \param rpcTempCU pointer to testing mode CU data structure
 * \returns Void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
Void TEncCu::xCheckIntraPCM( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
{
  if(getFastDeltaQp())
  {
    const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
    const UInt fastDeltaQPCuMaxPCMSize = Clip3((UInt)1<<sps.getPCMLog2MinSize(), (UInt)1<<sps.getPCMLog2MaxSize(), 32u);
    if (rpcTempCU->getWidth( 0 ) > fastDeltaQPCuMaxPCMSize)
    {
      return;   // only check necessary PCM in fast deltaqp mode
    }
  }
  
  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );

  rpcTempCU->setIPCMFlag(0, true);
  rpcTempCU->setIPCMFlagSubParts (true, 0, rpcTempCU->getDepth(0));
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setTrIdxSubParts ( 0, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth );

  m_pcPredSearch->IPCMSearch( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth]);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }

  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize ( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodeIPCMInfo ( rpcTempCU, 0, true );

  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );
  DEBUG_STRING_NEW(a)
  DEBUG_STRING_NEW(b)
  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(a) DEBUG_STRING_PASS_INTO(b));
}

/** check whether current try is the best with identifying the depth of current try
 * \param rpcBestCU
 * \param rpcTempCU
 * \param uiDepth
 */
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo) )
{
  if( rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost() )
  {
    TComYuv* pcYuv;
    // Change Information data
    TComDataCU* pcCU = rpcBestCU;
    rpcBestCU = rpcTempCU;
    rpcTempCU = pcCU;

    // Change Prediction data
    pcYuv = m_ppcPredYuvBest[uiDepth];
    m_ppcPredYuvBest[uiDepth] = m_ppcPredYuvTemp[uiDepth];
    m_ppcPredYuvTemp[uiDepth] = pcYuv;

    // Change Reconstruction data
    pcYuv = m_ppcRecoYuvBest[uiDepth];
    m_ppcRecoYuvBest[uiDepth] = m_ppcRecoYuvTemp[uiDepth];
    m_ppcRecoYuvTemp[uiDepth] = pcYuv;

    pcYuv = NULL;
    pcCU  = NULL;

    // store temp best CI for next CU coding
    m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);


#if DEBUG_STRING
    DEBUG_STRING_SWAP(sParent, sTest)
    const PredMode predMode=rpcBestCU->getPredictionMode(0);
    if ((DebugOptionList::DebugString_Structure.getInt()&DebugStringGetPredModeMask(predMode)) && bAddSizeInfo)
    {
      std::stringstream ss(stringstream::out);
      ss <<"###: " << (predMode==MODE_INTRA?"Intra   ":"Inter   ") << partSizeToString[rpcBestCU->getPartitionSize(0)] << " CU at " << rpcBestCU->getCUPelX() << ", " << rpcBestCU->getCUPelY() << " width=" << UInt(rpcBestCU->getWidth(0)) << std::endl;
      sParent+=ss.str();
    }
#endif
  }
}

Void TEncCu::xCheckDQP( TComDataCU* pcCU )
{
  UInt uiDepth = pcCU->getDepth( 0 );

  const TComPPS &pps = *(pcCU->getSlice()->getPPS());
  if ( pps.getUseDQP() && uiDepth <= pps.getMaxCuDQPDepth() )
  {
    if ( pcCU->getQtRootCbf( 0) )
    {
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( pcCU, 0, false );
      pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
      pcCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      pcCU->getTotalCost() = m_pcRdCost->calcRdCost( pcCU->getTotalBits(), pcCU->getTotalDistortion() );
    }
    else
    {
      pcCU->setQPSubParts( pcCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
    }
  }
}

Void TEncCu::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}
Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth )
{
  UInt uiAbsPartIdxInRaster = g_auiZscanToRaster[uiAbsPartIdx];
  UInt uiSrcBlkWidth = rpcPic->getNumPartInCtuWidth() >> (uiSrcDepth);
  UInt uiBlkWidth    = rpcPic->getNumPartInCtuWidth() >> (uiDepth);
  UInt uiPartIdxX = ( ( uiAbsPartIdxInRaster % rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdxY = ( ( uiAbsPartIdxInRaster / rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdx = uiPartIdxY * ( uiSrcBlkWidth / uiBlkWidth ) + uiPartIdxX;
  m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvRec (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);

  m_ppcPredYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvPred (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);
}

Void TEncCu::xCopyYuv2Tmp( UInt uiPartUnitIdx, UInt uiNextDepth )
{
  UInt uiCurrDepth = uiNextDepth - 1;
  m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv( m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx );
  m_ppcPredYuvBest[uiNextDepth]->copyToPartYuv( m_ppcPredYuvBest[uiCurrDepth], uiPartUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param pCU pointer to current CU
 * \param pOrgYuv pointer to original sample array
 */
Void TEncCu::xFillPCMBuffer     ( TComDataCU* pCU, TComYuv* pOrgYuv )
{
  const ChromaFormat format = pCU->getPic()->getChromaFormat();
  const UInt numberValidComponents = getNumberValidComponents(format);
  for (UInt componentIndex = 0; componentIndex < numberValidComponents; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    const UInt width  = pCU->getWidth(0)  >> getComponentScaleX(component, format);
    const UInt height = pCU->getHeight(0) >> getComponentScaleY(component, format);

    Pel *source      = pOrgYuv->getAddr(component, 0, width);
    Pel *destination = pCU->getPCMSample(component);

    const UInt sourceStride = pOrgYuv->getStride(component);

    for (Int line = 0; line < height; line++)
    {
      for (Int column = 0; column < width; column++)
      {
        destination[column] = source[column];
      }

      source      += sourceStride;
      destination += width;
    }
  }
}

#if ADAPTIVE_QP_SELECTION
/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples )
{
  for( Int n = 0; n < NumCoeffInCU; n++ )
  {
    TCoeff u = abs( rpcCoeff[ n ] );
    TCoeff absc = rpcArlCoeff[ n ];

    if( u != 0 )
    {
      if( u < LEVEL_RANGE )
      {
        cSum[ u ] += ( Double )absc;
        numSamples[ u ]++;
      }
      else
      {
        cSum[ LEVEL_RANGE ] += ( Double )absc - ( Double )( u << ARL_C_PRECISION );
        numSamples[ LEVEL_RANGE ]++;
      }
    }
  }

  return 0;
}

//! Collect ARL statistics from one CTU
Void TEncCu::xCtuCollectARLStats(TComDataCU* pCtu )
{
  Double cSum[ LEVEL_RANGE + 1 ];     //: the sum of DCT coefficients corresponding to data type and quantization output
  UInt numSamples[ LEVEL_RANGE + 1 ]; //: the number of coefficients corresponding to data type and quantization output

  TCoeff* pCoeffY = pCtu->getCoeff(COMPONENT_Y);
  TCoeff* pArlCoeffY = pCtu->getArlCoeff(COMPONENT_Y);
  const TComSPS &sps = *(pCtu->getSlice()->getSPS());

  const UInt uiMinCUWidth = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth(); // NOTE: ed - this is not the minimum CU width. It is the square-root of the number of coefficients per part.
  const UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;                          // NOTE: ed - what is this?

  memset( cSum, 0, sizeof( Double )*(LEVEL_RANGE+1) );
  memset( numSamples, 0, sizeof( UInt )*(LEVEL_RANGE+1) );

  // Collect stats to cSum[][] and numSamples[][]
  for(Int i = 0; i < pCtu->getTotalNumPart(); i ++ )
  {
    UInt uiTrIdx = pCtu->getTransformIdx(i);

    if(pCtu->isInter(i) && pCtu->getCbf( i, COMPONENT_Y, uiTrIdx ) )
    {
      xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
    }//Note that only InterY is processed. QP rounding is based on InterY data only.

    pCoeffY  += uiMinNumCoeffInCU;
    pArlCoeffY  += uiMinNumCoeffInCU;
  }

  for(Int u=1; u<LEVEL_RANGE;u++)
  {
    m_pcTrQuant->getSliceSumC()[u] += cSum[ u ] ;
    m_pcTrQuant->getSliceNSamples()[u] += numSamples[ u ] ;
  }
  m_pcTrQuant->getSliceSumC()[LEVEL_RANGE] += cSum[ LEVEL_RANGE ] ;
  m_pcTrQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[ LEVEL_RANGE ] ;
}
#endif
//! \}
