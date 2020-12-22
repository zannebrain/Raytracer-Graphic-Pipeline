#pragma once

/*
===================================
BlendColors
- Alpha blend two R8G8B8A8 values
===================================
*/
uint32_t BlendColors( uint32_t c1, uint32_t c2 )
{
	uint32_t color1 = ( 0xFFFFFF00 & c1 );
	uint32_t color2 = ( 0xFFFFFF00 & c2 );
	double alpha1 = ( (double) ( 0x000000FF & c1 ) ) / 255.0f;
	double alpha2 = ( (double) ( 0x000000FF & c2 ) ) / 255.0f;

	return   ( (uint32_t) ( ( color1 & 0xFF000000 ) * alpha1 + ( color2 & 0xFF000000 ) * alpha2 * ( 1 - alpha1 ) ) & 0xFF000000 )
		| ( (uint32_t) ( ( color1 & 0x00FF0000 ) * alpha1 + ( color2 & 0x00FF0000 ) * alpha2 * ( 1 - alpha1 ) ) & 0x00FF0000 )
		| ( (uint32_t) ( ( color1 & 0x0000FF00 ) * alpha1 + ( color2 & 0x0000FF00 ) * alpha2 * ( 1 - alpha1 ) ) & 0x0000FF00 );
}


/*
===================================
CompositeBitmaps
===================================
*/
Bitmap CompositeBitmaps( Bitmap& bitmap1, Bitmap& bitmap2 )
{
	Bitmap tbitmap( bitmap1.GetWidth(), bitmap1.GetHeight() );

	for ( uint32_t j = 0; j < bitmap1.GetHeight(); ++j ) {
		for ( uint32_t i = 0; i < bitmap1.GetWidth(); ++i ) {

			uint32_t newColor = BlendColors( bitmap1.GetPixel( i, j ), bitmap2.GetPixel( i, j ) );

			tbitmap.SetPixel( i, j, newColor );
		}
	}

	return tbitmap;
}


/*
===================================
DrawLine
- Optimized bresenham algorithm
===================================
*/
void DrawLine( Bitmap& bitmap, int32_t x0, int32_t y0, int32_t x1, int32_t y1, const Color& color )
{
	const int dx = abs( x1 - x0 );
	const int dy = abs( y1 - y0 );
	const int sx = ( x0 < x1 ) ? 1 : -1;
	const int sy = ( y0 < y1 ) ? 1 : -1;

	int32_t e = ( dx - dy );

	while ( ( x0 != x1 ) || ( y0 != y1 ) )
	{
		bitmap.SetPixel( x0, y0, color.AsR8G8B8A8() );

		const int e2 = 2 * e;

		if ( e2 > -dy )
		{
			e -= dy;
			x0 += sx;
		}

		if ( e2 < dx )
		{
			e += dx;
			y0 += sy;
		}
	}

	bitmap.SetPixel( x0, y0, color.AsR8G8B8A8() );
}

/*
===================================
FloodFill
- Performs standard floodfill algorithm
===================================
*/
void FloodFill( Bitmap& bitmap, uint32_t x, uint32_t y, uint32_t tcolor, uint32_t rcolor )
{
	bool outBounds = ( x >= bitmap.GetWidth() || x < 0 ) || ( y >= bitmap.GetHeight() || y < 0 );
	if ( outBounds )
	{
		return;
	}

	if ( bitmap.GetPixel( x, y ) != tcolor )
	{
		return;
	}

	struct node_t
	{
		node_t( uint32_t xi, uint32_t yi ) : x( xi ), y( yi ) {};
		uint32_t x, y;
	};

	std::deque<node_t> q;
	q.push_back( node_t( x, y ) );

	while ( !q.empty() ) {

		node_t n = q.front();
		q.pop_front();

		if ( bitmap.GetPixel( n.x, n.y ) == tcolor ) {
			node_t w( n ), e( n );

			for ( ; ( bitmap.GetPixel( w.x - 1, w.y ) == tcolor ) && ( w.x != 0 ); w.x-- ); // Find min horizonal bond
			for ( ; ( bitmap.GetPixel( e.x + 1, e.y ) == tcolor ) && ( e.x != bitmap.GetWidth() - 1 ); e.x++ ); // Find max bond

			for ( uint32_t i = w.x; i <= e.x; ++i )		// Fill line
				bitmap.SetPixel( i, w.y, rcolor );

			for ( uint32_t i( w.x + 1 ); i < e.x; ++i ) { // Explore outside of filled line
				node_t t_node( i, 0 );

				t_node.y = w.y + 1;	// North of node
				if ( ( t_node.y > 0 ) && ( bitmap.GetPixel( t_node.x, t_node.y ) == tcolor ) )	q.push_back( t_node );

				t_node.y = w.y - 1;	// South of node
				if ( ( t_node.y < bitmap.GetHeight() ) && ( bitmap.GetPixel( t_node.x, t_node.y ) == tcolor ) )	q.push_back( t_node );
			}
		}
	}

	return;
}


