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

class SurfHk: public TObject
{
 public:
  SurfHk();		 
  ~SurfHk();



  SurfHk(Int_t           trun,
	 UInt_t          trealTime,
	 UInt_t          tpayloadTime,
	 UInt_t          tpayloadTimeUs,
	 UShort_t        tglobalThreshold,
	 UShort_t        terrorFlag,
	 UShort_t        tscalerGoal,
	 UShort_t        tupperWords[ACTIVE_SURFS],
	 UShort_t        tscaler[ACTIVE_SURFS][SCALERS_PER_SURF],
	 UShort_t        tthreshold[ACTIVE_SURFS][SCALERS_PER_SURF],
	 UShort_t        tsetThreshold[ACTIVE_SURFS][SCALERS_PER_SURF],
	 UShort_t        trfPower[ACTIVE_SURFS][RFCHAN_PER_SURF],
	 UShort_t        tsurfTrigBandMask[ACTIVE_SURFS],
	 Int_t           tintFlag);

   Int_t           run;
   UInt_t          realTime;
   UInt_t          payloadTime;
   UInt_t          payloadTimeUs;
   UShort_t        globalThreshold;
   UShort_t        errorFlag;
   UShort_t        scalerGoal;
   UShort_t        upperWords[ACTIVE_SURFS];
   UShort_t        scaler[ACTIVE_SURFS][SCALERS_PER_SURF];
   UShort_t        threshold[ACTIVE_SURFS][SCALERS_PER_SURF];
   UShort_t        setThreshold[ACTIVE_SURFS][SCALERS_PER_SURF];
   UShort_t        rfPower[ACTIVE_SURFS][RFCHAN_PER_SURF];
   UShort_t        surfTrigBandMask[ACTIVE_SURFS];
   Int_t           intFlag;
   
   Int_t isBandMasked(int surf, int scl)
      { return (surfTrigBandMask[surf]&(1<<scl));}

   Int_t getScaler(int phi, AnitaRing::AnitaRing_t ring, AnitaBand::AnitaBand_t band);
   Int_t getThreshold(int phi, AnitaRing::AnitaRing_t ring, AnitaBand::AnitaBand_t band);
   Int_t getSetThreshold(int phi, AnitaRing::AnitaRing_t ring, AnitaBand::AnitaBand_t band);
   Int_t isBandMasked(int phi, AnitaRing::AnitaRing_t ring, AnitaBand::AnitaBand_t band);
   Int_t getLogicalIndex(int phi, AnitaRing::AnitaRing_t ring, AnitaBand::AnitaBand_t band);

  ClassDef(SurfHk,10);
};


#endif //SURFHK_H
