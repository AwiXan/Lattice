// COPIED FROM servers\rendering\renderer_rd\shaders\area_lights_inc.glsl
// Partially modified
// Functions related to area lights
#define M_PI 3.14159265359
#define M_TAU 6.28318530718

float acos_approx(float p_x) {
	float x = abs(p_x);
	float res = -0.156583f * x + (M_PI / 2.0);
	res *= sqrt(1.0f - x);
	return (p_x >= 0.0) ? res : M_PI - res;
}

vec3 integrate_edge_hill(vec3 p0, vec3 p1) {
	// Approximation suggested by Hill and Heitz, calculating the integral of the spherical cosine distribution over the line between p0 and p1.
	// Runs faster than the exact formula of Baum et al. (1989).
	float cosTheta = dot(p0, p1);

	float x = cosTheta;
	float y = abs(x);
	float a = 5.42031 + (3.12829 + 0.0902326 * y) * y;
	float b = 3.45068 + (4.18814 + y) * y;
	float theta_sintheta = a / b;

	if (x < 0.0) {
		theta_sintheta = M_PI * inversesqrt(1.0 - x * x) - theta_sintheta; // original paper: 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - theta_sintheta
	}
	return theta_sintheta * cross(p0, p1);
}

float integrate_edge(vec3 p_proj0, vec3 p_proj1, vec3 p0, vec3 p1) {
	float epsilon = 0.00001;
	bool opposite_sides = dot(p_proj0, p_proj1) < -1.0 + epsilon;
	if (opposite_sides) {
		// calculate the point on the line p0 to p1 that is closest to the vertex (origin)
		vec3 half_point_t = p0 + normalize(p1 - p0) * dot(p0, normalize(p0 - p1));
		vec3 half_point = normalize(half_point_t);
		return integrate_edge_hill(p_proj0, half_point).y + integrate_edge_hill(half_point, p_proj1).y;
	}
	return integrate_edge_hill(p_proj0, p_proj1).y;
}

// The point of an area light closest to a point of its plane, in the space of the light.
// For ellipses, the point of the edge towards it, which is close enough for attenuation.
vec2 area_light_closest_point(vec2 p_point, vec2 p_half_size, bool p_ellipse) {
	if (p_ellipse) {
		vec2 q = p_point / max(p_half_size, vec2(1e-6));
		float l = length(q);
		return l > 1.0 ? p_point / l : p_point;
	}
	return clamp(p_point, -p_half_size, p_half_size);
}

// Ellipses, after Heitz and Hill (2017), "Real-Time Line- and Disk-Light Shading with Linearly Transformed Cosines".

// The real roots of a cubic c.x + c.y * x + c.z * x^2 + c.w * x^3 with three of them, smallest to largest
// in the order the ellipse integral wants them (Blinn 2007, "How to Solve a Cubic Equation, Part 5").
vec3 ltc_solve_cubic(vec4 p_coef) {
	p_coef.xyz /= p_coef.w;
	p_coef.yz /= 3.0;

	float A = p_coef.w;
	float B = p_coef.z;
	float C = p_coef.y;
	float D = p_coef.x;

	vec3 delta = vec3(-p_coef.z * p_coef.z + p_coef.y, -p_coef.y * p_coef.z + p_coef.x, dot(vec2(p_coef.z, -p_coef.y), p_coef.xy));
	float discriminant = dot(vec2(4.0 * delta.x, -delta.y), delta.zy);
	float sqrt_discriminant = sqrt(max(discriminant, 0.0));

	vec2 xlc;
	{
		// Algorithm A.
		float C_a = delta.x;
		float D_a = -2.0 * B * delta.x + delta.y;
		float theta = atan(sqrt_discriminant, -D_a) / 3.0;
		float x_1a = 2.0 * sqrt(max(-C_a, 0.0)) * cos(theta);
		float x_3a = 2.0 * sqrt(max(-C_a, 0.0)) * cos(theta + (2.0 / 3.0) * M_PI);
		float xl = ((x_1a + x_3a) > 2.0 * B) ? x_1a : x_3a;
		xlc = vec2(xl - B, A);
	}

	vec2 xsc;
	{
		// Algorithm D.
		float C_d = delta.z;
		float D_d = -D * delta.y + 2.0 * C * delta.z;
		float theta = atan(D * sqrt_discriminant, -D_d) / 3.0;
		float x_1d = 2.0 * sqrt(max(-C_d, 0.0)) * cos(theta);
		float x_3d = 2.0 * sqrt(max(-C_d, 0.0)) * cos(theta + (2.0 / 3.0) * M_PI);
		float xs = (x_1d + x_3d < 2.0 * C) ? x_1d : x_3d;
		xsc = vec2(-D, xs + C);
	}

	float E = xlc.y * xsc.y;
	float F = -xlc.x * xsc.y - xlc.y * xsc.x;
	float G = xlc.x * xsc.x;
	vec2 xmc = vec2(C * F - B * G, -B * F + C * E);

	vec3 root = vec3(xsc.x / xsc.y, xmc.x / xmc.y, xlc.x / xlc.y);
	if (root.x < root.y && root.x < root.z) {
		root.xyz = root.yxz;
	} else if (root.z < root.x && root.z < root.y) {
		root.xyz = root.xzy;
	}
	return root;
}

