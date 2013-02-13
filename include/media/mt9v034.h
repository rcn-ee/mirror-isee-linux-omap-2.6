#ifndef _MEDIA_MT9V034_H
#define _MEDIA_MT9V034_H

struct v4l2_subdev;

struct mt9v034_platform_data {
	unsigned int clk_pol:1;

	void (*set_clock)(struct v4l2_subdev *subdev, unsigned int rate);

	const s64 *link_freqs;
	s64 link_def_freq;
};

#endif
