#include "pch.h"
#include "TransformAsync.h"
#include <CommCtrl.h>
long long g_now; // The time since the last call to FrameThreadProc

DWORD __stdcall FrameThreadProc(LPVOID lpParam)
{
    DWORD waitResult;
    // Get the handle from the lpParam pointer
    HANDLE event = lpParam;
    /*com_ptr<IUnknown> unk;
    com_ptr<TransformAsync> transform;
    unk.attach((IUnknown*)lpParam);
    transform = unk.as<TransformAsync>();*/

    //OutputDebugString(L"Thread %d waiting for Frame event...");
    waitResult = WaitForSingleObject(
        event,
        //transform->m_fenceEvent.get(),         // event handle
        INFINITE);      // indefinite wait


    switch (waitResult) {
    case WAIT_OBJECT_0:
        // TODO: Capture time and write to preview
        if (g_now == NULL) {
            g_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            //OutputDebugString(L"First time responding to event!");
        }
        else {
            auto l_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto timePassed = l_now - g_now;
            g_now = l_now;
            auto fps = 30000 / timePassed; // TODO: marco on num frames to update after? 
            OutputDebugString(L"THREAD: ");
            OutputDebugString(std::to_wstring(fps).c_str());
            OutputDebugString(L"\n");

            auto message = std::wstring(L"Frame Rate: ") + std::to_wstring(fps) + L" FPS";
            //transform->WriteFrameRate(message.c_str());
            //MainWindow::_SetStatusText(message.c_str());
            //TRACE(("Responded to event and it's been %d miliseconds", timePassed));
            // TODO: Call Set status text with new framerate
        }

        break;
    default:
        TRACE(("Wait error (%d)\n", GetLastError()));
        return 0;
    }
    return 1;
}

STDMETHODIMP TransformAsync::TransformAsyncCB::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(TransformAsyncCB, IMFVideoSampleAllocatorNotify),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) TransformAsync::TransformAsyncCB::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

STDMETHODIMP_(ULONG) TransformAsync::TransformAsyncCB::Release()
{
    LONG cRef = InterlockedDecrement(&m_ref);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

// Callback method to receive events from the capture engine.
STDMETHODIMP TransformAsync::TransformAsyncCB::NotifyRelease()
{
    RETURN_IF_NULL_ALLOC(m_transform);

    const UINT64 currFenceValue = m_transform->m_fenceValue;
    auto fenceComplete = m_transform->m_fence->GetCompletedValue();
    DWORD dwThreadID;

    // Fail fast if context doesn't exist anymore. 
    if (m_transform->m_context == nullptr)
    {
        return S_OK;
    }

    // SChedule a Signal command in the queue
    RETURN_IF_FAILED(m_transform->m_context->Signal(m_transform->m_fence.get(), currFenceValue));

    if (currFenceValue % FRAME_RATE_UPDATE == 0)
    {

        m_transform->m_fence->SetEventOnCompletion(currFenceValue, m_fenceEvent.get()); // Raise FenceEvent when done
        m_transform->m_frameThread = (CreateThread(NULL, 0, FrameThreadProc, m_fenceEvent.get(), 0, &dwThreadID));
    }

    m_transform->m_fenceValue = currFenceValue + 1;
    return S_OK;
}

void TransformAsync::TransformAsyncCB::StartThread()
{
    // Create event and thread for framerate
    m_fenceEvent.reset(CreateEvent(NULL,               // Security attributes
        FALSE,              // Reset token to false means system will auto-reset event object
        FALSE,              // Initial state is nonsignaled
        TEXT("FrameEvent")));  // Event object name
}


void TransformAsync::SetFrameRateWnd(HWND hwnd)
{
    m_callback->m_hwnd = hwnd;
}

void TransformAsync::WriteFrameRate(const WCHAR* frameRate)
{
    /*if (m_frameWnd) {
        SendMessage(m_frameWnd, SB_SETTEXT, (WPARAM)(0), (LPARAM)frameRate);
    }*/
}