//***************************************************************************************/
// 
// File name: Micro-Epsilon_scanCONTROL_M10PP3.cpp  
//
// Synopsis:  This program contains an example of 3d reconstruction by interfacing with
//            a MicroEpsilon ScanControl scanner.
//            See the PrintHeader() function below for detailed description.
//
// Copyright © 1992-2024 Zebra Technologies Corp. and/or its affiliates
// All Rights Reserved
//***************************************************************************************/
#include <mil.h>
#include "DataConversion.h"
#include "ProfileProcess.h"
#include "Micro-EpsilonToMIL.h"

//****************************************************************************
// Example description.
//****************************************************************************
void PrintHeader()
{
	MosPrintf(MIL_TEXT("[EXAMPLE NAME]\n")
	          MIL_TEXT("Micro-Epsilon_scanCONTROL_M10PP3\n\n")

	          MIL_TEXT("[SYNOPSIS]\n")
	          MIL_TEXT("This program acquires 3d data using a MicroEpsilon scanControl.\n")
	          MIL_TEXT("The 3d data from the scanner is grabbed using the GigeVision\n")
	          MIL_TEXT("interface provided by MicroEpsilon.\n\n")
             
	          MIL_TEXT("[MODULES USED]\n")
	          MIL_TEXT("Modules used: application, system, display, buffer, calibration,\n")
	          MIL_TEXT("              image processing, 3dMap.\n\n")
             MIL_TEXT("Press <Enter> to start.\n\n"));
   MosGetch();
}

//*****************************************************************************
// Acquisition parameter.
//*****************************************************************************
static const MIL_INT MIN_GRAB_TIMEOUT = 5000; // in ms
static const MIL_INT NB_PROFILES_PER_GRAB = 100;

static const MIL_DOUBLE CONVEYOR_SPEED = 0.05; // in mm/frame
static MIL_CONST_TEXT_PTR EXPECTED_DEVICE_VENDOR = MIL_TEXT("MICRO-EPSILON Optronic GmbH");
static const MIL_INT NB_MODELS = 2;
static MIL_STRING EXPECTED_DEVICE_MODEL[NB_MODELS] =
   {
   MIL_TEXT("scanCONTROL 26"),
   MIL_TEXT("scanCONTROL 29"),
   };


// Definition of the valid ranges in decreasing order.
static const MIL_INT NB_RANGE = 4;
static MIL_STRING EXPECTED_RANGE[NB_RANGE] =
   {
   MIL_TEXT("100"),
   MIL_TEXT("50"),
   MIL_TEXT("25"),
   MIL_TEXT("10")
   };

static const MIL_DOUBLE CONV_S[NB_RANGE] = {0.005, 0.002, 0.001, 0.0005};
static const MIL_DOUBLE CONV_OFF[NB_RANGE] = {250.0, 95.0, 65.0, 55.0};
static const SPCal CONVPCAL[NB_RANGE] =
   {
      {-32768 * CONV_S[0], -32768 * CONV_S[0] + CONV_OFF[0], CONV_S[0], CONV_S[0]},
      {-32768 * CONV_S[1], -32768 * CONV_S[1] + CONV_OFF[1], CONV_S[1], CONV_S[1]},
      {-32768 * CONV_S[2], -32768 * CONV_S[2] + CONV_OFF[2], CONV_S[2], CONV_S[2]},
      {-32768 * CONV_S[3], -32768 * CONV_S[3] + CONV_OFF[3], CONV_S[3], CONV_S[3]}
   };

static const SPRange PRANGE[NB_RANGE] =
   {
      {-143.5 / 2, 125.0, 143.5 / 2, 390.0},
      {-60.0  / 2, 65.0 , 60.0  / 2, 125.0},
      {-29.3  / 2, 53.0 , 29.3  / 2, 79.0},
      {-10.7  / 2, 52.5 , 10.7  / 2, 60.5}
   };

//*****************************************************************************
// Prototypes.
//*****************************************************************************
MIL_INT ChooseProfileMode();
bool VerifyDeviceCompatibility(MIL_ID MilDigitizer, MIL_INT* pCameraModeIndex);
MIL_INT SetupCamera(MIL_ID MilDigitizer, MIL_INT NbProfiles);
MIL_INT GetContainerResolution(MIL_ID MilDigitizer);

