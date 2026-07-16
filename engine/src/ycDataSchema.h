#pragma once

#include "ycData.h"
#include "ycStartup.h"
#include "ycStringRef.h"

struct ycDataString;
class ycString;

template<typename>
struct ycSet;

#define ycDataStartup ycStartup( g_dataStartup )
extern ycStartupEntry* g_dataStartup;

#define ycDataMetaStartup ycStartup( g_dataMetaStartup )
extern ycStartupEntry* g_dataMetaStartup;

#define ycDataUIStartup ycStartup( g_dataUIStartup )
extern ycStartupEntry* g_dataUIStartup;

namespace ycData
{
	//
	// PUBLIC API
	//

	struct Type;
	struct FieldDefData;
	struct RecordDefData;
	struct EnumValue;

	namespace Schema
	{
		enum FieldType : ureg
		{
			kFieldType_Record = 0,
			kFieldType_RecordPtr,
			kFieldType_Int8,
			kFieldType_Int16,
			kFieldType_Int32,
			kFieldType_Int64,
			kFieldType_UInt8,
			kFieldType_UInt16,
			kFieldType_UInt32,
			kFieldType_UInt64,
			kFieldType_Float32,
			kFieldType_Bool,
			kFieldType_String,
			kFieldType_Enum,
			kFieldType_BitField = kFieldType_Enum,
			kFieldType_BinaryBlob,
			kFieldType_Float64,

			kFieldType_End,

			//kFieldType_Invalid = ureg(-1),
			kFieldType_Int_First = kFieldType_Int8,
			kFieldType_Int_Last = kFieldType_UInt64,
		};

		// RecordDef* and EnumDef* will be aligned to this value so that their pointers can be union'd against the intrinsic type enum, taking the lower bits
		constexpr ureg kDefAlign = 32;
		static_assert( ( (kFieldType_End-1) & ~(kDefAlign-1) ) == 0, "not enough bits to fit intrinsic types next to a def ptr!" );

		typedef void Constructor( void* ptr );
		typedef UIInfo UI( const char * label, void * data, RecordDef* recordDef, FieldDef* fieldDef, bool arrayHead );
		typedef void UIName( void * data, RecordDef* recordDef, FieldDef* fieldDef, ycString* str );
		typedef void Gizmo( void * data, RecordDef* recordDef, FieldDef* fieldDef );
		typedef void InitField( void* data, Builder* builder, RecordDef* recordDef, FieldDef* fieldDef );
		typedef bool ValidateEntry( FieldDefData* fieldDefData, RecordDefData* recordDefData, ycData::EnumValue* );

		RecordDef* CreateRecordDef( const ycStringRef& name, RecordDef* parent = nullptr );
		RecordDef* FwdDeclRecordDef( const ycStringRef& name );
		void FinalizeRecordDef( RecordDef* recordDef );

		EnumDef* CreateEnumDef( const ycStringRef& name, FieldType base = kFieldType_Int32 );
		EnumDef* FwdDeclEnumDef( const ycStringRef& name );
		void FinalizeEnumDef( EnumDef* enumDef );

		// Record lookup. Returns null if no type exists with that name.
		RecordDef* GetRecordDef( const ycRtti rtti );
		RecordDef* GetRecordDef( const ycStringRef& name );

		// Enum lookup. Returns null if no type exists with that name.
		EnumDef* GetEnumDef( const u64 type );
		EnumDef* GetEnumDef( const ycStringRef& name );

		void SetConstructor( RecordDef* recordDef, Constructor* fn );
		void SetUI( RecordDef* recordDef, UI* fn );
		void SetUIName( RecordDef* recordDef, UIName* fn );
		void SetGizmo( RecordDef* recordDef, Gizmo* fn );
		void SetRecordSize( RecordDef* recordDef, ureg size, ureg align );

		FieldDef* CreateField( RecordDef* recordDef, const ycStringRef& name, const Type& type );
		FieldDef* CreateFieldFixedArray( RecordDef* recordDef, const ycStringRef& name, const Type& type, const ureg arraySize );
		FieldDef* CreateFieldDynamicArray( RecordDef* recordDef, const ycStringRef& name, const Type& type );
		void FreeField( FieldDef* field );

