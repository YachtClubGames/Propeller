#pragma once

/*! \class ycAtomic
*/

#include "ycCommon.h"

typedef enum ycMemoryOrder
{
	kMemoryOrder_Relaxed,
	kMemoryOrder_Consume,
	kMemoryOrder_Acquire,
	kMemoryOrder_Release,
	kMemoryOrder_AcqRel,
	kMemoryOrder_SeqCst
} ycMemoryOrder;

class ycAtomic
{
public:

	ycAtomic( const u32 initialValue = 0 ); //!< defaults to zero for safety... atomics are hard enough

	u32 Get() const;
	u32 Load( const ycMemoryOrder order = kMemoryOrder_SeqCst ) const;

	void Set( const u32 value );
	void Store( const u32 value, const ycMemoryOrder order = kMemoryOrder_SeqCst );

	bool CompareAndSwap( const u32 compare, const u32 swap ); //! returns true if the swap succeeded
	bool CompareAndSwap_Weak( const u32 compare, const u32 swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail );
	bool CompareAndSwap_Strong( const u32 compare, const u32 swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail );

	u32 Increment( const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u32 Decrement( const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value

	u32 BitwiseOr( const u32 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u32 BitwiseAnd( const u32 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u32 BitwiseXor( const u32 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value

	u32 Add( const u32 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u32 Subtract( const u32 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value

#if defined YC_MSFT
	#define YC_ATOMIC_HAVE_MSFT_WAITONADDRESS // Windows >=8 only
	//#define YC_ATOMIC_MAYBE_HAVE_MSFT_WAITONADDRESS // allow dynamic fallback
#endif

#if defined YC_ATOMIC_HAVE_MSFT_WAITONADDRESS || defined YC_ATOMIC_MAYBE_HAVE_MSFT_WAITONADDRESS
	static bool HasWaitOnAddress();
	// should we check for spurious wakeup? current code using this already protects against that -- however it is platform inconsistent!
	// - could add safe/unsafe variants
	void WaitOnAddress( const u32 val ); // waits until the value _differs_ from the passed value
	void WakeByAddressAll();
#endif

protected:

	u32 m_value;
};

/*! \class ycAtomicPtr
ycAtomicPtr exposes an atomic value the size of a pointer. The "Pointer" name is due only to the data size, not because
this is intended to only store pointers. 64 bit platforms generally have 64 bit atomics available, but 32 bit platforms
may not.
*/
class ycAtomicPtr
{
public:

	ycAtomicPtr( void* initialValue = nullptr );

	void* Get() const;
	template< class t_type > t_type* Get() const { return (t_type*)Get(); }
	uintptr_t GetUInt() const { return uintptr_t(Get()); }

	void* Load( const ycMemoryOrder order = kMemoryOrder_SeqCst ) const;
	template< class t_type > t_type* Load( const ycMemoryOrder order = kMemoryOrder_SeqCst ) const { return (t_type*)Load( order ); }
	uintptr_t LoadUInt( const ycMemoryOrder order = kMemoryOrder_SeqCst ) const { return uintptr_t(Load(order)); }

	void Set( void* value );
	void SetUInt( const uintptr_t value ) { Set( (void*)value ); }
	
	void Store( void* value, const ycMemoryOrder order = kMemoryOrder_SeqCst );
	void StoreUInt( const uintptr_t value, const ycMemoryOrder order = kMemoryOrder_SeqCst ) { Store( (void*)value, order ); }

	bool CompareAndSwap( void* compare, void* swap ); //! returns true if the swap succeeded
	bool CompareAndSwapUInt( const uintptr_t compare, const uintptr_t swap ) { return CompareAndSwap( (void*)compare, (void*)swap ); }
	
	bool CompareAndSwap_Weak( void* compare, void* swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail );
	bool CompareAndSwap_Strong( void* compare, void* swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail );
	bool CompareAndSwapUInt_Weak( const uintptr_t compare, const uintptr_t swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail ) { return CompareAndSwap_Weak( (void*)compare, (void*)swap, orderSuccess, orderFail ); }
	bool CompareAndSwapUInt_Strong( const uintptr_t compare, const uintptr_t swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail ) { return CompareAndSwap_Strong( (void*)compare, (void*)swap, orderSuccess, orderFail ); }

	u64 Increment( const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u64 Decrement( const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value

	u64 BitwiseOr( const u64 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u64 BitwiseAnd( const u64 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u64 BitwiseXor( const u64 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value

	u64 Add( const u64 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value
	u64 Subtract( const u64 val, const ycMemoryOrder order = kMemoryOrder_SeqCst ); //!< returns the previous value

protected:

	void* m_value;
};

typedef ycAtomicPtr ycAtomic64;
