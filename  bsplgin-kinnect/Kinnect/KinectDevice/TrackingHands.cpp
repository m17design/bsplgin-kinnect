#include "stdafx.h"
#include "TrackingHands.h"
#include "ControlKinect.h"
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnHash.h>
#include <XnLog.h>
// Header for NITE
#include "XnVNite.h"
#include "XnVDepthMessage.h"
#include <XnVHandPointContext.h>
#define  buffer_size 20
XnVSessionManager* g_pSessionManager;
XnVFlowRouter* g_pFlowRouter;
SessionState g_SessionState = NOT_IN_SESSION;
XnVPointDrawer* g_pDrawer;
//////////////////////////////////////////////////////////////////////////

XnVPointDrawer::XnVPointDrawer(XnUInt32 nHistory, xn::DepthGenerator depthGenerator) :
XnVPointControl("XnVPointDrawer"),
m_nHistorySize(nHistory), m_DepthGenerator(depthGenerator), m_bDrawDM(false), m_bFrameID(false)
{
	m_handCnt=0;
	m_pfPositionBuffer_1 = new XnFloat[nHistory*3];
	m_pfPositionBuffer_2 = new XnFloat[nHistory*3];
	m_handID=0;
}

// Destructor. Clear all data structures
XnVPointDrawer::~XnVPointDrawer()
{
	std::map<XnUInt32, std::list<XnPoint3D> >::iterator iter;
	for (iter = m_History.begin(); iter != m_History.end(); ++iter)
	{
		iter->second.clear();
	}
	m_History.clear();
	delete []m_pfPositionBuffer_1;
	delete []m_pfPositionBuffer_2;
}

// Change whether or not to draw the depth map
void XnVPointDrawer::SetDepthMap(XnBool bDrawDM)
{
	m_bDrawDM = bDrawDM;
}
// Change whether or not to print the frame ID
void XnVPointDrawer::SetFrameID(XnBool bFrameID)
{
	m_bFrameID = bFrameID;
}


void XnVPointDrawer::OnPointCreate(const XnVHandPointContext* cxt)
{
	printf("** %d\n", cxt->nID);
	// Create entry for the hand
	m_History[cxt->nID].clear();
	OnPointUpdate(cxt);
}
// Handle new position of an existing hand
void XnVPointDrawer::OnPointUpdate(const XnVHandPointContext* cxt)
{
	// positions are kept in projective coordinates, since they are only used for drawing
	XnPoint3D ptProjective(cxt->ptPosition);
	m_DepthGenerator.ConvertRealWorldToProjective(1, &ptProjective, &ptProjective);
	// Add new position to the history buffer
	m_History[cxt->nID].push_front(ptProjective);
	// Keep size of history buffer
	if (m_History[cxt->nID].size() > m_nHistorySize)

		m_History[cxt->nID].pop_back();
}

// Handle destruction of an existing hand
void XnVPointDrawer::OnPointDestroy(XnUInt32 nID)
{
	// No need for the history buffer
	m_History.erase(nID);
}

// Handle a new Message
void XnVPointDrawer::Update(XnVMessage* pMessage)
{
	// PointControl's Update calls all callbacks for each hand
	XnVPointControl::Update(pMessage);
	//todo: 	
	std::map<XnUInt32, std::list<XnPoint3D> >::const_iterator PointIterator;
	// Go over each existing hand
	for (PointIterator = m_History.begin();
		PointIterator != m_History.end();
		++PointIterator)
	{
		XnUInt32 nPoints = 0;
		XnUInt32 Id = PointIterator->first;
		XnUInt32 i=0;
		m_handID=Id;
		XnUInt32 pId=GetPrimaryID();
		std::list<XnPoint3D>::const_iterator PositionIterator;
		for (PositionIterator=PointIterator->second.begin();PositionIterator != PointIterator->second.end();++PositionIterator, ++i)
		{
			XnPoint3D pt(*PositionIterator);
			if(Id==pId){
				m_pfPositionBuffer_1[3*i] = pt.X;
				m_pfPositionBuffer_1[3*i + 1] = pt.Y;
				m_pfPositionBuffer_1[3*i + 2] = pt.Z;	
			}else{
				m_pfPositionBuffer_2[3*i] = pt.X;
				m_pfPositionBuffer_2[3*i + 1] = pt.Y;
				m_pfPositionBuffer_2[3*i + 2] = pt.Z;	
			}
		}
	}
	m_TouchingFOVEdge.clear();
}
///////////////////////////////end pointdraw///////////////////////////////////////////