		FieldDef* GetField( RecordDef* recordDef, const ycStringRef& name );
		FieldDef* GetField( void* record, void* data, RecordDef* recordDef = nullptr );

		void SetFieldOffset( FieldDef* fieldDef, ureg offset );

		EnumValue* CreateEnumValue( EnumDef* enumDef, const ycStringRef& name, s64 value );

		EnumValue* GetEnumValue( EnumDef* enumDef, const ycStringRef& name );
		EnumValue* GetEnumValue( EnumDef* enumDef, s64 value );

		struct ProtoData
		{
			void* data;
			RecordDef* recordDef;
		};

		void CreateProto( const ycStringRef& name, const ycStringRef& recordDefName, const ycStringRef& proto );
		ProtoData GetProto( u64 protoNameHash );
		void AddDefaultSibling( RecordDef* recordDef, const ycStringRef& recordDefName, const ycStringRef& protoName );
		void InitFieldProto( void* data, Builder* builder, RecordDef* recordDef, FieldDef* fieldDef );

		// Adds new metadata to a list.
		MetaData* AddMetaData( MetaData** list, const char* name, const Type& type, void* data );

		// Finds metadata by name, or null if none.
		// If 'type' is passed, will also return null if the typecheck fails.
		void * FindMetaData( MetaData * list, const char * name, const Type * type = nullptr );
		void * FindMetaData( MetaData * list, const char * name, const Type & type );
		void * FindMetaDataItem( MetaData * list, const char * name, const Type * type = nullptr );
		bool FindMetaDataBool( MetaData * list, const char * name, bool defaultVal = false );
		f32 FindMetaDataFloat32( MetaData * list, const char * name, f32 defaultVal = 0.0f );
		f64 FindMetaDataFloat64( MetaData * list, const char * name, f64 defaultVal = 0.0f );
		s32 FindMetaDataInt32( MetaData * list, const char * name, s32 defaultVal = 0.0f );
		u64 FindMetaDatU64( MetaData * list, const char * name, u64 defaultVal = 0 );

		// Finds string metadata by name, or null if none.
		ycDataString* FindMetaDataString( MetaData* list, const char* name );

		typedef ycSet< RecordDef* > IgnoreMatchList;

		// Returns true if 'query' inherits from 'parent'.
		bool Inherits( RecordDef* query, RecordDef* parent );
		// Returns true if any record inheritance from 'query' matches 'sibling' 
		bool InheritMatch( RecordDef* query, RecordDef* sibling, IgnoreMatchList* ignoreList = nullptr );
		RecordDef* GetInheritMatch( RecordDef* query, RecordDef* sibling, IgnoreMatchList* ignoreList = nullptr );

		void SerializeText( ycString* dst, RecordDef* recordDef );
		void SerializeHeader( ycString* dst, RecordDef* recordDef );
		bool DeserializeText( const ycStringRef& src, ycString* errMsg );

		void Init();
		void Shutdown();

		// Queries the list of all types that inherit from a base.
		// (base can be null to get all types).
		// Returns the number of items that would have been output if enough space existed.
		ycSize_t GetSubclasses( RecordDef* base, RecordDef** output, ycSize_t maxOutput, bool includeBase = false );
	}

	//
	// INTERNALS
	//

	namespace Schema
	{
		void Init();
		void Shutdown();

		extern Builder* s_builder; // Builder use for storing schema data.

		typedef void( *EnumRecordDefFuncPtr )( RecordDef*, void* );
		void EnumerateRecordDefs( EnumRecordDefFuncPtr cb, void* userData = nullptr );

		typedef void( *EnumEnumDefFuncPtr )( EnumDef*, void* );
		void EnumerateEnumDefs( EnumEnumDefFuncPtr cb, void* userData = nullptr );

		FieldDef* CreateField_Internal( RecordDef* recordDef, const ycStringRef& name, const ureg flags, const Type& type, const ureg arraySize );

		bool IsRecordDefFinalized( RecordDef* recordDef );
		bool IsEnumDefFinalized( EnumDef* enumDef );
		ycString GetTypeName( const Type& type ); //!< returns the type name used in ycSchema text format
		ycString GetTypeCodeName( const Type& type ); //!< returns the type name used in C++ code
		Type ParseTypeName( ycStringRef& src ); // modifies src!

