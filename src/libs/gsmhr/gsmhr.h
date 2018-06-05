#ifndef __GSM_HR_CODEC_H
#define __GSM_HR_CODEC_H

#include <stdint.h>
#include "gsmhr_sp_rom.h"

namespace GsmHr
{
  #define DATE    "August 8, 1996    "
  #define VERSION "Version 4.2       "

  #define LW_SIGN (long)0x80000000       /* sign bit */
  #define LW_MIN (long)0x80000000
  #define LW_MAX (long)0x7fffffff

  #define SW_SIGN (short)0x8000          /* sign bit for int16_t type */
  #define SW_MIN (short)0x8000           /* smallest Ram */
  #define SW_MAX (short)0x7fff           /* largest Ram */
  #define SPEECH      1
  #define CNIFIRSTSID 2
  #define PN_INIT_SEED (int32_t)0x1091988L       /* initial seed for Comfort
                                                 * noise pn-generator */

  #define CNICONT     3
  #define CNIBFI      4
  #define VALIDSID    11
  #define INVALIDSID  22
  #define GOODSPEECH  33
  #define UNUSABLE    44

  typedef short int int16_tRom;        /* 16 bit ROM data    (sr*) */
  typedef long int int32_tRom;          /* 32 bit ROM data    (L_r*)  */

  struct NormSw
  {                                      /* normalized int16_t fractional
                                          * number snr.man precedes snr.sh (the
                                          * shift count)i */
    int16_t man;                       /* "mantissa" stored in 16 bit
                                          * location */
    int16_t sh;                        /* the shift count, stored in 16 bit
                                          * location */
  };


  struct QuantList
  {
    /* structure which points to the beginning of a block of candidate vq
     * vectors.  It also stores the residual error for each vector. */
    int    iNum;                         /* total number in list */
    int    iRCIndex;                     /* an index to the first vector of the
                                          * block */
    int16_t pswPredErr[PREQ1_NUM_OF_ROWS];  /* PREQ1 is the biggest block */
  };

  /* Global constants *
   ********************/

  #define NP 10                          /* order of the lpc filter */
  #define N_SUB 4                        /* number of subframes */
  #define F_LEN 160                      /* number of samples in a frame */
  #define S_LEN 40                       /* number of samples in a subframe */
  #define A_LEN 170                      /* LPC analysis length */
  #define OS_FCTR 6                      /* maximum LTP lag oversampling
                                          * factor */

  #define OVERHANG 8                     /* vad parameter */
  #define strStr strStr16

  #define  LTP_LEN      147              /* 147==0x93 length of LTP history */
  #define CNINTPER    12

  class Codec
  {
  protected:

    // From typedefs.h
    int giFrmCnt;                   /* 0,1,2,3,4..... */
    int giSfrmCnt = 0;              /* 0,1,2,3 */

    int giDTXon = 1;                /* DTX Mode on/off */


    // From err_conc.c
    int32_t plSubfrEnergyMem[4];
    int16_t swLevelMem[4],
           lastR0,
           pswLastGood[18],
           swState,
           swLastFlag;


    // From sp_dec.c

    int16_t gswPostFiltAgcGain,
           gpswPostFiltStateNum[NP],
           gpswPostFiltStateDenom[NP],
           swPostEmphasisState,
           pswSynthFiltState[NP],
           pswOldFrmKsDec[NP],
           pswOldFrmAsDec[NP],
           pswOldFrmPFNum[NP],
           pswOldFrmPFDenom[NP],
           swOldR0Dec,
           pswLtpStateBaseDec[LTP_LEN + S_LEN],
           pswPPreState[LTP_LEN + S_LEN];


    int16_t swMuteFlagOld;             /* error concealment */



    int16_t swRxDTXState = CNINTPER - 1;        /* DTX State at the rx.
                                                   * Modulo */

    int16_t swDecoMode = SPEECH;
    int16_t swDtxMuting = 0;
    int16_t swDtxBfiCnt = 0;

    int16_t swOldR0IndexDec = 0;

