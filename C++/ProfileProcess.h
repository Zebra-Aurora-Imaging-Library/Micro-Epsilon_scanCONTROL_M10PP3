/************************************************************************************/
/*
* File name: ProfileProcess.h
*
* Synopsis:  This file contains the declaration of the CProfileProcess classes that
*            are processing some profile data(SPdata). 
*
* Copyright © 1992-2024 Zebra Technologies Corp. and/or its affiliates
* All Rights Reserved
*/
#include <vector>
#include "DataConversion.h"

#ifndef PROFILE_PROCESS_H
#define PROFILE_PROCESS_H

// DirectX display is only available under Windows.
#if M_MIL_USE_WINDOWS && !M_MIL_USE_RT
#define USE_D3D_DISPLAY  1
#include "MdispD3D.h"
#endif

//*****************************************************************************
// Structure defining the range of the sensor profile.
//*****************************************************************************
struct SPRange
   {
   MIL_DOUBLE MinX;
   MIL_DOUBLE MinZ;
   MIL_DOUBLE MaxX;
   MIL_DOUBLE MaxZ;
   };

//*****************************************************************************
// Base class of a processing to apply to some profile data.
//*****************************************************************************
class CProfileProcess
   {
   public:
      CProfileProcess(const SPCal& PCal, MIL_INT NbPoints);
      virtual ~CProfileProcess();
      virtual void Process(const SPData& Data) = 0;

   protected:
      MIL_UINT m_NbPoints;
      SPCal m_PCal;
      CDataConversion* m_pProcessProfileDataConversion;
   };

//*****************************************************************************
// Base class of a processing to apply to some profile data
// that requires to convert the profiles to 3d points.
//*****************************************************************************
class CProfile3dPointsProcess: public CProfileProcess
   {
   public:
      CProfile3dPointsProcess(MIL_ID MilSystem, const SPCal& PCal,
                              MIL_INT ProfileSize, MIL_INT NbProfiles);
   };

//*****************************************************************************
// Processing to be applied to the 3d points of a single profile.
//*****************************************************************************
class CProfileSingleProcess: public CProfile3dPointsProcess
   {
   public:
      CProfileSingleProcess(MIL_ID MilSystem, const SPCal& ConvertPCal,
                            const SPRange& DataRange, MIL_INT ProfileSize);
      virtual ~CProfileSingleProcess();
      virtual void Process(const SPData& Data);
   private:
      MIL_ID m_MilDisplay;
      MIL_ID m_MilGraList;
      MIL_ID m_MilDisplayedImage;
   };

//*****************************************************************************
// Processing to be applied to the 3d points of a multiple 
// profile in order to create and display a depth map.
//*****************************************************************************
class CProfileDepthMapProcess : public CProfile3dPointsProcess
   {
   public:
      CProfileDepthMapProcess(MIL_ID MilSystem, const SPCal& PCal, const SPRange& DataRange,
                              MIL_DOUBLE WorldPosY, MIL_DOUBLE ConveyorSpeed,
                              MIL_INT ProfileSize, MIL_INT NbProfiles);
      virtual ~CProfileDepthMapProcess();
      virtual void Process(const SPData& Data);

   private:
      MIL_ID  m_MilDisplay;
      MIL_ID  m_MilPointCloudContainer;
      MIL_ID  m_MilDepthMap;
      MIL_ID  m_MilDisplayLut;
      std::vector<MIL_FLOAT> m_ConvertedY;
#if USE_D3D_DISPLAY
      MIL_DISP_D3D_HANDLE m_3DDispHandle;
#endif
      MIL_INT m_NbFramesProcessed;
   };

#endif // PROFILE_PROCESS_H