// The form factor of a sphere seen under sin^2 = p_sin_sigma_sq, clipped by the horizon, divided by PI.
// Exact (Snyder 1996, "Area Light Sources for Real-Time Graphics"), as in Frostbite's area lights.
float ltc_clipped_sphere_form_factor(float p_cos_theta, float p_sin_sigma_sq) {
	if (p_cos_theta * p_cos_theta > p_sin_sigma_sq) {
		return max(p_sin_sigma_sq * p_cos_theta, 0.0);
	}
	float sin_theta = sqrt(max(1.0 - p_cos_theta * p_cos_theta, 1e-8));
	float x = sqrt(max(1.0 / p_sin_sigma_sq - 1.0, 0.0));
	float y = clamp(-x * (p_cos_theta / sin_theta), -1.0, 1.0);
	float sin_theta_sqrt_y = sin_theta * sqrt(1.0 - y * y);
	return max(((p_cos_theta * acos(y) - x * sin_theta_sqrt_y) * p_sin_sigma_sq + atan(sin_theta_sqrt_y, x)) / M_PI, 0.0);
}

// The integral of the clamped cosine around +Y over an ellipse, given by its center and two conjugate semi-axes,
// over PI (so 1.0 for the whole hemisphere). Exact while the ellipse is all above or all below the horizon; across it,
// the ellipse is turned into the sphere with the same vector form factor, whose horizon-clipped integral is known.
// r_dir is the average direction of the light that the ellipse sends, to look up textures with.
float ltc_integrate_ellipse(vec3 p_center, vec3 p_axis_a, vec3 p_axis_b, out vec3 r_dir) {
	// How far the ellipse reaches above and below the horizon.
	float reach = length(vec2(p_axis_a.y, p_axis_b.y));
	if (p_center.y + reach <= 0.0) {
		// All of it is below the horizon.
		r_dir = vec3(0.0, 1.0, 0.0);
		return 0.0;
	}

	vec3 V1 = p_axis_a;
	vec3 V2 = p_axis_b;

	// Turn the conjugate axes into the principal ones.
	float d11 = dot(V1, V1);
	float d22 = dot(V2, V2);
	float d12 = dot(V1, V2);
	float a;
	float b;
	if (abs(d12) / sqrt(d11 * d22) > 0.0001) {
		float tr = d11 + d22;
		float det = sqrt(max(d11 * d22 - d12 * d12, 0.0));
		float u = 0.5 * sqrt(max(tr - 2.0 * det, 0.0));
		float v = 0.5 * sqrt(tr + 2.0 * det);
		float e_max = (u + v) * (u + v);
		float e_min = (u - v) * (u - v);

		vec3 V1_;
		vec3 V2_;
		if (d11 > d22) {
			V1_ = d12 * V1 + (e_max - d11) * V2;
			V2_ = d12 * V1 + (e_min - d11) * V2;
		} else {
			V1_ = d12 * V2 + (e_max - d22) * V1;
			V2_ = d12 * V2 + (e_min - d22) * V1;
		}

		a = 1.0 / e_max;
		b = 1.0 / e_min;
		V1 = normalize(V1_);
		V2 = normalize(V2_);
	} else {
		a = 1.0 / d11;
		b = 1.0 / d22;
		V1 *= sqrt(a);
		V2 *= sqrt(b);
	}

	vec3 V3 = cross(V1, V2);
	if (dot(p_center, V3) < 0.0) {
		V3 = -V3;
	}

	float L = dot(V3, p_center);
	if (L < 1e-7) {
		// The shaded point is in the plane of the ellipse.
		r_dir = vec3(0.0, 1.0, 0.0);
		return 0.0;
	}
	float x0 = dot(V1, p_center) / L;
	float y0 = dot(V2, p_center) / L;

	a *= L * L;
	b *= L * L;

	float c0 = a * b;
	float c1 = a * b * (1.0 + x0 * x0 + y0 * y0) - a - b;
	float c2 = 1.0 - a * (1.0 + x0 * x0) - b * (1.0 + y0 * y0);
	vec3 roots = ltc_solve_cubic(vec4(c0, c1, c2, 1.0));
	float e1 = roots.x;
	float e2 = roots.y;
	float e3 = roots.z;

	r_dir = normalize(mat3(V1, V2, V3) * vec3(a * x0 / (a - e2), b * y0 / (b - e2), 1.0));

	float L1 = sqrt(-e2 / e3);
	float L2 = sqrt(-e2 / e1);
	float form_factor = L1 * L2 * inversesqrt((1.0 + L1 * L1) * (1.0 + L2 * L2));

	if (p_center.y - reach >= 0.0) {
		// All of it is above the horizon: no clipping, the integral is the projection of the vector form factor.
		return form_factor * r_dir.y;
	}
	return ltc_clipped_sphere_form_factor(r_dir.y, form_factor);
}

