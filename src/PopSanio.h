#pragma once
#include <ofxSoylent.h>
#include <SoyApp.h>
#include <TJob.h>
#include <TChannel.h>
#include "Sms1130Device.h"



class TPopSanio : public TJobHandler, public TChannelManager
{
public:
	TPopSanio();
	
	virtual void	AddChannel(std::shared_ptr<TChannel> Channel) override;

	void			OnExit(TJobAndChannel& JobAndChannel);
	void			OnGetFrame(TJobAndChannel& JobAndChannel);
	
public:
	Soy::Platform::TConsoleApp	mConsoleApp;
	SoyVideoCapture				mVideoCapture;
};



