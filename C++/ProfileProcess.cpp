/************************************************************************************/
/*
* File name: ProfileProcess.cpp
*
* Synopsis:  This file contains the implementation of the CProfileProcess classes that
*            are processing some profile data (SPdata).
*
* Copyright © 1992-2024 Zebra Technologies Corp. and/or its affiliates
* All Rights Reserved
*/
#include <mil.h>
#include <vector>
#include "DataConversion.h"
#include "ProfileProcess.h"

//*****************************************************************************
// Constants.
//*****************************************************************************
static const MIL_INT WINDOWS_OFFSET_X = 15;

//*****************************************************************************
// DirectX display.
//*****************************************************************************
#if USE_D3D_DISPLAY
// D3D display parameters.
const MIL_INT    D3D_DISPLAY_SIZE_X = 640;
const MIL_INT    D3D_DISPLAY_SIZE_Y = 480;
const MIL_DOUBLE MAX_Z_GAP_DISTANCE = 2.0; // in mm
#endif

//*****************************************************************************
// CProfileProcess. Interface to a profile process.
//*****************************************************************************

//*****************************************************************************
// Constructor.
//*****************************************************************************
CProfileProcess::CProfileProcess(const SPCal& PCal, MIL_INT NbPoints) :
   m_PCal(PCal),
   m_NbPoints(NbPoints),
   m_pProcessProfileDataConversion(NULL)
   {
   }

//*****************************************************************************
// Destructor. Deletes the profile process data conversion if required.
//*****************************************************************************
CProfileProcess::~CProfileProcess() 
   {
   if(m_pProcessProfileDataConversion)
      {
      delete m_pProcessProfileDataConversion;
      m_pProcessProfileDataConversion = NULL;
      }
   };


//*****************************************************************************
// CProfile3dPointsProcess. Base class for any profile process that needs to
//                          work on 3d points. The data from the 3d camera
//                          will be converted accordingly.
//*****************************************************************************

//*****************************************************************************
// Constructor.
//*****************************************************************************
CProfile3dPointsProcess::CProfile3dPointsProcess(MIL_ID MilSystem, const SPCal& PCal,
                                                 MIL_INT ProfileSize, MIL_INT NbProfiles) :
   CProfileProcess(PCal, ProfileSize * NbProfiles)
   {
   // Build the data conversion from fixed point Z and X coordinates to float flat array of X-Y coordinates. 
   m_pProcessProfileDataConversion = new CDataConversionToWorld(m_pProcessProfileDataConversion, MilSystem,
                                                                ProfileSize, NbProfiles, PCal);
   m_pProcessProfileDataConversion = new CDataConversionToFlat(m_pProcessProfileDataConversion, MilSystem,
                                                               ProfileSize, NbProfiles, 32 + M_FLOAT);
   m_pProcessProfileDataConversion = new CDataConversionApplyInvalid(m_pProcessProfileDataConversion);
   }


//*****************************************************************************
// CProfileSingleProcess. Process on 3dpoints coming from a single profile.
//                        This process display the 3d points.
//*****************************************************************************

//*****************************************************************************
// Constructor. Allocates objects for displaying the 3d profile.
//*****************************************************************************
CProfileSingleProcess::CProfileSingleProcess(MIL_ID MilSystem, const SPCal& ConvertPCal,
                                             const SPRange& DataRange, MIL_INT ProfileSize)
   :CProfile3dPointsProcess(MilSystem, ConvertPCal, ProfileSize, 1)
   {
   // Allocate the displayed image.
   MIL_DOUBLE WorldSizeX = DataRange.MaxX - DataRange.MinX;
   MIL_DOUBLE WorldSizeZ = DataRange.MaxZ - DataRange.MinZ;
   MIL_DOUBLE DisplayPixelSize = WorldSizeX / ProfileSize;
   MIL_INT DisplaySizeX = (MIL_INT)(WorldSizeX / DisplayPixelSize);
   MIL_INT DisplaySizeY = (MIL_INT)(WorldSizeZ / DisplayPixelSize);
   MbufAlloc2d(MilSystem, DisplaySizeX, DisplaySizeY,
               8 + M_UNSIGNED, M_IMAGE + M_DISP, &m_MilDisplayedImage);
   MbufClear(m_MilDisplayedImage, 192);
   McalUniform(m_MilDisplayedImage, DataRange.MinX, DataRange.MinZ,
               DisplayPixelSize, DisplayPixelSize, 0.0, M_DEFAULT);

   // Allocate the display.
   MdispAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &m_MilDisplay);
   MdispSelect(m_MilDisplay, m_MilDisplayedImage);

   // Allocate the graphic list and associate to the display.
   MgraAllocList(MilSystem, M_DEFAULT, &m_MilGraList);
   MdispControl(m_MilDisplay, M_ASSOCIATED_GRAPHIC_LIST_ID, m_MilGraList);
   }

