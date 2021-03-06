#include "../GfxCore/bitmap.h"
#include "../GfxCore/mathVector.h"
#include "../GfxCore/matrix.h"
#include "../GfxCore/image.h"
#include "scene.h"
#include "debug.h"
#include "globals.h"
#include "../GfxCore/resourceManager.h"
#include "../GfxCore/octree.h"
#include "../GfxCore/util.h"

Image<float> zBuffer( RenderWidth, RenderHeight, 1.0f, "_zbuffer" );

extern Scene scene;
extern Image<float> depthBuffer;
extern ResourceManager rm;

void OrthoMatrixToAxis( const mat4x4d& m, vec3d& origin, vec3d& xAxis, vec3d& yAxis, vec3d& zAxis );
void DrawWorldAxis( Image<Color>& image, const SceneView& view, double size, const vec3d& origin, const vec3d& X, const vec3d& Y, const vec3d& Z );

struct vertexOut_t
{
	vec4d	wsPosition[ 3 ];
	vec4d	viewPosition[ 3 ];
	vec4d	clipPosition[ 3 ];
	vec4d	ndc[ 3 ];
	vec3d	normal[ 3 ];
	vec2d	uv[ 3 ];
	Color	color[ 3 ];
};


struct fragmentInput_t
{
	vec4d	wsPosition;
	vec4d	clipPosition;
	vec3d	normal;
	vec2d	uv;
	Color	color;
};


void DrawCube( Image<Color>& image, const SceneView& view, const vec4d& minCorner, const vec4d& maxCorner, Color color = Color::Green )
{
	vec4d corners[ 8 ] = {
		// Bottom
		vec4d( minCorner[ 0 ], minCorner[ 1 ], minCorner[ 2 ], 1.0 ),
		vec4d( maxCorner[ 0 ], minCorner[ 1 ], minCorner[ 2 ], 1.0 ),
		vec4d( maxCorner[ 0 ], maxCorner[ 1 ], minCorner[ 2 ], 1.0 ),
		vec4d( minCorner[ 0 ], maxCorner[ 1 ], minCorner[ 2 ], 1.0 ),
		// Top
		vec4d( minCorner[ 0 ], minCorner[ 1 ], maxCorner[ 2 ], 1.0 ),
		vec4d( maxCorner[ 0 ], minCorner[ 1 ], maxCorner[ 2 ], 1.0 ),
		vec4d( maxCorner[ 0 ], maxCorner[ 1 ], maxCorner[ 2 ], 1.0 ),
		vec4d( minCorner[ 0 ], maxCorner[ 1 ], maxCorner[ 2 ], 1.0 ),
	};

	int edges[ 12 ][ 2 ] = {
		// Bottom
		{ 0, 1 },
		{ 0, 3 },
		{ 1, 2 },
		{ 2, 3 },
		// Top
		{ 4, 5 },
		{ 4, 7 },
		{ 5, 6 },
		{ 6, 7 },
		// Sides
		{ 0, 4 },
		{ 1, 5 },
		{ 2, 6 },
		{ 3, 7 },
	};

	vec2i ssPts[ 8 ];

	for ( int i = 0; i < 8; ++i )
	{
		vec4d pt;
		ProjectPoint( view.projView, RenderSize, corners[ i ], pt );
		ssPts[ i ] = vec2i( static_cast<int32_t>( pt[ 0 ] ), static_cast<int32_t>( pt[ 1 ] ) );
	}

	for ( int i = 0; i < 12; ++i )
	{
		vec2i  pt0 = ssPts[ edges[ i ][ 0 ] ];
		vec2i  pt1 = ssPts[ edges[ i ][ 1 ] ];

		color.a() = 0.4f;
		DrawLine( image, pt0[ 0 ], pt0[ 1 ], pt1[ 0 ], pt1[ 1 ], color );
	}

	for ( int i = 0; i < 8; ++i )
	{
		image.SetPixel( ssPts[ i ][ 0 ], ssPts[ i ][ 1 ], Color::Red );
	}
}