    int16_t swRxGsHistPtr = 0;
    int32_t pL_RxGsHist[(OVERHANG - 1) * N_SUB];

    int16_t swR0Dec;

    int16_t swVoicingMode,             /* MODE */
           pswVq[3],                     /* LPC1, LPC2, LPC3 */
           swSi,                         /* INT_LPC */
           swEngyRShift;                 /* for use by spectral postfilter */


    int16_t swR0NewCN;                 /* DTX mode */

    int32_tRom ppLr_gsTable[4][32];       /* DTX mode */

    int16_t
          *pswLtpStateOut = &pswLtpStateBaseDec[LTP_LEN],
           pswSythAsSpace[NP * N_SUB],
           pswPFNumAsSpace[NP * N_SUB],
           pswPFDenomAsSpace[NP * N_SUB],
          *ppswSynthAs[N_SUB] = {
      &pswSythAsSpace[0],
      &pswSythAsSpace[10],
      &pswSythAsSpace[20],
      &pswSythAsSpace[30],
    },

          *ppswPFNumAs[N_SUB] = {
      &pswPFNumAsSpace[0],
      &pswPFNumAsSpace[10],
      &pswPFNumAsSpace[20],
      &pswPFNumAsSpace[30],
    },
          *ppswPFDenomAs[N_SUB] = {
      &pswPFDenomAsSpace[0],
      &pswPFDenomAsSpace[10],
      &pswPFDenomAsSpace[20],
      &pswPFDenomAsSpace[30],
    };

    int16_tRom
           psrSPFDenomWidenCf[NP] = {
      0x6000, 0x4800, 0x3600, 0x2880, 0x1E60,
      0x16C8, 0x1116, 0x0CD0, 0x099C, 0x0735,
    };


    int32_t L_RxPNSeed;          /* DTX mode */
    int16_t swRxDtxGsIndex;     /* DTX mode */

    // From dtx.c
    int16_t swVadFrmCnt = 0;             /* Indicates the number of sequential
                                            * frames where VAD == 0 */

    short int siUpdPointer = 0;
    int16_t swNElapsed = 50;

    int32_t pL_GsHist[N_SUB * (OVERHANG - 1)];

    /* history of unquantized parameters */
    int32_t pL_R0Hist[OVERHANG];
    int32_t ppL_CorrHist[OVERHANG][NP + 1];

    /* quantized reference parameters */
    int16_t swQntRefR0,
           swRefGsIndex;
    int piRefVqCodewds[3];

    /* handling of short speech bursts */
    int16_t swShortBurst;

    /* state value of random generator */
    int32_t L_TxPNSeed;

    int16_t swR0OldCN;
    int32_t pL_OldCorrSeq[NP + 1],
           pL_NewCorrSeq[NP + 1],
           pL_CorrSeq[NP + 1];

    // From sp_enc.c
    int16_t swTxGsHistPtr = 0;

    int16_t pswCNVSCode1[N_SUB],
              pswCNVSCode2[N_SUB],
              pswCNGsp0Code[N_SUB],
              pswCNLpc[3],
              swCNR0;

    int16_t pswAnalysisState[NP];

    int16_t pswWStateNum[NP],
           pswWStateDenom[NP];

    int16_t swLastLag;
    /*_________________________________________________________________________
     |                                                                         |
     |                         Other External Variables                        |
     |_________________________________________________________________________|
    */

    int16_tRom *psrTable;         /* points to correct table of
                                   * vectors */
    int iLimit;                   /* accessible to all in this file
                                   * and to lpcCorrQntz() in dtx.c */
    int iLow;                     /* the low element in this segment */
    int iThree;                   /* boolean, is this a three element
                                            * vector */
    int iWordHalfPtr;             /* points to the next byte */
    int iWordPtr;                 /* points to the next word to be */

    // from vad.c

    int16_t
           pswRvad[9],
           swNormRvad,
           swPt_sacf,
           swPt_sav0,
           swE_thvad,
           swM_thvad,
           swAdaptCount,
           swBurstCount,
           swHangCount,
           swOldLagCount,
           swVeryOldLagCount,
           swOldLag;

