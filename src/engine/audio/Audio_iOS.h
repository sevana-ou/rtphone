#ifndef __AUDIO_IOS
#define __AUDIO_IOS

class IosInputDevice: public InputDevice
{
protected:
	
public:
	IosInputDevice();
	~IosInputDevice();
	
	
	
	void open();
	void close();
};

class IosOutputDevice: public OutputDevice
{
protected:
public:
	IosOutputDevice();
	~IosOutputDevice();
	enum
	{
		Receiver,
		Speaker,
		Bluetooth
	};
	
	int route();
	void setRoute(int route);
	
	void open();
	void close();
};

#endif