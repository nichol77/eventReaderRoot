//////////////////////////////////////////////////////////////////////////////
/////  AnitaEventCalibrator.cxx        ANITA Event Calibrator            /////
/////                                                                    /////
/////  Description:                                                      /////
/////     A simple class for calibrating UsefulAnitaEvents               /////
/////  Author: Ryan Nichol (rjn@hep.ucl.ac.uk)                           /////
//////////////////////////////////////////////////////////////////////////////
#include <fstream>
#include <iostream>
#include <cstring>

#include <TMath.h>
#include <TGraph.h>

#include "AnitaEventCalibrator.h"
#include "UsefulAnitaEvent.h"

#ifdef USE_FFT_TOOLS
#include "FFTtools.h"
#endif

//Clock Period Hard Coded
Double_t clockPeriod=8;

//Fitting function
Double_t funcSquareWave(Double_t *x, Double_t *par)
{
   Double_t phi=par[0];
   Double_t a=par[1];
   Double_t b=par[2];
   Double_t dtLow=par[3];
   Double_t dtHigh=par[4];
   Double_t sllh=par[5];
   Double_t slhl=par[6];


   Double_t period=dtLow+dtHigh+sllh+slhl;

   Double_t t=x[0]-phi;
   
   Double_t mlh=(a-b)/sllh;
   Double_t mhl=(b-a)/slhl;

   while(t<0) {
      t+=period;
   }
   while(t>period) {
      t-=period;
   }
   if(t<dtLow)
      return b;
   if(t<dtLow+sllh) {
      Double_t t1=t-dtLow;
      return (t1*mlh)+b;
   }
   if(t<dtLow+sllh+dtHigh)
      return a;
   if(t<dtLow+sllh+dtHigh+slhl) {
      Double_t t2=t-(dtLow+sllh+dtHigh);
      return (mhl*t2)+a;
   }
   
      
   return a;

}


Double_t newFuncSquareWave(Double_t *x, Double_t *par)
{
   Double_t phi=par[0];
   Double_t a=par[1];
   Double_t b=par[2];

   Double_t sllh=par[4];
   Double_t slhl=par[4];
   
   Double_t periodLeft=clockPeriod-2*par[4];   
   Double_t dtLow=par[3]*periodLeft;
   Double_t dtHigh=(1-par[3])*periodLeft;


   Double_t t=x[0]-phi;
   
   Double_t mlh=(a-b)/sllh;
   Double_t mhl=(b-a)/slhl;

   while(t<0) {
      t+=clockPeriod;
   }
   while(t>clockPeriod) {
      t-=clockPeriod;
   }
   if(t<dtLow)
      return b;
   if(t<dtLow+sllh) {
      Double_t t1=t-dtLow;
      return (t1*mlh)+b;
   }
   if(t<dtLow+sllh+dtHigh)
      return a;
   if(t<dtLow+sllh+dtHigh+slhl) {
      Double_t t2=t-(dtLow+sllh+dtHigh);
      return (mhl*t2)+a;
   }
   
      
   return a;



}




ClassImp(AnitaEventCalibrator);

AnitaEventCalibrator*  AnitaEventCalibrator::fgInstance = 0;


AnitaEventCalibrator::AnitaEventCalibrator()
   : TObject()
{
   fSquareWave=0;
   fFakeTemp=0;
   fClockUpSampleFactor=16;
   fEpsilonTempScale=1;
   //Default constructor
   std::cout << "AnitaEventCalibrator::AnitaEventCalibrator()" << std::endl;
   loadCalib();
   //   std::cout << "AnitaEventCalibrator::AnitaEventCalibrator() end" << std::endl;
   for(int surf=1;surf<NUM_SURF;surf++) {
     grCorClock[surf-1]=0;
   }
}

AnitaEventCalibrator::~AnitaEventCalibrator()
{
   //Default destructor
}



//______________________________________________________________________________
AnitaEventCalibrator*  AnitaEventCalibrator::Instance()
{
   //static function
  if(fgInstance)
    return fgInstance;

  fgInstance = new AnitaEventCalibrator();
  return fgInstance;
  //   return (fgInstance) ? (AnitaEventCalibrator*) fgInstance : new AnitaEventCalibrator();
}



int AnitaEventCalibrator::calibrateUsefulEvent(UsefulAnitaEvent *eventPtr, WaveCalType::WaveCalType_t calType)
{
 
   if(calType==WaveCalType::kJustTimeNoUnwrap)
      return justBinByBinTimebase(eventPtr);


   fApplyClockFudge=0;
   if(calType==WaveCalType::kVTFullJWPlusFudge || calType==WaveCalType::kVTFullJWPlusFancyClockZero)
      fApplyClockFudge=1;
   //   std::cout << "AnitaEventCalibrator::calibrateUsefulEvent():" << calType << std::endl;
   if(calType==WaveCalType::kVTLabAG || calType==WaveCalType::kVTLabAGFastClock || calType==WaveCalType::kVTLabAGCrossCorClock || calType==WaveCalType::kVTFullAGFastClock || calType==WaveCalType::kVTFullAGCrossCorClock) {
     processEventAG(eventPtr);
   }
   else if(calType==WaveCalType::kVTFullJW || calType==WaveCalType::kVTLabJW ||
      calType==WaveCalType::kVTFullJWPlus || calType==WaveCalType::kVTLabJWPlus ||
      calType==WaveCalType::kVTFullJWPlusClock || calType==WaveCalType::kVTLabJWPlusClock ||
      calType==WaveCalType::kVTLabJWPlusClockZero || calType==WaveCalType::kVTFullJWPlusClockZero ||
      calType==WaveCalType::kVTLabJWPlusFastClockZero || 
      calType==WaveCalType::kVTFullJWPlusFastClockZero ||
      calType==WaveCalType::kVTFullJWPlusFancyClockZero) {
      //Do nothing
      if(eventPtr->gotCalibTemp)
	 processEventJW(eventPtr);
      else
	 processEventJW(eventPtr); //For now using fixed temp
       // until I get the alligned trees set up.
   }
   else {
      processEventRG(eventPtr);
   }


   //Clock Jitter correction
   if(calType==WaveCalType::kVTLabJWPlusClock || calType==WaveCalType::kVTFullJWPlusClock ||
      calType==WaveCalType::kVTLabClockRG || calType==WaveCalType::kVTFullClockRG ||
      calType==WaveCalType::kVTLabJWPlusClockZero || calType==WaveCalType::kVTFullJWPlusClockZero) {
      processClockJitter();
   }

   if(calType==WaveCalType::kVTLabJWPlusFastClockZero || calType==WaveCalType::kVTFullJWPlusFastClockZero || calType==WaveCalType::kVTLabAGFastClock || calType==WaveCalType::kVTFullAGFastClock) {
     processClockJitterFast();
   }

   if(calType==WaveCalType::kVTFullJWPlusFancyClockZero || calType==WaveCalType::kVTLabAGCrossCorClock || calType==WaveCalType::kVTFullAGCrossCorClock) {
      processClockJitterCorrelation();
   }
	 

   //Zero Mean
   if(calType==WaveCalType::kVTLabClockZeroRG || calType==WaveCalType::kVTFullClockZeroRG ||
      calType==WaveCalType::kVTLabJWPlusClockZero || calType==WaveCalType::kVTFullJWPlusClockZero ||
      calType==WaveCalType::kVTLabJWPlusFastClockZero || 
      calType==WaveCalType::kVTFullJWPlusFastClockZero || 
      calType==WaveCalType::kVTFullJWPlusFancyClockZero || 
      calType==WaveCalType::kVTLabAGCrossCorClock ||
      calType==WaveCalType::kVTFullAGCrossCorClock) {

      zeroMean();
   }
    


   

   //   std::cout << "Done processEvent" << std::endl;
   return 0;
}