    int32_t
           pL_sacf[27],
           pL_sav0[36],
           L_lastdm;

  public:

    // From VAD
    void   vad_reset(void);

    void   vad_algorithm
           (
                   int32_t pL_acf[9],
                   int16_t swScaleAcf,
                   int16_t pswRc[4],
                   int16_t swPtch,
                   int16_t *pswVadFlag
    );

    void   energy_computation
           (
                   int32_t pL_acf[],
                   int16_t swScaleAcf,
                   int16_t pswRvad[],
                   int16_t swNormRvad,
                   int16_t *pswM_pvad,
                   int16_t *pswE_pvad,
                   int16_t *pswM_acf0,
                   int16_t *pswE_acf0
    );


    void   average_acf
           (
                   int32_t pL_acf[],
                   int16_t swScaleAcf,
                   int32_t pL_av0[],
                   int32_t pL_av1[]
    );

    void   predictor_values
           (
                   int32_t pL_av1[],
                   int16_t pswRav1[],
                   int16_t *pswNormRav1
    );

    void   schur_recursion
           (
                   int32_t pL_av1[],
                   int16_t pswVpar[]
    );

    void   step_up
           (
                   int16_t swNp,
                   int16_t pswVpar[],
                   int16_t pswAav1[]
    );

    void   compute_rav1
           (
                   int16_t pswAav1[],
                   int16_t pswRav1[],
                   int16_t *pswNormRav1
    );

    void   spectral_comparison
           (
                   int16_t pswRav1[],
                   int16_t swNormRav1,
                   int32_t pL_av0[],
                   int16_t *pswStat
    );

    void   tone_detection
           (
                   int16_t pswRc[4],
                   int16_t *pswTone
    );


    void   threshold_adaptation
           (
                   int16_t swStat,
                   int16_t swPtch,
                   int16_t swTone,
                   int16_t pswRav1[],
                   int16_t swNormRav1,
                   int16_t swM_pvad,
                   int16_t swE_pvad,
                   int16_t swM_acf0,
                   int16_t swE_acf0,
                   int16_t pswRvad[],
                   int16_t *pswNormRvad,
                   int16_t *pswM_thvad,
                   int16_t *pswE_thvad
    );

    void   vad_decision
           (
                   int16_t swM_pvad,
                   int16_t swE_pvad,
                   int16_t swM_thvad,
                   int16_t swE_thvad,
                   int16_t *pswVvad
    );

    void   vad_hangover
           (
                   int16_t swVvad,
                   int16_t *pswVadFlag
    );

    void   periodicity_update
           (
                   int16_t pswLags[4],
                   int16_t *pswPtch
    );


    // From err_conc.h

    void   para_conceal_speech_decoder(int16_t pswErrorFlag[],
                           int16_t pswSpeechPara[], int16_t *pswMutePermit);

    int16_t level_calc(int16_t swInd, int32_t *pl_en);

    void   level_estimator(int16_t update, int16_t *pswLevelMean,
                                    int16_t *pswLevelMax,
                                    int16_t pswDecodedSpeechFrame[]);

    void   signal_conceal_sub(int16_t pswPPFExcit[],
                         int16_t ppswSynthAs[], int16_t pswSynthFiltState[],
                           int16_t pswLtpStateOut[], int16_t pswPPreState[],
                                    int16_t swLevelMean, int16_t swLevelMax,
                                int16_t swErrorFlag1, int16_t swMuteFlagOld,
                                int16_t *pswMuteFlag, int16_t swMutePermit);



      void   speechDecoder(int16_t pswParameters[],
                                int16_t pswDecodedSpeechFrame[]);

      void   aFlatRcDp(int32_t *pL_R, int16_t *pswRc);

      void   b_con(int16_t swCodeWord, short siNumBits,
                          int16_t pswVectOut[]);

      void   fp_ex(int16_t swOrigLagIn, int16_t pswLTPState[]);

      int16_t g_corr1(int16_t *pswIn, int32_t *pL_out);