// Form factor function for area light, taken from Urena, Fajardo, et.al. (2013): An Area-Preserving Parametrization for Spherical Rectangles
float quad_solid_angle(vec3 L[4]) {
	// The solid angle of a spherical rectangle is the difference of the sum of its angles
	// and the sum of the angles of a plane rectangle (2*PI)
	vec3 c1 = cross(L[0], L[1]);
	vec3 c2 = cross(L[1], L[2]);
	vec3 c3 = cross(L[2], L[3]);
	vec3 c4 = cross(L[3], L[0]);
	vec3 n0 = normalize(c1);
	vec3 n1 = normalize(c2);
	vec3 n2 = normalize(c3);
	vec3 n3 = normalize(c4);
	float g0 = acos(clamp(dot(-n0, n1), -1.0, 1.0));
	float g1 = acos(clamp(dot(-n1, n2), -1.0, 1.0));
	float g2 = acos(clamp(dot(-n2, n3), -1.0, 1.0));
	float g3 = acos(clamp(dot(-n3, n0), -1.0, 1.0));

	float angle_sum = g0 + g1 + g2 + g3;

	return clamp(angle_sum - M_TAU, 0.0, M_TAU);
}

void clip_quad_to_horizon(inout vec3 L[5], out int vertex_count) {
	// detect clipping config
	int config = 0;
	if (L[0].y > 0.0) {
		config += 1;
	}
	if (L[1].y > 0.0) {
		config += 2;
	}
	if (L[2].y > 0.0) {
		config += 4;
	}
	if (L[3].y > 0.0) {
		config += 8;
	}

	// clip
	vertex_count = 0;

	if (config == 0) {
		// clip all
	} else if (config == 1) { // V1 clip V2 V3 V4
		vertex_count = 3;
		L[1] = -L[1].y * L[0] + L[0].y * L[1];
		L[2] = -L[3].y * L[0] + L[0].y * L[3];
	} else if (config == 2) { // V2 clip V1 V3 V4
		vertex_count = 3;
		L[0] = -L[0].y * L[1] + L[1].y * L[0];
		L[2] = -L[2].y * L[1] + L[1].y * L[2];
	} else if (config == 3) { // V1 V2 clip V3 V4
		vertex_count = 4;
		L[2] = -L[2].y * L[1] + L[1].y * L[2];
		L[3] = -L[3].y * L[0] + L[0].y * L[3];
	} else if (config == 4) { // V3 clip V1 V2 V4
		vertex_count = 3;
		L[0] = -L[3].y * L[2] + L[2].y * L[3];
		L[1] = -L[1].y * L[2] + L[2].y * L[1];
	} else if (config == 5) { // V1 V3 clip V2 V4) impossible
		vertex_count = 0;
	} else if (config == 6) { // V2 V3 clip V1 V4
		vertex_count = 4;
		L[0] = -L[0].y * L[1] + L[1].y * L[0];
		L[3] = -L[3].y * L[2] + L[2].y * L[3];
	} else if (config == 7) { // V1 V2 V3 clip V4
		vertex_count = 5;
		L[4] = -L[3].y * L[0] + L[0].y * L[3];
		L[3] = -L[3].y * L[2] + L[2].y * L[3];
	} else if (config == 8) { // V4 clip V1 V2 V3
		vertex_count = 3;
		L[0] = -L[0].y * L[3] + L[3].y * L[0];
		L[1] = -L[2].y * L[3] + L[3].y * L[2];
		L[2] = L[3];
	} else if (config == 9) { // V1 V4 clip V2 V3
		vertex_count = 4;
		L[1] = -L[1].y * L[0] + L[0].y * L[1];
		L[2] = -L[2].y * L[3] + L[3].y * L[2];
	} else if (config == 10) { // V2 V4 clip V1 V3) impossible
		vertex_count = 0;
	} else if (config == 11) { // V1 V2 V4 clip V3
		vertex_count = 5;
		L[4] = L[3];
		L[3] = -L[2].y * L[3] + L[3].y * L[2];
		L[2] = -L[2].y * L[1] + L[1].y * L[2];
	} else if (config == 12) { // V3 V4 clip V1 V2
		vertex_count = 4;
		L[1] = -L[1].y * L[2] + L[2].y * L[1];
		L[0] = -L[0].y * L[3] + L[3].y * L[0];
	} else if (config == 13) { // V1 V3 V4 clip V2
		vertex_count = 5;
		L[4] = L[3];
		L[3] = L[2];
		L[2] = -L[1].y * L[2] + L[2].y * L[1];
		L[1] = -L[1].y * L[0] + L[0].y * L[1];
	} else if (config == 14) { // V2 V3 V4 clip V1
		vertex_count = 5;
		L[4] = -L[0].y * L[3] + L[3].y * L[0];
		L[0] = -L[0].y * L[1] + L[1].y * L[0];
	} else if (config == 15) { // V1 V2 V3 V4
		vertex_count = 4;
	}

	if (vertex_count == 3) {
		L[3] = L[0];
	}
	if (vertex_count == 4) {
		L[4] = L[0];
	}
}