void AnitaEventCalibrator::processClockJitter() {
   if(!fSquareWave) {
      fSquareWave = new TF1("fSquareWave",newFuncSquareWave,5,90,5);
      fSquareWave->SetParameters(25,1,-1,0.439,0.33);
      fSquareWave->SetParLimits(0,0,35);
      fSquareWave->SetParLimits(1,1,1);
      fSquareWave->SetParLimits(2,-1,-1);
      fSquareWave->SetParLimits(3,0.439,0.439);
      fSquareWave->SetParLimits(4,0.33,0.33);
      
   }

  Double_t fLowArray[NUM_SAMP];
  Double_t fHighArray[NUM_SAMP];
   Double_t phi0=0;
   Double_t times[NUM_SAMP];
   Double_t volts[NUM_SAMP];


   for(int surf=0;surf<NUM_SURF;surf++) {
      //First fill temp arrays
      Int_t numPoints=numPointsArray[surf][8];
      Int_t numHigh=0;
      Int_t numLow=0;
      for(int samp=0;samp<numPoints;samp++) {
	 if(mvArray[surf][8][samp]>0) {
	    fHighArray[numHigh]=mvArray[surf][8][samp];
	    numHigh++;
	 }
	  else {
	    fLowArray[numLow]=mvArray[surf][8][samp];
	    numLow++;
	  }
      }
      Double_t meanHigh=TMath::Mean(numHigh,fHighArray);
      Double_t meanLow=TMath::Mean(numLow,fLowArray);
       Double_t offset=(meanHigh+meanLow)/2;
       Double_t maxVal=meanHigh-offset;
       //       Double_t minVal=meanLow-offset;

       Int_t gotPhiGuess=0;
       Double_t phiGuess=0;
       for(int i=0;i<numPoints;i++) {
	  times[i]=surfTimeArray[surf][i];
	 Double_t tempV=mvArray[surf][8][i]-offset;	
// 	 if(tempV>maxVal*0.6)
// 	   volts[i]=1;
// 	 else if(tempV<minVal*0.6)
// 	   volts[i]=-1;
// 	 else {
	 volts[i]=tempV/maxVal;
	   //	 }
	 
	 if(!gotPhiGuess) {
	    if(tempV>=0 && (mvArray[surf][8][i+1]-offset)<0) {
	       if(i>3) {
		  phiGuess=times[i];
		  gotPhiGuess=1;
	       }
	    }
	 }
	 
       }

           
      TGraph grTemp(numPoints,times,volts);
      fSquareWave->SetParameter(0,phiGuess);
      grTemp.Fit(fSquareWave,"QR","goff");


      if(surf==0) 
	 phi0=fSquareWave->GetParameter(0);
      
      
      double phi=fSquareWave->GetParameter(0);
      if((phi-phi0)>(clockPeriod/2))
	 phi-=clockPeriod;
      if((phi-phi0)<-1*(clockPeriod/2))
	 phi+=clockPeriod;
      
      Double_t clockCor=phi-phi0;
      clockPhiArray[surf]=(phi-phi0)-clockJitterOffset[surf][fLabChip[surf][8]];
      //      std::cout << phi << "\t"  << phi0 << "\t" << clockJitterOffset[surf][fLabChip[surf][8]]
      //		<< std::endl;


      //Now can actually shift times
      // Normal channels are corrected by DeltaPhi - <DeltaPhi>
      // Clock channels are corrected by DeltaPhi (just so the clocks line up)
      for(int chan=0;chan<NUM_CHAN;chan++) {
	 for(int samp=0;samp<numPoints;samp++) {
	    if(chan<8) {
	       timeArray[surf][chan][samp]=surfTimeArray[surf][samp]-clockPhiArray[surf];
	    }
	    else
	       timeArray[surf][chan][samp]=surfTimeArray[surf][samp]-clockCor;
	 }
      }


   }
   
}


void AnitaEventCalibrator::processClockJitterFast() {
 
  Double_t fLowArray[NUM_SAMP];
  Double_t fHighArray[NUM_SAMP];

  Double_t phi0=0;
  Double_t times[NUM_SAMP];
  Double_t volts[NUM_SAMP];
  
  for(int surf=0;surf<NUM_SURF;surf++) {
    //First fill temp arrays
    Int_t numPoints=numPointsArray[surf][8];
    Int_t numHigh=0;
    Int_t numLow=0;
    for(int samp=0;samp<numPoints;samp++) {
      if(mvArray[surf][8][samp]>0) {
	    fHighArray[numHigh]=mvArray[surf][8][samp];
	    numHigh++;
      }
      else {
	fLowArray[numLow]=mvArray[surf][8][samp];
	numLow++;
      }
    }
    Double_t meanHigh=TMath::Mean(numHigh,fHighArray);
    Double_t meanLow=TMath::Mean(numLow,fLowArray);
    Double_t offset=(meanHigh+meanLow)/2;
    Double_t maxVal=meanHigh-offset;
    //       Double_t minVal=meanLow-offset;
    //       cout << maxVal << "\t" << minVal << endl;
    //       std::cout << offset << "\t" << maxVal << "\t" << minVal << std::endl;
    
    for(int i=0;i<numPoints;i++) {
      times[i]=surfTimeArray[surf][i];
      Double_t tempV=mvArray[surf][8][i]-offset;	
      //	 if(tempV>maxVal*0.9)
      //	   volts[i]=1;
      //	 else if(tempV<minVal*0.9)
      //	   volts[i]=-1;
      //	 else {
      volts[i]=tempV/maxVal;
      //	 }
      
    }
    
    
    Double_t phiGuess=0;
    for(int i=0;i<numPoints-1;i++) {
      if(volts[i]>=0 &&
	 volts[i+1]<0) {
	phiGuess=Get_Interpolation_X(times[i],volts[i],times[i+1],volts[i+1],0);
	//	     	     std::cout << surf << "\t" << 8 << "\t" << times[i] << "\t" << times[i+1] 
	//	     		       << "\t" << volts[i] << "\t" << volts[i+1] << "\t" << phiGuess << std::endl;
	if(i>3)
	  break;
	  }
       }
       
       if(surf==0) 
	  phi0=phiGuess;
       
       double phi=phiGuess;
       if((phi-phi0)>(clockPeriod/2))
	 phi-=clockPeriod;
       if((phi-phi0)<-1*(clockPeriod/2))
	 phi+=clockPeriod;
       
       
       Double_t clockCor=phi-phi0;
       
       while((clockCor-fastClockPeakOffset[surf][fLabChip[surf][8]])>(clockPeriod/2)) {
	 clockCor-=clockPeriod;
       }
        while((clockCor-fastClockPeakOffset[surf][fLabChip[surf][8]])<(-1*clockPeriod/2)) {
	 clockCor+=clockPeriod;
       }

       clockPhiArray[surf]=clockCor;


       
       //Now can actually shift times
       // Normal channels are corrected by DeltaPhi - <DeltaPhi>
       // Clock channels are corrected by DeltaPhi (just so the clocks line up)
       //       std::cout << "clockCor:\t" << clockCor << "\t" << clockPhiArray[surf] << std::endl;
       for(int chan=0;chan<NUM_CHAN;chan++) {
	  for(int samp=0;samp<numPoints;samp++) {
	     if(chan<8) {
		timeArray[surf][chan][samp]=surfTimeArray[surf][samp]-clockPhiArray[surf];
	     }
	     else
		timeArray[surf][chan][samp]=surfTimeArray[surf][samp]-clockCor;
	  }
       }
       
       
   }
}



