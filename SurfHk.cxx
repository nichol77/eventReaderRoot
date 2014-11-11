//////////////////////////////////////////////////////////////////////////////
/////  SurfHk.cxx        ANITA ADU5 VTG reading class                   /////
/////                                                                    /////
/////  Description:                                                      /////
/////     A simple class that reads in ADU5 VTG and produces trees       ///// 
/////  Author: Ryan Nichol (rjn@hep.ucl.ac.uk)                           /////
//////////////////////////////////////////////////////////////////////////////

#include "SurfHk.h"
#include "AnitaPacketUtil.h"
#include "AnitaGeomTool.h"
#include "AnitaEventCalibrator.h"
#include <iostream>
#include <fstream>
#include <cstring>






ClassImp(SurfHk);

SurfHk::SurfHk() 
{
   //Default Constructor
}

SurfHk::~SurfHk() {
   //Default Destructor
}


SurfHk::SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStruct_t *surfPtr)
{
  if(surfPtr->gHdr.code!=PACKET_SURF_HK ||
     surfPtr->gHdr.verId!=VER_SURF_HK ||
     surfPtr->gHdr.numBytes!=sizeof(FullSurfHkStruct_t)) {
     std::cerr << "Mismatched packet\t" << packetCodeAsString(PACKET_SURF_HK)
	 
	       << "\ncode:\t" << (int)surfPtr->gHdr.code << "\t" << PACKET_SURF_HK 
	       << "\nversion:\t" << (int)surfPtr->gHdr.verId 
	       << "\t" << VER_SURF_HK 
	       << "\nsize:\t" << surfPtr->gHdr.numBytes << "\t"
	       << sizeof(FullSurfHkStruct_t) << std::endl;
  }
     

  run=trun;
  realTime=trealTime;
  payloadTime=surfPtr->unixTime;
  payloadTimeUs=surfPtr->unixTimeUs;
  globalThreshold=surfPtr->globalThreshold;
  errorFlag=surfPtr->errorFlag;
  memcpy(scalerGoals,surfPtr->scalerGoals,sizeof(UShort_t)*NUM_ANTENNA_RINGS);
  memcpy(upperWords,surfPtr->upperWords,sizeof(UShort_t)*ACTIVE_SURFS);
  
  for(int surf=0;surf<ACTIVE_SURFS;surf++) {
    for(int l1=0;l1<L1S_PER_SURF;l1++) {
      l1Scaler[surf][l1]=surfPtr->l1Scaler[surf][l1];
    }
    for(int i=0;i<12;i++) {
      scaler[surf][i]=surfPtr->scaler[surf][i];
      threshold[surf][i]=surfPtr->threshold[surf][i];
      setThreshold[surf][i]=surfPtr->setThreshold[surf][i];
    }
  }
  memcpy(rfPower,surfPtr->rfPower,sizeof(UShort_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
  memcpy(surfTrigBandMask,surfPtr->surfTrigBandMask,sizeof(UShort_t)*ACTIVE_SURFS);
  intFlag=0;
}



SurfHk::SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer30_t *surfPtr)
{
  if(surfPtr->gHdr.code!=PACKET_SURF_HK ||
     surfPtr->gHdr.verId!=VER_SURF_HK ||
     surfPtr->gHdr.numBytes!=sizeof(FullSurfHkStruct_t)) {
     std::cerr << "Mismatched packet\t" << packetCodeAsString(PACKET_SURF_HK)
	 
	       << "\ncode:\t" << (int)surfPtr->gHdr.code << "\t" << PACKET_SURF_HK 
	       << "\nversion:\t" << (int)surfPtr->gHdr.verId 
	       << "\t" << VER_SURF_HK 
	       << "\nsize:\t" << surfPtr->gHdr.numBytes << "\t"
	       << sizeof(FullSurfHkStruct_t) << std::endl;
  }
     

  run=trun;
  realTime=trealTime;
  payloadTime=surfPtr->unixTime;
  payloadTimeUs=surfPtr->unixTimeUs;
  globalThreshold=surfPtr->globalThreshold;
  errorFlag=surfPtr->errorFlag;
  memcpy(scalerGoals,surfPtr->scalerGoals,sizeof(UShort_t)*BANDS_PER_ANT);
  memcpy(scalerGoalsNadir,surfPtr->scalerGoalsNadir,sizeof(UShort_t)*BANDS_PER_ANT);
  memcpy(upperWords,surfPtr->upperWords,sizeof(UShort_t)*ACTIVE_SURFS);
  memcpy(scaler,surfPtr->scaler,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF_V30);
  memcpy(threshold,surfPtr->threshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF_V30);
  memcpy(setThreshold,surfPtr->setThreshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF_V30);
  memcpy(rfPower,surfPtr->rfPower,sizeof(UShort_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
  memcpy(surfTrigBandMask,surfPtr->surfTrigBandMask,sizeof(UShort_t)*ACTIVE_SURFS);
  intFlag=0;
}


SurfHk::SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer14_t *surfPtr)
{
  if(surfPtr->gHdr.code!=PACKET_SURF_HK ||
     surfPtr->gHdr.verId!=14 ||
     surfPtr->gHdr.numBytes!=sizeof(FullSurfHkStructVer14_t)) {
     std::cerr << "Mismatched packet\t" << packetCodeAsString(PACKET_SURF_HK)
	 
	       << "\ncode:\t" << (int)surfPtr->gHdr.code << "\t" << PACKET_SURF_HK 
	       << "\nversion:\t" << (int)surfPtr->gHdr.verId 
	       << "\t" << 14
	       << "\nsize:\t" << surfPtr->gHdr.numBytes << "\t"
	       << sizeof(FullSurfHkStructVer14_t) << std::endl;
  }
     

  run=trun;
  realTime=trealTime;
  payloadTime=surfPtr->unixTime;
  payloadTimeUs=surfPtr->unixTimeUs;
  globalThreshold=surfPtr->globalThreshold;
  errorFlag=surfPtr->errorFlag;
  memcpy(scalerGoals,surfPtr->scalerGoals,sizeof(UShort_t)*BANDS_PER_ANT);
  memcpy(scalerGoalsNadir,surfPtr->scalerGoalsNadir,sizeof(UShort_t)*BANDS_PER_ANT);
  memcpy(upperWords,surfPtr->upperWords,sizeof(UShort_t)*ACTIVE_SURFS);
  memcpy(scaler,surfPtr->scaler,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(threshold,surfPtr->threshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(setThreshold,surfPtr->setThreshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(rfPower,surfPtr->rfPower,sizeof(UShort_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
  memcpy(surfTrigBandMask,surfPtr->surfTrigBandMask,sizeof(UShort_t)*ACTIVE_SURFS);
  intFlag=0;
}

SurfHk::SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer13_t *surfPtr)
{
  if(surfPtr->gHdr.code!=PACKET_SURF_HK ||
     surfPtr->gHdr.verId!=13 ||
     surfPtr->gHdr.numBytes!=sizeof(FullSurfHkStructVer13_t)) {
     std::cerr << "Mismatched packet\t" << packetCodeAsString(PACKET_SURF_HK)
	 
	       << "\ncode:\t" << (int)surfPtr->gHdr.code << "\t" << PACKET_SURF_HK 
	       << "\nversion:\t" << (int)surfPtr->gHdr.verId 
	       << "\t" << 13
	       << "\nsize:\t" << surfPtr->gHdr.numBytes << "\t"
	       << sizeof(FullSurfHkStructVer13_t) << std::endl;
  }
     

  run=trun;
  realTime=trealTime;
  payloadTime=surfPtr->unixTime;
  payloadTimeUs=surfPtr->unixTimeUs;
  globalThreshold=surfPtr->globalThreshold;
  errorFlag=surfPtr->errorFlag;
  memcpy(scalerGoals,surfPtr->scalerGoals,sizeof(UShort_t)*BANDS_PER_ANT);
  for(int band=0;band<BANDS_PER_ANT;band++)
     scalerGoalsNadir[band]=scalerGoals[band];
  memcpy(upperWords,surfPtr->upperWords,sizeof(UShort_t)*ACTIVE_SURFS);
  memcpy(scaler,surfPtr->scaler,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(threshold,surfPtr->threshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(setThreshold,surfPtr->setThreshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(rfPower,surfPtr->rfPower,sizeof(UShort_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
  memcpy(surfTrigBandMask,surfPtr->surfTrigBandMask,sizeof(UShort_t)*ACTIVE_SURFS);
  intFlag=0;
}

SurfHk::SurfHk(Int_t trun, Int_t trealTime, FullSurfHkStructVer12_t *surfPtr)
{
  if(surfPtr->gHdr.code!=PACKET_SURF_HK ||
     surfPtr->gHdr.verId!=12 ||
     surfPtr->gHdr.numBytes!=sizeof(FullSurfHkStructVer12_t)) {
     std::cerr << "Mismatched packet\t" << packetCodeAsString(PACKET_SURF_HK)
	 
	       << "\ncode:\t" << (int)surfPtr->gHdr.code << "\t" << PACKET_SURF_HK 
	       << "\nversion:\t" << (int)surfPtr->gHdr.verId 
	       << "\t" << 12 
	       << "\nsize:\t" << surfPtr->gHdr.numBytes << "\t"
	       << sizeof(FullSurfHkStructVer12_t) << std::endl;
  }
     

  run=trun;
  realTime=trealTime;
  payloadTime=surfPtr->unixTime;
  payloadTimeUs=surfPtr->unixTimeUs;
  globalThreshold=surfPtr->globalThreshold;
  errorFlag=surfPtr->errorFlag;
  for(int band=0;band<BANDS_PER_ANT;band++) {
     scalerGoalsNadir[band]=surfPtr->scalerGoal;
     scalerGoals[band]=surfPtr->scalerGoal;
  }
  memcpy(upperWords,surfPtr->upperWords,sizeof(UShort_t)*ACTIVE_SURFS);
  memcpy(scaler,surfPtr->scaler,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(threshold,surfPtr->threshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(setThreshold,surfPtr->setThreshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
  memcpy(rfPower,surfPtr->rfPower,sizeof(UShort_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
  memcpy(surfTrigBandMask,surfPtr->surfTrigBandMask,sizeof(UShort_t)*ACTIVE_SURFS);
  intFlag=0;
}

SurfHk::SurfHk(Int_t           trun,
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
	       Int_t           tintFlag)
{

   run=trun;
   realTime=trealTime;
   payloadTime=tpayloadTime;
   payloadTimeUs=tpayloadTimeUs;
   globalThreshold=tglobalThreshold;
   errorFlag=terrorFlag;
   memcpy(scalerGoals,tscalerGoals,sizeof(UShort_t)*BANDS_PER_ANT);
   memcpy(scalerGoalsNadir,tscalerGoalsNadir,sizeof(UShort_t)*BANDS_PER_ANT);
   memcpy(upperWords,tupperWords,sizeof(UShort_t)*ACTIVE_SURFS);
   memcpy(scaler,tscaler,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
   memcpy(threshold,tthreshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
   memcpy(setThreshold,tsetThreshold,sizeof(UShort_t)*ACTIVE_SURFS*SCALERS_PER_SURF);
   memcpy(rfPower,trfPower,sizeof(UShort_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
   memcpy(surfTrigBandMask,tsurfTrigBandMask,sizeof(UShort_t)*ACTIVE_SURFS);
   intFlag=tintFlag;

}

Int_t SurfHk::getScaler(int phi, AnitaRing::AnitaRing_t ring, AnitaPol::AnitaPol_t pol)
{
   Int_t surf,scl;
   AnitaGeomTool::getSurfChanTriggerFromPhiRingPol(phi,ring,pol,surf,scl);
   if((surf>=0 && surf<ACTIVE_SURFS) && (scl>=0 && scl<SCALERS_PER_SURF))
     return scaler[surf][scl];
   return -1;
}

Int_t SurfHk::getThreshold(int phi, AnitaRing::AnitaRing_t ring, AnitaPol::AnitaPol_t pol)
{
  Int_t surf,scl;
  AnitaGeomTool::getSurfChanTriggerFromPhiRingPol(phi,ring,pol,surf,scl);
  if((surf>=0 && surf<ACTIVE_SURFS) && (scl>=0 && scl<SCALERS_PER_SURF))
    return threshold[surf][scl];
  return -1;

}

Int_t SurfHk::getSetThreshold(int phi, AnitaRing::AnitaRing_t ring, AnitaPol::AnitaPol_t pol)
{
  Int_t surf,scl;
  AnitaGeomTool::getSurfChanTriggerFromPhiRingPol(phi,ring,pol,surf,scl);
  if((surf>=0 && surf<ACTIVE_SURFS) && (scl>=0 && scl<SCALERS_PER_SURF))
    return setThreshold[surf][scl];
  return -1;
}


Int_t SurfHk::isMasked(int phi, AnitaRing::AnitaRing_t ring, AnitaPol::AnitaPol_t pol)
{
  Int_t surf,scl;
  AnitaGeomTool::getSurfChanTriggerFromPhiRingPol(phi,ring,pol,surf,scl);
  if((surf>=0 && surf<ACTIVE_SURFS) && (scl>=0 && scl<SCALERS_PER_SURF))
    return isBandMasked(surf,scl);
  return -1;
}

Int_t SurfHk::getLogicalIndex(int phi, AnitaRing::AnitaRing_t ring, AnitaPol::AnitaPol_t pol)
{
   Int_t surf,scl;
   AnitaGeomTool::getSurfChanTriggerFromPhiRingPol(phi,ring,pol,surf,scl);
   return scl +surf*SCALERS_PER_SURF;
}

Int_t SurfHk::getScalerGoalRing(AnitaRing::AnitaRing_t ring)
{
  return scalerGoals[ring];
}

Int_t SurfHk::getScalerGoal(int surf, int scl)
{
  Int_t phi=-1;
  AnitaRing::AnitaRing_t ring=AnitaRing::kTopRing;
  AnitaPol::AnitaPol_t pol=AnitaPol::kVertical;
  
  AnitaGeomTool::getPhiRingPolFromSurfChanTrigger(surf,scl,phi,ring,pol);  
  return getScalerGoalRing(ring);
}

Double_t SurfHk::getRFPowerInK(int surf, int chan)
{
  if(surf<0 || surf>=ACTIVE_SURFS)
    return -1;
  if(chan<0 || chan>=RFCHAN_PER_SURF)
    return -1;
  Int_t adc=rfPower[surf][chan];
  Double_t kelvin=AnitaEventCalibrator::Instance()->convertRfPowToKelvin(surf,chan,adc);
  return kelvin;
}


Double_t SurfHk::getMeasuredRFPowerInK(int surf, int chan)
{
  if(surf<0 || surf>=ACTIVE_SURFS)
    return -1;
  if(chan<0 || chan>=RFCHAN_PER_SURF)
    return -1;
  Int_t adc=rfPower[surf][chan];
  Double_t kelvin=AnitaEventCalibrator::Instance()->convertRfPowToKelvinMeasured(surf,chan,adc);
  return kelvin;
}

Double_t SurfHk::getRawRFPower(int surf, int chan)
{
  if(surf<0 || surf>=ACTIVE_SURFS)
    return -1;
  if(chan<0 || chan>=RFCHAN_PER_SURF)
    return -1;
  Int_t adc=rfPower[surf][chan]&0x7FFF; //mask off top bit
  return adc;
}
  
