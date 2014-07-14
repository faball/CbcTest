#ifndef __CALIBRATIONRESULT_H__
#define __CALIBRATIONRESULT_H__
#include <map>
#include <iostream>
#include <vector>
#include <TH1F.h>
#include <TROOT.h>
#include <TGraphErrors.h>
#include <TCanvas.h>

class TPad;
namespace ICCalib{

	class Channel {

		public:

			Channel( UInt_t pFeId=0, UInt_t pCbcId=0, UInt_t pChannelId=0 ):
				fFeId(pFeId), fCbcId(pCbcId), fChannelId(pChannelId), fOffset(0), fNextOffset(0), fHist(0), fVCth0(0), fVCth0Error(0){}

			Channel( const Channel &pC ): fFeId(pC.fFeId), fCbcId(pC.fCbcId), fChannelId(pC.fChannelId), 
			fOffset(pC.fOffset), fNextOffset(pC.fNextOffset), fHist(pC.fHist), fVCth0(pC.fVCth0), fVCth0Error(pC.fVCth0Error) {}

			~Channel(){}

			void Id( UInt_t &pFeId, UInt_t &pCbcId, UInt_t &pChannelId ){ pFeId = fFeId; pCbcId = fCbcId; pChannelId = fChannelId; }
			UInt_t FeId()const{ return fFeId; } 
			UInt_t CbcId()const{ return fCbcId; } 
			UInt_t ChannelId()const{ return fChannelId; }
			UInt_t Offset()const{ return fOffset; }
			TH1F * Hist(){ return fHist; }
			float VCth0()const{ return fVCth0; }
			float VCth0Error()const{ return fVCth0Error; }

			void SetOffset( UInt_t pOffset ){ fOffset = pOffset; }
			void SetNextOffset( UInt_t pOffset ){ fNextOffset = pOffset; }
			void SetHist( TH1F *pH ){ fHist = pH; }
			void SetVCth0( float pVCth0, float pVCth0Error ){ fVCth0 = pVCth0; fVCth0Error = pVCth0Error; }

			void UpdateOffset(){ fOffset = fNextOffset; }
			void ResetHist(){ if( fHist ) fHist->Reset(); }

		private:
			UInt_t fFeId;
			UInt_t fCbcId;
			UInt_t fChannelId;
			UInt_t fOffset;
			UInt_t fNextOffset;
			TH1F * fHist;
			float fVCth0;
			float fVCth0Error;
	};

	class CbcInfo: public std::map<UInt_t, Channel *> {

		public:
			CbcInfo( UInt_t pFeId, UInt_t pCbcId ): std::map<UInt_t, Channel *>(), 
			fFeId(pFeId), fCbcId(pCbcId), fGraphVplusVCth0(), fVplus(0),
			fVplusVsVCth0GraphDisplayPad(0),
			fGraphGain(),//fb
			fGainGraphDisplayPad(0),//fb
			fDummyPad(0){}
			CbcInfo(const CbcInfo &pC): std::map<UInt_t, Channel *>(pC), 
			fFeId(pC.fFeId), fCbcId(pC.fCbcId), fGraphVplusVCth0(pC.fGraphVplusVCth0), fVplus(pC.fVplus),
			fScurveHistogramDisplayPad(pC.fScurveHistogramDisplayPad),
			fVplusVsVCth0GraphDisplayPad(pC.fVplusVsVCth0GraphDisplayPad),
			fGraphGain(pC.fGraphGain),//fb
			fGainGraphDisplayPad(pC.fGainGraphDisplayPad),//fb
			fDummyPad(pC.fDummyPad){}
			~CbcInfo(){}

			UInt_t FeId()const{ return fFeId; } 
			UInt_t CbcId()const{ return fCbcId; } 
			const Channel * GetChannel( UInt_t pChannelId )const;
			std::map<UInt_t, TGraphErrors *> * GetGraphVplusVCth0(){ return &fGraphVplusVCth0; }
			TGraphErrors *GetGraphVplusVCth0( UInt_t pGroupId );
			std::map<UInt_t, TGraphErrors *> * GetGraphGain(){ return &fGraphGain; }
			TGraphErrors *GetGraphGain( UInt_t pGroupId ); //fb
			UInt_t Vplus()const{ return fVplus; }
			TPad *GetVplusVsVCth0GraphDisplayPad(){ return fVplusVsVCth0GraphDisplayPad; }
			TPad *GetGainGraphDisplayPad(){ return fGainGraphDisplayPad; }
			TPad *GetScurveHistogramDisplayPad( UInt_t pGroupId ){ return fScurveHistogramDisplayPad.find( pGroupId )->second; }
			TPad *GetDummyPad(){ if( fDummyPad == 0 ) fDummyPad =
                        new TCanvas( Form( "cFE%dCBC%d", fFeId, fCbcId ), Form( "cFE%dCBC%d", fFeId, fCbcId ), 100, 100 ); return fDummyPad; }