void DrawWorldAxis( Image<Color>& image, const SceneView& view, double size, const vec3d& origin, const vec3d& X, const vec3d& Y, const vec3d& Z )
{
	vec4d points[ 4 ] = {
		{ vec4d( origin, 1.0 ) },
		{ vec4d( origin + size * X.Normalize(), 1.0 ) },
		{ vec4d( origin + size * Y.Normalize(), 1.0 ) },
		{ vec4d( origin + size * Z.Normalize(), 1.0 ) },
	};

	vec2i ssPts[ 4 ];
	for ( int i = 0; i < 4; ++i )
	{
		vec4d pt;
		ProjectPoint( view.projView, RenderSize, points[ i ], pt );
		ssPts[ i ] = vec2i( static_cast<int32_t>( pt[ 0 ] ), static_cast<int32_t>( pt[ 1 ] ) );
	}

	DrawLine( image, ssPts[ 0 ][ 0 ], ssPts[ 0 ][ 1 ], ssPts[ 1 ][ 0 ], ssPts[ 1 ][ 1 ], Color::Red );
	DrawLine( image, ssPts[ 0 ][ 0 ], ssPts[ 0 ][ 1 ], ssPts[ 2 ][ 0 ], ssPts[ 2 ][ 1 ], Color::Green );
	DrawLine( image, ssPts[ 0 ][ 0 ], ssPts[ 0 ][ 1 ], ssPts[ 3 ][ 0 ], ssPts[ 3 ][ 1 ], Color::Blue );
	image.SetPixel( ssPts[ 0 ][ 0 ], ssPts[ 0 ][ 1 ], Color::Black );
}


void DrawWorldPoint( Image<Color>& image, const SceneView& view, const vec4d& point, const int32_t size, const Color& color )
{
	vec4d projPt;
	ProjectPoint( view.projView, view.targetSize, point, projPt );

	vec2i ssPt = vec2i( static_cast<int32_t>( projPt[ 0 ] ), static_cast<int32_t>( projPt[ 1 ] ) );
	vec2d halfPt = vec2d( 0.5 * ( (double)size + 0.5 ) );
	vec2d start = Trunc<4, 2>( projPt ) - halfPt;

	int32_t i0 = static_cast<int32_t>( start[ 0 ] );
	int32_t iN = i0 + size;

	int32_t j0 = static_cast<int32_t>( start[ 1 ] );
	int32_t jN = j0 + size;

	for ( int32_t j = j0; j < jN; ++j )
	{
		for ( int32_t i = i0; i < iN; ++i )
		{
			image.SetPixel( i, j, color );
		}
	}
}


void DrawRay( Image<Color>& image, const SceneView& view, const Ray& ray, const Color& color )
{
	vec4d ssPt[ 2 ];
	vec4d wsPt[ 2 ];
	wsPt[ 0 ] = vec4d( ray.o, 1.0 );
	wsPt[ 1 ] = vec4d( ray.GetEndPoint(), 1.0 );
	ProjectPoint( view.projView, view.targetSize, wsPt[ 0 ], ssPt[ 0 ] );
	ProjectPoint( view.projView, view.targetSize, wsPt[ 1 ], ssPt[ 1 ] );
	DrawLine( image, (int)ssPt[ 0 ][ 0 ], (int)ssPt[ 0 ][ 1 ], (int)ssPt[ 1 ][ 0 ], (int)ssPt[ 1 ][ 1 ], color.AsR8G8B8A8() );
}


template<typename T>
void DrawOctree( Image<Color>& image, const SceneView& view, const Octree<T>& octree, const Color& color )
{
	AABB bounds = octree.GetAABB();
	DrawCube( image, view, vec4d( bounds.min, 1.0 ), vec4d( bounds.max, 1.0 ), color );

	auto nodeEnd = octree.children.end();
	for ( auto node = octree.children.begin(); node != nodeEnd; ++node )
	{
		DrawOctree( image, view, *node, color );
	}
}


