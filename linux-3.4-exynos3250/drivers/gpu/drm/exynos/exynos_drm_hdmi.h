/* exynos_drm_hdmi.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 * Authoer: Inki Dae <inki.dae@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _EXYNOS_DRM_HDMI_H_
#define _EXYNOS_DRM_HDMI_H_

#define MIXER_WIN_NR		3
#define MIXER_DEFAULT_WIN	0

/*
 * exynos hdmi common context structure.
 *
 * @drm_dev: pointer to drm_device.
 * @client: pointer to the context of specific device driver.
 *	this context should be hdmi_context or mixer_context.
 */
struct exynos_drm_hdmi_context {
	struct drm_device	*drm_dev;
	void			*client;
};

struct exynos_hdmi_ops {
	/* display */
	bool (*is_connected)(void *ctx);
	int (*get_edid)(void *ctx, struct drm_connector *connector,
			u8 *edid, int len);
	int (*check_timing)(void *ctx, void *timing);
	int (*power_on)(void *ctx, int mode);

	/* manager */
	int (*iommu_on)(void *ctx, bool enable);
	void (*mode_fixup)(void *ctx, struct drm_connector *connector,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode);
	void (*mode_set)(void *ctx, void *mode);
	void (*get_max_resol)(void *ctx, unsigned int *width,
				unsigned int *height);
	void (*commit)(void *ctx);
	void (*dpms)(void *ctx, int mode);
	void (*audio_control)(void *ctx, struct drm_exynos_hdmi_audio *audio);
};

struct exynos_mixer_ops {
	/* manager */
	int (*enable_vblank)(void *ctx, int pipe);
	void (*disable_vblank)(void *ctx);
	void (*wait_for_vblank)(void *ctx);
	void (*dpms)(void *ctx, int mode);

	/* overlay */
	void (*win_mode_set)(void *ctx, struct exynos_drm_overlay *overlay);
	void (*win_commit)(void *ctx, int zpos);
	void (*win_disable)(void *ctx, int zpos);
};

void exynos_hdmi_ops_register(struct exynos_hdmi_ops *ops);
void exynos_mixer_ops_register(struct exynos_mixer_ops *ops);

#ifdef CONFIG_DRM_EXYNOS_HDMI
extern int exynos_drm_hdmi_audio(struct drm_device *drm_dev, void *data,
					 struct drm_file *file);
#else
static inline int exynos_drm_hdmi_audio(struct drm_device *drm_dev,
						void *data,
						struct drm_file *file_priv)
{
	return -ENOTTY;
}
#endif
#endif
