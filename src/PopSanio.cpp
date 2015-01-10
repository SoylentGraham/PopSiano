#include "PopSanio.h"
#include <TParameters.h>
#include <SoyDebug.h>
#include <TProtocolCli.h>
#include <TProtocolHttp.h>
#include <SoyApp.h>
#include <PopMain.h>
#include <TJobRelay.h>
#include <SoyPixels.h>
#include <SoyString.h>
#include <SortArray.h>
#include <TChannelLiteral.h>
#include "Sms1130Device.h"


TPopSanio::TPopSanio()
{
	std::shared_ptr<SoyVideoContainer> SianoContainer( new class SianoContainer() );
	mVideoCapture.AddContainer( SianoContainer );

	AddJobHandler("exit", TParameterTraits(), *this, &TPopSanio::OnExit );
		
	AddJobHandler("getframe", TParameterTraits(), *this, &TPopSanio::OnGetFrame );
}

void TPopSanio::AddChannel(std::shared_ptr<TChannel> Channel)
{
	TChannelManager::AddChannel( Channel );
	if ( !Channel )
		return;
	TJobHandler::BindToChannel( *Channel );
}


void TPopSanio::OnExit(TJobAndChannel& JobAndChannel)
{
	mConsoleApp.Exit();
	
	//	should probably still send a reply
	TJobReply Reply( JobAndChannel );
	Reply.mParams.AddDefaultParam(std::string("exiting..."));
	TChannel& Channel = JobAndChannel;
	Channel.OnJobCompleted( Reply );
}


void TPopSanio::OnGetFrame(TJobAndChannel& JobAndChannel)
{
	const TJob& Job = JobAndChannel;
	TJobReply Reply( JobAndChannel );
	
	auto Serial = Job.mParams.GetParamAs<std::string>("serial");
	auto AsMemFile = Job.mParams.GetParamAsWithDefault<bool>("memfile",true);
	
	std::stringstream Error;
	auto Device = mVideoCapture.GetDevice( Serial, Error );
	
	if ( !Device )
	{
		std::stringstream ReplyError;
		ReplyError << "Device " << Serial << " not found " << Error.str();
		Reply.mParams.AddErrorParam( ReplyError.str() );
		TChannel& Channel = JobAndChannel;
		Channel.OnJobCompleted( Reply );
		return;
	}
	
	//	grab pixels
	auto& LastFrame = Device->GetLastFrame( Error );
	if ( LastFrame.IsValid() )
	{
		if ( AsMemFile )
		{
			TYPE_MemFile MemFile( LastFrame.mPixels.mMemFileArray );
			Reply.mParams.AddDefaultParam( MemFile );
		}
		else
		{
			SoyPixels Pixels;
			Pixels.Copy( LastFrame.mPixels );
			Reply.mParams.AddDefaultParam( Pixels );
		}
	}
	
	//	add error if present (last frame could be out of date)
	if ( !Error.str().empty() )
		Reply.mParams.AddErrorParam( Error.str() );
	
	//	add other stats
	auto FrameRate = Device->GetFps();
	auto FrameMs = Device->GetFrameMs();
	Reply.mParams.AddParam("fps", FrameRate);
	Reply.mParams.AddParam("framems", FrameMs );
	Reply.mParams.AddParam("serial", Device->GetMeta().mSerial );
	
	TChannel& Channel = JobAndChannel;
	Channel.OnJobCompleted( Reply );
	
}



//	horrible global for lambda
std::shared_ptr<TChannel> gStdioChannel;
std::shared_ptr<TChannel> gCaptureChannel;