float ltc_integrate_clipped_quad(vec3 L[5], vec3 L_proj[5], int vertices_above_horizon) {
	float I;
	I = integrate_edge(L_proj[0], L_proj[1], L[0], L[1]);
	I += integrate_edge(L_proj[1], L_proj[2], L[1], L[2]);
	I += integrate_edge(L_proj[2], L_proj[3], L[2], L[3]);
	if (vertices_above_horizon >= 4) {
		I += integrate_edge(L_proj[3], L_proj[4], L[3], L[4]);
	}
	if (vertices_above_horizon == 5) {
		I += integrate_edge(L_proj[4], L_proj[0], L[4], L[0]);
	}
	return abs(I);
}

// points are the corners of the light, or of the rectangle an ellipse fills when ellipse is true.
void ltc_evaluate(vec3 normal, vec3 eye_vec, mat3 M_inv, vec3 points[4], bool ellipse, out float integral) {
	// construct the orthonormal basis around the normal vector
	vec3 x, z;
	z = -normalize(eye_vec - normal * dot(eye_vec, normal)); // expanding the angle between view and normal vector to 90 degrees, this gives a normal vector
	x = cross(normal, z);

	// rotate area light in (T1, normal, T2) basis
	M_inv = M_inv * transpose(mat3(x, normal, z));

	vec3 L[5];
	L[0] = M_inv * points[0];
	L[1] = M_inv * points[1];
	L[2] = M_inv * points[2];
	L[3] = M_inv * points[3];

	if (ellipse) {
		vec3 dir;
		integral = ltc_integrate_ellipse(0.5 * (L[0] + L[2]), 0.5 * (L[1] - L[0]), 0.5 * (L[3] - L[0]), dir);
		return;
	}

	int n;
	clip_quad_to_horizon(L, n);
	if (n == 0) {
		integral = 0.0;
		return;
	}

	// project onto unit sphere
	vec3 L_proj[5];
	L_proj[0] = normalize(L[0]);
	L_proj[1] = normalize(L[1]);
	L_proj[2] = normalize(L[2]);
	L_proj[3] = normalize(L[3]);
	L_proj[4] = normalize(L[4]);

	// Prevent abnormal values when the light goes through (or close to) the fragment
	vec3 pnorm = normalize(cross(L_proj[0] - L_proj[1], L_proj[2] - L_proj[1]));
	if (abs(dot(pnorm, L_proj[0])) < 1e-10) {
		// we could just return black, but that would lead to some black pixels in front of the light.
		// Better, we check if the fragment is on the light, and return white if so.
		vec3 r10 = points[0] - points[1];
		vec3 r12 = points[2] - points[1];
		float alpha = -dot(points[1], r10) / dot(r10, r10);
		float beta = -dot(points[1], r12) / dot(r12, r12);
		if (0.0 < alpha && alpha < 1.0 && 0.0 < beta && beta < 1.0) { // fragment is on light {
			integral = 1.0;
			return;
		} else {
			integral = 0.0;
			return;
		}
	}

	float I = ltc_integrate_clipped_quad(L, L_proj, n);
	integral = I / (2.0 * M_PI);
}

