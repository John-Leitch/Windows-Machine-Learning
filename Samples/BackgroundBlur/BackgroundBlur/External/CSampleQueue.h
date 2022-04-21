#pragma once

#include <windows.h>
#include <Mfidl.h>
#include "common.h"
#include <mutex> 

using namespace MediaFoundationSamples;

class CSampleQueue: public IUnknown
{
public:
    class   ILockedSample;

    static  HRESULT Create(
                            CSampleQueue**   ppQueue
                            );

#pragma region IUnknown
    // IUnknown Implementations
    ULONG   __stdcall   AddRef(void);
    HRESULT __stdcall   QueryInterface(
                                REFIID  riid,
                                void**  ppvObject
                                );
    ULONG   __stdcall   Release(void);
#pragma endregion IUnknown

    // ILockedSampleCallback Implementation
            
            HRESULT AddSample(                      // Add a sample to the back of the queue
                            IMFSample*  pSample
                            );
            HRESULT GetNextSample(                  // Remove a sample from the front of the queue
                            IMFSample** pSample
                            );
            HRESULT RemoveAllSamples(void);
            HRESULT MarkerNextSample(
                            const ULONG_PTR pulID
                            );
            BOOL    IsQueueEmpty(void);

            ULONG GetLength();
protected:
                class CNode;

                    CSampleQueue(void);
                    ~CSampleQueue(void);

    volatile    ULONG               m_ulRef;
                CNode*              m_pHead;
                CNode*              m_pTail;
                BOOL                m_bAddMarker;
                ULONG_PTR           m_pulMarkerID;
                std::recursive_mutex    m_mutex;
                ULONG               m_length;

};

class CSampleQueue::ILockedSample
{
public:
    virtual HRESULT GetSample(
                            IMFSample** ppSample
                            )       = 0;
    virtual HRESULT Unlock(void)    = 0;
};