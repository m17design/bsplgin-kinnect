// dllmain.h : ģ�����������

class CKinectDeviceModule : public CAtlDllModuleT< CKinectDeviceModule >
{
public :
	DECLARE_LIBID(LIBID_KinectDeviceLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_KINECTDEVICE, "{15970A1D-FCB5-48D5-ABAC-5305858ADA7C}")
};

extern class CKinectDeviceModule _AtlModule;