// Callback for when the focus is in progress
void XN_CALLBACK_TYPE FocusProgress(const XnChar* strFocus, const XnPoint3D& ptPosition, XnFloat fProgress, void* UserCxt)
{
		printf("Focus progress: %s @(%f,%f,%f): %f\n", strFocus, ptPosition.X, ptPosition.Y, ptPosition.Z, fProgress);
}
// callback for session start
void XN_CALLBACK_TYPE SessionStarting(const XnPoint3D& ptPosition, void* UserCxt)
{
	printf("Session start: (%f,%f,%f)\n", ptPosition.X, ptPosition.Y, ptPosition.Z);
	g_SessionState = IN_SESSION;
	writeHandStatusField(g_SessionState);
}
// Callback for session end
void XN_CALLBACK_TYPE SessionEnding(void* UserCxt)
{
	printf("Session end\n");
	g_SessionState = NOT_IN_SESSION;
	writeHandStatusField(g_SessionState);
}
void XN_CALLBACK_TYPE NoHands(void* UserCxt)
{
	if (g_SessionState != NOT_IN_SESSION)
	{
		printf("Quick refocus\n");
		g_SessionState = QUICK_REFOCUS;
		writeHandStatusField(g_SessionState);
	}
}

void XN_CALLBACK_TYPE TouchingCallback(xn::HandTouchingFOVEdgeCapability& generator, XnUserID id, const XnPoint3D* pPosition, XnFloat fTime, XnDirection eDir, void* pCookie)
{

}

void XN_CALLBACK_TYPE GestureIntermediateStageCompletedHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
	printf("Gesture %s: Intermediate stage complete (%f,%f,%f)\n", strGesture, pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE GestureReadyForNextIntermediateStageHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
	printf("Gesture %s: Ready for next intermediate stage (%f,%f,%f)\n", strGesture, pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE GestureProgressHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie)
{
	printf("Gesture %s progress: %f (%f,%f,%f)\n", strGesture, fProgress, pPosition->X, pPosition->Y, pPosition->Z);
}
VOID updateHands(){
	g_pSessionManager->Update(&g_context);
}

SessionState GetSessionStatus(){
	return g_SessionState;
}
XnUInt32 GetBufferSize(){
	return g_pDrawer->m_nHistorySize;
}

XnUInt32 GetHandCnt(){
	return g_pDrawer->m_handCnt;
}
XnUInt32 GetHandID(){
	return g_pDrawer->m_handID;
}
XnStatus init_hands(){
	XnStatus rc = XN_STATUS_OK;
	XnCallbackHandle h;
	if (g_hands.IsCapabilitySupported(XN_CAPABILITY_HAND_TOUCHING_FOV_EDGE))
	{
		g_hands.GetHandTouchingFOVEdgeCap().RegisterToHandTouchingFOVEdge(TouchingCallback, NULL, h);
	}
	XnCallbackHandle hGestureIntermediateStageCompleted, hGestureProgress, hGestureReadyForNextIntermediateStage;
	g_gesture.RegisterToGestureIntermediateStageCompleted(GestureIntermediateStageCompletedHandler, NULL, hGestureIntermediateStageCompleted);
	g_gesture.RegisterToGestureReadyForNextIntermediateStage(GestureReadyForNextIntermediateStageHandler, NULL, hGestureReadyForNextIntermediateStage);
	g_gesture.RegisterGestureCallbacks(NULL, GestureProgressHandler, NULL, hGestureProgress);
	g_pSessionManager = new XnVSessionManager;
	rc = g_pSessionManager->Initialize(&g_context, "Click,Wave", "RaiseHand");
	CHECK_RC(rc, "SessionManager::Initialize");
	g_pSessionManager->RegisterSession(NULL, SessionStarting, SessionEnding, FocusProgress);
//////////////////////////////////////////////////////////////////////////
	g_pDrawer= new XnVPointDrawer(buffer_size, g_depth); 
	g_pFlowRouter = new XnVFlowRouter;
	g_pFlowRouter->SetActive(g_pDrawer);
	g_pSessionManager->AddListener(g_pFlowRouter);
	g_pDrawer->RegisterNoPoints(NULL, NoHands);
	g_pDrawer->SetDepthMap(true);
	return rc;
}

XnStatus GetHandTrackPoint(int i,float** vec){
	if(i==1){	
		*vec=g_pDrawer->m_pfPositionBuffer_1;
	}
	if(i==2){
		*vec=g_pDrawer->m_pfPositionBuffer_2;
	}
	return XN_STATUS_OK;
}


