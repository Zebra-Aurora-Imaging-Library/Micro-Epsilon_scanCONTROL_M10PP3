/************************************************************************************/
/*
* File name: DataConversion.h
*
* Synopsis:  This file contains the declaration of everything related to the input data
*            and its conversion to a given format. The CDataConversion derived class
*            can include a chain of other CDataConversion each preformed one after the other.
*
* Copyright © 1992-2024 Zebra Technologies Corp. and/or its affiliates
* All Rights Reserved
*/

#ifndef DATA_CONVERSION_H
#define DATA_CONVERSION_H

//*****************************************************************************
// Structure defining the profile data in the form of 3 images.
//*****************************************************************************
struct SPData
   {
   SPData(): MilX(M_NULL), MilZ(M_NULL), MilValidMask(M_NULL){};
   void ReleaseData()
      {
      if(MilX) MbufFree(MilX);
      if(MilZ) MbufFree(MilZ);
      }

   void ReleaseMask()
      {
      if(MilValidMask) MbufFree(MilValidMask);
      }

   MIL_ID MilX;
   MIL_ID MilZ;
   MIL_ID MilValidMask;
   };

//*****************************************************************************
// Structure defining a calibration that could be associated to some
// profile data.
//*****************************************************************************
struct SPCal
   {
   MIL_DOUBLE WorldX;
   MIL_DOUBLE WorldZ;   
   MIL_DOUBLE GrayLevelSX;
   MIL_DOUBLE GrayLevelSZ;
   };

//*****************************************************************************
// Base class defining the data conversion interface. 
//*****************************************************************************
class CDataConversion
   {
   public:
      CDataConversion(CDataConversion* pPrevConv): m_pPrevConv(pPrevConv) {};
      virtual ~CDataConversion() { if(m_pPrevConv) delete m_pPrevConv; }
      virtual SPData Convert(const SPData& Data) = 0;

   protected:
      virtual SPData ConvertPrev(const SPData& Data) { return m_pPrevConv ? m_pPrevConv->Convert(Data) : Data; }
      CDataConversion* m_pPrevConv;
   };

//*****************************************************************************
// Data conversion that creates a valid mask from the input Z data.
//*****************************************************************************
class CDataConversionAddMask : public CDataConversion
   {
   public:
      CDataConversionAddMask(CDataConversion* pPrevConv, MIL_ID MilSystem,
                             MIL_INT ProfileSize, MIL_INT NbProfiles, MIL_DOUBLE InvalidValue)
         : CDataConversion(pPrevConv),
           m_InvalidValue(InvalidValue)
         {
         MbufAlloc2d(MilSystem, ProfileSize, NbProfiles, 8 + M_UNSIGNED, M_IMAGE + M_PROC, &m_MilValidMask);
         }
      virtual ~CDataConversionAddMask()
         {
         MbufFree(m_MilValidMask);
         }

      virtual SPData Convert(const SPData& Data)
         {
         SPData ConvertedData = ConvertPrev(Data);
         MimArith(ConvertedData.MilZ, m_InvalidValue, m_MilValidMask, M_NOT_EQUAL_CONST);
         ConvertedData.MilValidMask = m_MilValidMask;
         return ConvertedData;
         };
   private:
      MIL_ID m_MilValidMask;
      MIL_DOUBLE m_InvalidValue;
   };

//*****************************************************************************
// Base class defining a data conversion that modifies 
// the data type or data structure which requires the allocation
// of a new SPData structure.
//*****************************************************************************
class CDataConversionData: public CDataConversion
   {
   public:
      CDataConversionData(CDataConversion* pPrevConv): CDataConversion(pPrevConv) {};
      virtual ~CDataConversionData() { m_ConvertedData.ReleaseData(); }
   protected:
      SPData m_ConvertedData;
   };

//*****************************************************************************
// Data conversion from data whose calibration is
// described by an external calibration to actual calibrated
// data of float type.
//*****************************************************************************
class CDataConversionToWorld: public CDataConversionData
   {
   public:
      CDataConversionToWorld(CDataConversion* pPrevConv, MIL_ID MilSystem,
                             MIL_INT ProfileSize, MIL_INT NbProfiles, SPCal PCal) :
         CDataConversionData(pPrevConv),
         m_PCal(PCal)
         {
         MbufAlloc2d(MilSystem, ProfileSize, NbProfiles, 32 + M_FLOAT, M_IMAGE + M_PROC, &m_ConvertedData.MilX);
         MbufAlloc2d(MilSystem, ProfileSize, NbProfiles, 32 + M_FLOAT, M_IMAGE + M_PROC, &m_ConvertedData.MilZ);
         };

      virtual SPData Convert(const SPData& Data)
         {         
         SPData ConvertedData = ConvertPrev(Data);
         MimArith(ConvertedData.MilX, m_PCal.GrayLevelSX, m_ConvertedData.MilX, M_MULT_CONST + M_FLOAT_PROC);
         MimArith(m_ConvertedData.MilX, m_PCal.WorldX, m_ConvertedData.MilX, M_ADD_CONST + M_FLOAT_PROC);
         MimArith(ConvertedData.MilZ, m_PCal.GrayLevelSZ, m_ConvertedData.MilZ, M_MULT_CONST + M_FLOAT_PROC);
         MimArith(m_ConvertedData.MilZ, m_PCal.WorldZ, m_ConvertedData.MilZ, M_ADD_CONST + M_FLOAT_PROC);
         m_ConvertedData.MilValidMask = ConvertedData.MilValidMask;
         return m_ConvertedData;
         }

   private:
      SPCal m_PCal;
   };

