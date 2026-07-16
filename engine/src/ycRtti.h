#pragma once

/*! \class ycRtti

RTTI (run time type information) is used to allow introspection of ycObject derived classes.

This class exists mainly to provide a "strong" typedef for our RTTI. It's just u64s, but we don't want to allow
implicit casting with integral types.
*/

#include "ycCommon.h"

class ycStringRef;

class ycRtti
{
public:

	static constexpr u64 kInvalidTypeId = 0;

	ycRtti() = default;
	constexpr inline explicit ycRtti( const u64 _typeId ) : typeId( _typeId ) {}
	static inline ycRtti Create( const u64 _typeId ) { return ycRtti(_typeId); }
	static ycRtti Create( const ycStringRef& str );

	u64 typeId;
	
	inline bool operator == ( const ycRtti& rhs ) const { return typeId == rhs.typeId; }
	inline bool operator != ( const ycRtti& rhs ) const { return typeId != rhs.typeId; }

	template< typename t_type >
	static ycRtti Get()
	{
		extern ycRtti ycGetRtti( t_type* );
		return ycGetRtti( ( t_type * )nullptr );
	}

	inline operator bool() { return typeId != kInvalidTypeId; }

	bool Isa( const ycRtti parent ) const;

	template< typename t_type >
	bool Isa() const { return Isa( ycRtti::Get< t_type >() ); }

	template< typename t_type >
	bool Equals() const { return operator==( ycRtti::Get< t_type >() ); }

	bool Equals( u64 id ) const { return operator==( ycRtti( id ) ); }

	// internal / interface for generated code
	typedef bool ( *IsaFuncPtr )( const ycRtti rtti );
	static void Register( const u64 typeId, IsaFuncPtr isaFunc );

#ifdef YC_DEBUG
	static void DumpLookupStats();
#endif // YC_DEBUG
};