bool VertexShader( const SceneView& view, const Triangle& tri, vertexOut_t& outVertex )
{
	const mat4x4d& mvp = view.projView;

	const vec4d* wsPts[ 3 ];
	vec4d ssPts[ 3 ];
	bool culled = false;

	wsPts[ 0 ] = &tri.v0.pos;
	wsPts[ 1 ] = &tri.v1.pos;
	wsPts[ 2 ] = &tri.v2.pos;

	ProjectPoint( mvp, RenderSize, *wsPts[ 0 ], ssPts[ 0 ] );
	ProjectPoint( mvp, RenderSize, *wsPts[ 1 ], ssPts[ 1 ] );
	ProjectPoint( mvp, RenderSize, *wsPts[ 2 ], ssPts[ 2 ] );

	const bool nearClip = ( ssPts[ 0 ][ 2 ] < -1.0 ) && ( ssPts[ 1 ][ 2 ] < -1.0 ) && ( ssPts[ 2 ][ 2 ] < -1.0 );

	if( !culled && !nearClip )
	{
		outVertex.clipPosition[ 0 ] = ssPts[ 0 ];
		outVertex.wsPosition[ 0 ] = tri.v0.pos;
		outVertex.color[ 0 ] = tri.v0.color;
		outVertex.uv[ 0 ] = tri.v0.uv;
		outVertex.normal[ 0 ] = tri.v0.normal;

		outVertex.clipPosition[ 1 ] = ssPts[ 1 ];
		outVertex.wsPosition[ 1 ] = tri.v1.pos;
		outVertex.color[ 1 ] = tri.v1.color;
		outVertex.uv[ 1 ] = tri.v1.uv;
		outVertex.normal[ 1 ] = tri.v1.normal;

		outVertex.clipPosition[ 2 ] = ssPts[ 2 ];
		outVertex.wsPosition[ 2 ] = tri.v2.pos;
		outVertex.color[ 2 ] = tri.v2.color;
		outVertex.uv[ 2 ] = tri.v2.uv;
		outVertex.normal[ 2 ] = tri.v2.normal;

		return true;
	}
	else
	{
		return false;
	}
}


bool EmitFragment( const vec3d& baryPt, const vertexOut_t& vo, fragmentInput_t& outFragment )
{
	if( ( baryPt[ 0 ] < 0.0 ) || ( baryPt[ 1 ] < 0.0 ) || ( baryPt[ 2 ] < 0.0 ) )
	{
		return false;
	}

	if ( ( baryPt[ 0 ] > 1.0 ) || ( baryPt[ 1 ] > 1.0 ) || ( baryPt[ 2 ] > 1.0 ) )
	{
	//	return false;
	}

	outFragment.wsPosition = Interpolate( baryPt, vo.wsPosition ); // TODO: is this ok?
	outFragment.clipPosition = Interpolate( baryPt, vo.clipPosition );
	outFragment.normal = Interpolate( baryPt, vo.normal );
	outFragment.uv = Interpolate( baryPt, vo.uv );
	outFragment.color = Interpolate( baryPt, vo.color );

	return true;
}


bool PixelShader( const fragmentInput_t & frag )
{
	return false;
}