		/*
		GetField___ returns the size of the in-struct representation of a field. For simple scalar types this is
		straightforward, for arrays this is different. Fixed-size arrays return scalar size*count, dynamic arrays
		return sizeof(DynamicArray)
		*/
		ureg GetFieldSize( FieldDef* fieldDef );
		ureg GetFieldAlign( FieldDef* fieldDef );

		/*
		GetType___ returns the size of a single scalar item regardless of whether it is in an array.
		*/
		ureg GetTypeSize( Type type );
		ureg GetTypeAlign( Type type );

		/*
		Returns a byte offset into an array for a given array element
		*/
		ureg GetArrayFieldDataOffset( FieldDef* fieldDef, const ureg arrayIdx );

		/*
		Dependencies are sometimes handled as bitmaps to make OR-ing them together quick, the maximum size of that
		bitmap is hardcoded.
		*/
		static constexpr ureg kDepBitmapRegs = 20; // how many u64s

		void GetDep( const u64 idx, u64* dep, u64* checksum );
	}

	struct MetaData;

	struct DefaultSibling
	{
		u64 recordDefHash;
		u64 protoNameHash;
		DefaultSibling* next;
	};

	enum
	{
		kRecordFlag_Finalized = 1<<0, // record has been fully defined
		kRecordFlag_AutoSize  = 1<<1, // decide size automatically based on members
		kRecordFlag_HasRtti  = 1<<2, // emit RTTI data in binaries TODO HACK FIXME: rename!
		kRecordFlag_FwdDecl   = 1<<3, // only information we have is that the type exists
		kRecordFlag_POD       = 1<<4, // record is memcpyable
		kRecordFlag_NeedsPad  = 1<<5, // record def is not portable, may not be set if a check has not been run
	};
	struct RecordDef
	{
		ycStringRef name;
		u64         size; // size of the 'struct' itself, not external things it may point to (eg dynamic arrays, binary blobs, other records)
		u32         flags;
		u32         align; // alignment requirement of the record itself
		RecordDef*  parent;

		Schema::Constructor* constructor; // TODO HACK FIXME: remove; atm still necessary to handle some ordering issues
		void* prototype;
		void* uiHandle;
		Schema::UI* ui;
		Schema::UIName* uiName;
		Schema::Gizmo* gizmo;
		FieldDef*   firstMember;
		FieldDef*   lastMember;
		FieldDef*   stringMember;
		MetaData*	meta;
		ycRtti rtti;
		DefaultSibling* defaultSibling;

		u64 checksum; // for versioning/comparison ('has this recorddef changed'), top bit is not set for records, is set for enums
		u32 depIdx; // idx of this def into depMap and the global dep table
		u32 _pad;
		u64 depMap[ ycData::Schema::kDepBitmapRegs ];

		inline bool IsPOD() const { return (flags&kRecordFlag_POD) != 0; }
		inline bool IsVoid() const { return rtti.typeId == PHASH64( "void", 0xD15943FAB490214B ); }
		inline bool HasRtti() const { return (flags&kRecordFlag_HasRtti) != 0; }
	};

	struct RecordDefData
	{
		void* data;
		RecordDef* recordDef;
	};

	struct Type
	{
		Type() = default; // constructs an invalid type
		explicit Type( Schema::FieldType fieldType ) : data( fieldType ) { ycAssert( !(IsRecord() || IsRecordPtr() || IsEnum()) ); }
		explicit Type( Schema::FieldType fieldType, void* subType ) : data( uintptr_t(subType) | fieldType ) {}

		inline Schema::FieldType GetFieldType() const { return Schema::FieldType( data & 0xf ); }
		inline bool IsRecord() const { return GetFieldType() == Schema::kFieldType_Record; }
		inline bool IsRecordPtr() const { return GetFieldType() == Schema::kFieldType_RecordPtr; }
		inline bool IsEnum() const { return GetFieldType() == Schema::kFieldType_Enum; }
		inline bool IsBinaryBlob() const { return GetFieldType() == Schema::kFieldType_BinaryBlob; }
		inline bool IsString() const { return GetFieldType() == Schema::kFieldType_String; }
		inline RecordDef* GetRecordDef() const { return ( RecordDef* )( data & 0xfffffffffffffff0llu ); }
		inline EnumDef* GetEnumDef() const { return ( EnumDef* )( data & 0xfffffffffffffff0llu ); }

