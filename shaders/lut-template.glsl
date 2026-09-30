#version 320 es
precision highp float;

/* Enable 1DLUT code */
#define _1DLUT
//#define _1DLUTPNG
/* Enable 3DLUT code */
//#define _3DLUT
/* Cutoff bits before transformation */
//#define TVCUTOFF
/* No interpolation for 1DLUTs */
//#define NOINTERPOLATION

in vec2 v_texcoord;
uniform vec2 screen_size;
uniform sampler2D tex;
out vec4 fragColor;

#ifdef _1DLUT
	/* check vars */
	#ifndef _3DLUT
		#define O1DLUT
		#define PRIMARY1DCOLOR color
	#else
		#define PRIMARY1DCOLOR _3dlutcache
	#endif
	#ifdef _1DLUTPNG
		uniform sampler2D lut1;
		uniform vec2 lut_size1;
	#endif
	#define USELUT
#endif

#ifdef _3DLUT
	#define PRIMARY3DCOLOR color
	uniform sampler2D lut0;
	uniform vec2 lut_size0;

	const float mult = 1.0;
	#ifdef _1DLUT
		/* check vars */
		#define _13DLUT
	#else
		/* check vars */
		#define O3DLUT
		#define USELUT
	#endif
#endif

#ifdef USELUT
	//#define DEBUG
	#ifdef DEBUG
		/* Enable two different color transformations */
		#define TEST
		#ifdef TEST
			#ifdef _3DLUT
				/* Enable 3DLUT image preview + mixing */
				#define LUTPREVIEW
			#endif

			/* Where second transformation should start and how big */
			#define XCUTSTART screen_size.x/2.0
			#define YCUTSTART 0.0
			#define XCUTSPACING screen_size.x
			#define YCUTSPACING screen_size.y
			/* Change relative spacing to absolute position */
			#define ABSOLUTE
			#ifndef ABSOLUTE
				#define XABSOLUTEVAR XCUTSTART+XCUTSPACING
				#define YABSOLUTEVAR YCUTSTART+YCUTSPACING
			#else
				#define XABSOLUTEVAR XCUTSPACING
				#define YABSOLUTEVAR YCUTSPACING
			#endif

			/* Bypass TVCUTOFF */
			//#define REAL
			#ifndef REAL
				#define SECONDARYCOLOR color
			#else
				#define SECONDARYCOLOR realcolor
			#endif

			/* Select 1DLUT/13DLUT pathway */
			#ifdef _1DLUT
				//#define _1DSEL
			#endif

			/* Select 3DLUT pathway */
			#ifdef _3DLUT
				//#define _3DSEL
			#endif

			#ifdef _1DSEL
				#ifdef _3DLUT
					/* Enable 13DLUT */
					//#define COL3D
					#ifdef COL3D
						#define SECONDARY1DCOLOR _3dlutsecondcache
					#endif
				#endif

				#ifndef SECONDARY1DCOLOR
					#define SECONDARY1DCOLOR SECONDARYCOLOR
				#endif

				#define SECONDARYFUNCT _1dlutfunct(SECONDARY1DCOLOR)
			#elif defined _3DSEL
				#define SECONDARYFUNCT mixLUTs2(sampleLUTf2(SECONDARYCOLOR), SECONDARYCOLOR)
			#else
				#define SECONDARYFUNCT SECONDARYCOLOR
			#endif
		#endif
	#endif
#endif

#ifdef TVCUTOFF
	/* Custom cutoff point */
	#define CUSTOMCUTOFF
	#ifndef CUSTOMCUTOFF
		/* Cutoff <17(256) bits before transformation */
		#define CUTOFFFLOAT 0.06640625
	#else
		#define CUTOFFFLOAT 0.12109375
		//#define CUTOFFFLOAT 0.87890625
	#endif
	// 8-bit range
	#define CUTOFFRANGE 4
#endif

#ifdef USELUT
	#ifdef O3DLUT
		#define PRIMARYFUNCT mixLUTs2(sampleLUTf2(PRIMARY3DCOLOR), PRIMARY3DCOLOR);
	#else
		#define PRIMARYFUNCT _1dlutfunct(PRIMARY1DCOLOR);
	#endif
#endif

#if defined _1DLUT && !defined _1DLUTPNG
	const vec3 LUT[1DLUT_REPLACE_NUMBER] = vec3[](
	1DLUT_REPLACE_LIST
	);