void AnitaEventCalibrator::processClockJitterCorrelation() {
#ifdef USE_FFT_TOOLS
   // This calibration works by first normalising the clock to be between -1 and 1
   // then calculates the cross-correlation between SURF 0 and the other SURFs
   // the peak of the cross-corrleation is taken as the time offset between
   // the channels.

   Double_t fLowArray[NUM_SAMP];
   Double_t fHighArray[NUM_SAMP];
   
   Double_t times[NUM_SURF][NUM_SAMP];
   Double_t volts[NUM_SURF][NUM_SAMP];
   TGraph *grClock[NUM_SURF];
   TGraph *grClockFiltered[NUM_SURF];
   for(int surf=0;surf<NUM_SURF;surf++) {
      //First up we normalise the signals

      //By filling temp arrays
      Int_t numPoints=numPointsArray[surf][8];
      Int_t numHigh=0;
      Int_t numLow=0;
      for(int samp=0;samp<numPoints;samp++) {
	 if(mvArray[surf][8][samp]>0) {
	    fHighArray[numHigh]=mvArray[surf][8][samp];
	    numHigh++;
	 }
	  else {
	    fLowArray[numLow]=mvArray[surf][8][samp];
	    numLow++;
	  }
      }
      //Calculating the min, max and midpoints.
      Double_t meanHigh=TMath::Mean(numHigh,fHighArray);
      Double_t meanLow=TMath::Mean(numLow,fLowArray);
      Double_t offset=(meanHigh+meanLow)/2;
      Double_t maxVal=meanHigh-offset;
      //      Double_t minVal=meanLow-offset;

      //       cout << maxVal << "\t" << minVal << endl;
      //       std::cout << offset << "\t" << maxVal << "\t" << minVal << std::endl;
      
      for(int i=0;i<numPoints;i++) {
	 times[surf][i]=surfTimeArray[surf][i];
	 Double_t tempV=mvArray[surf][8][i]-offset;	
	 // This might be reinstated but I need to test to see if the correlation works better
	 // with or without stamping on the overshoot
// 	 if(tempV>maxVal*0.9)
// 	    volts[surf][i]=1;
// 	 else if(tempV<minVal*0.9)
// 	    volts[surf][i]=-1;
// 	 else {
	 volts[surf][i]=tempV/maxVal;
	    //	 }	 
      }
      grClock[surf] = new TGraph(numPoints,times[surf],volts[surf]);
      TGraph *grTemp = FFTtools::getInterpolatedGraph(grClock[surf],1./2.6);
      grClockFiltered[surf]=FFTtools::simplePassBandFilter(grTemp,0,400);
      delete grTemp;
   }

   // At this point we have filled the normalised voltage arrays and created TGraphs
   // we can now correlate  and extract the offsets
   Double_t deltaT=1./(2.6*fClockUpSampleFactor);
   correlateTenClocks(grClockFiltered,deltaT);


   for(int surf=0;surf<NUM_SURF;surf++) {
      Double_t clockCor=0;
      if(surf>0) {
	TGraph *grCor = grCorClock[surf-1];
	Int_t dtInt=FFTtools::getPeakBin(grCor);
	Double_t peakVal,phiDiff;
	grCor->GetPoint(dtInt,phiDiff,peakVal);
	clockCor=phiDiff;
	
	if(TMath::Abs(clockCor-clockCrossCorr[surf][fLabChip[surf][8]])>clockPeriod/2) {
	  //Need to try again
	  if(clockCor>clockCrossCorr[surf][fLabChip[surf][8]]) {
	     if(dtInt>2*fClockUpSampleFactor) {
	       Int_t dt2ndInt=FFTtools::getPeakBin(grCor,0,dtInt-(2*fClockUpSampleFactor));
	       grCor->GetPoint(dt2ndInt,phiDiff,peakVal);
	       clockCor=phiDiff;		 
	     }
	     else {
	       std::cerr << "What's going on here then??\n";
	     }
	   }
	   else {
	     if(dtInt<(grCor->GetN()-2*fClockUpSampleFactor)) {
	       Int_t dt2ndInt=FFTtools::getPeakBin(grCor,dtInt+(2*fClockUpSampleFactor),grCor->GetN());
	       grCor->GetPoint(dt2ndInt,phiDiff,peakVal);
	       clockCor=phiDiff;
	     }
	     else {
	       std::cerr << "What's going on here then??\n";
	     }
	   }	     	       	   	    	   
	 }

	 //	 delete grCor;
      }
	 
      clockPhiArray[surf]=clockCor-fancyClockJitterOffset[surf][fLabChip[surf][8]];
      
      
      //Now can actually shift times
      // Normal channels are corrected by DeltaPhi - <DeltaPhi>
      // Clock channels are corrected by DeltaPhi (just so the clocks line up)
      //       std::cout << "clockCor:\t" << clockCor << "\t" << clockPhiArray[surf] << std::endl;
      Int_t numPoints=numPointsArray[surf][8];
      for(int chan=0;chan<NUM_CHAN;chan++) {
	 for(int samp=0;samp<numPoints;samp++) {
	    if(chan<8) {
	       timeArray[surf][chan][samp]=surfTimeArray[surf][samp]-clockPhiArray[surf];
	    }
	    else
	       timeArray[surf][chan][samp]=surfTimeArray[surf][samp]-clockCor;
	 }
      }
      
      
   }

   for(int surf=0;surf<NUM_SURF;surf++) {
     if(grClock[surf]) delete grClock[surf];
     if(grClockFiltered[surf]) delete grClockFiltered[surf];
   }
#else 
   printf("FFTTools currently disabled\n");
#endif
}



void AnitaEventCalibrator::zeroMean() {
   //Won't do it for the clock channel due to the assymmetry of the clock signal
   for(int surf=0;surf<NUM_SURF;surf++) {
      for(int chan=0;chan<8;chan++) {
	 Double_t mean=TMath::Mean(numPointsArray[surf][chan],mvArray[surf][chan]);
	 for(int i=0;i<NUM_SAMP;i++) {
	    mvArray[surf][chan][i]-=mean;
	 }
      }
   }

}