//*****************************************************************************
int MosMain(void)
   {
   PrintHeader();

   // Allocate the application.
   MIL_ID MilApplication = MappAlloc(M_DEFAULT, M_NULL);

   // Try to allocate the GigE Vision(R) system and digitizer.
   MappControl(M_ERROR, M_PRINT_DISABLE);
   MIL_ID MilSystem = MsysAlloc(M_SYSTEM_GIGE_VISION, M_DEFAULT, M_DEFAULT, M_NULL);
   MIL_ID MilDigitizer = MdigAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, M_NULL);
   
   // Verify the compatibility of the camera and get the model index.
   MIL_INT CameraModelIndex;
   if(MappGetError(M_CURRENT, M_NULL) == M_NULL_ERROR && VerifyDeviceCompatibility(MilDigitizer, &CameraModelIndex))
      {
      // Choose the profile process.
      MIL_INT NbProfiles = ChooseProfileMode();

      // Set up the camera mode for the example.
      MIL_INT ProfileSize = SetupCamera(MilDigitizer, NbProfiles);
      
      // Allocate the grab buffers.
      MIL_ID MilGrabBuffers[2];
      MIL_INT ImageSizeX = MdigInquire(MilDigitizer, M_SIZE_X, M_NULL);
      MIL_INT ImageSizeY = MdigInquire(MilDigitizer, M_SIZE_Y, M_NULL);
      MIL_INT ImageSizeBit = MdigInquire(MilDigitizer, M_SIZE_BIT, M_NULL);
      MbufAlloc2d(MilSystem, ImageSizeX, ImageSizeY, ImageSizeBit + M_UNSIGNED,
                  M_IMAGE + M_PROC + M_GRAB + M_DISP, &MilGrabBuffers[0]);
      MbufAlloc2d(MilSystem, ImageSizeX, ImageSizeY, ImageSizeBit + M_UNSIGNED,
                  M_IMAGE + M_PROC + M_GRAB + M_DISP, &MilGrabBuffers[1]);

      // Try grabbing an image.
      MdigGrab(MilDigitizer, MilGrabBuffers[0]);

      if(MappGetError(M_CURRENT, M_NULL) == M_NULL_ERROR)
         {
         MappControl(M_ERROR, M_PRINT_ENABLE);

         // Allocate the profile processing object.
         CProfileProcess* pProfileProcess;
         if(NbProfiles == 1)
            pProfileProcess = new CProfileSingleProcess(MilSystem, CONVPCAL[CameraModelIndex],
                                                        PRANGE[CameraModelIndex], ProfileSize);
         else
            pProfileProcess = new CProfileDepthMapProcess(MilSystem, CONVPCAL[CameraModelIndex],
                                                          PRANGE[CameraModelIndex], 0.0,
                                                          CONVEYOR_SPEED, ProfileSize, NB_PROFILES_PER_GRAB);

         // Allocate the interface between MicroEpsilon and MIL.
         CMicroEpsilonToMIL MicroEpsilonToMILInterface(ProfileSize, NbProfiles);
         MicroEpsilonToMILInterface.BuildInterface(MilDigitizer, pProfileProcess);

         // Process 3d data.
         MdigProcess(MilDigitizer, MilGrabBuffers, 2, M_START, M_DEFAULT,
                     CMicroEpsilonToMIL::MilInterfaceHook, &MicroEpsilonToMILInterface);

         // Wait for the user to stop the grab and terminate the application.
         MosPrintf(MIL_TEXT("Press <Enter> to end.\n\n"));
         MosGetch();
         MdigProcess(MilDigitizer, MilGrabBuffers, 2, M_STOP, M_DEFAULT,
                     CMicroEpsilonToMIL::MilInterfaceHook, &MicroEpsilonToMILInterface);

         // Free the profile process.
         delete pProfileProcess;
         }
      else
         {
         MosPrintf(MIL_TEXT("Unable to grab from the MicroEpsilon scanControl camera!\n")
                   MIL_TEXT("Verify the configuration of the camera:\n")
                   MIL_TEXT("   - Profile frequency vs measuring field.\n")
                   MIL_TEXT("   - ...\n")
                   MIL_TEXT("The default configuration of the camera upon powerup should work.\n\n")
                   MIL_TEXT("Press <Enter> to end.\n"));
         MosGetch();
         }      

      // Free the grab buffers
      MbufFree(MilGrabBuffers[1]);
      MbufFree(MilGrabBuffers[0]);
      }
   else
      {
      MosPrintf(MIL_TEXT("The example requires the following conditions to run:\n")
                MIL_TEXT("   - MIL GigE Vision(R) driver must be installed.\n")
                MIL_TEXT("   - An actual MicroEpsilon scanControl must be\n")
                MIL_TEXT("     connected to your computer or network.\n\n")
                MIL_TEXT("Press <Enter> to end.\n"));
      MosGetch();
      }

   // Free the MIL objects.
   MdigFree(MilDigitizer);
   MsysFree(MilSystem);
   MappFree(MilApplication);

	return 0;
   }

