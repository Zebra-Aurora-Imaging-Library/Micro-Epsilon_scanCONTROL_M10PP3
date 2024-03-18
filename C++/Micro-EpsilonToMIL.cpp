/************************************************************************************/
/*
* File name: Micro-EpsilonToMIL.cpp
*
* Synopsis:  This file contains the implementation of the CMicroEpsilonToMIL class that
*            interfaces the grab of the MicroEpsilon profile container buffer to
*            MIL. It makes the necessary band extraction and data conversion before
*            calling the profile processing on the data.
*
* Copyright © 1992-2024 Zebra Technologies Corp. and/or its affiliates
* All Rights Reserved
*/

#include <mil.h>
#include "ProfileProcess.h"
#include "Micro-EpsilonToMIL.h"

//*****************************************************************************
// Constants.
//*****************************************************************************
static MIL_DOUBLE INVALID_VALUE = 0;

//*****************************************************************************
// Constructor.
//*****************************************************************************
CMicroEpsilonToMIL::CMicroEpsilonToMIL(MIL_INT SizeX, MIL_INT SizeY)
   : m_SizeX(SizeX), m_SizeY(SizeY), m_pDataConversion(0)
   {
   }

//*****************************************************************************
// Destructor.
//*****************************************************************************
CMicroEpsilonToMIL::~CMicroEpsilonToMIL()
   {
   if(m_pDataConversion)
      delete m_pDataConversion;
   }

//*****************************************************************************
// BuildInterface. Builds the interface between the MicroEpsilon data and 
//                 MIL. Gets the profile processing to perform and build the
//                 data conversion.
//*****************************************************************************
void CMicroEpsilonToMIL::BuildInterface(MIL_ID MilDigitizer, CProfileProcess* pProfileProcess)
   {
   // Set the Profile process to apply to the acquired data.
   m_pProfileProcess = pProfileProcess;

   // Build the data conversion due to the digitizer.
   BuildDigitizerDataConversion(MilDigitizer);
   }

//*****************************************************************************
// BuildDigitizerDataConversion. Creates the CDataConversion objects that
//                               will put the data provided by the digitizer
//                               in a format that can be processed by the profile
//                               process.
//*****************************************************************************
void CMicroEpsilonToMIL::BuildDigitizerDataConversion(MIL_ID MilDigitizer)
   {
   // Convert the data to have a valid mask.
   MIL_ID MilSystem = MdigInquire(MilDigitizer, M_OWNER_SYSTEM, M_NULL);
   m_pDataConversion = new CDataConversionAddMask(m_pDataConversion, MilSystem,
                                                  m_SizeX, m_SizeY, INVALID_VALUE);

   // Flip the X position values if necessary.
   MIL_BOOL FlipPosition = M_FALSE;
   MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("FlipPos"), M_TYPE_BOOLEAN, &FlipPosition);
   if(FlipPosition)
      m_pDataConversion = new CDataConversionFlipXVal(m_pDataConversion);

   // Flip the Z position values if necessary.
   MIL_BOOL FlipDistance = M_FALSE;
   MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("FlipDist"), M_TYPE_BOOLEAN, &FlipDistance);
   if(FlipDistance)
      m_pDataConversion = new CDataConversionFlipZVal(m_pDataConversion);
   }

//*****************************************************************************
// MilInterfaceHook. Hook function called by the MdigProcess function.
//*****************************************************************************
MIL_INT MFTYPE CMicroEpsilonToMIL::MilInterfaceHook(MIL_INT HookType, MIL_ID MilEvent, void *pUserData)
   {
   CMicroEpsilonToMIL* pInterface = (CMicroEpsilonToMIL*)pUserData;
   return pInterface->MilInterface(HookType, MilEvent);
   }

//*****************************************************************************
// MilInterface. Actual interface function that is being called at each MdigProcess
//               hook. Separates, converts and processes the data.
//*****************************************************************************
MIL_INT CMicroEpsilonToMIL::MilInterface(MIL_INT HookType, MIL_ID MilEvent)
   {
   // Get the grab buffer.
   MIL_ID MilGrabBuffer;
   MdigGetHookInfo(MilEvent, M_MODIFIED_BUFFER + M_BUFFER_ID, &MilGrabBuffer);

   // Separate the Z and X data buffers into child buffers.
   SPData Data;
   Data.MilZ = MbufChild2d(MilGrabBuffer, 0, 0, m_SizeX, m_SizeY, M_NULL);
   Data.MilX = MbufChild2d(MilGrabBuffer, m_SizeX, 0, m_SizeX, m_SizeY, M_NULL);

   // Convert the data.
   SPData ConvertedData = m_pDataConversion ? m_pDataConversion->Convert(Data) : Data;

   // Process the data.
   m_pProfileProcess->Process(ConvertedData);

   // Free the child buffers.
   Data.ReleaseData();

   return 0;
   }
