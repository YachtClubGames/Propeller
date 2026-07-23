#pragma once
#ifndef YC_STD_H
#define YC_STD_H

#include "ycCommon.h"

/*! This exists to encapsulate #includes and some platform differences */

#ifndef PRId64 // sometimes this is included after something that pulls in inttypes.h
	#if defined _MSC_VER
		#define PRId64 "lld"
		#define PRIu64 "llu"
		#define PRIx64 "llx"
		#define PRIX64 "llX"
	#elif defined __INT64_FMTd__ && defined __UINT64_FMTu__ && defined __UINT64_FMTx__ && defined __UINT64_FMTX__ // clang
		#define PRId64 __INT64_FMTd__
		#define PRIu64 __UINT64_FMTu__
		#define PRIx64 __UINT64_FMTx__
		#define PRIX64 __UINT64_FMTX__
	#else
		#define __STDC_FORMAT_MACROS
		#include <inttypes.h>
	#endif
#endif

void* ycMemSet( void* dst, const int value, const size_t numBytes ); //!< intentionally does not use ycSize_t, to match std memset

template< class t_type, size_t t_len >
void ycZeroArray( t_type (&array)[ t_len ] ) { ycMemSet( array, 0, sizeof(t_type)*t_len ); }

void* ycMemCpy( void* YC_RESTRICT dst, const void* YC_RESTRICT src, const size_t numBytes ); //!< intentionally does not use ycSize_t, to match std memcpy

void* ycMemMove( void* dst, const void* src, const size_t numBytes ); //!< intentionally does not use ycSize_t, to match std memcpy

bool ycMemEq( const void* a, const void* b, const size_t numBytes ); // similar to memcmp but doesnt have the <0, 0, >0 behavior

#if defined _MSC_VER
	#define YC_UNALIGNED_READ(  typeName, dst, src ) *reinterpret_cast< typeName* >( dst ) = *reinterpret_cast< const typeName* >( src )
	#define YC_UNALIGNED_WRITE( typeName, dst, src ) *reinterpret_cast< typeName* >( dst ) = *reinterpret_cast< const typeName* >( src )
#else
	/* I don't like having this include effectively global, but I think the gcc/clang versions of it don't cascade #includes too badly. */
	#include <string.h>
	#define YC_UNALIGNED_READ(  typeName, dst, src ) memcpy( dst, src, sizeof(typeName) )
	#define YC_UNALIGNED_WRITE( typeName, dst, src ) memcpy( dst, src, sizeof(typeName) )
#endif

#define YC_ARRAY_SIZE( x ) (sizeof(x)/sizeof(x[0]))

// safety check, and sometimes performance optimization if no default is needed
#if defined YC_MSVC
	#ifdef YC_ENABLE_ASSERT
		#define YC_NO_DEFAULT_ASSERT             default: { ycFatal(); __assume(0); break; }
		#define YC_NO_DEFAULT_ASSERT_RETURN( x ) default: { ycFatal(); __analysis_assume(0); return x; } // using __assume(0) here creates an unreachable code warning (one that is difficult to pragma warning away)
	#else
		#define YC_NO_DEFAULT_ASSERT             default: { ycFatal(); __assume(0); break; }
		#define YC_NO_DEFAULT_ASSERT_RETURN( x ) default: { ycFatal(); __analysis_assume(0); return x; }
	#endif
#elif defined YC_GCC || defined YC_CLANG
	#define YC_NO_DEFAULT_ASSERT             default: { ycFatal(); __builtin_unreachable(); break; }
	#define YC_NO_DEFAULT_ASSERT_RETURN( x ) default: { ycFatal(); __builtin_unreachable(); return x; }
#else
	#define YC_NO_DEFAULT_ASSERT             default: { ycFatal(); break; }
	#define YC_NO_DEFAULT_ASSERT_RETURN( x ) default: { ycFatal(); return x; }
#endif

// usually used to avoid a 'not all control paths return a value' warning
template< class t_type >
t_type* ycTypedNullPtr() { return reinterpret_cast< t_type* >( 0 ); }

#if !defined YC_CLANG
template< class t_type >
t_type& ycTypedNullRef() { return *reinterpret_cast< t_type* >( 0 ); }
#endif