//*****************************************************************************
// Ask the user to choose a profile mode. Allocate the profile process.
//*****************************************************************************
MIL_INT ChooseProfileMode()
   {
   while(1)
      {
      MosPrintf(MIL_TEXT("Please choose the profile mode of the example.\n")
                MIL_TEXT("   a. Single profile mode.\n")
                MIL_TEXT("   b. Multiple profiles mode.\n\n"));
      switch(MosGetch())
         {
         case MIL_TEXT('a'):
         case MIL_TEXT('A'):
            MosPrintf(MIL_TEXT("Single profile mode\n")
                      MIL_TEXT("--------------------\n")
                      MIL_TEXT("The camera is configured to transmit one profile per frame.\n")
                      MIL_TEXT("The profile is first converted to 3d points. The points are then displayed in\n")
                      MIL_TEXT("the camera world system. The displayed gray region represents the bounding\n")
                      MIL_TEXT("area of meaurement specified by the scanControl camera.\n\n"));
            return 1;
            break;
           
         case MIL_TEXT('b'):
         case MIL_TEXT('B'):            
            MosPrintf(MIL_TEXT("Multiple profiles mode\n")
                      MIL_TEXT("-----------------------\n")
                      MIL_TEXT("The camera is configured to transmit multiple profiles per frame.\n")
                      MIL_TEXT("The profiles are first converted to a MIL 3d point cloud. The point cloud\n")
                      MIL_TEXT("is then extracted into a calibrated depth map that is displayed.\n\n"));
            return NB_PROFILES_PER_GRAB;
         default:
            break;
         }
      } 
   }

//*****************************************************************************
// Checks whether the GigE Vision(R) camera used is expected.
//*****************************************************************************
bool VerifyDeviceCompatibility(MIL_ID MilDigitizer, MIL_INT* pCameraRangeIndex)
   {
   *pCameraRangeIndex = -1;

   // Get the device vendor.
   MIL_INT64 DeviceVendorNameStringSize = 0;
   MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE + M_STRING_SIZE, MIL_TEXT("DeviceVendorName"),
                      M_TYPE_INT64, &DeviceVendorNameStringSize);
   MIL_TEXT_PTR DeviceVendorNamePtr = new MIL_TEXT_CHAR[(MIL_INT)DeviceVendorNameStringSize];
   MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceVendorName"),
                      M_TYPE_STRING, DeviceVendorNamePtr);

   if(MosStrcmp(DeviceVendorNamePtr, EXPECTED_DEVICE_VENDOR) == 0)
      {
      // Get the device model.
      MIL_INT64 DeviceModelNameStringSize = 0;
      MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE + M_STRING_SIZE, MIL_TEXT("DeviceModelName"),
                         M_TYPE_INT64, &DeviceModelNameStringSize);
      MIL_TEXT_PTR DeviceModelNamePtr = new MIL_TEXT_CHAR[(MIL_INT)DeviceModelNameStringSize];
      MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceModelName"),
                         M_TYPE_STRING, &DeviceModelNamePtr[0]);

      // Check the model.
      MIL_STRING DeviceModelName(DeviceModelNamePtr);
      for(MIL_INT m = 0; m < NB_MODELS; m++)
         {
         size_t pos = DeviceModelName.find(EXPECTED_DEVICE_MODEL[m]);
         if(pos != MIL_STRING::npos)
            {
            // Verify that the model name is long enough.
            MIL_INT MaxRangeName = EXPECTED_RANGE[0].length();
            if(DeviceModelName.length() >= (pos + EXPECTED_DEVICE_MODEL[m].length() + MaxRangeName))
               {
               // Check the range.
               MIL_STRING RangeName = DeviceModelName.substr(pos + EXPECTED_DEVICE_MODEL[m].length() + MaxRangeName);
               for(MIL_INT m = 0; m < NB_RANGE; m++)
                  {
               if(RangeName.find(EXPECTED_RANGE[m]) != MIL_STRING::npos)
                     {
                     *pCameraRangeIndex = m;
                     break;
                     }
                  }
               }
            break;
            }
         }
      
      delete[] DeviceModelNamePtr;
      }

   delete[] DeviceVendorNamePtr;

   return *pCameraRangeIndex >= 0;
   }