void RasterScene( Image<Color>& image, const SceneView& view, bool wireFrame = true )
{
	mat4x4d mvp = view.projTransform * view.viewTransform;
	const uint32_t modelCnt = scene.models.size();

#if DRAW_WIREFRAME
	for ( uint32_t m = 0; m < modelCnt; ++m )
	{
		const ModelInstance& model = scene.models[ m ];
		const Triangle* triCache = model.triCache.data();

		const size_t triCnt = model.triCache.size();
		for ( uint32_t i = 0; i < triCnt; ++i )
		{
			vertexOut_t vo;
			if ( !VertexShader( view, triCache[ i ], vo ) )
			{
				continue;
			}

#if USE_RASTERIZE
			if ( !wireFrame )
			{
				// Scanline Rasterizer
				AABB ssBox;
				for ( int i = 0; i < 3; ++i )
				{
					ssBox.Expand( Trunc<4, 1>( vo.clipPosition[ i ] ) );
				}

				const vec3d tPt0 = Trunc<4, 1>( vo.clipPosition[ 0 ] );
				const vec3d tPt1 = Trunc<4, 1>( vo.clipPosition[ 1 ] );
				const vec3d tPt2 = Trunc<4, 1>( vo.clipPosition[ 2 ] );

				const int32_t x0 = std::max( 0,										static_cast<int>( ssBox.min[ 0 ] ) );
				const int32_t x1 = std::min( static_cast<int>( image.GetWidth() ),	static_cast<int>( ssBox.max[ 0 ] + 0.5 ) );
				const int32_t y0 = std::max( 0,										static_cast<int>( ssBox.min[ 1 ] ) );
				const int32_t y1 = std::min( static_cast<int>( image.GetHeight() ),	static_cast<int>( ssBox.max[ 1 ] + 0.5 ) );

				for ( int32_t y = y0; y <= y1; ++y )
				{
					for ( int32_t x = x0; x <= x1; ++x )
					{
						const vec3d baryPt = PointToBarycentric( vec3d( x, y, 0.0 ), tPt0, tPt1, tPt2 );

						fragmentInput_t fragmentInput;
						if( !EmitFragment( baryPt, vo, fragmentInput ) )
							continue;

						const float depth = (float)fragmentInput.clipPosition[ 2 ];

						if ( depth >= zBuffer.GetPixel( x, y ) )
							continue;

						const vec3d normal = fragmentInput.normal.Normalize();

						const light_t& L = scene.lights[ 0 ];
						const vec4d intensity = vec4d( L.intensity, 1.0f );

						vec3d lightDir = L.pos - Trunc<4, 1>( fragmentInput.wsPosition );
						lightDir = lightDir.Normalize();

						const vec3d viewVector = Trunc<4, 1>( view.camera.origin - fragmentInput.wsPosition ).Normalize();
						const Color viewDiffuse = Color( (float)Dot( viewVector, normal ) );

						const vec3d halfVector = ( viewVector + lightDir ).Normalize();

						Color surfaceColor = Color::Black;

						const material_t* material = rm.GetMaterialRef( triCache[ i ].materialId );

						if( material->textured )
						{
							const Image<Color>* texture = rm.GetImageRef( material->colorMapId );
							surfaceColor = texture->GetPixelUV( fragmentInput.uv[ 0 ], fragmentInput.uv[ 1 ] );
						}
						else
						{
							surfaceColor += fragmentInput.color;
						}

						const vec4d D = ColorToVector( Color( material->Kd ) );
						const vec4d S = ColorToVector( Color( material->Ks ) );

						const vec4d diffuseIntensity = Multiply( D, intensity ) * std::max( 0.0, Dot( normal, lightDir ) );
						const vec4d specularIntensity = S * pow( std::max( 0.0, Dot( normal, halfVector ) ), SpecularPower );
						const Color ambient = AmbientLight * ( Color( material->Ka ) * surfaceColor );

						Color shadingColor;
						shadingColor += Vec4dToColor( specularIntensity );
						shadingColor += Vec4dToColor( Multiply( diffuseIntensity, ColorToVector( surfaceColor ) ) );
						shadingColor += ambient;

						const Color normalColor = Vec3dToColor( 0.5 * normal + vec3d( 0.5 ) );
						
						image.SetPixel( x, y, LinearToSrgb( shadingColor ).AsR8G8B8A8() );
						zBuffer.SetPixel( x, y, depth );
					}
				}
			}
			else
#endif
			{
				Color color = vo.color[ 0 ];
				color.rgba().a = 0.1f;

				vec2i pxPts[ 3 ];
				pxPts[ 0 ] = vec2i( static_cast<int32_t>( vo.clipPosition[ 0 ][ 0 ] ), static_cast<int32_t>( vo.clipPosition[ 0 ][ 1 ] ) );
				pxPts[ 1 ] = vec2i( static_cast<int32_t>( vo.clipPosition[ 1 ][ 0 ] ), static_cast<int32_t>( vo.clipPosition[ 1 ][ 1 ] ) );
				pxPts[ 2 ] = vec2i( static_cast<int32_t>( vo.clipPosition[ 2 ][ 0 ] ), static_cast<int32_t>( vo.clipPosition[ 2 ][ 1 ] ) );

				DrawLine( image, pxPts[ 0 ][ 0 ], pxPts[ 0 ][ 1 ], pxPts[ 1 ][ 0 ], pxPts[ 1 ][ 1 ], color );
				DrawLine( image, pxPts[ 0 ][ 0 ], pxPts[ 0 ][ 1 ], pxPts[ 2 ][ 0 ], pxPts[ 2 ][ 1 ], color );
				DrawLine( image, pxPts[ 1 ][ 0 ], pxPts[ 1 ][ 1 ], pxPts[ 2 ][ 0 ], pxPts[ 2 ][ 1 ], color );
			}
		}
	}
#endif

	if ( wireFrame )
	{
		for ( uint32_t m = 0; m < modelCnt; ++m )
		{
			const ModelInstance& model = scene.models[ m ];
#if DRAW_AABB
			const AABB bounds = model.octree.GetAABB();
			DrawCube( image, view, vec4d( bounds.min, 1.0 ), vec4d( bounds.max, 1.0 ) );
			DrawCube( image, view, vec4d( bounds.min, 1.0 ), vec4d( bounds.max, 1.0 ) );
#endif
			vec3d origin;
			vec3d xAxis;
			vec3d yAxis;
			vec3d zAxis;
			OrthoMatrixToAxis( model.transform, origin, xAxis, yAxis, zAxis );
			DrawWorldAxis( image, view, 20.0, origin, xAxis, yAxis, zAxis );
		}
	}
	
	// DrawOctree( image, view, scene.models[ 0 ].octree, Color::Red );
}