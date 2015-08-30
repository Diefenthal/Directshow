#include "udpfilter.h"

#ifndef __propp_h
#define __propp_h

#define PORTMAXCHAR 6

class CUdpFilterProp : public CBasePropertyPage
{
private:
    IUdpMulticast *m_pUdpMulticastIface;    // Pointer to the filter's custom interface.
    ULONG        m_lMip;     
    ULONG        m_lNewMip; 
    
	ULONG        m_lNip;     
    ULONG        m_lNewNip; 

    u_short     m_usPort;
	u_short     m_usNewPort;
	char       m_cPort[PORTMAXCHAR];
	char       m_cNewPort[PORTMAXCHAR];

	// helper function
	void CUdpFilterProp::SetDirty()
    {
        m_bDirty = TRUE;
        if (m_pPageSite)
        {
            m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    }


public:
    CUdpFilterProp(IUnknown *pUnk);

    HRESULT OnConnect(IUnknown *pUnk);
    HRESULT OnActivate();
	INT_PTR OnReceiveMessage(HWND, UINT, WPARAM, LPARAM );
    HRESULT OnApplyChanges();
	HRESULT OnDisconnect();

	static CUnknown * WINAPI CUdpFilterProp::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
	{
		CUdpFilterProp *pNewObject = new CUdpFilterProp(pUnk);
		if (pNewObject == NULL) 
		{
			*pHr = E_OUTOFMEMORY;
		}
		return pNewObject;
	} 


};

#endif