#include "CalibrationResult.h"
#include <TPad.h>

namespace ICCalib{

	const Channel *CbcInfo::GetChannel( UInt_t pChannelId )const{

		std::map<UInt_t, Channel *>::const_iterator cIt = this->find( pChannelId );
		if( cIt != this->end() ) return cIt->second;
		return 0;
	}
	TGraphErrors *CbcInfo::GetGraphVplusVCth0( UInt_t pGroupId ){
		std::map<UInt_t, TGraphErrors *>::iterator cIt = fGraphVplusVCth0.find( pGroupId ); 
		if( cIt == fGraphVplusVCth0.end() ) return 0;
		return cIt->second;
	}

	TGraphErrors *CbcInfo::GetGraphGain( UInt_t pGroupId ){
		std::map<UInt_t, TGraphErrors *>::iterator cIt = fGraphGain.find( pGroupId );
		if( cIt == fGraphGain.end() ) {
			return 0;
		}
		return cIt->second;
	}

	void CbcInfo::AddChannel( UInt_t pChannelId, Channel * pChannel ){ 

		this->insert( std::pair<UInt_t, Channel *>( pChannelId, pChannel ) );
	}
	void CbcInfo::AddGraphVplusVCth0( UInt_t pGroup, TGraphErrors *pGraph ){

		fGraphVplusVCth0.insert( std::pair< UInt_t, TGraphErrors *>( pGroup, pGraph ) );
	}

	void CbcInfo::AddGraphGain( UInt_t pGroup, TGraphErrors *pGraph ){ //fb

		fGraphGain.insert( std::pair< UInt_t, TGraphErrors *>( pGroup, pGraph ) );
	}

	void CbcInfo::SetVplusVsVCth0GraphDisplayPad( TPad *pPad ){

		fVplusVsVCth0GraphDisplayPad = pPad;
	}

	void CbcInfo::SetGainGraphDisplayPad( TPad *pPad ){ //fb

		fGainGraphDisplayPad = pPad;
	}

	void CbcInfo::SetScurveHistogramDisplayPad( UInt_t pGroupId, TPad *pPad ){ 

		fScurveHistogramDisplayPad.insert( std::pair< UInt_t, TPad *>( pGroupId, pPad ) );
	}

	const CbcInfo *FeInfo::GetCbcInfo( UInt_t pCbcId )const{

		const CbcInfo *cCbcInfo(0);
		std::map<UInt_t, CbcInfo>::const_iterator cIt = this->find( pCbcId );
		if( cIt != this->end() ){
			cCbcInfo = &(cIt->second);
		}
		return cCbcInfo;
	}
	const Channel *FeInfo::GetChannel( UInt_t pCbcId, UInt_t pChannelId )const{

		const CbcInfo *cCbcInfo = GetCbcInfo( pCbcId );
		if( cCbcInfo == 0 ) return 0;
		return cCbcInfo->GetChannel( pChannelId );
	}
	void FeInfo::AddCbcInfo( UInt_t pCbcId, CbcInfo pCbcInfo ){ 

		fCbcIds.push_back( pCbcId); this->insert( std::pair<UInt_t, CbcInfo>( pCbcId, pCbcInfo ) ); 
	}
	void FeInfo::AddChannel( UInt_t pCbcId, UInt_t pChannelId, Channel* pChannel ){ 

		CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
		if( cCbcInfo == 0 ){
			AddCbcInfo( pCbcId, CbcInfo( fFeId, pCbcId ) );
			cCbcInfo = &(this->find(pCbcId)->second);
		}
		cCbcInfo->AddChannel( pChannelId, pChannel );
	}
	void FeInfo::AddGraphVplusVCth0( UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG ){

		CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
		if( cCbcInfo != 0 ){
			cCbcInfo->AddGraphVplusVCth0( pGroupId, pG );
		}
	}

	void FeInfo::AddGraphGain( UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG ){ //fb
			CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
			if( cCbcInfo != 0 ){
				cCbcInfo->AddGraphGain( pGroupId, pG );
			}
		}

	void FeInfo::SetScurveHistogramDisplayPad( UInt_t pCbcId, UInt_t pGroupId, TPad *pPad ){

		CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
		if( cCbcInfo ){
			cCbcInfo->SetScurveHistogramDisplayPad( pGroupId, pPad );
		}
	}
	void FeInfo::SetVplusVsVCth0GraphDisplayPad( UInt_t pCbcId, TPad *pPad ){

		CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
		if( cCbcInfo ){
			cCbcInfo->SetVplusVsVCth0GraphDisplayPad( pPad );
		}
	}

	void FeInfo::SetGainGraphDisplayPad( UInt_t pCbcId, TPad *pPad ){ //fb

		CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
		if( cCbcInfo ){
			cCbcInfo->SetGainGraphDisplayPad( pPad );
		}
	}