#endif

#ifdef _1DLUT
	vec4 _1dlutfunct(vec4 color) {
		#ifndef _1DLUTPNG
			float LUT_max = float(LUT.length()-1);
		#else
			float LUT_max = lut_size1.x - 1.0;
		#endif
		vec3 LUT_color = color.rgb * LUT_max;

		int rIndexLo = int(LUT_color.r);
		int gIndexLo = int(LUT_color.g);
		int bIndexLo = int(LUT_color.b);
		#ifndef NOINTERPOLATION
			int rIndexHi = int(ceil(LUT_color.r));
			int gIndexHi = int(ceil(LUT_color.g));
			int bIndexHi = int(ceil(LUT_color.b));
		#endif

		#ifdef _1DLUTPNG
			float texredl = texture(lut1, vec2(float(rIndexLo) / lut_size1.x, 0.5)).r;
			float texgreenl = texture(lut1, vec2(float(gIndexLo) / lut_size1.x, 0.5)).g;
			float texbluel = texture(lut1, vec2(float(bIndexLo) / lut_size1.x, 0.5)).b;
			float texredh = texture(lut1, vec2(float(rIndexHi) / lut_size1.x, 0.5)).r;
			float texgreenh = texture(lut1, vec2(float(gIndexHi) / lut_size1.x, 0.5)).g;
			float texblueh = texture(lut1, vec2(float(bIndexHi) / lut_size1.x, 0.5)).b;
		#endif

		return vec4(
		#ifndef _1DLUTPNG
			#ifndef NOINTERPOLATION
				mix(LUT[rIndexLo].r, LUT[rIndexHi].r, fract(LUT_color.r)),
				mix(LUT[gIndexLo].g, LUT[gIndexHi].g, fract(LUT_color.g)),
				mix(LUT[bIndexLo].b, LUT[bIndexHi].b, fract(LUT_color.b)),
			#else
				LUT[rIndexLo].r,
				LUT[gIndexLo].g,
				LUT[bIndexLo].b,
			#endif
		#else
			mix(texredl, texredh, fract(LUT_color.r)),
			mix(texgreenl, texgreenh, fract(LUT_color.g)),
			mix(texbluel, texblueh, fract(LUT_color.b)),
		#endif
				color.a
		);
	}
#endif

