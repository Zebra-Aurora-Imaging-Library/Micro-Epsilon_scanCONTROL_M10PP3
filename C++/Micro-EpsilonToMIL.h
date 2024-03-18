/************************************************************************************/
/*
* File name: Micro-EpsilonToMIL.h
*
* Synopsis:  This file contains the declaration of the CMicroEpsilonToMIL class that
*            interfaces the grab of the MicroEpsilon profile container buffer to
*            MIL. It makes the necessary band extraction and data conversion before
*            calling the profile processing on the data.
*
* Copyright © 1992-2024 Zebra Technologies Corp. and/or its affiliates
* All Rights Reserved
*/

#ifndef MICRO_EPSILON_TO_MIL_H
#define MICRO_EPSILON_TO_MIL_H

// Forward declares.
class CDataConversion;
class CProfileProcess;

class CMicroEpsilonToMIL
   {
   public:
      CMicroEpsilonToMIL(MIL_INT SizeX, MIL_INT SizeY);
      virtual ~CMicroEpsilonToMIL();

      static MIL_INT MFTYPE MilInterfaceHook(MIL_INT HookType, MIL_ID MilEvent, void *pUserData);
      void BuildInterface(MIL_ID MilDigitizer, CProfileProcess* pProfileProcess);

   private:

      void BuildDigitizerDataConversion(MIL_ID MilDigitizer);
      MIL_INT MilInterface(MIL_INT HookType, MIL_ID MilEvent);

      CDataConversion* m_pDataConversion;
      CProfileProcess* m_pProfileProcess;
      MIL_INT m_SizeX;
      MIL_INT m_SizeY;
   };

#endif // MICRO_EPSILON_TO_MIL_H
