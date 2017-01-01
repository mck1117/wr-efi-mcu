#include "tune.h"

#include <string.h>

tune_t tune;

// This gets defined in the linker as 8K from the end of flash
extern void* _tuneloc;

void Load_Tune()
{
	// Copy tune from flash
	memcpy(&tune, _tuneloc, sizeof(tune_t));
}

void Save_Tune()
{
	// TODO: implement flash writing
}

static uint8_t find_index(float* axis, float value, uint8_t axis_size)
{
	uint8_t idx = 0;

	while((idx < (axis_size - 1)) && axis[idx] > value)
	{
		idx++;
	}

	return idx;
}

static float pct_along(float* axis, float value, uint8_t idx_low)
{
	float val_low = axis[idx_low];
	float val_hi = axis[idx_low + 1];

	if(value < val_low) return 0.0f;
	if(value > val_hi) return 1.0f;

	float width = val_hi - val_low;
	float pos = value - val_low;

	if(width < 0.001f)
	{
		return 0;
	}
	else
	{
		return pos / width;
	}
}

static float weighted_avg(float pct, float lhs, float rhs)
{
	return lhs * (1 - pct) + rhs * pct;
}

float lut_table2d16(tune_table2d16_t* table, float x, float y)
{
	uint8_t x_idx = find_index(table->x_axis, x, 16);
	uint8_t y_idx = find_index(table->y_axis, y, 16);

	// Get corresponding cells from table
	float ll = table->values[x_idx * 16 + y_idx];
	float ul = table->values[x_idx * 16 + y_idx + 1];
	float lr = table->values[(x_idx + 1) * 16 + y_idx];
	float ur = table->values[(x_idx + 1) * 16 + y_idx + 1];

	float xpos = pct_along(table->x_axis, x, x_idx);
	float ypos = pct_along(table->y_axis, y, y_idx);

	/*
	 * How does 2d weighted average work?
	 *
	 *      ul                     ur
	 *      |                       |
	 *      |                       |
	 *      |                       |
	 *      a-----*-----------------b
	 *      ll                     lr
	 *
	 * Here, xpos is ~0.2, and ypos is ~0.25
	 * lhs is the value at position a.
	 * rhs is the value at position b.
	 * We then figure out the val we want, which is xpos% between them
	 */

	// Average the sides vertically
	float lhs = weighted_avg(ypos, ll, ul);
	float rhs = weighted_avg(ypos, lr, ur);

	// Average left to right
	return weighted_avg(xpos, lhs, rhs);
}

float lut_table1d16(tune_table1d16_t* table, float x)
{
	// This works the same as the 2d case, but only on 1 axis

	uint8_t idx = find_index(table->x_axis, x, 16);

	float pos = pct_along(table->x_axis, x, idx);

	float lhs = table->values[idx];
	float rhs = table->values[idx + 1];

	return weighted_avg(pos, lhs, rhs);
}
