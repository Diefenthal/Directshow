#include "proppage.h"
#include <commctrl.h>


// constructor
CUdpFilterProp::CUdpFilterProp(IUnknown *pUnk) : 
  //initialization list
  CBasePropertyPage(NAME("UdpFilterProp"), pUnk, IDD_DIALOG1, IDS_TITLE)
  { 
	 CUdpFilterProp::m_pUdpMulticastIface=NULL;
     CUdpFilterProp::m_lMip=0;
     CUdpFilterProp::m_lNip=0;
     CUdpFilterProp::m_usPort=0;
}

// is called when the client creates the property page. It sets the IUnknown pointer to the filter.
HRESULT CUdpFilterProp::OnConnect(IUnknown *pUnk)
{
    if (pUnk == NULL)
    {
        return E_POINTER;
    }
    ASSERT(CUdpFilterProp::m_pUdpMulticastIface == NULL);
    return pUnk->QueryInterface(IID_IUdpMulticast, 
        reinterpret_cast<void**>(&(CUdpFilterProp::m_pUdpMulticastIface)));
}

// OnActivate is called when the dialog is created.
HRESULT CUdpFilterProp::OnActivate(void)
{

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_BAR_CLASSES;
    if (InitCommonControlsEx(&icc) == FALSE)
    {
        return E_FAIL;
    }

    ASSERT(CUdpFilterProp::m_pUdpMulticastIface != NULL);
	HRESULT hr;

	hr = CUdpFilterProp::m_pUdpMulticastIface->GetMulticastIp(&(CUdpFilterProp::m_lMip));
    
	if (SUCCEEDED(hr))
    {
        SendDlgItemMessage(CUdpFilterProp::m_Dlg, IDC_IPADDRESS1, IPM_SETADDRESS, 0,
           htonl(CUdpFilterProp::m_lMip) );
    }

	hr = CUdpFilterProp::m_pUdpMulticastIface->GetNicIp(&(CUdpFilterProp::m_lNip));
    
	if (SUCCEEDED(hr))
    {
        SendDlgItemMessage(CUdpFilterProp::m_Dlg, IDC_IPADDRESS2, IPM_SETADDRESS, 0,
          htonl(CUdpFilterProp::m_lNip) );
    }

	 hr = CUdpFilterProp::m_pUdpMulticastIface->GetPort(&(CUdpFilterProp::m_usPort));

	if (SUCCEEDED(hr))
    {
		SetDlgItemInt(CUdpFilterProp::m_Dlg,IDC_EDIT1,CUdpFilterProp::m_usPort,FALSE);
    }

    return hr;
}



// is called when the dialog receives a window message.
INT_PTR CUdpFilterProp::OnReceiveMessage(HWND hwnd,
    UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_NOTIFY:
            SetDirty();
            return (LRESULT) 1;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_EDIT1)
			{
				SetDirty();
				return (LRESULT) 1;
			}
	}


    // Let the parent class handle the message.
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

}


// OnApplyChanges is called when the user commits the property changes by clicking the OK or Apply button.

HRESULT CUdpFilterProp::OnApplyChanges()
{

	SendDlgItemMessage(CUdpFilterProp::m_Dlg, IDC_IPADDRESS1, IPM_GETADDRESS, 0,
				   (LPARAM)&(CUdpFilterProp::m_lNewMip) ); 
	
	SendDlgItemMessage(CUdpFilterProp::m_Dlg, IDC_IPADDRESS2, IPM_GETADDRESS, 0,
				   (LPARAM)&(CUdpFilterProp::m_lNewNip) );
	
    CUdpFilterProp::m_usNewPort = (u_short) GetDlgItemInt(CUdpFilterProp::m_Dlg, IDC_EDIT1, FALSE, FALSE);
	
	//CUdpFilterProp::m_usPort = CUdpFilterProp::m_cNewPort;

    CUdpFilterProp::m_lMip = htonl(CUdpFilterProp::m_lNewMip);  
    CUdpFilterProp::m_lNip = htonl(CUdpFilterProp::m_lNewNip);   
	CUdpFilterProp::m_usPort= CUdpFilterProp::m_usNewPort;
	return S_OK;
}

// OnDisconnect is called when the user dismisses the property sheet.
HRESULT CUdpFilterProp::OnDisconnect(void)
{
    if (CUdpFilterProp::m_pUdpMulticastIface)
    {
		CUdpFilterProp::m_pUdpMulticastIface->SetMulticastIp(CUdpFilterProp::m_lMip);
		CUdpFilterProp::m_pUdpMulticastIface->SetNicIp(CUdpFilterProp::m_lNip);
		CUdpFilterProp::m_pUdpMulticastIface->SetPortA(CUdpFilterProp::m_usPort);

        CUdpFilterProp::m_pUdpMulticastIface->Release();
        CUdpFilterProp::m_pUdpMulticastIface = NULL;
    }
    return S_OK;
}

