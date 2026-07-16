#pragma once

#include "ycCommon.h"
#include "ycRtti.h"

namespace ycData
{
	struct RecordDef;
	struct FieldDef;
	struct EnumDef;
	struct EnumValue;
	struct MetaData;

	class Builder;
	class UndoStack;

	void Test();

	//! Returns a RecordDef for a compiled-in ycData type (supplied by ycCodeGen)
	template< typename t_record >
	RecordDef* GetRecordDef()
	{
		extern RecordDef* GetRecordDef( t_record* dummy );
		return GetRecordDef( ( t_record * )nullptr );
	}

	//! Returns an EnumDef for a compiled-in ycData type (supplied by ycCodeGen)
	template< typename t_enum >
	EnumDef* GetEnumDef()
	{
		extern EnumDef* GetEnumDef( t_enum* dummy );
		return GetEnumDef( ( t_enum * )nullptr );
	}

	// maybe ycDataUI.h or something?
	struct UIInfo
	{
		bool dataModified = false;
		bool uiCreated = false;

		/*
		Additional field-specific information which we might want to communicate downstream.
		E.g. a compound-field like Vec3 might indicate which field(s) the user interacted with.
		Useful for Undo-Intercept / Recording Context-Menu Options.
		*/
		u8 userMask = 0; 

		inline UIInfo& operator||( const UIInfo& rhs )
		{
			dataModified = dataModified || rhs.dataModified; 
			uiCreated    = uiCreated    || rhs.uiCreated; 
			userMask     = userMask     |  rhs.userMask;
			return *this; 
		}
	};

	enum
	{
		kBinaryVer = 8
	};
}
