#include "HL_Usb.h"
#include "HL_Exception.h"

#ifdef TARGET_WIN
#include <devguid.h>

#define ADR_WINDOW_CLASS_NAME L"HIDDEN_USB_CHANGE_DELEGATE_WINDOWCLASS_%u"
#define ADR_WINDOW_NAME L"HIDDEN_USB_CHANGE_DELEGATE_WINDOW_%u"

UsbChangeListener::UsbChangeListener()
:mNotifyHandle(NULL), mHiddenWindow(NULL), mDelegate(NULL)
{
  wsprintfW(mWindowClassName, ADR_WINDOW_CLASS_NAME, (unsigned int)rand());
}

UsbChangeListener::~UsbChangeListener()
{
  stop();
}

void UsbChangeListener::setDelegate(Delegate* d)
{
  mDelegate = d;
}

UsbChangeListener::Delegate* UsbChangeListener::getDelegate() const
{
  return mDelegate;
}

void UsbChangeListener::start()
{
  // Exposing Window to Mixer
    WNDCLASSEXW wcx;
    memset( &wcx, 0, sizeof(WNDCLASSEXW) );
    wcx.cbSize = sizeof(WNDCLASSEXW);
    wcx.lpszClassName = mWindowClassName;
	wcx.lpfnWndProc = (WNDPROC)ADRWindowProc;
    ::RegisterClassExW(&wcx);

  wchar_t windowname[128]; 
  wsprintfW(windowname, ADR_WINDOW_NAME, rand());
    mHiddenWindow = CreateWindowW(	mWindowClassName,
							windowname,
							WS_POPUP | WS_DISABLED,
							0, 0, 0, 0,
							NULL, NULL, NULL, NULL );
	if (!mHiddenWindow)
    throw Exception(ERR_CREATEWINDOW, GetLastError());
  if (!SetWindowLongPtr(mHiddenWindow, GWLP_USERDATA, (LONG_PTR)this))
    throw Exception(ERR_CREATEWINDOW, GetLastError());

  // Adjust notification filter
  memset(&mNotificationFilter, 0, sizeof(mNotificationFilter));
 
  mNotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE); 
  mNotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
 
  // Register notification
  if (!RegisterDeviceNotification(mHiddenWindow, &mNotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES))
    throw Exception(ERR_REGISTERNOTIFICATION, GetLastError());
}

LRESULT CALLBACK UsbChangeListener::ADRWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == WM_DEVICECHANGE )
	{
    if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
    {
      LONG_PTR l = GetWindowLongPtr(hwnd, GWLP_USERDATA);
      if (l)
      {
        UsbChangeListener* obj = reinterpret_cast<UsbChangeListener*>(l);
        switch (wParam)
        {
        case DBT_DEVICEARRIVAL:
          obj->getDelegate()->onDeviceInsert(NULL);
          break;

        case DBT_DEVICEREMOVECOMPLETE:
          obj->getDelegate()->onDeviceRemove(NULL);
          break;
        }
      }
    }
	}
	return ::DefWindowProc( hwnd, uMsg, wParam, lParam);
}


void UsbChangeListener::stop()
{
  // Unregister
  if (mNotifyHandle != NULL)
  {
    ::UnregisterDeviceNotification(mNotifyHandle); 
    mNotifyHandle = NULL;
  }

  //Destroy the window
  if (mHiddenWindow != NULL)
  {
    ::DestroyWindow(mHiddenWindow);
    mHiddenWindow = NULL;

    ::UnregisterClassW(mWindowClassName, NULL);
  }
}

#endif