	void FeInfo::SetDummyPad( UInt_t pCbcId, TPad *pPad ){

		CbcInfo *cCbcInfo = getCbcInfo( pCbcId );
		if( cCbcInfo ){
			cCbcInfo->SetDummyPad( pPad );
		}
	}

	CbcInfo *FeInfo::getCbcInfo( UInt_t pCbcId ){

		CbcInfo *cCbcInfo(0);
		std::map<UInt_t, CbcInfo>::iterator cIt = this->find( pCbcId );
		if( cIt != this->end() ){
			cCbcInfo = &(cIt->second);
		}
		return cCbcInfo;
	}

	const FeInfo *CalibrationResult::GetFeInfo( UInt_t pFeId )const{

		const FeInfo *cFeInfo(0);
		std::map<UInt_t, FeInfo>::const_iterator cIt = this->find( pFeId );
		if( cIt != this->end() ){
			cFeInfo = &(cIt->second);
		}
		return cFeInfo;
	}
	const CbcInfo *CalibrationResult::GetCbcInfo( UInt_t pFeId, UInt_t pCbcId )const{

		const FeInfo *cFeInfo = GetFeInfo( pFeId );
		if( cFeInfo == 0 ) return 0;
		return cFeInfo->GetCbcInfo( pCbcId );
	}
	const Channel *CalibrationResult::GetChannel( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannelId )const{

		const FeInfo *cFeInfo = GetFeInfo( pFeId );
		if( cFeInfo == 0 ) return 0;
		return cFeInfo->GetChannel( pCbcId, pChannelId );
	}
	void CalibrationResult::AddFeInfo( UInt_t pFeId, FeInfo pFeInfo ){ 

		fFeIds.push_back(pFeId); insert( std::pair<UInt_t, FeInfo>( pFeId, pFeInfo ) ); 
	}
	void CalibrationResult::AddCbcInfo( UInt_t pFeId, UInt_t pCbcId, CbcInfo pCbcInfo ){ 

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo == 0 ){
			AddFeInfo( pFeId, FeInfo( pFeId ) );
			cFeInfo =  &(this->find(pFeId)->second);
		}
		cFeInfo->AddCbcInfo( pCbcId, pCbcInfo );
	}
	void CalibrationResult::AddChannel( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannelId, Channel *pChannel ){ 

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo == 0 ){
			AddFeInfo( pFeId, FeInfo( pFeId ) );
			cFeInfo =  &(this->find(pFeId)->second);
		}
		cFeInfo->AddChannel( pCbcId, pChannelId, pChannel );
	}
	void CalibrationResult::AddGraphVplusVCth0( UInt_t pFeId, UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG ){

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo != 0 ){
			cFeInfo->AddGraphVplusVCth0( pCbcId, pGroupId, pG );
		}
	}
	void CalibrationResult::SetVplusVsVCth0GraphDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad ){

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo ){
			cFeInfo->SetVplusVsVCth0GraphDisplayPad( pCbcId, pPad );
		}
	}

	void CalibrationResult::SetGainGraphDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad ){ //fb

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo ){
			cFeInfo->SetGainGraphDisplayPad( pCbcId, pPad );
		}
	}

	void CalibrationResult::SetScurveHistogramDisplayPad( UInt_t pFeId, UInt_t pCbcId, UInt_t pGroupId, TPad *pPad ){

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo ){
			cFeInfo->SetScurveHistogramDisplayPad( pCbcId, pGroupId, pPad );
		}
	}

	void CalibrationResult::AddGraphGain( UInt_t pFeId, UInt_t pCbcId, UInt_t pGroupId, TGraphErrors *pG ){

			FeInfo *cFeInfo = getFeInfo( pFeId );
			if( cFeInfo != 0 ){
				cFeInfo->AddGraphGain( pCbcId, pGroupId, pG );
			}
		}

	void CalibrationResult::SetDummyPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad ){

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo ){
			cFeInfo->SetDummyPad( pCbcId, pPad );
		}
	}
	FeInfo *CalibrationResult::getFeInfo( UInt_t pFeId ){

		FeInfo *cFeInfo(0);
		std::map<UInt_t, FeInfo>::iterator cIt = this->find( pFeId );
		if( cIt != this->end() ){
			cFeInfo = &(cIt->second);
		}
		return cFeInfo;
	}
	CbcInfo *CalibrationResult::getCbcInfo( UInt_t pFeId, UInt_t pCbcId ){

		FeInfo *cFeInfo = getFeInfo( pFeId );
		if( cFeInfo ){
			return cFeInfo->getCbcInfo( pCbcId );
		}
		return 0;	
	}
}

