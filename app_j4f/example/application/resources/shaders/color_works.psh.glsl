float textureProj(vec4 shCoord, vec2 offset, uint cascade, float minLight) {
	float d = step(-0.0, shCoord.z) * step(shCoord.z, 1.0);
	if (d > 0.0) {
		float dist = texture(u_shadow_map, vec3(shCoord.xy + offset, cascade)).r;
		float bias = 0.001;
		float depth = shCoord.z - bias;
		return mix(1.0, minLight, step(dist, depth));
	}
	return 1.0;
}

float filterPCF(vec4 sc, uint cascade, float minLight) {
	ivec2 texDim = textureSize(u_shadow_map, 0).xy;
	float scale = 0.7;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascade, minLight);
			count++;
		}
	}
	return shadowFactor / count;
}

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 k = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(vec3(c.r) + k.xyz) * 6.0 - k.www);
    return c.b * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), c.g);
}

vec3 saturate(vec3 c, float s) {
	c.rgb = rgb2hsv(c.rgb);
	c.g *= s;
	return hsv2rgb(c.rgb);
}

vec3 calculateNormal(mat3 tbn, vec3 tangentNormal) {
	return normalize(tbn * tangentNormal);
}

vec3 gammaCorrection(vec3 color, float gamma) {
	return pow(color, vec3(1.0 / gamma));
}