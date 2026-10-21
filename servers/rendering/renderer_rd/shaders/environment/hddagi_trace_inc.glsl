// HDDAGI's hierarchical DDA through the voxel occupancy of its cascades, shared by
// its own gather (gi.glsl) and Lumen Lite's world-space traces (lumen_lite.glsl).
// Expects REGION_SIZE, the hddagi UBO, the voxel_cascades and
// voxel_region_cascades images and bayer_dither() to be declared first.

bool trace_ray_hdda(vec3 ray_pos, vec3 ray_dir, int p_cascade, out ivec3 r_cell, out ivec3 r_side, out int r_cascade) {
	const int LEVEL_CASCADE = -1;
	const int LEVEL_REGION = 0;
	const int LEVEL_BLOCK = 1;
	const int LEVEL_VOXEL = 2;
	const int MAX_LEVEL = 3;

	//#define HQ_RAY

#ifdef HQ_RAY
	const int fp_bits = 16;
#else
	const int fp_bits = 10;
#endif
	const int fp_block_bits = fp_bits + 2;
	const int fp_region_bits = fp_block_bits + 1;
	const int fp_cascade_bits = fp_region_bits + 4;

	bvec3 limit_dir = greaterThan(ray_dir, vec3(0.0));
	ivec3 step = mix(ivec3(0), ivec3(1), limit_dir);
	ivec3 ray_sign = ivec3(sign(ray_dir));

	ivec3 ray_dir_fp = ivec3(ray_dir * float(1 << fp_bits));

#ifdef HQ_RAY
	const float limit = 1.0 / 65535.0;
#else
	const float limit = 1.0 / 127.0;
#endif
	bvec3 ray_zero = lessThan(abs(ray_dir), vec3(limit));
	ivec3 inv_ray_dir_fp = ivec3(float(1 << fp_bits) / ray_dir);

	const ivec3 level_masks[MAX_LEVEL] = ivec3[](
			ivec3(1 << fp_region_bits) - ivec3(1),
			ivec3(1 << fp_block_bits) - ivec3(1),
			ivec3(1 << fp_bits) - ivec3(1));

	ivec3 region_offset_mask = (ivec3(hddagi.grid_size) / REGION_SIZE) - ivec3(1);

	ivec3 limits[MAX_LEVEL];

	limits[LEVEL_REGION] = ((ivec3(hddagi.grid_size) << fp_bits) - ivec3(1)) * step; // Region limit does not change, so initialize now.
	ivec3 scroll_limit;

	// Initialize to cascade
	int level = LEVEL_CASCADE;
	int cascade = p_cascade - 1;

	ivec3 cascade_base;
	ivec3 region_base;
	uvec2 block;
	bool hit = false;
	ivec3 pos;

	while (true) {
		// This loop is written so there is only one single main iteration.
		// This ensures that different compute threads working on different
		// levels can still run together without blocking each other.

		if (level == LEVEL_VOXEL) {
			// The first level should be (in a worst case scenario) the most used
			// so it needs to appear first. The rest of the levels go from more to least used order.

			ivec3 block_local = (pos & level_masks[LEVEL_BLOCK]) >> fp_bits;
			uint block_index = uint(block_local.z * 16 + block_local.y * 4 + block_local.x);
			if (block_index < 32) {
				// Low 32 bits.
				if (bool(block.x & uint(1 << block_index))) {
					hit = true;
					break;
				}
			} else {
				// High 32 bits.
				block_index -= 32;
				if (bool(block.y & uint(1 << block_index))) {
					hit = true;
					break;
				}
			}
		} else if (level == LEVEL_BLOCK) {
			ivec3 block_local = (pos & level_masks[LEVEL_REGION]) >> fp_block_bits;
			block = imageLoad(voxel_cascades, region_base + block_local).rg;
			if (block != uvec2(0)) {
				// Have voxels inside
				level = LEVEL_VOXEL;
				limits[LEVEL_VOXEL] = pos - (pos & level_masks[LEVEL_BLOCK]) + step * (level_masks[LEVEL_BLOCK] + ivec3(1));
				continue;
			}
		} else if (level == LEVEL_REGION) {
			ivec3 region = pos >> fp_region_bits;
			region = (hddagi.cascades[cascade].region_world_offset + region) & region_offset_mask; // Scroll to world
			region += cascade_base;
			bool region_used = imageLoad(voxel_region_cascades, region).r > 0;

			if (region_used) {
				// The region has contents.
				region_base = (region << 1);
				level = LEVEL_BLOCK;
				limits[LEVEL_BLOCK] = pos - (pos & level_masks[LEVEL_REGION]) + step * (level_masks[LEVEL_REGION] + ivec3(1));
				continue;
			}
		} else if (level == LEVEL_CASCADE) {
			// Return to global
			if (cascade >= p_cascade) {
				ray_pos = vec3(pos) / float(1 << fp_bits);
				ray_pos /= hddagi.cascades[cascade].to_cell;
				ray_pos += hddagi.cascades[cascade].position;
			}

			cascade++;
			if (cascade == hddagi.max_cascades) {
				break;
			}

			ray_pos -= hddagi.cascades[cascade].position;
			ray_pos *= hddagi.cascades[cascade].to_cell;
			pos = ivec3(ray_pos * float(1 << fp_bits));
			if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(hddagi.grid_size) << fp_bits))) {
				// Outside this cascade, go to next.
				continue;
			}

			cascade_base = ivec3(0, int(hddagi.grid_size.y / REGION_SIZE) * cascade, 0);
			level = LEVEL_REGION;

			// Put a limit so the jump to the next level is not so strong

			{
				vec3 box = (vec3(hddagi.grid_size * step) - ray_pos) / ray_dir;
				vec3 axis = vec3(1, 0, 0);
				float m = box.x;
				if (box.y < m) {
					m = box.y;
					axis = vec3(0, 1, 0);
				}

				if (box.z < m) {
					axis = vec3(0, 0, 1);
				}

				vec3 half_size = vec3(hddagi.grid_size) / 2.0;
				vec3 inner_pos = -hddagi.cascades[cascade].position * hddagi.cascades[cascade].to_cell - half_size;

				float inner_dir = dot(axis, inner_pos);
				float blend = abs(inner_dir) / float(REGION_SIZE * 0.5);

				scroll_limit = limits[LEVEL_REGION];
				if (bayer_dither(blend)) {
					scroll_limit += (ivec3(axis * sign(inner_dir)) * REGION_SIZE) << fp_bits;
				}
			}

			continue;
		}

		// Fixed point, multi-level DDA.

		ivec3 mask = level_masks[level];
		ivec3 box = mask * step;
		ivec3 pos_diff = box - (pos & mask);
