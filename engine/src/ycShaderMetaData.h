#pragma once

/*
\struct ycShaderMetaData


*/

#include "ycCommon.h"
#include "ycStd.h"

struct ycShaderMetaData
{
	enum
	{
		kAttributeMapping,
		kUniformBufferMapping,
		kSamplerMapping,

		kEnd
	};

	struct Item
	{
		Item( const uint32_t _type ) : type(_type) {}
		uint32_t type;
	};

	enum
	{
		kPadNamesToAlign = 4
	};

	struct AttributeMapping : public Item
	{
		AttributeMapping() : Item(kAttributeMapping) {}
		uint32_t index;
		uint32_t nameSize; //!< number of bytes to the next item, this will be arbitrarily greater than strlen(name)
		YC_ZERO_SIZED_ARRAY( char, name )
	};

	struct UniformBufferMapping : public Item
	{
		UniformBufferMapping() : Item(kUniformBufferMapping) {}
		uint32_t index;
		uint32_t descriptorSet;
		uint32_t descriptorBinding;
		uint32_t nameSize; //!< number of bytes to the next item, this will be arbitrarily greater than strlen(name)
		YC_ZERO_SIZED_ARRAY( char, name )
	};

	struct SamplerMapping : public Item
	{
		SamplerMapping() : Item(kSamplerMapping) {}
		uint32_t index;
		uint32_t descriptorSet;
		uint32_t descriptorBinding;
		uint32_t nameSize; //!< number of bytes to the next item, this will be arbitrarily greater than strlen(name)
		YC_ZERO_SIZED_ARRAY( char, name )
	};

	class Iterator
	{
	public:

		/*! Returns the type of the next item in the meta data, use this to
		decide what to cast GetNext() as.  Returns kEnd when no items remain.
		*/
		uint32_t GetNextType() const { return reinterpret_cast< Item* >( curItem )->type; }

		//! Returns the next item and advances the iterator
		template< class t_itemType >
		t_itemType* GetNext()
		{
			t_itemType* item = (t_itemType*)curItem;
			Advance();
			return item;
		}

		Iterator( ycShaderMetaData* metaData )
		{
			curItem = metaData->data;
		}

		void Advance()
		{
			switch( GetNextType() )
			{
			case kAttributeMapping:
				curItem += sizeof(AttributeMapping) + reinterpret_cast<AttributeMapping*>(curItem)->nameSize;
				break;
			case kUniformBufferMapping:
				curItem += sizeof(UniformBufferMapping) + reinterpret_cast<UniformBufferMapping*>(curItem)->nameSize;
				break;
			case kSamplerMapping:
				curItem += sizeof(SamplerMapping) + reinterpret_cast<SamplerMapping*>(curItem)->nameSize;
				break;
			case kEnd: default: break;
			}
		}

	protected:
		uint8_t* curItem;
	};

	YC_ZERO_SIZED_ARRAY( uint8_t, data )
};