//*****************************************************************************
// Destructor. Frees display resources.
//*****************************************************************************
CProfileSingleProcess::~CProfileSingleProcess()
   {
   MbufFree(m_MilDisplayedImage);
   MgraFree(m_MilGraList);
   MdispFree(m_MilDisplay);
   }

//*****************************************************************************
// Function that processes the profile data. Converts the profile data
// to be in 3d points format. Draws the points as dots in the calibrated
// display.
//*****************************************************************************
void CProfileSingleProcess::Process(const SPData& Data)
   {
   // Convert the data.
   SPData ConvertedData = m_pProcessProfileDataConversion->Convert(Data);

   // Draw the points in the displayed graphic list.
   MIL_FLOAT* pConvertedX = (MIL_FLOAT*)MbufInquire(ConvertedData.MilX, M_HOST_ADDRESS, M_NULL);
   MIL_FLOAT* pConvertedZ = (MIL_FLOAT*)MbufInquire(ConvertedData.MilZ, M_HOST_ADDRESS, M_NULL);
   std::vector<MIL_DOUBLE> ConvertedXDouble(m_NbPoints);
   std::vector<MIL_DOUBLE> ConvertedZDouble(m_NbPoints);
   for(MIL_UINT i = 0; i < m_NbPoints; i++)
      {
      ConvertedXDouble[i] = pConvertedX[i];
      ConvertedZDouble[i] = pConvertedZ[i];
      }

   // Disable the updates of the graphic list.
   MdispControl(m_MilDisplay, M_UPDATE, M_DISABLE);

   // Clear the graphic list.
   MgraClear(M_DEFAULT, m_MilGraList);

   // Draw the calibration.
   MgraControl(M_DEFAULT, M_BACKGROUND_MODE, M_TRANSPARENT);
   MgraColor(M_DEFAULT, M_COLOR_BLUE);
   McalDraw(M_DEFAULT, this->m_MilDisplayedImage, m_MilGraList,
            M_DRAW_ABSOLUTE_COORDINATE_SYSTEM, M_DEFAULT, M_DEFAULT);
   MgraColor(M_DEFAULT, M_COLOR_RED);
   MgraControl(M_DEFAULT, M_INPUT_UNITS, M_WORLD);
   MgraDots(M_DEFAULT, m_MilGraList, m_NbPoints,
            &ConvertedXDouble[0], &ConvertedZDouble[0], M_DEFAULT);
   MdispControl(m_MilDisplay, M_UPDATE, M_ENABLE);
   }

//*****************************************************************************
// CProfileDepthMapProcess. Process on 3dpoints coming from multiple profiles.
//                          This process extracts and displays a depth map extracted
//                          from the 3d points.
//*****************************************************************************

//*****************************************************************************
// Constructor. Allocates point cloud container, depth map.
//*****************************************************************************
CProfileDepthMapProcess::CProfileDepthMapProcess(MIL_ID MilSystem, const SPCal& ConvPCal,
                                                 const SPRange& DataRange, MIL_DOUBLE WorldPosY,
                                                 MIL_DOUBLE ConveyorSpeed, MIL_INT ProfileSize, MIL_INT NbProfiles)
   :CProfile3dPointsProcess(MilSystem, ConvPCal, ProfileSize, NbProfiles),
    m_ConvertedY(m_NbPoints),
    m_NbFramesProcessed(0)
   {
#if USE_D3D_DISPLAY
   m_3DDispHandle = M_NULL;
#endif
   // Create the Y coordinate data.
   for(MIL_INT y = 0; y < NbProfiles; y++)
      {
      MIL_DOUBLE CurY = WorldPosY + y * ConveyorSpeed;
      for(MIL_INT x = 0; x < ProfileSize; x++)
         m_ConvertedY[x + y*ProfileSize] = (MIL_FLOAT)CurY;
      }
   
   // Allocate the point cloud container.
   M3dmapAllocResult(MilSystem, M_POINT_CLOUD_CONTAINER, M_DEFAULT, &m_MilPointCloudContainer);

   // Allocate the depth map.
   MbufAlloc2d(MilSystem, ProfileSize, NbProfiles, 16 + M_UNSIGNED, M_IMAGE + M_PROC + M_DISP, &m_MilDepthMap);
   MbufClear(m_MilDepthMap, 65535);

   // Calibrate the depth map.
   MIL_DOUBLE WorldSizeX = DataRange.MaxX - DataRange.MinX;
   MIL_DOUBLE WorldSizeZ = DataRange.MaxZ - DataRange.MinZ;
   MIL_DOUBLE PixelSizeX = WorldSizeX / ProfileSize;
   MIL_DOUBLE GrayLevelSizeZ = WorldSizeZ / 65535;
   McalUniform(m_MilDepthMap, DataRange.MinX, WorldPosY, PixelSizeX, ConveyorSpeed, 0.0, M_DEFAULT);
   McalControl(m_MilDepthMap, M_WORLD_POS_Z, DataRange.MinZ);
   McalControl(m_MilDepthMap, M_GRAY_LEVEL_SIZE_Z, GrayLevelSizeZ);

   // Set the extraction box of the point cloud to the depth map.
   M3dmapSetBox(m_MilPointCloudContainer, M_EXTRACTION_BOX, M_DEPTH_MAP, (MIL_DOUBLE)m_MilDepthMap,
                M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT);

   // Set the extraction parameters.
   M3dmapControl(m_MilPointCloudContainer, M_GENERAL, M_AUTO_SCALE_ASPECT_RATIO, M_UNCONSTRAINED);

   // Allocate the display.
   MdispAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &m_MilDisplay);
   
   // Create the LUT for the display.
   MbufAllocColor(MilSystem, 3, 65536, 1, 8 + M_UNSIGNED, M_LUT, &m_MilDisplayLut);
   MgenLutFunction(m_MilDisplayLut, M_COLORMAP_JET, M_DEFAULT,
                   M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT);
   MIL_UINT8 ColorForInvalidDepth[3] = {128, 128, 128};
   MbufPutColor2d(m_MilDisplayLut, M_PLANAR, M_ALL_BANDS, 65535, 0, 1, 1, ColorForInvalidDepth);
   MdispLut(m_MilDisplay, m_MilDisplayLut);