      int16_t g_corr1s(int16_t pswIn[], int16_t swEngyRShft,
                                int32_t *pL_out);

      void   getSfrmLpc(short int siSoftInterpolation,
                               int16_t swPrevR0, int16_t swNewR0,
                               int16_t pswPrevFrmKs[],
                               int16_t pswPrevFrmAs[],
                               int16_t pswPrevFrmPFNum[],
                               int16_t pswPrevFrmPFDenom[],
                               int16_t pswNewFrmKs[],
                               int16_t pswNewFrmAs[],
                               int16_t pswNewFrmPFNum[],
                               int16_t pswNewFrmPFDenom[],
                               struct NormSw *psnsSqrtRs,
                               int16_t *ppswSynthAs[],
                               int16_t *ppswPFNumAs[],
                               int16_t *ppswPFDenomAs[]);

      void   get_ipjj(int16_t swLagIn,
                             int16_t *pswIp, int16_t *pswJj);

      short int interpolateCheck(int16_t pswRefKs[],
                                        int16_t pswRefCoefsA[],
                                        int16_t pswOldCoefsA[],
                                        int16_t pswNewCoefsA[],
                                        int16_t swOldPer,
                                        int16_t swNewPer,
                                        int16_t swRq,
                                        struct NormSw *psnsSqrtRsOut,
                                        int16_t pswCoefOutA[]);

      void   lpcFir(int16_t pswInput[], int16_t pswCoef[],
                           int16_t pswState[], int16_t pswFiltOut[]);

      void   lpcIir(int16_t pswInput[], int16_t pswCoef[],
                           int16_t pswState[], int16_t pswFiltOut[]);

      void   lpcIrZsIir(int16_t pswCoef[], int16_t pswFiltOut[]);

      void   lpcZiIir(int16_t pswCoef[], int16_t pswState[],
                             int16_t pswFiltOut[]);

      void   lpcZsFir(int16_t pswInput[], int16_t pswCoef[],
                             int16_t pswFiltOut[]);

      void   lpcZsIir(int16_t pswInput[], int16_t pswCoef[],
                             int16_t pswFiltOut[]);

      void   lpcZsIirP(int16_t pswCommonIO[], int16_t pswCoef[]);

      int16_t r0BasedEnergyShft(int16_t swR0Index);

      short  rcToADp(int16_t swAscale, int16_t pswRc[],
                            int16_t pswA[]);

      void   rcToCorrDpL(int16_t swAshift, int16_t swAscale,
                                int16_t pswRc[], int32_t pL_R[]);

      void   res_eng(int16_t pswReflecCoefIn[], int16_t swRq,
                            struct NormSw *psnsSqrtRsOut);

      void   rs_rr(int16_t pswExcitation[], struct NormSw snsSqrtRs,
                          struct NormSw *snsSqrtRsRr);

      void   rs_rrNs(int16_t pswExcitation[], struct NormSw snsSqrtRs,
                            struct NormSw *snsSqrtRsRr);

      int16_t scaleExcite(int16_t pswVect[],
                                   int16_t swErrTerm, struct NormSw snsRS,
                                   int16_t pswScldVect[]);

      int16_t sqroot(int32_t L_SqrtIn);

      void   v_con(int16_t pswBVects[], int16_t pswOutVect[],
                          int16_t pswBitArray[], short int siNumBVctrs);

      void a_sst(int16_t swAshift, int16_t swAscale,
                               int16_t pswDirectFormCoefIn[],
                               int16_t pswDirectFormCoefOut[]);

      int16_t agcGain(int16_t pswStateCurr[],
                              struct NormSw snsInSigEnergy, int16_t swEngyRShft);

      void pitchPreFilt(int16_t pswExcite[],
                                      int16_t swRxGsp0,
                                      int16_t swRxLag, int16_t swUvCode,
                                    int16_t swSemiBeta, struct NormSw snsSqrtRs,
                                      int16_t pswExciteOut[],
                                      int16_t pswPPreState[]);

