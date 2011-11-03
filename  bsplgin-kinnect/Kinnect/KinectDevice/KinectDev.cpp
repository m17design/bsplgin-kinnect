// KinectDev.cpp : CKinectDev ��ʵ��

#include "stdafx.h"
#include "ControlKinect.h"
#include "QueryNode.h"
#include "KinectDev.h"
#include "blaxxunVRML.h"
#include <iostream>

CKinectDev::CKinectDev():
m_user_com(NULL),
m_status(NULL),
m_skeletonNode(NULL),
m_floorNode(NULL),
m_handsTrack(NULL)
{

}
CKinectDev::~CKinectDev(){
	Close_Kinect();
}

STDMETHODIMP CKinectDev::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IbxxHID
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CKinectDev::Init(BSTR Device, int DeviceNo, Browser *pBrowser, int *pDeviceNoUsed)
{	
	DeviceNo=1;
	*pDeviceNoUsed= 1;
	return  S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectDev::AddDeviceSensor(BSTR eventType, Node *pEventNode, EventInSFBool *pIsActive, BOOL Enabled, int ID, int *pRetVal)
{
	if(Enabled){
		Node *skltn,*hnz,*flr;
		QuerySFNode(pEventNode,L"skeleton",IID_EventOutSFNode,&m_skeletonNode,&skltn);
		QuerySFNode(pEventNode,L"handsTrack",IID_EventOutSFNode,&m_handsTrack,&hnz);
		QuerySFNode(pEventNode,L"floor",IID_EventOutSFNode,&m_floorNode,&flr);
		QuerySFNode(skltn,L"enabled",IID_EventOutSFBool,&m_enabledSkltn);
		QuerySFNode(hnz,L"enabled",IID_EventOutSFBool,&m_enabledHnz);
		skltn->Release();
		hnz->Release();
		flr->Release();
		skltn=hnz=flr=NULL;
		if (Init_Kinect(m_skeletonNode,m_handsTrack,m_floorNode)!=XN_STATUS_OK)
		{
			Close_Kinect();
			return E_FAIL;
		}
	
	}
	*pRetVal=1;
	return  S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectDev::RemoveDeviceSensor(int ID){
	return  S_OK;
}


HRESULT STDMETHODCALLTYPE CKinectDev::Tick(double SimTime, double FrameRate){
	VARIANT_BOOL vlu,vlu1;
	m_enabledSkltn->getValue(&vlu);
	m_enabledHnz->getValue(&vlu1);
	updateKinect(vlu,vlu1);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectDev::EnabledChanged(int ID, BOOL Enabled, BOOL *pSetIsActive){
	*pSetIsActive=true;
	return  S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectDev::FocusChanged(BOOL HasFocusNow, BOOL *pNeedTickCalls){
	*pNeedTickCalls=true;
	return  S_OK;
}