/*
===================================
ApplyBlur
- Performs blur on image
===================================
*/
void ApplyBlur( Bitmap& bitmap, Bitmap& output )
{
	uint32_t kernelDim = 3;

	int32_t values[] = { 0, 0, 0,
							0, 1, 0,
							0, 0, 0 };

	const uint32_t kernelWidth = 3;
	const uint32_t kernelSize = ( kernelWidth * kernelWidth );

	using matKern_t = Matrix<kernelWidth, kernelWidth, int32_t>;

	matKern_t kernel( values );
	double divisor = 1; // shouldn't blur more than # pixels in covolution
	uint8_t mask = 0xE;

	for ( uint32_t i = ( kernelDim / 2 ); i < bitmap.GetWidth() - ( kernelDim / 2 ); ++i ) {

		for ( uint32_t j = ( kernelDim / 2 ); j < bitmap.GetHeight() - ( kernelDim / 2 ); ++j ) {

			uint32_t kValues[] = { bitmap.GetPixel( i - 1, j - 1 ), bitmap.GetPixel( i, j - 1 ), bitmap.GetPixel( i + 1, j - 1 ),
									bitmap.GetPixel( i - 1, j ), bitmap.GetPixel( i, j ), bitmap.GetPixel( i + 1, j ),
									bitmap.GetPixel( i - 1, j + 1 ), bitmap.GetPixel( i, j + 1 ), bitmap.GetPixel( i + 1, j + 1 )
			};

			RGBA rgba = Color( kValues[ 4 ] ).AsRGBA();

			//RED CHANNEL
			int32_t rSum = rgba.r;

			if ( mask & 0x8 ) {
				int32_t rChannel[ kernelSize ];
				for ( uint32_t v = 0; v < kernelSize; ++v )
				{
					rChannel[ v ] = Color( kValues[ v ] ).AsRGBA().r;
				}

				Matrix<3, 3, int32_t> rSample( rChannel );
				rSum = Convolution( rSample, kernel );
				rSum = (int32_t) ( (double) rSum / divisor );
			}

			// GREEN CHANNEL
			int32_t gSum = rgba.g;

			if ( mask & 0x4 ) {
				int32_t gChannel[ kernelSize ];
				for ( uint32_t v = 0; v < kernelSize; ++v )
					gChannel[ v ] = Color( kValues[ v ] ).AsRGBA().g;

				matKern_t g_sample( gChannel );
				gSum = Convolution( g_sample, kernel );
				gSum = (int32_t) ( (double) gSum / divisor );
			}

			// BLUE CHANNEL
			int32_t bSum = rgba.b;

			if ( mask & 0x2 ) {
				int32_t bChannel[ kernelSize ];
				for ( uint32_t v = 0; v < kernelSize; ++v )
					bChannel[ v ] = Color( (uint32_t) kValues[ v ] ).AsRGBA().b;

				matKern_t bSample( bChannel );
				bSum = Convolution( bSample, kernel );
				bSum = (int32_t) ( (double) bSum / divisor );
			}

			// ALPHA CHANNEL
			int32_t aSum = rgba.a;

			if ( mask & 0x1 ) {
				int32_t aChannel[ kernelSize ];
				for ( uint32_t v = 0; v < kernelSize; ++v )
					aChannel[ v ] = Color( kValues[ v ] ).AsRGBA().a;

				matKern_t aSample( aChannel );
				aSum = Convolution( aSample, kernel );
				aSum = (int32_t) ( (double) aSum / divisor );
			}

			//if(r_sum > 0xFF) cout << r_sum << endl;

			//if(r_sum < 0) r_sum = ~abs(r_sum);
			//if(g_sum < 0) g_sum = ~abs(g_sum);
			//if(b_sum < 0) b_sum = ~abs(b_sum);
			//if(a_sum < 0) r_sum = ~r_sum;

			Pixel pc;
			pc.rgba.r = (uint32_t) ( rSum < 0 ? 0 : rSum );
			pc.rgba.g = (uint32_t) ( gSum < 0 ? 0 : gSum );
			pc.rgba.b = (uint32_t) ( bSum < 0 ? 0 : bSum );
			pc.rgba.a = (uint32_t) ( aSum < 0 ? 0 : aSum );

			//cout << hex << convo.GetPixel(i,j) << endl;
			//cout << hex << pc.rawGetColor() << endl;

			output.SetPixel( i, j, pc.r8g8b8a8 );
		}
	}

	for ( uint32_t i = 0; i < bitmap.GetWidth(); ++i ) {

		for ( uint32_t j = 0; j < bitmap.GetHeight(); ++j ) {

			output.SetPixel( i, j, bitmap.GetPixel( i, j ) | output.GetPixel( i, j ) );
		}
	}

	output.Write( "convolution.bmp" );
}


/*
===================================
ProjectPoint
- Projects point from world space into screen space
===================================
*/
bool ProjectPoint( const mat4x4d& mvp, const vec2i& screenSize, const bool invertY, const vec4d& worldSpacePt, vec2d& outPoint )
{
	// Clip-Space
	vec4d csPt = mvp * vec4d( worldSpacePt[ 0 ], worldSpacePt[ 1 ], worldSpacePt[ 2 ], 1.0 );

	// Normalized-Device-Coordinates
	const double w = csPt[ 3 ] + 1e-7;
	vec4d ndsPt = vec4d( csPt[ 0 ] / w, csPt[ 1 ] / w, csPt[ 2 ] / w, w );

	// Screen-Space
	outPoint = vec2d( 0.5 * screenSize[ 0 ] * ( ndsPt[ 0 ] + 1.0 ), 0.5 * screenSize[ 1 ] * ( ndsPt[ 1 ] + 1.0 ) );

	if ( invertY )
	{
		outPoint[ 1 ] = screenSize[ 1 ] - outPoint[ 1 ];
	}

	return false;
}