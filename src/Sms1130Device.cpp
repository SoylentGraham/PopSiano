#include "Sms1130Device.h"
#include <SoyDebug.h>
#include <SoyTypes.h>
#include "TJob.h"
#include "TParameters.h"
#include "TChannel.h"



SianoContainer::SianoContainer() :
	SoyWorkerThread	( "SianoContainer", SoyWorkerWaitMode::NoWait )
{
	std::stringstream Error;
	if ( !CreateContext(Error) )
		return;
	Start();
}

SianoContainer::~SianoContainer()
{
	WaitToFinish();
}

void SianoContainer::GetDevices(ArrayBridge<TVideoDeviceMeta>& Metas)
{
	if ( mDevice )
	{
		Metas.PushBack( mDevice->GetMeta() );
	}
}


bool SianoContainer::CreateContext(std::stringstream& Error)
{
	mDevice.reset( new SianoDevice("tv", Error) );
	return false;
}

bool SianoContainer::Iteration()
{
	/*
	std::lock_guard<std::recursive_mutex> Lock(mContextLock);

	//	gr: use this for the thread block
	int TimeoutMs = 10;
	int TimeoutSecs = 0;
	int TimeoutMicroSecs = TimeoutMs*1000;
	timeval Timeout = {TimeoutSecs,TimeoutMicroSecs};
	auto Result = freenect_process_events_timeout( mContext, &Timeout );
	if ( Result < 0 )
	{
		std::Debug << "Freenect_events error: " << Result;
	}
*/
	return true;
}

std::shared_ptr<TVideoDevice> SianoContainer::AllocDevice(const std::string& Serial,std::stringstream& Error)
{
	return mDevice;
}


SianoDevice::SianoDevice(const std::string& Serial,std::stringstream& Error) :
	TVideoDevice	( Serial, Error ),
	mSerial			( Serial )
{
	Open(Error);
}

TVideoDeviceMeta SianoDevice::GetMeta() const
{
	TVideoDeviceMeta Meta("tv");
	return Meta;
}

bool SianoDevice::Open(std::stringstream& Error)
{
	return true;
}

void SianoDevice::Close()
{
}

void SianoDevice::OnVideo(void *rgb, uint32_t timestamp)
{
	if ( !Soy::Assert( rgb, "rgb data expected" ) )
		return;
	/*
	//std::Debug << "On video " << timestamp << std::endl;

	auto& Pixels = mVideoBuffer.GetPixelsArray();
	int Bytes = std::min( mVideoMode.bytes, Pixels.GetDataSize() );
	memcpy( Pixels.GetArray(), rgb, Bytes );

	//	notify change
	SoyTime Timecode( static_cast<uint64>(timestamp) );
	OnNewFrame( mVideoBuffer, Timecode );
	 */
}