#ifdef _3DLUT
	vec4 sampleLUTf(vec4 color) {
		float u = (floor(color.b * (lut_size0.y-1.0)) / (lut_size0.y-1.0)) * ((lut_size0.x-1.0) - (lut_size0.y-1.0));
			  u += (floor(color.r * (lut_size0.y-1.0)) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  u += 0.5;
			  u /= lut_size0.x;
		float v = (floor(color.g * (lut_size0.y-1.0)) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  v += 0.5;
			  v /= lut_size0.y;

		return texture(lut0, vec2(u, v));
	}

	vec4 sampleLUTc(vec4 color) {
		float u = (ceil(color.b * (lut_size0.y-1.0)) / (lut_size0.y-1.0)) * ((lut_size0.x-1.0) - (lut_size0.y-1.0));
			  u += (ceil(color.r * (lut_size0.y-1.0)) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  u += 0.5;
			  u /= lut_size0.x;
		float v = (ceil(color.g * (lut_size0.y-1.0)) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  v += 0.5;
			  v /= lut_size0.y;

		return texture(lut0, vec2(u, v));
	}

	vec4 mixLUTs(vec4 left, vec4 right, vec4 color) {
		return vec4(
			mix(left.r, right.r, fract(color.r * (lut_size0.y-1.0))),
			mix(left.g, right.g, fract(color.g * (lut_size0.y-1.0))),
			mix(left.b, right.b, fract(color.b * (lut_size0.y-1.0))),
			color.a
		);
	}

	vec4[2] sampleLUTf2(vec4 color) {
		vec3 temp = color.rgb * (lut_size0.y-1.0);
		float u = (floor(temp.b) / (lut_size0.y-1.0)) * ((lut_size0.x-1.0) - (lut_size0.y-1.0));
			  u += (floor(temp.r) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  u += 0.5;
			  u /= lut_size0.x;
		float v = (floor(temp.g) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  v += 0.5;
			  v /= lut_size0.y;

		float u2 = (ceil(temp.b) / (lut_size0.y-1.0)) * ((lut_size0.x-1.0) - (lut_size0.y-1.0));
			  u2 += (ceil(temp.r) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  u2 += 0.5;
			  u2 /= lut_size0.x;
		float v2 = (ceil(temp.g) / (lut_size0.y-1.0)) * (lut_size0.y-1.0);
			  v2 += 0.5;
			  v2 /= lut_size0.y;

		return vec4[2](texture(lut0, vec2(u, v)), texture(lut0, vec2(u2, v2)));
	}

	vec4 mixLUTs2(vec4 lutarray[2], vec4 color) {
		return vec4(
			mix(lutarray[0].r, lutarray[1].r, fract(color.r * (lut_size0.y-1.0))),
			mix(lutarray[0].g, lutarray[1].g, fract(color.g * (lut_size0.y-1.0))),
			mix(lutarray[0].b, lutarray[1].b, fract(color.b * (lut_size0.y-1.0))),
			color.a
		);
	}
#endif


void main() {
	vec4 color = texture(tex, v_texcoord);
	#ifdef DEBUG
		#ifdef REAL
			vec4 realcolor = color;
		#endif
		float x = gl_FragCoord.x;
		float y = gl_FragCoord.y;
	#endif

	#ifdef TVCUTOFF
		if (color.r < CUTOFFFLOAT) {
			const float rangecheck = CUTOFFRANGE.01 / 256.0;
			float colg = abs(color.g - color.r);
			float colb = abs(color.b - color.r);
			if (colg < rangecheck && colb < rangecheck) {
				fragColor = vec4(0.0, 0.0, 0.0, color.a);
				return;
			}
		}
	#endif

	#ifdef TEST
		#ifdef _13DLUT
			vec4 PRIMARY1DCOLOR = mixLUTs2(sampleLUTf2(PRIMARY3DCOLOR), PRIMARY3DCOLOR);
			#ifdef COL3D
				vec4 SECONDARY1DCOLOR = mixLUTs2(sampleLUTf2(SECONDARYCOLOR), SECONDARYCOLOR);
			#endif
		#endif

		#ifdef LUTPREVIEW
			const vec4 testColor = vec4(133./255., 133./255., 133./255., 1);
			vec4 testLeft = sampleLUTf(testColor);
			vec4 testRight = sampleLUTc(testColor);

			/* LUT texture preview */
			if ( x < lut_size0.x*mult && y < lut_size0.y*mult ) {
				fragColor = texture(lut0, vec2(x,y) / lut_size0 / mult);
			/* Left LUT sampler preview */
			} else if ( x < lut_size0.y*mult && y < lut_size0.y*mult*2.0 && y > lut_size0.y*mult ) {
				fragColor = testLeft;
			/* Right LUT sampler preview */
			} else if ( x < lut_size0.y*mult*2.0 && y < lut_size0.y*mult*2.0 && y > lut_size0.y*mult ) {
				fragColor = testRight;
			/* Mixed LUT sampler preview */
			} else if ( x < lut_size0.y*mult*3.0 && y < lut_size0.y*mult*2.0 && y > lut_size0.y*mult ) {
				fragColor = mixLUTs(testLeft, testRight, testColor);
			/* Left screen (LUTs applied) */
			} else if ( x > XCUTSTART && x < XABSOLUTEVAR && y > YCUTSTART && y < YABSOLUTEVAR ) {
		#else
			/* Left screen (LUTs applied) */
			if ( x > XCUTSTART && x < XABSOLUTEVAR && y > YCUTSTART && y < YABSOLUTEVAR ) {
		#endif
			fragColor = SECONDARYFUNCT;
		/* Right screen (No/Secondary LUTs applied) */
		} else {
			fragColor = PRIMARYFUNCT;
		}
	#elif defined USELUT
		#ifdef _13DLUT
//			vec4 PRIMARY1DCOLOR = mixLUTs(sampleLUTf(PRIMARY3DCOLOR), sampleLUTc(PRIMARY3DCOLOR), PRIMARY3DCOLOR);
			vec4 PRIMARY1DCOLOR = mixLUTs2(sampleLUTf2(PRIMARY3DCOLOR), PRIMARY3DCOLOR);
		#endif
		fragColor = PRIMARYFUNCT;
	#else
		fragColor = color;
	#endif
}