#ifdef HQ_RAY
		ivec3 mul_res = mul64(pos_diff, inv_ray_dir_fp, fp_bits);
#else
		ivec3 mul_res = (pos_diff * inv_ray_dir_fp) >> fp_bits;
#endif
		ivec3 tv = mix(mul_res, ivec3(0x7FFFFFFF), ray_zero);
		int t = min(tv.x, min(tv.y, tv.z));

		// The general idea here is that we _always_ need to increment to the closest next cell
		// (this is a DDA after all), so adv_box forces this increment for the minimum axis.

		ivec3 adv_box = pos_diff + ray_sign;
#ifdef HQ_RAY
		ivec3 adv_t = mul64(ray_dir_fp, ivec3(t), fp_bits);
#else
		ivec3 adv_t = (ray_dir_fp * t) >> fp_bits;
#endif
		pos += mix(adv_t, adv_box, equal(ivec3(t), tv));

		{ // Test against scroll limit.
			bvec3 limit = lessThan(pos, scroll_limit);
			bvec3 eq = equal(limit, limit_dir);
			if (!all(eq)) {
				// Hit scroll limit, clamp limit and go to next cascade.
				level = LEVEL_CASCADE;
				pos = mix(scroll_limit, pos, eq);
				continue;
			}
		}

		while (true) {
			bvec3 limit = lessThan(pos, limits[level]);
			bool inside = all(equal(limit, limit_dir));
			if (inside) {
				break;
			}
			level -= 1;
			if (level == LEVEL_CASCADE) {
				break;
			}
		}
	}

	if (hit) {
		ivec3 mask = level_masks[LEVEL_VOXEL];
		ivec3 box = mask * (step ^ ivec3(1));
		ivec3 pos_diff = box - (pos & mask);
#ifdef HQ_RAY
		ivec3 mul_res = mul64(pos_diff, -inv_ray_dir_fp, fp_bits);
#else
		ivec3 mul_res = (pos_diff * -inv_ray_dir_fp);
#endif

		ivec3 tv = mix(mul_res, ivec3(0x7FFFFFFF), ray_zero);

		int m;
		if (tv.x < tv.y) {
			r_side = ivec3(1, 0, 0);
			m = tv.x;
		} else {
			r_side = ivec3(0, 1, 0);
			m = tv.y;
		}
		if (tv.z < m) {
			r_side = ivec3(0, 0, 1);
		}

		r_side *= -ray_sign;

		r_cell = pos >> fp_bits;

		r_cascade = cascade;
	}

	return hit;
}
