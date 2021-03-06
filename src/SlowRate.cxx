//////////////////////////////////////////////////////////////////////////////
/////  SlowRate.h        Slow Rate data structure                        /////
/////                                                                    /////
/////  Description:                                                      /////
/////     A simple class for storing slow rate data                      /////
/////  Author: Ryan Nichol (rjn@hep.ucl.ac.uk)                           /////
//////////////////////////////////////////////////////////////////////////////


#include "TMath.h"
#include "SlowRate.h"
#include "AnitaPacketUtil.h"
#include "AnitaEventCalibrator.h"
#include <iostream>
#include <fstream>
#include <cstring>

ClassImp(SlowRate);

SlowRate::SlowRate() 
{
   //Default Constructor
}

SlowRate::~SlowRate() {
   //Default Destructor
}

SlowRate::SlowRate(Int_t trun, UInt_t trealTime, SlowRateFull_t *slowPtr)
{
  simplePacketCheck(&(slowPtr->gHdr),PACKET_SLOW_FULL);

   run=trun;
   realTime=trealTime;
   payloadTime=slowPtr->unixTime;
   eventNumber=slowPtr->rf.eventNumber;

   eventRate1Min=slowPtr->rf.eventRate1Min;
   eventRate10Min=slowPtr->rf.eventRate10Min;

   latitude=slowPtr->hk.latitude;
   longitude=slowPtr->hk.longitude;
   altitude=slowPtr->hk.altitude;

   memcpy(rfPwrAvg,&(slowPtr->rf.rfPwrAvg[0][0]),sizeof(UChar_t)*ACTIVE_SURFS*RFCHAN_PER_SURF);
   memcpy(avgScalerRates,&(slowPtr->rf.avgScalerRates[0][0]),sizeof(UChar_t)*TRIGGER_SURFS*SCALERS_PER_SURF);

   memcpy(temps,&(slowPtr->hk.temps[0]),sizeof(UChar_t)*4);
   memcpy(powers,&(slowPtr->hk.powers[0]),sizeof(UChar_t)*4);
}

Double_t SlowRate::getRFPowerInK(int surf, int chan)
{
  if(surf<0 || surf>=ACTIVE_SURFS)
    return -1;
  if(chan<0 || chan>=RFCHAN_PER_SURF)
    return -1;
  Int_t adc=rfPwrAvg[surf][chan];
  adc*=128;
  Double_t kelvin=AnitaEventCalibrator::Instance()->convertRfPowToKelvin(surf,chan,adc);
  if(TMath::IsNaN(kelvin)) return 290;
  if(kelvin<0) return 290;
  if(kelvin>1e6) return 290;
  return kelvin;
}


// Double_t SlowRate::getMeasuredRFPowerInK(int surf, int chan)
// {
//   if(surf<0 || surf>=ACTIVE_SURFS)
//     return -1;
//   if(chan<0 || chan>=RFCHAN_PER_SURF)
//     return -1;
//   Int_t adc=rfPwrAvg[surf][chan];
//   adc*=4;
//   Double_t kelvin=AnitaEventCalibrator::Instance()->convertRfPowToKelvinMeasured(surf,chan,adc);
//   return kelvin;
// }

Double_t SlowRate::getRawRFPower(int surf,int chan) {
  return rfPwrAvg[surf][chan]*128; 
}

Int_t SlowRate::getAvgScaler(int surf, int ant)
{
  if(surf<0 || surf>=ACTIVE_SURFS)
    return -1;
  if(ant<0 || ant>=SCALERS_PER_SURF) 
    return -1; 
  return Int_t(avgScalerRates[surf][ant])*4;

}

Float_t SlowRate::getAltitude()
{
  Float_t alt=altitude;
  if(altitude<-5000) {
    alt=65536+alt;
  }
  return alt;
}

Float_t SlowRate::getPower(int powerInd)
{
  float powerCal[4]={19.252,10.1377,20,20};
  if(powerInd<0 || powerInd>3)
    return 0;

  float adc=powers[powerInd];
  float reading=((10*adc)/255.)-5;
  float value=reading*powerCal[powerInd];
  return value;
}

Float_t SlowRate::getTemp(int tempInd)
{
  if(tempInd<0 || tempInd>3)
    return -273.15;
  if(tempInd==0) return 4*temps[tempInd];
  float adc=temps[tempInd];
  float reading=((10*adc)/255.)-5;
  float value=(reading*100)-273.15;
  return value;
}

const char *SlowRate::getPowerName(int powerInd)
{
  const char *slowPowerNames[4]={"PV V","24 V","PV I","24 I"};
  if(powerInd<0 || powerInd>3)
    return NULL;
  return slowPowerNames[powerInd];

}

const char *SlowRate::getTempName(int tempInd) 
{
  const char *slowTempNames[4]={"SBS","SURF","SHORT","Rad. Plate"};
  if(tempInd<0 || tempInd>3)
    return NULL;
  return slowTempNames[tempInd];
}
