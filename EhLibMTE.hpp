// CodeGear C++Builder
// Copyright (c) 1995, 2022 by Embarcadero Technologies, Inc.
// All rights reserved

// (DO NOT EDIT: machine generated header) 'EhLibMTE.pas' rev: 35.00 (Windows)

#ifndef EhlibmteHPP
#define EhlibmteHPP

#pragma delphiheader begin
#pragma option push
#pragma option -w-      // All warnings off
#pragma option -Vx      // Zero-length empty class member 
#pragma pack(push,8)
#include <System.hpp>
#include <SysInit.hpp>
#include <System.Contnrs.hpp>
#include <System.Variants.hpp>
#include <System.Types.hpp>
#include <EhLibVCL.hpp>
#include <DBGridEh.hpp>
#include <DBUtilsEh.hpp>
#include <Data.DB.hpp>
#include <MemTableEh.hpp>
#include <MemTableDataEh.hpp>
#include <DataDriverEh.hpp>
#include <System.SysUtils.hpp>
#include <ToolCtrlsEh.hpp>
#include <System.Classes.hpp>

//-- user supplied -----------------------------------------------------------

namespace Ehlibmte
{
//-- forward type declarations -----------------------------------------------
class DELPHICLASS TMTEDatasetFeaturesEh;
//-- type declarations -------------------------------------------------------
#pragma pack(push,4)
class PASCALIMPLEMENTATION TMTEDatasetFeaturesEh : public Dbutilseh::TSQLDatasetFeaturesEh
{
	typedef Dbutilseh::TSQLDatasetFeaturesEh inherited;
	
protected:
	Memtabledataeh::TMemRecViewEh* FBaseNode;
	
public:
	__fastcall virtual TMTEDatasetFeaturesEh();
	virtual System::UnicodeString __fastcall GetDataSetLikeWildcardForSeveralCharacters();
	virtual Dbutilseh::TLocalFilterApplyingWayEh __fastcall GetLocalFilterApplyingWay();
	virtual bool __fastcall LocateText(Dbgrideh::TCustomDBGridEh* AGrid, const System::UnicodeString FieldName, const System::UnicodeString Text, Toolctrlseh::TLocateTextOptionsEh AOptions, Toolctrlseh::TLocateTextDirectionEh Direction, Toolctrlseh::TLocateTextMatchingEh Matching, Toolctrlseh::TLocateTextTreeFindRangeEh TreeFindRange, unsigned TimeOut = (unsigned)(0x0), Dbgrideh::TCheckColumnValueAcceptEventEh CheckValueEvent = 0x0);
	virtual bool __fastcall IsInOperatorSupported(Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual void __fastcall ApplyFilter(System::TObject* Sender, Data::Db::TDataSet* DataSet, bool IsReopen);
	virtual void __fastcall ApplySorting(System::TObject* Sender, Data::Db::TDataSet* DataSet, bool IsReopen);
	virtual void __fastcall ExecuteFindDialog(System::TObject* Sender, const System::UnicodeString Text, const System::UnicodeString FieldName, bool Modal);
	virtual void __fastcall FillSTFilterListCommandValues(Dbgrideh::TCustomDBGridEh* AGrid, Dbgrideh::TColumnEh* Column, System::Classes::TStrings* Items);
	virtual void __fastcall FillSTFilterListDataValues(Dbgrideh::TCustomDBGridEh* AGrid, Dbgrideh::TColumnEh* Column, System::Classes::TStrings* Items);
	virtual void __fastcall FillFieldUniqueValues(Data::Db::TField* Field, System::Classes::TStrings* Items);
	virtual System::UnicodeString __fastcall VarValueToFilterStrValue(Dbgrideh::TSTFilterOperatorEh AnOperator, const System::Variant &Value, Dbgrideh::TColumnEh* Column, Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual System::UnicodeString __fastcall DateTimeValueToFilterStrValue(Dbgrideh::TSTFilterOperatorEh AnOperator, System::TDateTime Value, Dbgrideh::TColumnEh* Column, Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual System::UnicodeString __fastcall FilterFieldNameToStrValue(System::UnicodeString FieldName, Dbgrideh::TColumnEh* Column, Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
public:
	/* TObject.Destroy */ inline __fastcall virtual ~TMTEDatasetFeaturesEh() { }
	
};

#pragma pack(pop)

//-- var, const, procedure ---------------------------------------------------
extern DELPHI_PACKAGE bool SortInView;
extern DELPHI_PACKAGE System::UnicodeString __fastcall WrapFieldName(const System::UnicodeString FieldName);
}	/* namespace Ehlibmte */
#if !defined(DELPHIHEADER_NO_IMPLICIT_NAMESPACE_USE) && !defined(NO_USING_NAMESPACE_EHLIBMTE)
using namespace Ehlibmte;
#endif
#pragma pack(pop)
#pragma option pop

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// EhlibmteHPP