      void spectralPostFilter(int16_t pswSPFIn[],
                                            int16_t pswNumCoef[],
                                  int16_t pswDenomCoef[], int16_t pswSPFOut[]);
      // From dtx.c
      void   avgCNHist(int32_t pL_R0History[],
                              int32_t ppL_CorrHistory[OVERHANG][NP + 1],
                              int32_t *pL_AvgdR0,
                              int32_t pL_AvgdCorrSeq[]);

        void   avgGsHistQntz(int32_t pL_GsHistory[], int32_t *pL_GsAvgd);

        int16_t swComfortNoise(int16_t swVadFlag,
                                  int32_t L_UnqntzdR0, int32_t *pL_UnqntzdCorr);

        int16_t getPnBits(int iBits, int32_t *L_PnSeed);

        int16_t gsQuant(int32_t L_GsIn, int16_t swVoicingMode);

        void   updateCNHist(int32_t L_UnqntzdR0,
                                   int32_t *pL_UnqntzdCorr,
                                   int32_t pL_R0Hist[],
                                   int32_t ppL_CorrHist[OVERHANG][NP + 1]);

        void   lpcCorrQntz(int32_t pL_CorrelSeq[],
                                  int16_t pswFinalRc[],
                                  int piVQCodewds[]);

        int32_t linInterpSid(int32_t L_New, int32_t L_Old, int16_t swDtxState);

        int16_t linInterpSidShort(int16_t swNew,
                                           int16_t swOld,
                                           int16_t swDtxState);

        void   rxInterpR0Lpc(int16_t *pswOldKs, int16_t *pswNewKs,
                                    int16_t swRxDTXState,
                                    int16_t swDecoMode, int16_t swFrameType);

        // From sp_frm.c
        void   iir_d(int16_t pswCoeff[], int16_t pswIn[],
                            int16_t pswXstate[],
                            int16_t pswYstate[],
                            int npts, int shifts,
                            int16_t swPreFirDownSh,
                            int16_t swFinalUpShift);


          void   filt4_2nd(int16_t pswCoeff[],
                                  int16_t pswIn[],
                                  int16_t pswXstate[],
                                  int16_t pswYstate[],
                                  int npts,
                                  int shifts);

          void   initPBarVBarL(int32_t pL_PBarFull[],
                                      int16_t pswPBar[],
                                      int16_t pswVBar[]);

          void   initPBarFullVBarFullL(int32_t pL_CorrelSeq[],
                                              int32_t pL_PBarFull[],
                                              int32_t pL_VBarFull[]);

          int16_t aflatRecursion(int16_t pswQntRc[],
                                          int16_t pswPBar[],
                                          int16_t pswVBar[],
                                          int16_t *ppswPAddrs[],
                                          int16_t *ppswVAddrs[],
                                          int16_t swSegmentOrder);

          void   aflatNewBarRecursionL(int16_t pswQntRc[],
                                              int iSegment,
                                              int32_t pL_PBar[],
                                              int32_t pL_VBar[],
                                              int16_t pswPBar[],
                                              int16_t pswVBar[]);


          void   setupPreQ(int iSeg, int iVector);

          void   setupQuant(int iSeg, int iVector);

          void   getNextVec(int16_t pswRc[]);

          void   aflat(int16_t pswSpeechToLPC[],
                              int piR0Index[],
                              int16_t pswFinalRc[],
                              int piVQCodewds[],
                              int16_t swPtch,
                              int16_t *pswVadFlag,
                              int16_t *pswSP);


          int16_t fnExp2(int32_t L_Input);

          int16_t fnLog2(int32_t L_Input);

          void   weightSpeechFrame(int16_t pswSpeechFrm[],
                                          int16_t pswWNumSpace[],
                                          int16_t pswWDenomSpace[],
                                          int16_t pswWSpeechBuffBase[]);

