//////////////////////////////////////////////////////////////////////////////
/////  SurfHk.h        SURF hk class                                     /////
/////                                                                    /////
/////  Description:                                                      /////
/////     A simple class for storing SURF Hk objects in a TTree          /////
/////  Author: Ryan Nichol (rjn@hep.ucl.ac.uk)                           /////
//////////////////////////////////////////////////////////////////////////////

#ifndef SURFHK_H
#define SURFHK_H

//Includes
#include <TObject.h>
#include "AnitaConventions.h"
#include "simpleStructs.h"
#include "oldStructs.h"


//!  SurfHk -- The raw SURF scaler+threshold data
/*!
  The ROOT implementation of the SURF scaler, threshold and RF power data
  \ingroup rootclasses
*/
class SurfHk: public TObject
{
 public:
   SurfHk(); ///< Default constructor
   ~SurfHk(); ///< Destructor

   SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStruct_t *surfPtr); ///< Assignment constructor
   SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer40_t *surfPtr); ///< Version 40 constructor
   SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer30_t *surfPtr); ///< Version 30 constructor
   SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer14_t *surfPtr); ///< Version 14 constructor
   SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer13_t *surfPtr); ///< Version 13 constructor 
   SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer12_t *surfPtr); ///< Version 12 constructor

   SurfHk(Int_t           trun,
	 UInt_t          trealTime,
	 UInt_t          tpayloadTime,
	 UInt_t          tpayloadTimeUs,
	 UShort_t        tglobalThreshold,
	 UShort_t        terrorFlag,
	 UShort_t        tscalerGoals[BANDS_PER_ANT],
	 UShort_t        tscalerGoalsNadir[BANDS_PER_ANT],
	 UShort_t        tupperWords[ACTIVE_SURFS],
	 UShort_t        tscaler[ACTIVE_SURFS][SCALERS_PER_SURF],
	 UShort_t        tthreshold[ACTIVE_SURFS][SCALERS_PER_SURF],
	 UShort_t        tsetThreshold[ACTIVE_SURFS][SCALERS_PER_SURF],
	 UShort_t        trfPower[ACTIVE_SURFS][RFCHAN_PER_SURF],
	 UShort_t        tsurfTrigBandMask[ACTIVE_SURFS],
	 Int_t           tintFlag); ///< Old assignment constructor

  Int_t           run; ///< Run number from offline
  UInt_t          realTime; ///< Time in unixTime
  UInt_t          payloadTime; ///< Time in unixTime
  UInt_t          payloadTimeUs; ///< Subsecond time of readout in us
  UShort_t        globalThreshold; ///< Global threshold if set
  UShort_t        errorFlag; ///< Error flag
  UShort_t        scalerGoals[4]; ///< Scaler goal for each ring, only use 3 
  UShort_t        scalerGoalsNadir[BANDS_PER_ANT]; ///< Deprecated
  UShort_t        upperWords[ACTIVE_SURFS]; ///< Upper words of each SURF... for debugging
  UShort_t        scaler[ACTIVE_SURFS][SCALERS_PER_SURF_V30]; ///< Scaler values, multiple by 1000 to get Hz.
  UShort_t        l1Scaler[ACTIVE_SURFS][L1S_PER_SURF]; ///< L1 Scaler values.
  UShort_t        l2Scaler[ACTIVE_SURFS][L2S_PER_SURF]; ///< L2 Scaler values
  UShort_t        threshold[ACTIVE_SURFS][SCALERS_PER_SURF_V30]; ///< Threshold values in DAC counts
  UShort_t        setThreshold[ACTIVE_SURFS][SCALERS_PER_SURF_V30]; ///< Threshold values intedned, should match threshold array exactly
  UShort_t        rfPower[ACTIVE_SURFS][RFCHAN_PER_SURF]; ///< RF power per input channel in ADC counts
  UShort_t        surfTrigBandMask[ACTIVE_SURFS]; ///< Which bands are masked off?
  Int_t           intFlag; ///< Interpolation flag, should be zero for raw data
   
   Int_t isBandMasked(int surf, int scl)
      { return (surfTrigBandMask[surf]&(1<<scl));} ///< Returns 1 if band is masked


   Int_t getL2Scaler(int phi); ///<Returns the L2 scaler value for given phi-pol
   Int_t getL1Scaler(int phi, AnitaPol::AnitaPol_t pol,  AnitaRing::AnitaRing_t ring); ///<Returns the L1 scaler value for given phi-ring-pol 
   Int_t getL1Scaler(int phi, AnitaRing::AnitaRing_t ring) { return getL1Scaler(phi,ring) ; } ///<Returns the L1 scaler value for given phi-ring-pol 

   Int_t getScalerFromSurfChan(int surf, int chan) {return scaler[surf][chan];}
   Int_t getThresholdFromSurfChan(int surf, int chan) {return threshold[surf][chan];}
   Int_t getSetThresholdFromSurfChan(int surf, int chan) {return setThreshold[surf][chan];}
   Int_t getL2ScalerFromSurfChan(int surf, int chan) { return l2Scaler[surf][chan]; }
   Int_t getL1ScalerFromSurfChan(int surf, int chan) { return l1Scaler[surf][chan]; }
   
   
   Int_t getScaler(int phi, AnitaRing::AnitaRing_t ring, AnitaTrigPol::AnitaTrigPol_t pol); ///< Returns scaler value for given ring-phi-pol
   Int_t getThreshold(int phi, AnitaRing::AnitaRing_t ring, AnitaTrigPol::AnitaTrigPol_t pol) ;///< Returns readback threhsold value for given ring-phi-pol
   Int_t getSetThreshold(int phi, AnitaRing::AnitaRing_t ring, AnitaTrigPol::AnitaTrigPol_t pol); ///< Returns set threshold value for given ring-phi-pol
   Int_t isMasked(int phi, AnitaRing::AnitaRing_t ring, AnitaTrigPol::AnitaTrigPol_t pol); ///< Returns 1 if given phi-ring-pol is masked
   Int_t getLogicalIndex(int phi, AnitaRing::AnitaRing_t ring, AnitaTrigPol::AnitaTrigPol_t pol); ///< Returns logical pol index

   Int_t getScalerGoalRing(AnitaRing::AnitaRing_t ring);
   Int_t getScalerGoal(int surf, int scl); ///< Returns scaler goal of surf-scaler
   Double_t getRFPowerInK(int surf, int chan); ///< Returns the pseudo-calibrated RF power in K.
   //   Double_t getMeasuredRFPowerInK(int surf, int chan); ///< Returns the pseudo-calibrated RF power in K.
   Double_t getRawRFPower(int surf, int chan); ///< Returns the measured adc value of the RF power while masking the top bit (the "phase" bit)
   UInt_t getRFPowerTimeOffset(int surf, int chan); //returns the us time-since-measured of the power monitor
   UInt_t getRFPowerPayloadTime(UInt_t surf, UInt_t chan); //returns the corrected payloadTime of the measured power monitor sample
   UInt_t getRFPowerPayloadTimeUs(UInt_t surf, UInt_t chan); //returns the corrected payloadTimeUs of the measured power monitor sample

   ClassDef(SurfHk,VER_SURF_HK);
};


#endif //SURFHK_H
