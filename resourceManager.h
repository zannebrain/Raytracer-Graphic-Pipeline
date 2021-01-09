#pragma once

#include <assert.h>
#include <vector>
#include "geom.h"
#include "bitmap.h"

class ResourceManager
{
private:
	std::vector<vertexBuffer_t>	vertexBuffers;
	std::vector<indexBuffer_t>	indexBuffers;
	std::vector<Model>			modelBuffer;
	std::vector<Bitmap>			imageBuffer; // TODO: use image class instead of bitmap
public:

	static const uint32_t InvalidModelIx = ~0x00;

	uint32_t AllocVB()
	{
		vertexBuffers.resize( vertexBuffers.size() + 1 );
		return static_cast<uint32_t>( vertexBuffers.size() - 1 );
	}

	uint32_t AllocIB()
	{
		indexBuffers.resize( indexBuffers.size() + 1 );
		return static_cast<uint32_t>( indexBuffers.size() - 1 );
	}

	uint32_t AllocModel()
	{
		modelBuffer.push_back( Model() );
		return static_cast<uint32_t>( modelBuffer.size() - 1 );
	}

	uint32_t StoreImageCopy( const Bitmap& image )
	{
		imageBuffer.push_back( image );
		return static_cast<uint32_t>( imageBuffer.size() - 1 );
	}

	void AddVertex( const uint32_t vbIx, const vertex_t& vertex )
	{
		vertexBuffers[ vbIx ].buffer.push_back( vertex );
	}

	void AddIndex( const uint32_t ibIx, const uint32_t index )
	{
		indexBuffers[ ibIx ].buffer.push_back( index );
	}

	uint32_t GetVbOffset( const uint32_t vbIx ) const
	{
		return vertexBuffers[ vbIx ].buffer.size();
	}

	uint32_t GetIbOffset( const uint32_t ibIx ) const
	{
		return indexBuffers[ ibIx ].buffer.size();
	}

	vertex_t* GetVertex( const uint32_t vbIx, const uint32_t i )
	{
		if ( vbIx >= vertexBuffers.size() )
		{
			assert( false );
			return nullptr;
		}

		vertexBuffer_t& vb = vertexBuffers[ vbIx ];
		if ( i >= vb.buffer.size() )
		{
			assert( false );
			return nullptr;
		}

		return &vb.buffer[ i ];
	}

	uint32_t GetIndex( const uint32_t ibIx, const uint32_t i ) const
	{
		if ( ibIx >= indexBuffers.size() )
		{
			assert( false );
			return 0;
		}

		const indexBuffer_t& ib = indexBuffers[ ibIx ];
		if ( i >= ib.buffer.size() )
		{
			assert( false );
			return 0;
		}

		return ib.buffer[ i ];
	}

	Model* GetModel( const uint32_t modelIx )
	{
		if ( modelIx >= modelBuffer.size() )
		{
			assert( false );
			return nullptr;
		}

		return &modelBuffer[ modelIx ];
	}

	const Bitmap* GetImageRef( const uint32_t imageIx ) const
	{
		if ( imageIx >= imageBuffer.size() )
		{
			assert( false );
			return nullptr;
		}

		return &imageBuffer[ imageIx ];
	}
};