		inline bool IsValid() const { return data != 0; }
		inline bool operator == ( const Type& rhs ) const { return data == rhs.data; }

		uintptr_t data = 0; // lower bits kFieldType_*, upper bits ptr to RecordDef/EnumDef
	};

	enum
	{
		kFieldFlag_FixedArray            = 1<<0, //!< if set, FieldDef::arrayCount indicates the number of elements; mutually exclusive with kFieldFlag_DynamicArray
		kFieldFlag_DynamicArray          = 1<<1, //!< indicates field is a DynamicArray/ycDataArray
		kFieldFlag_AutoOffset            = 1<<2, //!< member offset is calculated automatically rather than specified via SetFieldOffset
		kFieldFlag_Reference             = 1<<3, //!< for pointer fields, the field does not own the thing it points to
		kFieldFlag_BinMutable            = 1<<4, //!< for resizable fields (ycDataArray, ycDataString, etc), the field should be mutable even when inside a use-in-place serialized binary
		kFieldFlag_IgnoreTextSerialize   = 1<<5,  //!< this field will not be serialized in the text format
		kFieldFlag_IgnoreDependencies    = 1<<6,  //!< ignore dependencies for this field
		kFieldFlag_IgnoreBinarySerialize = 1<<7,  //!< this field will not be serialized in the binary (will be null or default)
		kFieldFlag_IgnoreDataIter        = 1<<8,  //!< ycDataIter will skip over this field
		kFieldFlag_AlwaysWrite           = 1<<9,  //!< always write default

		kFieldFlag_IgnoreDependenciesAll  = kFieldFlag_IgnoreBinarySerialize | kFieldFlag_IgnoreBinarySerialize,  //!< this field will not be serialized in the binary (will be null or default)
	};

	struct FieldDef
	{
		FieldDef*   prev;
		FieldDef*   next;
		u64         key;
		ycStringRef name;
		u32         flags;
		Type		type;
		u32         arrayCount;
		ycData::RecordDef* recordDef;
		u64			offset;
		MetaData*	meta;
		const char* defaultRecordName;
		Schema::InitField* init;

		inline bool IsPOD() const
		{
			bool pod = true;
			if( type.IsRecordPtr() || type.GetFieldType() == Schema::kFieldType_String || type.GetFieldType() == Schema::kFieldType_BinaryBlob || (flags&kFieldFlag_DynamicArray) )
			{
				pod = false;
			}
			else if( type.IsRecord() && !type.GetRecordDef()->IsPOD() )
			{
				pod = false;
			}
			return pod;
		}

		inline bool IsArray() const
		{
			return (flags & (kFieldFlag_FixedArray | kFieldFlag_DynamicArray)) != 0;
		}
	};

	struct FieldDefData
	{
		void* data;
		FieldDef* fieldDef;
	};

	struct MetaData
	{
		MetaData* next;
		ycStringRef name;
		Type type;
		void* data;
	};

	enum
	{
		kEnumFlag_Finalized = 1<<0, // enum has been fully defined
		kEnumFlag_FwdDecl = 1<<1, // only information we have is that the type exists
	};
	struct EnumDef
	{
		ycStringRef name;
		u32         flags;
		Schema::FieldType base;
		EnumValue*  firstValue;
		EnumValue*  lastValue;
		Schema::ValidateEntry* validateEntry;
		MetaData*	meta;
		ycRtti rtti;

		u64 checksum; // for versioning/comparison ('has this recorddef changed'), top bit is not set for records, is set for enums
		u32 depIdx; // idx of this def into the global dep table
		//u32 _pad;
	};

	struct EnumValue
	{
		EnumValue*  prev;
		EnumValue*  next;
		ycStringRef name;
		s64         value;
		MetaData*	meta;
	};
}