#if USE_D3D_DISPLAY
   // Try to allocate D3D display.
   m_3DDispHandle = MdepthD3DAlloc(m_MilDepthMap,
                                   M_NULL,
                                   D3D_DISPLAY_SIZE_X,
                                   D3D_DISPLAY_SIZE_Y,
                                   M_DEFAULT,
                                   M_DEFAULT,
                                   M_DEFAULT,
                                   0,
                                   65534,
                                   MAX_Z_GAP_DISTANCE,
                                   0);
   if(m_3DDispHandle)
      {
      MdispD3DControl(m_3DDispHandle, MD3D_POINT, MD3D_ENABLE);
      MdispD3DControl(m_3DDispHandle, MD3D_ROTATE, MD3D_FALSE);
      MdispD3DShow(m_3DDispHandle);
      MdispControl(m_MilDisplay, M_WINDOW_INITIAL_POSITION_X, D3D_DISPLAY_SIZE_X + WINDOWS_OFFSET_X);
      }   
#endif

   // Select the image on the display.
   MdispSelect(m_MilDisplay, m_MilDepthMap);
   }

//*****************************************************************************
// Destructor. Frees resources.
//*****************************************************************************
CProfileDepthMapProcess::~CProfileDepthMapProcess()
   {
   MbufFree(m_MilDisplayLut);
   MdispFree(m_MilDisplay);
   MbufFree(m_MilDepthMap);
   M3dmapFree(m_MilPointCloudContainer);
#if USE_D3D_DISPLAY
   if(m_3DDispHandle)MdispD3DFree(m_3DDispHandle);
#endif
   }

//*****************************************************************************
// Function that processes the profile data. Converts the profile data
// to be in 3d points format. Extracts the points into a displayed
// depth map.
//*****************************************************************************
void CProfileDepthMapProcess::Process(const SPData& Data)
   {
   // Convert the data.
   SPData ConvertedData = m_pProcessProfileDataConversion->Convert(Data);

   // Put the data in a point cloud container.
   MIL_FLOAT* pConvertedX = (MIL_FLOAT*)MbufInquire(ConvertedData.MilX, M_HOST_ADDRESS, M_NULL);
   MIL_FLOAT* pConvertedZ = (MIL_FLOAT*)MbufInquire(ConvertedData.MilZ, M_HOST_ADDRESS, M_NULL);
   M3dmapPut(m_MilPointCloudContainer, M_POINT_CLOUD_LABEL(1), M_POSITION, M_FLOAT + 32,
             m_NbPoints, pConvertedX, &m_ConvertedY[0], pConvertedZ, M_NULL, M_DEFAULT);

   // Extract the data in the depth map.
   M3dmapExtract(m_MilPointCloudContainer, m_MilDepthMap, M_NULL, M_CORRECTED_DEPTH_MAP, M_ALL, M_DEFAULT);

#if USE_D3D_DISPLAY
   if(m_3DDispHandle)
      {
      MdepthD3DSetImages(m_3DDispHandle, m_MilDepthMap, M_NULL);
      if(m_NbFramesProcessed == 0)
         MdispD3DPrintHelp(m_3DDispHandle);
      }
#endif

   m_NbFramesProcessed++;
   }