void AnitaEventCalibrator::processEventAG(UsefulAnitaEvent *eventPtr)
{  
  //  std::cout << "processEventAG\n";
  if(eventPtr->fC3poNum) {
    clockPeriod=1e9/eventPtr->fC3poNum;
  }
  Double_t tempFactor=eventPtr->getTempCorrectionFactor();
  for(Int_t surf=0;surf<NUM_SURF;surf++) {
    for(Int_t chan=0;chan<NUM_CHAN;chan++) {
      Int_t chanIndex=eventPtr->getChanIndex(surf,chan);
      Int_t labChip=eventPtr->getLabChip(chanIndex);
      fLabChip[surf][chan]=labChip;
      Int_t rco=eventPtr->guessRco(chanIndex);
      

      Int_t earliestSample=eventPtr->getEarliestSample(chanIndex);
      Int_t latestSample=eventPtr->getLatestSample(chanIndex);

      if(earliestSample==0)
	earliestSample++;

      if(latestSample==0)
	latestSample=259;
      
      //      if(surf==7) 
      //	std::cout << chan << "\t" << earliestSample << "\t" << latestSample 
      //		  << "\n";

      //Raw array
      for(Int_t samp=0;samp<NUM_SAMP;samp++) {
	rawArray[surf][chan][samp]=eventPtr->data[chanIndex][samp];
      }
      
      //Now do the unwrapping
      Int_t index=0;
      Double_t time=0;
      if(latestSample<earliestSample) {
	//	std::cout << "Two RCO's\t" << surf << "\t" << chan << "\n";
	//We have two RCOs
	Int_t nextExtra=256;
	Double_t extraTime=0;	
	if(earliestSample<256) {
	  //Lets do the first segment up	
	  for(Int_t samp=earliestSample;samp<256;samp++) {
	    int binRco=1-rco;
	    scaArray[surf][chan][index]=samp;
	    unwrappedArray[surf][chan][index]=rawArray[surf][chan][samp];
	    mvArray[surf][chan][index]=double(rawArray[surf][chan][samp])*2*mvCalibVals[surf][chan][labChip]; //Need to add mv calibration at some point
	    timeArray[surf][chan][index]=time;
	    if(samp==255) {
	      extraTime=time+(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	      //	      extraTime=time+0.5*(justBinByBin[surf][labChip][binRco][samp]+justBinByBin[surf][labChip][binRco][samp+1])*tempFactor;
	    }
	    else {
	      time+=(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	      //	      time+=0.5*(justBinByBin[surf][labChip][binRco][samp]+justBinByBin[surf][labChip][binRco][samp+1])*tempFactor;
	    }
	    index++;
	  }
	  time+=epsilonFromAbby[surf][labChip][rco]*tempFactor*fEpsilonTempScale; ///<This is the time of the first capacitor.
	}
	else {
	  //Will just ignore the first couple of samples.
	  nextExtra=260;
	  extraTime=0;
	}
	
	
	if(latestSample>=1) {
	  //We are going to ignore sample zero for now
	  time+=(justBinByBin[surf][labChip][rco][0])*tempFactor;
	  //	  time+=0.5*(justBinByBin[surf][labChip][rco][0]+justBinByBin[surf][labChip][rco][1])*tempFactor;
	  for(Int_t samp=1;samp<=latestSample;samp++) {
	    int binRco=rco;
	    if(nextExtra<260 && samp==1) {
	      if(extraTime<time-0.22) { ///This is Andres's 220ps minimum sample separation
		//Then insert the next extra capacitor
	      binRco=1-rco;
	      scaArray[surf][chan][index]=nextExtra;
	      unwrappedArray[surf][chan][index]=rawArray[surf][chan][nextExtra];
	      mvArray[surf][chan][index]=double(rawArray[surf][chan][nextExtra])*2*mvCalibVals[surf][chan][labChip]; //Need to add mv calibration at some point
	      timeArray[surf][chan][index]=extraTime;
	      if(nextExtra<259) {
		extraTime+=(justBinByBin[surf][labChip][binRco][nextExtra])*tempFactor;
		//		extraTime+=0.5*(justBinByBin[surf][labChip][binRco][nextExtra]+justBinByBin[surf][labChip][binRco][nextExtra+1])*tempFactor;
	      }
	      nextExtra++;
	      index++;	 
   	      samp--;
	      continue;
	      }
	    }
	    scaArray[surf][chan][index]=samp;
	    unwrappedArray[surf][chan][index]=rawArray[surf][chan][samp];
	    mvArray[surf][chan][index]=double(rawArray[surf][chan][samp])*2*mvCalibVals[surf][chan][labChip]; //Need to add mv calibration at some point
	    timeArray[surf][chan][index]=time;
	    if(samp<259) {
	      time+=(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	      //	      time+=0.5*(justBinByBin[surf][labChip][binRco][samp]+justBinByBin[surf][labChip][binRco][samp+1])*tempFactor;
	    }
	    index++;
	  }
	}
      }
      else {
	//	std::cout << "One RCO\t" << surf << "\t" << chan << "\n";
	//Only one rco
	time=0;
	for(Int_t samp=earliestSample;samp<=latestSample;samp++) {
	  int binRco=rco;
	  scaArray[surf][chan][index]=samp;
	  unwrappedArray[surf][chan][index]=rawArray[surf][chan][samp];
	  mvArray[surf][chan][index]=double(rawArray[surf][chan][samp])*2*mvCalibVals[surf][chan][labChip]; //Need to add mv calibration at some point
	  timeArray[surf][chan][index]=time;
	  if(samp<259) {
	    time+=(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	    //	    time+=0.5*(justBinByBin[surf][labChip][binRco][samp]+justBinByBin[surf][labChip][binRco][samp+1])*tempFactor;
	  }
	  index++;
	}
      }
      numPointsArray[surf][chan]=index;
    }
    //Okay now add Stephen's check to make sure that all the channels on the SURF have the same number of points.
    for(int chan=0;chan<8;chan++) {
      if(numPointsArray[surf][chan]<numPointsArray[surf][8]) {
	numPointsArray[surf][chan]=numPointsArray[surf][8];
      }
    }
    
    //And fill in surfTimeArray if we need it for anything
    for(int samp=0;samp<numPointsArray[surf][8];samp++) {
      surfTimeArray[surf][samp]=timeArray[surf][8][samp];
    }    
  }	  		  	  
}

Int_t AnitaEventCalibrator::justBinByBinTimebase(UsefulAnitaEvent *eventPtr)
{
//    Int_t refEvNum=31853801;
//    if(!fFakeTemp) {
//       fFakeTemp= new TF1("fFakeTemp","[0] + [1]*exp(-x*[2])",0,100000);
//       fFakeTemp->SetParameters(8.07,0.13,1./30000);
//    }
//    Double_t tempFactor=1;
//    if(doFakeTemp) {
//       tempFactor=fFakeTemp->Eval(100000)/fFakeTemp->Eval(eventPtr->eventNumber-refEvNum);
//    }
  Double_t tempFactor=eventPtr->getTempCorrectionFactor();
        

   for(int surf=0;surf<NUM_SURF;surf++) {
      for(int chan=0;chan<NUM_CHAN;chan++) {	 	 
	 int chanIndex=getChanIndex(surf,chan);
	 int labChip=eventPtr->getLabChip(chanIndex);
	 int rco=eventPtr->guessRco(chanIndex); ///< Is this the right thing to do??
	 
	 Int_t earliestSample=eventPtr->getEarliestSample(chanIndex);
	 Int_t latestSample=eventPtr->getLatestSample(chanIndex);
	 

	 double time=0;
	 for(int samp=0;samp<NUM_SAMP;samp++) {
	   int binRco=rco;
	   rawArray[surf][chan][samp]=eventPtr->data[chanIndex][samp];
	   timeArray[surf][chan][samp]=time;
	   if(latestSample<earliestSample) {
	     //We have two rcos
	     if(samp>=earliestSample)
	       binRco=1-rco;
	     else 
	       binRco=rco;
	   }
	   time+=justBinByBin[surf][labChip][binRco][samp]*tempFactor;
	 }
      }
   }  
  return 0;
}


void AnitaEventCalibrator::processEventRG(UsefulAnitaEvent *eventPtr) {
   //   std::cout << "processEventRG" << std::endl;
   //Now we'll actually try and process the data
   for(int surf=0;surf<NUM_SURF;surf++) {
      for(int chan=0;chan<NUM_CHAN;chan++) {	 
	    int goodPoints=0;
	    int chanIndex=getChanIndex(surf,chan);
	    int firstHitbus=eventPtr->getFirstHitBus(chanIndex);
	    int lastHitbus=eventPtr->getLastHitBus(chanIndex);
	    //	    int wrappedHitbus=((eventPtr->chipIdFlag[chanIndex])&0x8)>>3;
	    int wrappedHitbus=eventPtr->getWrappedHitBus(chanIndex);
	       	    
	    int labChip=(eventPtr->chipIdFlag[chanIndex])&0x3;
	    fLabChip[surf][chan]=labChip;
	    int rcoBit=((eventPtr->chipIdFlag[chanIndex])&0x4)>>2;

	    
	    for(int samp=0;samp<NUM_SAMP;samp++) {
	       rawArray[surf][chan][samp]=eventPtr->data[chanIndex][samp];
	    }
	    
	    //Hack for sample zero weirdness
	    //This should go away at some point
	    if(chan==0) {
	       rawArray[surf][chan][0]=rawArray[surf][chan][259];
	    }
    
	    if(!wrappedHitbus) {
		int numHitBus=1+lastHitbus-firstHitbus;
		goodPoints=NUM_EFF_SAMP-numHitBus;
	    }
	    else {
		goodPoints=lastHitbus-(firstHitbus+1);
	    }
	    
	    int firstSamp,lastSamp;
	    if(!wrappedHitbus) {
		firstSamp=lastHitbus+1;
		lastSamp=NUM_SAMP+lastHitbus;
	    }
	    else {
		firstSamp=firstHitbus+1;
		lastSamp=lastHitbus;	    
	    }
	    numPointsArray[surf][chan]=goodPoints;


	    //Timebase calib
	    double timeVal=0;

	    //First we have to work out which phase we are in
	    int startRco=rcoBit;
	    if(!wrappedHitbus) 
		startRco=1-startRco;
	    if(firstSamp<rcoLatchCalib[surf][labChip] && !wrappedHitbus) 
	       startRco=1-startRco;


	    //Now we do the calibration
	    double time255=0;
	    for(int samp=firstSamp;samp<lastSamp;samp++) {
		int currentRco=startRco;
		int index=samp;
		int subtractOffset=0;
		if (index>=NUM_SAMP-1) {
		    index-=NUM_SAMP-1;	
		    subtractOffset=1;
		    currentRco=1-startRco;
		}
		
		unwrappedArray[surf][chan][samp-firstSamp]=rawArray[surf][chan][index];	       
		mvArray[surf][chan][samp-firstSamp]=double(rawArray[surf][chan][index])*mvCalibVals[surf][chan][labChip]*2;

		if(chan==8) {
		   if(index==0) {
		      //I think this works as we have switched rco's
		      if(time255>0) 
			 timeVal=time255+epsilonCalib[surf][labChip][currentRco];
		   }		
		   surfTimeArray[surf][samp-firstSamp]=timeVal;
		   //		if(surf==0 && chan==8)
		   //		   cout << samp-firstSamp << " " << timeVal << " " << timeBaseCalib[surf][labChip][currentRco] << " " << surf << " " << labChip << " " << currentRco << endl;
		   
		   timeVal+=1.0/timeBaseCalib[surf][labChip][currentRco];
		   if(index==255) {
		      time255=timeVal;
		   }

		   //		if(surf==0 && chan==8)
		   //		   cout << samp-firstSamp << " " << timeVal << endl;
		}
	    }
	    
      }      	           
   }
     
}


void AnitaEventCalibrator::processEventJW(UsefulAnitaEvent *eventPtr)
{  
   //   std::cout << "processEventJW" << std::endl;
   //  int word;
  int chanIndex=0;
  int labChip=0;    
  double temp_scale=29.938/(31.7225-0.054*33.046);
  if(eventPtr->gotCalibTemp) {
     temp_scale=eventPtr->getTempCorrectionFactor();
  }


  for (int surf=0; surf<NUM_SURF; surf++){
    for (int chan=0; chan<NUM_CHAN; chan++){ 
      int goodPoints=0;
      chanIndex=getChanIndex(surf,chan);
      int firstHitbus=eventPtr->getFirstHitBus(chanIndex);
      int lastHitbus=eventPtr->getLastHitBus(chanIndex);
//      int wrappedHitbus=((eventPtr->chipIdFlag[chanIndex])&0x8)>>3;
      int wrappedHitbus=eventPtr->getWrappedHitBus(chanIndex);
      //Inset fix to sort out dodgy channel zero problems
      if(chan==0 && firstHitbus==0) {
	 firstHitbus=eventPtr->getFirstHitBus(chanIndex+1);
	 lastHitbus=eventPtr->getLastHitBus(chanIndex+1);
	 wrappedHitbus=eventPtr->getWrappedHitBus(chanIndex+1);
      }

	

      labChip=(eventPtr->chipIdFlag[chanIndex])&0x3;
      fLabChip[surf][chan]=labChip;       	    
      int rcoBit=((eventPtr->chipIdFlag[chanIndex])&0x4)>>2;

      //Ryans dodgy-ness
      double fudgeScale=1;
      if(fApplyClockFudge)
	 fudgeScale=tcalFudgeFactor[surf][labChip][rcoBit];
      //      std::cout << surf << "\t" << labChip << "\t" << rcoBit << "\t" << fudgeScale << "\n";

      for(int samp=0;samp<NUM_SAMP;samp++) {
	 rawArray[surf][chan][samp]=eventPtr->data[chanIndex][samp];
      }

      if(!wrappedHitbus) {
	int numHitBus=1+lastHitbus-firstHitbus;
	goodPoints=NUM_SAMP-numHitBus;
      }
      else {
	goodPoints=lastHitbus-(firstHitbus+1);
      }
      
      //      std::cout << surf << "\t" << chan << "\t" << firstHitbus << "\t" << lastHitbus << "\t" << wrappedHitbus << "\t" << goodPoints << "\n";
      if(firstHitbus==lastHitbus ||goodPoints<100) {
	 std::cout << "Something wrong with HITBUS of event:\t" << eventPtr->eventNumber << "\t" << surf 
		   << "\t" << chan << "\n";
	 //Something wrong add this hack	 
	 for(int tempChan=chan+1;tempChan<chan+2;tempChan++) {
	    Int_t tempChanIndex=getChanIndex(surf,tempChan);
	    firstHitbus=eventPtr->getFirstHitBus(tempChanIndex);
	    lastHitbus=eventPtr->getLastHitBus(tempChanIndex);
	    wrappedHitbus=eventPtr->getWrappedHitBus(tempChanIndex);
	    if(!wrappedHitbus) {
	      int numHitBus=1+lastHitbus-firstHitbus;
	      goodPoints=NUM_SAMP-numHitBus;
	    }
	    else {
	      goodPoints=lastHitbus-(firstHitbus+1);
	    }
	    
	    if(goodPoints) break;	       
	 }
      }


//       if(surf==0 && chan==0) {
// 	 std::cout << std::hex << (int)eventPtr->chipIdFlag[chanIndex] << "\n";
// 	 std::cout << std::dec << firstHitbus << "\t" << lastHitbus << "\t" << wrappedHitbus
// 		   << "\t" << goodPoints << std::endl; 
//       }

      int firstSamp,lastSamp;
      if(!wrappedHitbus) {
	firstSamp=lastHitbus+1;
	//	lastSamp=(NUM_SAMP-1)+lastHitbus;//Ryan's error?
	lastSamp=NUM_SAMP+firstHitbus;//My fix
      }
      else {
	firstSamp=firstHitbus+1;
	lastSamp=lastHitbus;
      }

      int startRco=rcoBit;
      if(!wrappedHitbus) 
	startRco=1-startRco;

      //switch RCO info for RCO delay
      if(firstHitbus<=tcalRcoDelayBin[surf][labChip][startRco] && !wrappedHitbus) startRco=1-startRco;

      int ibin=0;
      for(int samp=firstSamp;samp<lastSamp;samp++) {
	int index=samp;
	int irco=startRco;
	if (index>=NUM_SAMP) { 
	  index-=(NUM_SAMP);
	  irco=1-startRco;
	}
	
	if (index==0) { //temp. fix to skip sca=0 where unexpected voltage apears
	   goodPoints--;
	   continue;
	}
	
	unwrappedArray[surf][chan][ibin]=rawArray[surf][chan][index];
	mvArray[surf][chan][ibin]=double(rawArray[surf][chan][index])*mvCalibVals[surf][chan][labChip]*2;
	scaArray[surf][chan][ibin]=index; 
	rcobit[surf][chan][ibin]=irco;

	if (chan==8){//timing calibraion
	   double dt_bin=tcalTBin[surf][labChip][irco][index]*temp_scale*fudgeScale;	  
	  int index_prev=index-1;
	  if (index_prev==-1) index_prev=259;
//	  double dt_bin_prev=tcalTBin[surf][labChip][irco][index_prev];

	  if (ibin==0) surfTimeArray[surf][ibin]=dt_bin;       
	  else surfTimeArray[surf][ibin]=surfTimeArray[surf][ibin-1]+dt_bin;	

	  if (index==1) {	  
	    double epsilon_eff=tcalEpsilon[surf][labChip][irco];
	    surfTimeArray[surf][ibin]=surfTimeArray[surf][ibin]-epsilon_eff;
	    //	    std::cout << surf << "\t" << chan << "\t" << labChip << "\t" << irco << "\t" << epsilon_eff << "\n";
	    
	    //////////////////////////////////////////////
	    //swapping time and voltage for non-monotonic time.
	    if (ibin>0 && surfTimeArray[surf][ibin-1]>surfTimeArray[surf][ibin]){
	       double tmp_time=surfTimeArray[surf][ibin];
	       surfTimeArray[surf][ibin]=surfTimeArray[surf][ibin-1];
	       surfTimeArray[surf][ibin-1]=tmp_time;
	       for (int chan=0; chan<NUM_CHAN; chan++){ 
		  double tmp_v=mvArray[surf][chan][ibin];		
		  mvArray[surf][chan][ibin]=mvArray[surf][chan][ibin-1];
		  mvArray[surf][chan][ibin-1]=tmp_v;
		  tmp_v=unwrappedArray[surf][chan][ibin];		
		  unwrappedArray[surf][chan][ibin]=unwrappedArray[surf][chan][ibin-1];
		  unwrappedArray[surf][chan][ibin-1]=(int)tmp_v;
	      }	      
	    }
	    //end of time swapping
	    //////////////////////
	    
	    
	  }
	}
	ibin++;	
      }
      
      numPointsArray[surf][chan]=goodPoints;

      
      //2nd correction for RCO phase info. delay, RCO is determined by measuring clock period. 
      //to day CPU time, this method is used only if around the boundary of tcalRcoDelayBin.
      if (chan==8){
	/** Check how many points we say we have.  Don't let it be more than the clock channel -
	    that can cause problems.  SH **/
	for (int chan_sub_index = 0; chan_sub_index < 8; ++chan_sub_index)
	  if (numPointsArray[surf][chan_sub_index] > numPointsArray[surf][8])
	    numPointsArray[surf][chan_sub_index] = numPointsArray[surf][8];


	if (firstHitbus>tcalRcoDelayBin[surf][labChip][startRco] && 
	    firstHitbus<=tcalRcoDelayBin[surf][labChip][startRco]+2 && !wrappedHitbus){
	
	double t_LE[3];
	double t_TE[3];
	int LE_count=0;
	int TE_count=0;
	int ibin=0;
	for (ibin=0;ibin<goodPoints-1;ibin++){
	  double mv1=unwrappedArray[surf][chan][ibin];
	  double mv2=unwrappedArray[surf][chan][ibin+1];
	  if (LE_count<3 && mv1<0 && mv2>=0){
	    double t1=surfTimeArray[surf][ibin];
	    double t2=surfTimeArray[surf][ibin+1];
	    t_LE[LE_count]=Get_Interpolation_X(t1, mv1, t2, mv2, 0);
	    LE_count++;
	  }	    
	  if (TE_count<3 && mv1>0 && mv2<=0){
	    double t1=surfTimeArray[surf][ibin];
	    double t2=surfTimeArray[surf][ibin+1];
	    t_TE[TE_count]=Get_Interpolation_X(t1, mv1, t2, mv2, 0);
	    TE_count++;
	  }	    
	}	  
	
	if (LE_count>2 && TE_count>2){	    

	  double clock_pulse_width_LE=0;
	  if (LE_count==2) clock_pulse_width_LE=t_LE[1]-t_LE[0];
	  if (LE_count==3) clock_pulse_width_LE=(t_LE[2]-t_LE[0])/2.;
	  double clock_pulse_width_TE=0;
	  if (TE_count==2) clock_pulse_width_TE=t_TE[1]-t_TE[0];
	  if (TE_count==3) clock_pulse_width_TE=(t_TE[2]-t_TE[0])/2.;
	  double clock_pulse_width=(clock_pulse_width_LE+clock_pulse_width_TE)/2.;

	  if (clock_pulse_width<29.75 || clock_pulse_width>30.2){
	      for (int ibin=0;ibin<goodPoints;ibin++){
		
		  for (int ichan=0; ichan<NUM_CHAN; ichan++) {
		      rcobit[surf][ichan][ibin]=1-rcobit[surf][ichan][ibin];
		  }
		  //may be one rcobit array per board might be enough, need to modify later./jwnam
		int irco=rcobit[surf][chan][ibin];
		int index=scaArray[surf][chan][ibin];
		double dt_bin=tcalTBin[surf][labChip][irco][index]*temp_scale*fudgeScale;	  
		if (ibin==0) surfTimeArray[surf][ibin]=dt_bin;       
		else surfTimeArray[surf][ibin]=surfTimeArray[surf][ibin-1]+dt_bin;	
		if (index==1) {	  
		  double epsilon_eff=tcalEpsilon[surf][labChip][irco];
		  surfTimeArray[surf][ibin]=surfTimeArray[surf][ibin]-epsilon_eff;
		}
		
	      }
	    }
	  }
	}
      } //if chan==8
    } //chan loop        
    /** Make certain that time is monotonically increasing in this surf! **/
    bool did_swap = false;
    do {
      did_swap = false;
      for(int samp=1;samp<numPointsArray[surf][8];samp++) {
     
   //////////////////////////////////////////////
   //swapping time and voltage for non-monotonic time.
   if (samp>0 && surfTimeArray[surf][samp-1]>surfTimeArray[surf][samp]){
     did_swap = true;
     double tmp_time=surfTimeArray[surf][samp];
     surfTimeArray[surf][samp]=surfTimeArray[surf][samp-1];
     surfTimeArray[surf][samp-1]=tmp_time;
     for (int chan=0; chan<NUM_CHAN; chan++){ 
       if (samp >= numPointsArray[surf][chan]) continue;
       double tmp_v=mvArray[surf][chan][samp];      
       mvArray[surf][chan][samp]=mvArray[surf][chan][samp-1];
       mvArray[surf][chan][samp-1]=tmp_v;
       tmp_v=unwrappedArray[surf][chan][samp];      
       unwrappedArray[surf][chan][samp]=unwrappedArray[surf][chan][samp-1];
       unwrappedArray[surf][chan][samp-1]=(int)tmp_v;
     }         
   }
   //end of time swapping
   //////////////////////
      }
    } while (did_swap);

    
  } //SURF loop
}





void AnitaEventCalibrator::loadCalib() {
  char calibDir[FILENAME_MAX];
  char fileName[FILENAME_MAX];
  char *calibEnv=getenv("ANITA_CALIB_DIR");
  if(!calibEnv) {
     char *utilEnv=getenv("ANITA_UTIL_INSTALL_DIR");
     if(!utilEnv)
	sprintf(calibDir,"calib");
     else
	sprintf(calibDir,"%s/share/anitaCalib",utilEnv);
  }
  else {
    strncpy(calibDir,calibEnv,FILENAME_MAX);
  }

  for(int surf=0;surf<NUM_SURF;surf++) {
     for(int chip=0;chip<NUM_CHIP;chip++) {
	fancyClockJitterOffset[surf][chip]=0;
	clockCrossCorr[surf][chip]=0;
     }
  }
   //Set up some default numbers
    for(int surf=0;surf<NUM_SURF;surf++) {
	for(int chan=0;chan<NUM_CHAN;chan++) {
	    for(int chip=0;chip<NUM_CHIP;chip++) {
		mvCalibVals[surf][chan][chip]=1;
		chipByChipDeltats[surf][chan][chip]=0;
	    }
	}
    }
    
    for(int surf=0;surf<NUM_SURF;surf++) {
       for(int chip=0;chip<NUM_CHIP;chip++) {
	  rcoLatchCalib[surf][chip]=36;
	  for(int rco=0;rco<NUM_RCO;rco++) {
	     timeBaseCalib[surf][chip][rco]=2.6;
	     epsilonCalib[surf][chip][rco]=1.2;
	     epsilonFromAbby[surf][chip][rco]=1.25*(rco+1);
	     for(int samp=0;samp<NUM_SAMP;samp++)
	       justBinByBin[surf][chip][rco][samp]=1./2.6;
	  }
       }
    }
    
   
    int surf,chan,chip,rco,samp;
    int ant;
    char pol;
    double mean,rms,calib;
    int numEnts;
    int icalib;
    //    sprintf(fileName,"%s/rfcmCalibFile.txt",calibDir);
    sprintf(fileName,"%s/mattsFirstGainCalib.dat",calibDir);
    std::ifstream CalibFile(fileName);
    char firstLine[180];
    CalibFile.getline(firstLine,179);
    while(CalibFile >> surf >> chan >> chip >> ant >> pol >> mean >> rms >> calib) {      
      if(pol=='H') 
	calib*=-1;
      mvCalibVals[surf-1][chan-1][chip-1]=calib;
      //	cout << surf << " " << chan << " " << chip << " " << calib << std::endl;
    }
//    cout << surf << " " << chan << " " << chip << " " << calib << std::endl;
//    exit(0);

    sprintf(fileName,"%s/firstPassDeltaT.dat",calibDir);
    std::ifstream DeltaTCalibFile(fileName);
    DeltaTCalibFile.getline(firstLine,179);
    while(DeltaTCalibFile >> surf >> chan >> chip >> calib >> rms >> numEnts) {      
      chipByChipDeltats[surf][chan][chip]=calib;
      //	cout << surf << " " << chan << " " << chip << " " << calib << std::endl;
    }
    
    sprintf(fileName,"%s/timeBaseCalib.dat",calibDir);
    std::ifstream TimeCalibFile(fileName);
    TimeCalibFile.getline(firstLine,179);
    while(TimeCalibFile >> surf >> chip >> rco >> calib) {
	timeBaseCalib[surf][chip][rco]=calib;
	//	std::cout << surf << " " << chip << " " << rco << " " << timeBaseCalib[surf][chip][rco] << std::endl;
    }

    
    sprintf(fileName,"%s/justBinByBin.dat",calibDir);
    std::ifstream BinByBinCalibFile(fileName);
    BinByBinCalibFile.getline(firstLine,179);
    while(BinByBinCalibFile >> surf >> chip >> rco >> samp >> calib) {
	justBinByBin[surf][chip][rco][samp]=calib;
	//	std::cout << surf << " " << chip << " " << rco << " " << samp << " " << justBinByBin[surf][chip][rco][samp] << std::endl;
    }


    sprintf(fileName,"%s/epsilonFromAbby.dat",calibDir);
    std::ifstream EpsilonAbbyFile(fileName);
    EpsilonAbbyFile.getline(firstLine,179);
    while(EpsilonAbbyFile >> surf >> chip >> rco >> calib) {
      epsilonFromAbby[surf][chip][rco]=calib; 
	//	cout << surf << " " << chan << " " << chip << " " << calib << std::endl;
    }

    sprintf(fileName,"%s/rcoLatchCalibWidth.txt",calibDir);
    std::ifstream RcoCalibFile(fileName);
    RcoCalibFile.getline(firstLine,179);
    while(RcoCalibFile >> surf >> chip >> icalib) {
	rcoLatchCalib[surf][chip]=icalib;
//	cout << surf << " " << chip << " " << icalib << std::endl;
    }
    RcoCalibFile.close();
    
    sprintf(fileName,"%s/epsilonCalib.dat",calibDir);
    std::ifstream EpsilonCalibFile(fileName);
    EpsilonCalibFile.getline(firstLine,179);
    while(EpsilonCalibFile >> surf >> chip >> rco >> calib) {
	epsilonCalib[surf][chip][rco]=calib;
//	cout << surf << " " << chan << " " << chip << " " << calib << std::endl;
    }


    sprintf(fileName,"%s/groupDelayCalibFile.dat",calibDir);
    std::ifstream GroupDelayCalibFile(fileName);
    GroupDelayCalibFile.getline(firstLine,179);
    while(GroupDelayCalibFile >> surf >> chan >> calib) {
       groupDelayCalib[surf][chan]=calib;
       //       std::cout << surf <<  " " << chan << " " << calib << std::endl;
    }


    sprintf(fileName,"%s/newSlowClockCalibNums.dat",calibDir);
    std::ifstream ClockCalibFile(fileName);
    ClockCalibFile.getline(firstLine,179);
    while(ClockCalibFile >> surf >> chip >> calib) {
      clockJitterOffset[surf][chip]=calib;
      //      clockJitterOffset[surf][chip]=0;  //RJN hack for test
      //      std::cout << "clockJitterOffset:\t" << surf <<  " " << chip << " " << calib << std::endl;
    }

    sprintf(fileName,"%s/fastClocksPeakPhi.dat",calibDir);
    std::ifstream FastClockCalibFile(fileName);
    FastClockCalibFile.getline(firstLine,179);
    while(FastClockCalibFile >> surf >> chip >> calib) {
      fastClockPeakOffset[surf][chip]=calib;
    }

    sprintf(fileName,"%s/newFancyClockCalibNums.dat",calibDir);
    std::ifstream FancyClockCalibFile(fileName);
    FancyClockCalibFile.getline(firstLine,179);
    while(FancyClockCalibFile >> surf >> chip >> calib) {
      //       fancyClockJitterOffset[surf][chip]=calib;
       fancyClockJitterOffset[surf][chip]=0; //RJN hack for test
       //       std::cout << "fancyClockJitterOffset:\t" << surf <<  " " << chip << " " << calib << std::endl;
    }

    sprintf(fileName,"%s/crossCorrClocksPeakPhi.dat",calibDir);
    std::ifstream CrossCorrClockCalibFile(fileName);
    CrossCorrClockCalibFile.getline(firstLine,179);
    while(CrossCorrClockCalibFile >> surf >> chip >> calib) {
      clockCrossCorr[surf][chip]=calib;
    }

    //Load Jiwoo calibrations
    sprintf(fileName,"%s/jiwoo_timecode/anita_surf_time_constant_epsilon.txt",calibDir);
    std::ifstream JiwooEpsilonCalibFile(fileName);
    while(JiwooEpsilonCalibFile >> surf >> chip >> rco >> calib) {
	int tmpRco=1-rco;
	tcalEpsilon[surf][chip][tmpRco]=calib;
	//	cout << surf << " " << chip << " " << rco << "\t" << calib << endl;
	   
    }
    sprintf(fileName,"%s/jiwoo_timecode/anita_surf_time_constant_differeniial.txt",calibDir);
    std::ifstream JiwooDifferentialCalibFile(fileName);
    while(JiwooDifferentialCalibFile >> surf >> chip >> rco) {
	for(int samp=0;samp<NUM_SAMP;samp++) {
	    JiwooDifferentialCalibFile >> calib;
	    tcalTBin[surf][chip][rco][samp]=calib;
	}
    }
    sprintf(fileName,"%s/jiwoo_timecode/anita_surf_time_constant_rco_delay.txt",calibDir);
    std::ifstream JiwooRcoDelayCalibFile(fileName);
    while(JiwooRcoDelayCalibFile>> surf >> chip >> rco >> calib) {
	tcalRcoDelayBin[surf][chip][rco]=calib;
    }
    
    //Load Ryan's dodgy fudge factors
    sprintf(fileName,"%s/clockFudgeFactors.dat",calibDir);
    std::ifstream RyansFudgeFactorFile(fileName);
    RyansFudgeFactorFile.getline(firstLine,179);
    while(RyansFudgeFactorFile >> surf >> chip >> rco >> calib) {
	tcalFudgeFactor[surf][chip][rco]=calib;
	//	std::cout << surf << " " << chip << " " << rco << "\t" << calib << std::endl;
	   
    }

}


double AnitaEventCalibrator::Get_Interpolation_X(double x1, double y1, double x2, double y2, double y){
  double x=(x2-x1)/(y2-y1)*(y-y1)+x1;
  return x;
}


void AnitaEventCalibrator::correlateTenClocks(TGraph *grClock[NUM_SURF], Double_t deltaT)
{
#ifdef USE_FFT_TOOLS
  for(int surf=1;surf<NUM_SURF;surf++) {
    if(grCorClock[surf-1])
      delete grCorClock[surf-1];
  }

//   TGraph *grDeriv[NUM_SURF]={0};
//   TGraph *grInt[NUM_SURF]={0};
//   Int_t maxLength=0;
//   Int_t lengths[10]={0};
//   for(int surf=0;surf<NUM_SURF;surf++) {
//     //    grDeriv[surf]=FFTtools::getDerviative(grClock[surf]);
//         grInt[surf]=FFTtools::getInterpolatedGraph(grClock[surf],deltaT);
//     //    grInt[surf]=FFTtools::getInterpolatedGraph(grDeriv[surf],deltaT);
//     lengths[surf]=grInt[surf]->GetN();
//     if(lengths[surf]>maxLength)
//       maxLength=lengths[surf];

//     //    delete grDeriv[surf];
//   }
  
//   Int_t paddedLength=int(TMath::Power(2,int(TMath::Log2(maxLength))+2));
//   FFTWComplex *theFFT[NUM_SURF]={0};
//   Double_t *oldY[NUM_SURF] = {0};
//   Double_t *corVals[NUM_SURF] = {0};

//   Double_t *outputX[NUM_SURF] = {0};
//   Double_t *outputY[NUM_SURF] = {0};

//   Double_t firstX,firstY;
//   Double_t secondX,secondY;
//   grInt[0]->GetPoint(0,firstX,firstY);
//   grInt[0]->GetPoint(1,secondX,secondY);
//   Double_t actualDt=secondX-firstX;

//   int tempLength=(paddedLength/2)+1;
//   int firstRealSamp=(paddedLength-lengths[0])/2;
//   for(int surf=0;surf<NUM_SURF;surf++) {
//     Double_t thisX,thisY;
//     grInt[surf]->GetPoint(0,thisX,thisY);
//     Double_t waveOffset=firstX-thisX;

//     oldY[surf]= new Double_t [paddedLength];

//     for(int i=0;i<paddedLength;i++) {
//       if(i<firstRealSamp || i>=firstRealSamp+lengths[surf])
// 	thisY=0;
//        else {
// 	  grInt[surf]->GetPoint(i-firstRealSamp,thisX,thisY);
//        }
//        oldY[surf][i]=thisY;
//     }
//     theFFT[surf]=FFTtools::doFFT(paddedLength,oldY[surf]);    

//     //Now do the correlation
//     if(surf>0) {
//       FFTWComplex *tempStep = new FFTWComplex [tempLength];
//       Int_t no2=paddedLength>>1;
//       for(int i=0;i<tempLength;i++) {
// 	double reFFT1=theFFT[surf][i].re;
// 	double imFFT1=theFFT[surf][i].im;
// 	double reFFT2=theFFT[0][i].re;
// 	double imFFT2=theFFT[0][i].im;
// 	//Real part of output 
// 	tempStep[i].re=(reFFT1*reFFT2+imFFT1*imFFT2)/double(no2);
// 	//Imaginary part of output 
// 	tempStep[i].im=(imFFT1*reFFT2-reFFT1*imFFT2)/double(no2);
//       }
//       corVals[surf] = FFTtools::doInvFFT(paddedLength,tempStep);
//       delete [] tempStep;
      
//       outputX[surf] = new Double_t [paddedLength];
//       outputY[surf] = new Double_t [paddedLength];
//       for(int i=0;i<paddedLength;i++) {
// 	if(i<paddedLength/2) {
// 	  //Positive	  
// 	  outputX[surf][i+(paddedLength/2)]=(i*actualDt)+waveOffset;
// 	  outputY[surf][i+(paddedLength/2)]=corVals[surf][i];
// 	}
// 	else {
// 	  //Negative
// 	  outputX[surf][i-(paddedLength/2)]=((i-paddedLength)*actualDt)+waveOffset;
// 	  outputY[surf][i-(paddedLength/2)]=corVals[surf][i];	
// 	}  
//       }
      
//       grCorClock[surf-1]= new TGraph(paddedLength,outputX[surf],outputY[surf]);
//       delete [] outputX[surf];
//       delete [] outputY[surf];
//       delete [] corVals[surf];
//     }
//   }
    
//   for(int surf=0;surf<NUM_SURF;surf++) {
//     delete [] oldY[surf];
//     delete [] theFFT[surf];
//     delete grInt[surf];
//   }
  

  //Use the FFTtools package to interpolate and correlate the clock channels

  //  TGraph *grDeriv[NUM_SURF];
  //  TGraph *grInt[NUM_SURF];
  //  for(int surf=0;surf<NUM_SURF;surf++) {
  //    grInt[surf] = FFTtools::getInterpolatedGraph(grClock[surf],deltaT);
  //    grDeriv[surf] = FFTtools::getDerviative(grInt[surf]);
  //    if(surf>0) {
  //      grCorClock[surf-1] = FFTtools::getCorrelationGraph(grDeriv[surf],grDeriv[0]);
  //    }
  //  }
  //  for(int surf=0;surf<NUM_SURF;surf++) {
  //    delete grDeriv[surf];
  //    delete grInt[surf];
  //  }
  for(int surf=1;surf<NUM_SURF;surf++) {
    grCorClock[surf-1]= FFTtools::getInterpolatedCorrelationGraph(grClock[surf],grClock[0],deltaT);
  }
    
#endif
}