          void   getSfrmLpcTx(int16_t swPrevR0, int16_t swNewR0,
                                     int16_t pswPrevFrmKs[],
                                     int16_t pswPrevFrmAs[],
                                     int16_t pswPrevFrmSNWCoef[],
                                     int16_t pswNewFrmKs[],
                                     int16_t pswNewFrmAs[],
                                     int16_t pswNewFrmSNWCoef[],
                                     int16_t pswHPFSpeech[],
                                     short *pswSoftInterp,
                                     struct NormSw *psnsSqrtRs,
                                     int16_t ppswSynthAs[][NP],
                                     int16_t ppswSNWCoefAs[][NP]);

          short int fnBest_CG(int16_t pswCframe[],
                                     int16_t pswGframe[],
                                     int16_t *pswCmaxSqr,
                                     int16_t *pswGmax,
                                     short int siNumPairs);

          short  compResidEnergy(int16_t pswSpeech[],
                                        int16_t ppswInterpCoef[][NP],
                                        int16_t pswPreviousCoef[],
                                        int16_t pswCurrentCoef[],
                                        struct NormSw psnsSqrtRs[]);

          int16_t r0Quant(int32_t L_UnqntzdR0);

          int16_t cov32(int16_t pswIn[],
                                 int32_t pppL_B[NP][NP][2],
                                 int32_t pppL_F[NP][NP][2],
                                 int32_t pppL_C[NP][NP][2],
                                 int32_t *pL_R0,
                                 int32_t pL_VadAcf[],
                                 int16_t *pswVadScalAuto);

          int32_t flat(int16_t pswSpeechIn[],
                               int16_t pswRc[],
                               int *piR0Inx,
                               int32_t pL_VadAcf[],
                               int16_t *pswVadScalAuto);



          void   openLoopLagSearch(int16_t pswWSpeech[],
                                          int16_t swPrevR0Index,
                                          int16_t swCurrR0Index,
                                          int16_t *psiUVCode,
                                          int16_t pswLagList[],
                                          int16_t pswNumLagList[],
                                          int16_t pswPitchBuf[],
                                          int16_t pswHNWCoefBuf[],
                                          struct NormSw psnsWSfrmEng[],
                                          int16_t pswVadLags[],
                                          int16_t swSP);

          int16_t getCCThreshold(int16_t swRp0,
                                          int16_t swCC,
                                          int16_t swG);

          void   pitchLags(int16_t swBestIntLag,
                                  int16_t pswIntCs[],
                                  int16_t pswIntGs[],
                                  int16_t swCCThreshold,
                                  int16_t pswLPeaksSorted[],
                                  int16_t pswCPeaksSorted[],
                                  int16_t pswGPeaksSorted[],
                                  int16_t *psiNumSorted,
                                  int16_t *pswPitch,
                                  int16_t *pswHNWCoef);

          short  CGInterpValid(int16_t swFullResLag,
                                      int16_t pswCIn[],
                                      int16_t pswGIn[],
                                      int16_t pswLOut[],
                                      int16_t pswCOut[],
                                      int16_t pswGOut[]);

          void   CGInterp(int16_t pswLIn[],
                                 short siNum,
                                 int16_t pswCIn[],
                                 int16_t pswGIn[],
                                 short siLoIntLag,
                                 int16_t pswCOut[],
                                 int16_t pswGOut[]);

          int16_t quantLag(int16_t swRawLag,
                                    int16_t *psiCode);

          void   findBestInQuantList(struct QuantList psqlInList,
                                            int iNumVectOut,
                                            struct QuantList psqlBestOutList[]);

          int16_t findPeak(int16_t swSingleResLag,
                                    int16_t pswCIn[],
                                    int16_t pswGIn[]);

          void   bestDelta(int16_t pswLagList[],
                                  int16_t pswCSfrm[],
                                  int16_t pswGSfrm[],
                                  short int siNumLags,
                                  short int siSfrmIndex,
                                  int16_t pswLTraj[],
                                  int16_t pswCCTraj[],
                                  int16_t pswGTraj[]);

          int16_t
                 maxCCOverGWithSign(int16_t pswCIn[],
                                           int16_t pswGIn[],
                                           int16_t *pswCCMax,
                                           int16_t *pswGMax,
                                           int16_t swNum);