// inherited from to prevent copy construction/assignment
class ycNoCopy
{
	public: ycNoCopy() = default;
	        ycNoCopy( const ycNoCopy& ) = delete;
	        ycNoCopy( ycNoCopy&& ) = delete;
	        ycNoCopy& operator= ( const ycNoCopy& ) = delete;
	        ycNoCopy& operator= ( ycNoCopy&& ) = delete;
};

template <typename T> struct ycRemoveReference		{ typedef T type; };
template <typename T> struct ycRemoveReference<T&>	{ typedef T type; };
template <typename T> struct ycRemoveReference<T&&>	{ typedef T type; };
template <typename T> using  ycRemoveReferenceT		= typename ycRemoveReference< T >::type;
template <typename T> struct ycRemoveConst			{ typedef T type; };
template <typename T> struct ycRemoveConst<const T>	{ typedef T type; };
template <typename T> using  ycRemoveConstT			= typename ycRemoveConst< T >::type;

// STL-free versions of std::move and std::forward (see https://foonathan.net/2020/09/move-forward/)
#define YC_MOVE( ... )    static_cast< ycRemoveConstT< ycRemoveReferenceT< decltype(__VA_ARGS__) > >&& >( __VA_ARGS__ )
#define YC_FORWARD( ... ) static_cast< decltype(__VA_ARGS__)&& >( __VA_ARGS__ )

template< typename t_type >
inline void ycSwap( t_type& a, t_type& b )
{
	if( &a == &b ) { return; }

	t_type temp( YC_MOVE( a ) );
	a = YC_MOVE( b );
	b = YC_MOVE( temp );
}

template< typename t_type >
inline void ycReverse( t_type* begin, t_type* end )
{
	while( begin < end )
	{
		ycSwap( *( begin++ ), *( --end ) );
	}
}

#ifdef YC_MSVC
	#define YC_ZERO_SIZED_ARRAY( type, name ) \
		__pragma( warning( push ) )           \
		__pragma( warning( disable: 4200 ) )  \
			type name[];                      \
		__pragma( warning( pop ) )

	#define YC_INITIALIZER_THIS               \
		__pragma( warning( push ) )           \
		__pragma( warning( disable : 4355 ) ) \
		this                                  \
		__pragma( warning( pop ) )

	#if YC_NDA
	#endif
#else
	#define YC_ZERO_SIZED_ARRAY( type, name ) type name[];
	#define YC_INITIALIZER_THIS this
#endif

// Preprocesor token pasting.
#define YC_JOIN2( _A, _B ) _A##_B
#define YC_JOIN( _A, _B ) YC_JOIN2( _A, _B )

// Helper to allow ORing enum values together.
#define YC_ENUM_BITOPS( E ) \
	constexpr static inline YC_INLINE E  operator~ ( E a )				{ return (E)~(ureg)a; } \
	constexpr static inline YC_INLINE E  operator| ( E a, E b )			{ return (E)(a | (ureg)b); } \
	constexpr static inline YC_INLINE E  operator& ( E a, E b )			{ return (E)(a & (ureg)b); } \
	constexpr static inline YC_INLINE E  operator^ ( E a, E b )			{ return (E)(a ^ (ureg)b); } \
	constexpr static inline YC_INLINE E& operator|= ( E& a, E b )		{ a = (E)(a | (ureg)b); return a; } \
	constexpr static inline YC_INLINE E& operator&= ( E& a, E b )		{ a = (E)(a & (ureg)b); return a; } \
	constexpr static inline YC_INLINE E& operator^= ( E& a, E b )		{ a = (E)(a ^ (ureg)b); return a; }

#define YC_MEMBER_ENUM_BITOPS( E ) \
	constexpr friend inline YC_INLINE E  operator~ ( E a )		{ return (E)~(ureg)a; } \
	constexpr friend inline YC_INLINE E  operator| ( E a, E b )	{ return (E)(a | (ureg)b); } \
	constexpr friend inline YC_INLINE E  operator& ( E a, E b )	{ return (E)(a & (ureg)b); } \
	constexpr friend inline YC_INLINE E  operator^ ( E a, E b )	{ return (E)(a ^ (ureg)b); } \
	constexpr friend inline YC_INLINE E& operator|= ( E& a, E b ){ a = (E)(a | (ureg)b); return a; } \
	constexpr friend inline YC_INLINE E& operator&= ( E& a, E b ){ a = (E)(a & (ureg)b); return a; } \
	constexpr friend inline YC_INLINE E& operator^= ( E& a, E b ){ a = (E)(a ^ (ureg)b); return a; }