TPopAppError::Type PopMain(TJobParams& Params)
{
	std::cout << Params << std::endl;
	
	TPopSanio App;

	auto CommandLineChannel = std::shared_ptr<TChan<TChannelLiteral,TProtocolCli>>( new TChan<TChannelLiteral,TProtocolCli>( SoyRef("cmdline") ) );
	
	//	create stdio channel for commandline output
	auto StdioChannel = CreateChannelFromInputString("std:", SoyRef("stdio") );
	gStdioChannel = StdioChannel;
	auto HttpChannel = CreateChannelFromInputString("http:8080-8090", SoyRef("http") );
//	auto WebSocketChannel = CreateChannelFromInputString("ws:json:9090-9099", SoyRef("websock") );
	//auto WebSocketChannel = CreateChannelFromInputString("ws:cli:9090-9099", SoyRef("websock") );
//	auto SocksChannel = CreateChannelFromInputString("cli:7070-7079", SoyRef("socks") );
	
	
	App.AddChannel( CommandLineChannel );
	App.AddChannel( StdioChannel );
	App.AddChannel( HttpChannel );
//	App.AddChannel( WebSocketChannel );
//	App.AddChannel( SocksChannel );

	//	when the commandline SENDs a command (a reply), send it to stdout
	auto RelayFunc = [](TJobAndChannel& JobAndChannel)
	{
		if ( !gStdioChannel )
			return;
		TJob Job = JobAndChannel;
		Job.mChannelMeta.mChannelRef = gStdioChannel->GetChannelRef();
		Job.mChannelMeta.mClientRef = SoyRef();
		gStdioChannel->SendCommand( Job );
	};
	CommandLineChannel->mOnJobSent.AddListener( RelayFunc );
	
	//	connect to another app, and subscribe to frames
	bool CreateCaptureChannel = false;
	if ( CreateCaptureChannel )
	{
		auto CaptureChannel = CreateChannelFromInputString("cli://localhost:7070", SoyRef("capture") );
		gCaptureChannel = CaptureChannel;
		CaptureChannel->mOnJobRecieved.AddListener( RelayFunc );
		App.AddChannel( CaptureChannel );
		
		//	send commands from stdio to new channel
		auto SendToCaptureFunc = [](TJobAndChannel& JobAndChannel)
		{
			TJob Job = JobAndChannel;
			Job.mChannelMeta.mChannelRef = gStdioChannel->GetChannelRef();
			Job.mChannelMeta.mClientRef = SoyRef();
			gCaptureChannel->SendCommand( Job );
		};
		gStdioChannel->mOnJobRecieved.AddListener( SendToCaptureFunc );
		
		auto StartSubscription = [](TChannel& Channel)
		{
			TJob GetFrameJob;
			GetFrameJob.mChannelMeta.mChannelRef = Channel.GetChannelRef();
			//GetFrameJob.mParams.mCommand = "subscribenewframe";
			//GetFrameJob.mParams.AddParam("serial", "isight" );
			GetFrameJob.mParams.mCommand = "getframe";
			GetFrameJob.mParams.AddParam("serial", "isight" );
			GetFrameJob.mParams.AddParam("memfile", "1" );
			Channel.SendCommand( GetFrameJob );
		};
		
		CaptureChannel->mOnConnected.AddListener( StartSubscription );
	}
	
	std::string TestFilename = "/users/grahamr/Desktop/ringo.png";
	
	//	gr: bootup commands
	auto BootupGet = [TestFilename](TChannel& Channel)
	{
		TJob GetFrameJob;
		GetFrameJob.mChannelMeta.mChannelRef = Channel.GetChannelRef();
		GetFrameJob.mParams.mCommand = "getfeature";
		GetFrameJob.mParams.AddParam("x", 120 );
		GetFrameJob.mParams.AddParam("y", 120 );
		GetFrameJob.mParams.AddParam("image", TestFilename, TJobFormat("text/file/png") );
		Channel.OnJobRecieved( GetFrameJob );
	};
	
	auto BootupMatch = [TestFilename](TChannel& Channel)
	{
		TJob GetFrameJob;
		GetFrameJob.mChannelMeta.mChannelRef = Channel.GetChannelRef();
		GetFrameJob.mParams.mCommand = "findfeature";
		GetFrameJob.mParams.AddParam("feature", "01011000000000001100100100000000" );
		GetFrameJob.mParams.AddParam("image", TestFilename, TJobFormat("text/file/png") );
		Channel.OnJobRecieved( GetFrameJob );
	};
	
	//	auto BootupFunc = BootupMatch;
	//auto BootupFunc = BootupGet;
	auto BootupFunc = BootupMatch;
	if ( CommandLineChannel->IsConnected() )
		BootupFunc( *CommandLineChannel );
	else
		CommandLineChannel->mOnConnected.AddListener( BootupFunc );
	

	
	
	//	run
	App.mConsoleApp.WaitForExit();

	gStdioChannel.reset();
	return TPopAppError::Success;
}