//*****************************************************************************
// SetupCamera. Minimal setup of the camera to be in a mode that is easy to
//              interface with MIL.
//*****************************************************************************
MIL_INT SetupCamera(MIL_ID MilDigitizer, MIL_INT NbProfiles)
   {
   // Set the camera in container mode.
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TransmissionType"),
                      M_TYPE_STRING, MIL_TEXT("TypeContainer"));
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"),
                      M_TYPE_STRING, MIL_TEXT("Mono16"));

   // Enable the Z and the X content and disable the others.
   MIL_BOOL Enable = M_TRUE;
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerZ"), M_TYPE_BOOLEAN, &Enable);
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerX"), M_TYPE_BOOLEAN, &Enable);
   Enable = M_FALSE;
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerH"), M_TYPE_BOOLEAN, &Enable);
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerW"), M_TYPE_BOOLEAN, &Enable);
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerM01"), M_TYPE_BOOLEAN, &Enable);
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerM02"), M_TYPE_BOOLEAN, &Enable);
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerTS"), M_TYPE_BOOLEAN, &Enable);

   // Set the packet size to the value recommended by MicroEpsilon.
   MIL_INT64 GevSCPSPacketSize = 356;
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("GevSCPSPacketSize"),
                      M_TYPE_INT64, &GevSCPSPacketSize);

   // Always enable the calibration of the data.
   Enable = M_TRUE;
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("Calibration"), M_TYPE_BOOLEAN, &Enable);

   // Get the size of the profile.
   MIL_INT ProfileSize = GetContainerResolution(MilDigitizer);

   // Set the grab image accordingly.
   MIL_INT64 Width = ProfileSize * 2;
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("Width"), M_TYPE_INT64, &Width);
   MIL_INT64 Height = NbProfiles;
   MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("Height"), M_TYPE_INT64, &Height);

   // Get the profile rate of the camera.
   MIL_DOUBLE ProfileFrequencyInHz = 0;
   MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("AcquisitionFrameRate"),
                      M_TYPE_DOUBLE, &ProfileFrequencyInHz);

   // Set the grab timeout.
   MIL_DOUBLE GrabTimeout = 2 * Height * 1000.0 / ProfileFrequencyInHz;
   GrabTimeout = GrabTimeout < MIN_GRAB_TIMEOUT ? MIN_GRAB_TIMEOUT : GrabTimeout;
   MdigControl(MilDigitizer, M_GRAB_TIMEOUT, GrabTimeout);
   
   return ProfileSize;
   }

//*****************************************************************************
// GetContainerResolution. Gets the resolution of the container.
//*****************************************************************************
MIL_INT GetContainerResolution(MIL_ID MilDigitizer)
   {  
   MIL_INT ContainerResolution = 0;
   MIL_INT64 EntryToRead = 0; 
   MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ContainerResolution"),
                      M_TYPE_INT64, &EntryToRead);

   MIL_INT Count = 0;
   MdigInquireFeature(MilDigitizer, M_FEATURE_ENUM_ENTRY_COUNT, MIL_TEXT("ContainerResolution"),
                      M_TYPE_MIL_INT, &Count);
   if(Count)
      {
      for(MIL_INT i = 0; i < Count; i++)
         {
         MIL_INT64 CurEntry;
         MdigInquireFeature(MilDigitizer, M_FEATURE_ENUM_ENTRY_VALUE + i, MIL_TEXT("ContainerResolution"),
                            M_TYPE_INT64, &CurEntry);

         if(EntryToRead == CurEntry)
            {
            MIL_INT64 StringSize;
            MdigInquireFeature(MilDigitizer, M_FEATURE_ENUM_ENTRY_DISPLAY_NAME + i + M_STRING_SIZE,
                               MIL_TEXT("ContainerResolution"), M_TYPE_INT64, &StringSize);
            MIL_TEXT_PTR ContainerResolutionString = new MIL_TEXT_CHAR[(MIL_INT)StringSize];
            MdigInquireFeature(MilDigitizer, M_FEATURE_ENUM_ENTRY_DISPLAY_NAME + i,
                               MIL_TEXT("ContainerResolution"), M_TYPE_STRING, ContainerResolutionString);
            MIL_STRING_STREAM Stream(ContainerResolutionString);
            Stream >> ContainerResolution;
            delete [] ContainerResolutionString;
            break;
            }
         }
      }

   return ContainerResolution;
   }