// ycFunction - std::function replacement (generic container for any object callable with a specific set of arguments)
// Differences from std::function:
//    capture size is strictly limited, no overflow (will not allocate unless as part of the copy of a capture-by-value)
//    cannot be copied or moved, limited to assignment from a callable type of the appropriate signature
// --------------------------
template <typename>
class ycFunction; // no definition, so bad syntax will result in compile error

template <typename ReturnValue, typename ... Args>
class ycFunction<ReturnValue( Args... )> 
{
public:
	ycFunction() {}

	template <typename t_lambda>
	ycFunction( t_lambda&& ll )
	{
		static_assert( sizeof( CallableImpl<t_lambda> ) <= sizeof( callableBuf ), "ycFunction is limited to capturing 48 bytes" );
		static_assert( alignof( CallableImpl<t_lambda> ) <= alignof( ycFunction ), "Capturing something with high alignment" );
		m_callable = new ( callableBuf ) CallableImpl<t_lambda>( YC_FORWARD( ll ) );
	}

	template <typename t_lambda>
	ycFunction& operator=( t_lambda&& ll )
	{
		static_assert( sizeof( CallableImpl<t_lambda> ) <= sizeof( callableBuf ), "ycFunction is limited to capturing 48 bytes" );
		static_assert( alignof( CallableImpl<t_lambda> ) <= alignof( ycFunction ), "Capturing something with high alignment" );
		if( m_callable ) { m_callable->~Callable(); }
		m_callable = new ( callableBuf ) CallableImpl<t_lambda>( YC_FORWARD( ll ) );
		return *this;
	}

	template <typename ... Args2>
	ReturnValue operator()( Args2&&... args ) const 
	{
		ycAssert( IsValid() );
		ycAssert( m_callable == (Callable*)callableBuf );
		return m_callable->Invoke( YC_FORWARD( args )... );
	}

	template <typename ... Args2>
	ReturnValue Invoke( Args2&&... args ) const
	{
		ycAssert( IsValid() );
		ycAssert( m_callable == (Callable*)callableBuf );
		return m_callable->Invoke( YC_FORWARD( args )... );
	}

	bool IsValid() const
	{
		return m_callable != nullptr;
	}

	void Clear()
	{
		if( m_callable != nullptr )
		{
			ycAssert( m_callable == (Callable*)callableBuf );
			m_callable->~Callable();
			m_callable = nullptr;
		}
	}

	~ycFunction()
	{
		Clear();
	}

	ycFunction( const ycFunction& ) = delete;
	ycFunction( ycFunction&& ) = delete;
	ycFunction& operator=( const ycFunction& ) = delete;
	ycFunction& operator=( ycFunction&& ) = delete;

private:

	// Callable and CallableImpl type-erase the specific lambda type so it can be accessed through a vptr
	class Callable 
	{
	public:
		virtual ~Callable() = default;
		virtual ReturnValue Invoke( Args... ) = 0;
		virtual Callable* CopyInto( void* ) const = 0;
		virtual Callable* MoveInto( void* ) = 0;
	};

	template <typename t_lambda>
	class CallableImpl : public Callable 
	{
	public:
		CallableImpl( const t_lambda& ll )
			: m_l( ll ) {}

		CallableImpl( t_lambda&& ll )
			: m_l( YC_FORWARD( ll ) ) {}

		ReturnValue Invoke( Args... args ) override 
		{
			return m_l( args... );
		}

		Callable* CopyInto( void* buf ) const override
		{
			return new ( buf ) CallableImpl( m_l );
		}

		Callable* MoveInto( void* buf ) override
		{
			return new ( buf ) CallableImpl( YC_MOVE( m_l ) );
		}

	private:
		t_lambda m_l;
	};

	// Buffer to store the Callable (vptr + captured data) 
	// vptr takes 8 bytes, leaving 48 for captures
	u8 callableBuf[ 56 ];
	Callable* m_callable = nullptr;
};

#endif // !YC_STD_H