void ltc_evaluate_specular(vec3 normal, vec3 eye_vec, float roughness, vec3 points[4], bool ellipse, sampler2D ltc_lut1, sampler2D ltc_lut2, out float ltc_specular, out vec2 fresnel) {
	float theta = acos_approx(dot(normal, eye_vec));
	const float LTC_LUT_SIZE = float(64.0);
	vec2 lut_pos = vec2(max(roughness, float(0.02)), theta / float(0.5 * M_PI));
	vec2 lut_uv = vec2(lut_pos * (float(63.0) / LTC_LUT_SIZE) + vec2(float(0.5) / LTC_LUT_SIZE)); // offset by 1 pixel
	vec4 M_brdf_abcd = texture(ltc_lut1, lut_uv);
	vec3 M_brdf_e_mag_fres = texture(ltc_lut2, lut_uv).xyz;
	float scale = 1.0 / (M_brdf_abcd.x * M_brdf_e_mag_fres.x - M_brdf_abcd.y * M_brdf_abcd.w);

	mat3 M_inv = mat3(
			vec3(0, 0, 1.0 / M_brdf_abcd.z),
			vec3(-M_brdf_abcd.w * scale, M_brdf_abcd.x * scale, 0),
			vec3(-M_brdf_e_mag_fres.x * scale, M_brdf_abcd.y * scale, 0));

	ltc_evaluate(normal, eye_vec, M_inv, points, ellipse, ltc_specular);
	fresnel = vec2(M_brdf_e_mag_fres.yz);
}
