#define USE_PIX
//#define DBG
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.media.core.interop.h>
#include <winrt/Windows.Devices.Display.Core.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>
#include <dxgi.h>
#include <d3d11.h>
#include <mutex>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Media.h>
//#include <DXProgrammableCapture.h>
#include "External/common.h"


using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;

// Model-agnostic helper LearningModels
LearningModel Normalize0_1ThenZScore(long height, long width, long channels, const std::array<float, 3>& means, const std::array<float, 3>& stddev);
LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
LearningModel Invert(long n, long c, long h, long w);

class StreamModelBase
{
public:
	StreamModelBase(): 
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL), 
		m_syncStarted(FALSE)
	{}
	StreamModelBase(int w, int h) :
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL),
		m_syncStarted(FALSE)
	{}
	virtual ~StreamModelBase() {
		if(m_session) m_session.Close();
		if(m_binding) m_binding.Clear();
		if (m_inputVideoFrame) m_inputVideoFrame.Close();
		if (m_outputVideoFrame) m_outputVideoFrame.Close(); 
	};

	virtual void InitializeSession(int w, int h) =0;
	virtual void Run(IDirect3DSurface src, IDirect3DSurface dest) =0;
	
	// Synchronous eval status
	BOOL m_syncStarted; 
	VideoFrame m_outputVideoFrame;
	static const int m_scale = 5;

protected:
	// Cache input frames into a shareable d3d-backed VideoFrame
	void SetVideoFrames(VideoFrame inVideoFrame, VideoFrame outVideoFrame) 
	{
		if (m_useGPU)
		{
			auto device = m_session.Device().Direct3D11Device();
			auto inDesc = inVideoFrame.Direct3DSurface().Description();
			auto outDesc = outVideoFrame.Direct3DSurface().Description();
			/*
				NOTE: VideoFrame::CreateAsDirect3D11SurfaceBacked takes arguments in (width, height) order
				whereas every model created with LearningModelBuilder takes arguments in (height, width) order. 
			*/ 
			auto format = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8X8UIntNormalized;
			m_inputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(format, m_imageWidthInPixels, m_imageHeightInPixels, device);
			m_outputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(format, m_imageWidthInPixels, m_imageHeightInPixels, device);
			m_videoFramesSet = true;
		}
		// TODO: Fix bug in WinML so that the surfaces from capture engine are shareable, remove copy. 
		inVideoFrame.CopyToAsync(m_inputVideoFrame).get();
		outVideoFrame.CopyToAsync(m_outputVideoFrame).get();
	}

	void SetImageSize(int w, int h) {
		m_imageWidthInPixels = w;
		m_imageHeightInPixels = h;
	}

	LearningModelSession CreateLearningModelSession(const LearningModel& model, bool closedModel = true) {
		auto device = m_useGPU ? LearningModelDevice(LearningModelDeviceKind::DirectXHighPerformance) : LearningModelDevice(LearningModelDeviceKind::Default);
		auto options = LearningModelSessionOptions();
		options.BatchSizeOverride(0);
		options.CloseModelOnSessionCreation(closedModel);
		auto session = LearningModelSession(model, device, options);
		return session;
	}

	bool						m_useGPU = true;
	bool						m_videoFramesSet = false;
	VideoFrame					m_inputVideoFrame;
								
	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;

	// Learning Model Binding and Session. 
	LearningModelSession m_session;
	LearningModelBinding m_binding;
}; 


class StyleTransfer : public StreamModelBase {
public:
	StyleTransfer(int w, int h) : StreamModelBase(w, h) 
	{
		InitializeSession(w, h); 
	}
	StyleTransfer() : StreamModelBase() {};
	virtual ~StyleTransfer(){};
	void InitializeSession(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);
private: 
	LearningModel GetModel();
};


class BackgroundBlur : public StreamModelBase
{
public:
	BackgroundBlur(int w, int h) : 
		StreamModelBase(w, h)
	{
		InitializeSession(w, h);
	}
	BackgroundBlur() : 
		StreamModelBase()
	{};
	virtual ~BackgroundBlur();
	void InitializeSession(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);

private:
	LearningModel GetModel();
	LearningModel PostProcess(long n, long c, long h, long w, long axis);
	
	// Mean and standard deviation for z-score normalization during preprocessing. 
	std::array<float, 3> m_mean = { 0.485f, 0.456f, 0.406f };
	std::array<float, 3> m_stddev = { 0.229f, 0.224f, 0.225f };


};