//*****************************************************************************
// Data conversion that takes data that is in an image
// format (with a pitch) and flattens it into a 1d buffer.
//*****************************************************************************
class CDataConversionToFlat: public CDataConversionData
   {
   public:
      CDataConversionToFlat(CDataConversion* pPrevConv, MIL_ID MilSystem,
                            MIL_INT ProfileSize, MIL_INT NbProfiles, MIL_INT Type) :
         CDataConversionData(pPrevConv)
         {         
         MbufAlloc1d(MilSystem, ProfileSize*NbProfiles, Type, M_IMAGE + M_PROC, &m_ConvertedData.MilX);
         MbufAlloc1d(MilSystem, ProfileSize*NbProfiles, Type, M_IMAGE + M_PROC, &m_ConvertedData.MilZ);
         MbufAlloc1d(MilSystem, ProfileSize*NbProfiles,
                     8+M_UNSIGNED, M_IMAGE + M_PROC, &m_ConvertedData.MilValidMask);
         }

      virtual ~CDataConversionToFlat() { m_ConvertedData.ReleaseMask(); }

      virtual SPData Convert(const SPData& Data)
         {
         SPData ConvertedData = ConvertPrev(Data);
         MbufGet(ConvertedData.MilX, (void*)MbufInquire(m_ConvertedData.MilX, M_HOST_ADDRESS, M_NULL));
         MbufGet(ConvertedData.MilZ, (void*)MbufInquire(m_ConvertedData.MilZ, M_HOST_ADDRESS, M_NULL));
         MbufGet(ConvertedData.MilValidMask, (void*)MbufInquire(m_ConvertedData.MilValidMask, M_HOST_ADDRESS, M_NULL));
         return m_ConvertedData;
         }
   };

//*****************************************************************************
// Base class defining a data conversion that simply 
// performs an operation on the input data without actually
// needing to change the type or buffer format.
//*****************************************************************************
struct CDataConversionOp: public CDataConversion
   {
   CDataConversionOp(CDataConversion* pPrevConv) : CDataConversion(pPrevConv) {};
   
   virtual SPData Convert(const SPData& Data) 
      { 
      SPData ConvertedData = ConvertPrev(Data);
      ConvertOp(ConvertedData);
      return ConvertedData;
      }

   virtual void ConvertOp(const SPData& pData) = 0;
   };

//*****************************************************************************
// Flips the values of the X data.
//*****************************************************************************
struct CDataConversionFlipXVal : public CDataConversionOp
   {
   CDataConversionFlipXVal(CDataConversion* pOtherConversion): CDataConversionOp(pOtherConversion) {};
   virtual void ConvertOp(const SPData& Data) { MimArith(65535, Data.MilX, Data.MilX, M_CONST_SUB); };
   };

//*****************************************************************************
// Flips the values of the Z data.
//*****************************************************************************
struct CDataConversionFlipZVal : public CDataConversionOp
   {
   CDataConversionFlipZVal(CDataConversion* pOtherConversion): CDataConversionOp(pOtherConversion) {};
   virtual void ConvertOp(const SPData& Data) { MimArith(65535, Data.MilZ, Data.MilZ, M_CONST_SUB); }
   };

//*****************************************************************************
// Applies the invalid mask into the X and Y data.
//*****************************************************************************
struct CDataConversionApplyInvalid : public CDataConversionOp
   {
   CDataConversionApplyInvalid(CDataConversion* pOtherConversion): CDataConversionOp(pOtherConversion) {};
   virtual void ConvertOp(const SPData& Data)
      {
      ApplyInvalidMask(Data.MilX, Data.MilValidMask);
      ApplyInvalidMask(Data.MilZ, Data.MilValidMask);
      }

   void ApplyInvalidMask(MIL_ID MilImage, MIL_ID MilMask)
      {
      MIL_INT Type = MbufInquire(MilImage, M_TYPE, M_NULL);
      MIL_DOUBLE InvalidValue;
      MbufInquire(MilImage, M_MAX, &InvalidValue);
      MbufClearCond(MilImage, InvalidValue, 0, 0, MilMask, M_EQUAL, 0);
      }
   };

#endif // DATA_CONVERSION_H
