#ifndef __SCURVEANALYSER_H__
#define __SCURVEANALYSER_H__
#include "CalibrationResult.h"
#include "../Strasbourg/Data.h"
#include "TH1F.h"
#include <vector>
#include <set>
#include <TString.h>
#include "TestGroup.h"
#include "CbcDaq/Analyser.h"

#include <map>

namespace Cbc{
	class CbcRegMap;
}

using namespace Cbc;
using namespace Strasbourg;
using namespace CbcDaq;

namespace ICCalib{

	class ScurveAnalyser : public Analyser{

		public:
			enum { OFFSETCALIB, VPLUSSEARCH, SINGLEVCTHSCAN, DELAYSCAN };
		public:
			ScurveAnalyser( UInt_t pBeId, UInt_t pNFe, UInt_t pNCbc, 
					TestGroupMap *pGroupMap, const CbcRegMap *pMap,
					Bool_t pNegativeLogicCbc, UInt_t pTargetVCth, const char *pOutputDir, GUIFrame *pGUIFrame );
			virtual ~ScurveAnalyser(){}
			void   Initialise();
			void   SetOffsets();
			void   SetOffsets( UInt_t pInitalOffset ); 
			void   Configure( Int_t pType, Int_t pOffsetTargetBit=8 );
			int    FillGraphVplusVCth0();
			void   DrawVplusVCth0();
			void   PrintVplusVsVCth0DisplayPads();
			int    FillGraphGain();//fb
			//void   SetMidPoint(TString cHtotalName);
			void   DrawGain();//fb
			void   PrintGainDisplayPads();//fb
			int    FillHists( UInt_t pVcth, const Event *pEvent );
			void   FitHists( UInt_t pMin, UInt_t pMax );
			void   DrawHists();
			void   PrintScurveHistogramDisplayPads();
			UInt_t GetOffset( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannel );
			UInt_t GetVplus( UInt_t pFeId, UInt_t pCbcId );
			UInt_t GetMinVCth0(){ return fMinVCth0; }
			UInt_t GetMaxVCth0(){ return fMaxVCth0; }
			void   DumpResult();
			const  CalibrationResult &GetResult()const{ return fResult; }
			std::vector<Channel *>   *GetChannelList(){ return &fChannelList; }
			void   SetVplus();
			void   SetGain();//fb
			void   SetScurveHistogramDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad );
			void   SetVplusVsVCth0GraphDisplayPad( UInt_t pFeId, TPad *pPad );
			void   SetGainGraphDisplayPad( UInt_t pFeId, TPad *pPad ); //fb


		private:
			TH1F  *createScurveHistogram( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannel );
			void  resetHistograms();
			void  updateOffsets();
			TString getScanId();
			TString getScanType();
			void  setNextOffsets();



			std::map<Double_t , Double_t > fMidPoints;//fb


			TestGroupMap           *fGroupMap;
			UInt_t                 fTargetVCth;
			Int_t                  fScanType;
			Int_t                  fCurrentTargetBit;
			Int_t                  fNthVplusPoint;
			std::vector<Channel *> fChannelList;
			CalibrationResult      fResult;
			TH1F                   *fHtotal;
			TH1F                   *fDummyHist;
			UInt_t                 fMinVCth0;
			UInt_t                 fMaxVCth0;
	};

}
#endif