          void   getNWCoefs(int16_t pswACoefs[],
                                   int16_t pswHCoefs[]);

          int16_t lagDecode(int16_t swDeltaLag);

  };

  // From mathdp31
  extern int32_t L_mpy_ls(int32_t L_var2, int16_t var1);
  extern int32_t L_mpy_ll(int32_t L_var1, int32_t L_var2);
  extern short  isSwLimit(int16_t swIn);
  extern short  isLwLimit(int32_t L_In);

  // From mathhalf
  /* addition */
  /************/

  extern int16_t add(int16_t var1, int16_t var2);  /* 1 ops */
  extern int16_t sub(int16_t var1, int16_t var2);  /* 1 ops */
  extern int32_t L_add(int32_t L_var1, int32_t L_var2);       /* 2 ops */
  extern int32_t L_sub(int32_t L_var1, int32_t L_var2);       /* 2 ops */

  /* multiplication */
  /******************/

  extern int16_t mult(int16_t var1, int16_t var2); /* 1 ops */
  extern int32_t L_mult(int16_t var1, int16_t var2);        /* 1 ops */
  extern int16_t mult_r(int16_t var1, int16_t var2);       /* 2 ops */


  /* arithmetic shifts */
  /*********************/

  extern int16_t shr(int16_t var1, int16_t var2);  /* 1 ops */
  extern int16_t shl(int16_t var1, int16_t var2);  /* 1 ops */
  extern int32_t L_shr(int32_t L_var1, int16_t var2);        /* 2 ops */
  extern int32_t L_shl(int32_t L_var1, int16_t var2);        /* 2 ops */
  extern int16_t shift_r(int16_t var, int16_t var2);       /* 2 ops */
  extern int32_t L_shift_r(int32_t L_var, int16_t var2);     /* 3 ops */

  /* absolute value  */
  /*******************/

  extern int16_t abs_s(int16_t var1);       /* 1 ops */
  extern int32_t L_abs(int32_t var1);         /* 3 ops */


  /* multiply accumulate  */
  /************************/

  extern int32_t L_mac(int32_t L_var3,
                        int16_t var1, int16_t var2);  /* 1 op */
  extern int16_t mac_r(int32_t L_var3,
                         int16_t var1, int16_t var2); /* 2 op */
  extern int32_t L_msu(int32_t L_var3,
                        int16_t var1, int16_t var2);  /* 1 op */
  extern int16_t msu_r(int32_t L_var3,
                         int16_t var1, int16_t var2); /* 2 op */

  /* negation  */
  /*************/

  extern int16_t negate(int16_t var1);      /* 1 ops */
  extern int32_t L_negate(int32_t L_var1);    /* 2 ops */


  /* Accumulator manipulation */
  /****************************/

  extern int32_t L_deposit_l(int16_t var1);  /* 1 ops */
  extern int32_t L_deposit_h(int16_t var1);  /* 1 ops */
  extern int16_t extract_l(int32_t L_var1);  /* 1 ops */
  extern int16_t extract_h(int32_t L_var1);  /* 1 ops */

  /* Round */
  /*********/

  extern int16_t round(int32_t L_var1);      /* 1 ops */

  /* Normalization */
  /*****************/

  extern int16_t norm_l(int32_t L_var1);     /* 30 ops */
  extern int16_t norm_s(int16_t var1);      /* 15 ops */

  /* Division */
  /************/
  extern int16_t divide_s(int16_t var1, int16_t var2);     /* 18 ops */

  /* Non-saturating instructions */
  /*******************************/
  extern int32_t L_add_c(int32_t L_Var1, int32_t L_Var2);     /* 2 ops */
  extern int32_t L_sub_c(int32_t L_Var1, int32_t L_Var2);     /* 2 ops */
  extern int32_t L_sat(int32_t L_var1);       /* 4 ops */
  extern int32_t L_macNs(int32_t L_var3,
                          int16_t var1, int16_t var2);        /* 1 ops */
  extern int32_t L_msuNs(int32_t L_var3,
                          int16_t var1, int16_t var2);        /* 1 ops */


}

#endif
