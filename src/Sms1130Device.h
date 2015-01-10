#pragma once
#include <ofxSoylent.h>
#include "TJob.h"
#include <SoyVideoDevice.h>




//	runtime device
class SianoDevice : public TVideoDevice
{
public:
	SianoDevice(const std::string& Serial,std::stringstream& Error);

	virtual TVideoDeviceMeta	GetMeta() const override;

	bool					Open(std::stringstream& Error);
	void					Close();
	
	void					OnVideo(void *rgb, uint32_t timestamp);

	
public:
	SoyPixels				mVideoBuffer;
	
	std::string				mSerial;
};



class SianoContainer : public SoyVideoContainer, public SoyWorkerThread
{
public:
	SianoContainer();
	virtual ~SianoContainer();
	
	virtual void							GetDevices(ArrayBridge<TVideoDeviceMeta>& Metas) override;
	virtual std::shared_ptr<TVideoDevice>	AllocDevice(const std::string& Serial,std::stringstream& Error) override;
	
private:
	void				Shutdown();
	virtual bool		Iteration();
	bool				CreateContext(std::stringstream& Error);

private:
	std::shared_ptr<SianoDevice>	mDevice;	//	only one device
};


