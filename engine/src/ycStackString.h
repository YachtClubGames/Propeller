#pragma once

/*! \class ycStackString

A mutable string that resides on the stack but can grow past its templated size unless set to not be growable

*/

#include "ycString.h"

//256 = YC_MAX_PATH
template <ycSize_t S=256> 
class ycStackString : public ycString
{
public:
	ycStackString() : ycString() 
	{
		SetMemoryManagedExternal();
		SetGrowable();
		m_capacity = S;
		m_str = m_buffer;
		m_alloc = m_str;
		m_str[0] = '\0';
	}

	ycStackString( const ycStackString<S>& other )
		: ycString()
	{
		SetMemoryManagedExternal();
		SetGrowable();
		m_capacity = S;
		m_str = m_buffer;
		m_alloc = m_str;
		Set( other );
	}

	ycStackString( ycStackString<S>&& other ) noexcept
		: ycString()
	{
		SetGrowable();
		if( !other.IsMemoryManagedExternal() )
		{
			m_str = other.m_str;
			m_alloc = other.m_alloc;
			m_mem = other.m_mem;
			m_len = other.m_len;
			m_capacity = other.m_capacity;

			other.m_str = nullptr;
			other.m_alloc = nullptr;
			other.m_capacity = 0;
		}
		else
		{
			SetMemoryManagedExternal();
			m_capacity = S;
			m_str = m_buffer;
			m_alloc = m_str;
			Set( other );
		}
	}

	explicit ycStackString( const ycStringRef& r ) 
		: ycString()
	{
		SetMemoryManagedExternal();
		SetGrowable();
		m_capacity = S;
		m_str = m_buffer;
		m_alloc = m_str;
		if( r )
		{
			Set( r );
		}
		else
		{
			m_str[0] = '\0';
		}
	}

	explicit ycStackString( const char *str ) 
		: ycString()
	{
		SetMemoryManagedExternal();
		SetGrowable();
		m_capacity = S;
		m_str = m_buffer;
		m_alloc = m_str;
		if( str )
		{
			Set( str );
		}
		else
		{
			m_str[0] = '\0';
		}
	}

	explicit ycStackString( const char *str, ycSize_t len ) 
		: ycString()
	{
		SetMemoryManagedExternal();
		SetGrowable();
		m_capacity = S;
		m_str = m_buffer;
		m_alloc = m_str;
		if( str )
		{
			Set( str, len );
		}
		m_str[len] = '\0';
	}

	ycStackString& operator =( const char* rhs )
	{
		Set( rhs );
		return *this;
	}

	ycStackString& operator =( const ycStackString<S>& rhs )
	{
		Set( rhs );
		return *this;
	}

	ycStackString& operator =( ycStackString<S>&& rhs ) noexcept
	{
		if( !rhs.IsMemoryManagedExternal() )
		{
			SetMemoryManagedExternal( false );
			m_str = rhs.m_str;
			m_alloc = rhs.m_alloc;
			m_mem = rhs.m_mem;
			m_len = rhs.m_len;
			m_capacity = rhs.m_capacity;

			rhs.m_str = nullptr;
			rhs.m_alloc = nullptr;
			rhs.m_capacity = 0;
		}
		else
		{
			SetMemoryManagedExternal( true );
			m_capacity = S;
			m_str = m_buffer;
			m_alloc = m_str;
			Set( rhs );
		}
		return *this;
	}

protected:

	char m_buffer[ S ];
};


