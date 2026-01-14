// CodeGear C++Builder
// Copyright (c) 1995, 2022 by Embarcadero Technologies, Inc.
// All rights reserved

// (DO NOT EDIT: machine generated header) 'EhLibADO.pas' rev: 35.00 (Windows)

#ifndef EhlibadoHPP
#define EhlibadoHPP

#pragma delphiheader begin
#pragma option push
#pragma option -w-      // All warnings off
#pragma option -Vx      // Zero-length empty class member 
#pragma pack(push,8)
#include <System.hpp>
#include <SysInit.hpp>
#include <System.Variants.hpp>
#include <DBUtilsEh.hpp>
#include <Data.Win.ADODB.hpp>
#include <Data.DB.hpp>
#include <DBGridEh.hpp>
#include <ToolCtrlsEh.hpp>
#include <MemTableEh.hpp>
#include <MemTableDataEh.hpp>

//-- user supplied -----------------------------------------------------------

namespace Ehlibado
{
//-- forward type declarations -----------------------------------------------
class DELPHICLASS TADOSQLDatasetFeaturesEh;
class DELPHICLASS TADOCommandTextDatasetFeaturesEh;
//-- type declarations -------------------------------------------------------
#pragma pack(push,4)
class PASCALIMPLEMENTATION TADOSQLDatasetFeaturesEh : public Dbutilseh::TSQLDatasetFeaturesEh
{
	typedef Dbutilseh::TSQLDatasetFeaturesEh inherited;
	
public:
	virtual Dbutilseh::TNullComparisonSyntaxEh __fastcall NullComparisonSyntax(Dbgrideh::TCustomDBGridEh* AGrid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual void __fastcall ApplySorting(System::TObject* Sender, Data::Db::TDataSet* DataSet, bool IsReopen);
	void __fastcall SortDataInADODataSet(Dbgrideh::TCustomDBGridEh* Grid, Data::Win::Adodb::TCustomADODataSet* DataSet);
	virtual Dbutilseh::TLocalFilterApplyingWayEh __fastcall GetLocalFilterApplyingWay();
	virtual System::UnicodeString __fastcall GetSQLFilterMarker(Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet);
	virtual System::UnicodeString __fastcall GetServerTypeName(Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet);
	virtual bool __fastcall IsInOperatorSupported(Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual int __fastcall WriteDataSetToMemTable(Data::Db::TDataSet* DataSet, Memtableeh::TCustomMemTableEh* MemTable, int RecordCount, Memtableeh::TLoadMode Mode, bool UseCachedUpdates);
	__fastcall virtual TADOSQLDatasetFeaturesEh();
public:
	/* TObject.Destroy */ inline __fastcall virtual ~TADOSQLDatasetFeaturesEh() { }
	
};

#pragma pack(pop)

#pragma pack(push,4)
class PASCALIMPLEMENTATION TADOCommandTextDatasetFeaturesEh : public Dbutilseh::TCommandTextDatasetFeaturesEh
{
	typedef Dbutilseh::TCommandTextDatasetFeaturesEh inherited;
	
public:
	virtual Dbutilseh::TLocalFilterApplyingWayEh __fastcall GetLocalFilterApplyingWay();
	virtual Dbutilseh::TNullComparisonSyntaxEh __fastcall NullComparisonSyntax(Dbgrideh::TCustomDBGridEh* AGrid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual void __fastcall ApplySorting(System::TObject* Sender, Data::Db::TDataSet* DataSet, bool IsReopen);
	void __fastcall SortDataInADODataSet(Dbgrideh::TCustomDBGridEh* Grid, Data::Win::Adodb::TCustomADODataSet* DataSet);
	virtual bool __fastcall IsInOperatorSupported(Dbgrideh::TCustomDBGridEh* Grid, Data::Db::TDataSet* DataSet, bool IsLocalFilter);
	virtual int __fastcall WriteDataSetToMemTable(Data::Db::TDataSet* DataSet, Memtableeh::TCustomMemTableEh* MemTable, int RecordCount, Memtableeh::TLoadMode Mode, bool UseCachedUpdates);
	__fastcall virtual TADOCommandTextDatasetFeaturesEh();
public:
	/* TObject.Destroy */ inline __fastcall virtual ~TADOCommandTextDatasetFeaturesEh() { }
	
};

#pragma pack(pop)

//-- var, const, procedure ---------------------------------------------------
extern DELPHI_PACKAGE int __fastcall WriteADODataSetToMemTable(Data::Win::Adodb::TCustomADODataSet* DataSet, Memtableeh::TCustomMemTableEh* MemTable, int RecordCount, Memtableeh::TLoadMode Mode, bool UseCachedUpdates);
}	/* namespace Ehlibado */
#if !defined(DELPHIHEADER_NO_IMPLICIT_NAMESPACE_USE) && !defined(NO_USING_NAMESPACE_EHLIBADO)
using namespace Ehlibado;
#endif
#pragma pack(pop)
#pragma option pop

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// EhlibadoHPP
