#ifndef __HELPER_USB_H
#define __HELPER_USB_H

#ifdef TARGET_WIN

#include <winsock2.h>
#include <windows.h>
#include <Dbt.h>

  class UsbChangeListener
  {
  public:
    class Delegate
    {
    public:
      virtual void onDeviceInsert(const wchar_t* name) = 0;
      virtual void onDeviceRemove(const wchar_t* name) = 0;
    };
    
    UsbChangeListener();
    ~UsbChangeListener();
    
    void setDelegate(Delegate* d);
    Delegate* getDelegate() const;

    void start();
    void stop();

  protected:
    HDEVNOTIFY                    mNotifyHandle;            /// Handle to track notifications about USB insert/removal.
    HWND                          mHiddenWindow;            /// Hidden window to receive notifications
    DEV_BROADCAST_DEVICEINTERFACE mNotificationFilter;      /// Notifications filter
    wchar_t                       mWindowClassName[256];       /// Hidden window class
    Delegate*                     mDelegate;                 /// Event handler pointer

    /// Hidden window procedure.
    /// @param hwnd Window handle
    /// @param uMsg Message ID
    /// @param wParam First param
    /// @param lParam Second param
    static LRESULT CALLBACK ADRWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
  };


#endif

#endif