			void AddChannel( UInt_t pChannelId, Channel * pChannel );
			void AddGraphVplusVCth0( UInt_t pGroup, TGraphErrors *pGraph );
			void AddGraphGain( UInt_t pGroup, TGraphErrors *pGraph );//fb
			void SetVplus( UInt_t pValue ){ fVplus = pValue; }
			void SetVplusVsVCth0GraphDisplayPad( TPad *pPad );
			void SetGainGraphDisplayPad( TPad *pPad );//fb
			void SetScurveHistogramDisplayPad( UInt_t pGroupId, TPad *pPad ); 
			void SetDummyPad( TPad *pPad ){ fDummyPad = pPad; }
		private:
			UInt_t                           fFeId;
			UInt_t                           fCbcId;
			std::map<UInt_t, TGraphErrors *> fGraphVplusVCth0;
			UInt_t                           fVplus;
			std::map<UInt_t, TPad *>         fScurveHistogramDisplayPad;
			TPad                             *fVplusVsVCth0GraphDisplayPad;

			std::map<UInt_t, TGraphErrors *> fGraphGain;//fb
			TPad                             *fGainGraphDisplayPad; //fb
			TPad                             *fDummyPad;


	};

	class FeInfo: public std::map<UInt_t, CbcInfo>{ 

		friend class CalibrationResult;

		public:
			FeInfo(UInt_t pFeId): fFeId(pFeId), fCbcIds(){}
			FeInfo(const FeInfo &pFeInfo): std::map<UInt_t, CbcInfo>(pFeInfo), fFeId(pFeInfo.fFeId), fCbcIds(pFeInfo.fCbcIds){}
			~FeInfo(){}
			const std::vector<UInt_t> &GetCbcIds(){ return fCbcIds; }
			const CbcInfo *GetCbcInfo( UInt_t pCbcId )const;
			const Channel *GetChannel( UInt_t pCbcId, UInt_t pChannelId )const;
			void AddCbcInfo( UInt_t pCbcId, CbcInfo pCbcInfo ); 
			void AddChannel( UInt_t pCbcId, UInt_t pChannelId, Channel* pChannel ); 
			void AddGraphVplusVCth0( UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG );
			void AddGraphGain( UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG ); //fb
			void SetScurveHistogramDisplayPad( UInt_t pCbcId, UInt_t pGroupId, TPad *pPad );
			void SetVplusVsVCth0GraphDisplayPad( UInt_t pCbcId, TPad *pPad );
			void SetGainGraphDisplayPad( UInt_t pCbcId, TPad *pPad ); //fb
			void SetDummyPad( UInt_t pCbcId, TPad *pPad );
		private:
			CbcInfo *getCbcInfo( UInt_t pCbcId );
			UInt_t fFeId;
			std::vector<UInt_t> fCbcIds;
	};

	class CalibrationResult : public std::map<UInt_t, FeInfo>{ 

		friend class ScurveAnalyser;
		public:
			CalibrationResult(){}
			CalibrationResult( const CalibrationResult &pC ): std::map<UInt_t, FeInfo>(pC), fFeIds(pC.fFeIds){}
			~CalibrationResult(){}
			const std::vector<UInt_t> GetFeIds()const{ return fFeIds; }
			const FeInfo *GetFeInfo( UInt_t pFeId )const;
			const CbcInfo *GetCbcInfo( UInt_t pFeId, UInt_t pCbcInfo )const;
			const Channel *GetChannel( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannelId )const;
			void AddFeInfo( UInt_t pFeId, FeInfo pFeInfo );
			void AddCbcInfo( UInt_t pFeId, UInt_t pCbcId, CbcInfo pCbcInfo );
			void AddChannel( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannelId, Channel *pChannel ); 
			void AddGraphVplusVCth0( UInt_t pFeId, UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG );
			void SetVplusVsVCth0GraphDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad );
			void AddGraphGain( UInt_t pFeId, UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG ); //fb
			void SetGainGraphDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad );//fb
			void SetScurveHistogramDisplayPad( UInt_t pFeId, UInt_t pCbcId, UInt_t pGroupId, TPad *pPad );
			void SetDummyPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad );
		private:
			FeInfo *getFeInfo( UInt_t pFeId );
			CbcInfo *getCbcInfo( UInt_t pFeId, UInt_t pCbcId );
			std::vector<UInt_t> fFeIds;
	};
}
#endif

