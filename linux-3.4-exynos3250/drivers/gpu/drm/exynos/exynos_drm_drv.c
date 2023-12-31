/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *	Joonyoung Shim <jy0922.shim@samsung.com>
 *	Seung-Woo Kim <sw0312.kim@samsung.com>
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

#include "drmP.h"
#include "drm.h"
#include "drm_crtc_helper.h"

#include <drm/exynos_drm.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_crtc.h"
#include "exynos_drm_encoder.h"
#include "exynos_drm_connector.h"
#include "exynos_drm_fbdev.h"
#include "exynos_drm_fb.h"
#include "exynos_drm_gem.h"
#ifdef CONFIG_DRM_EXYNOS_G2D
#include "exynos_drm_g2d.h"
#endif
#include "exynos_drm_ipp.h"
#include "exynos_drm_hdmi.h"
#include "exynos_drm_plane.h"
#include "exynos_drm_vidi.h"
#include "exynos_drm_dmabuf.h"
#include "exynos_drm_iommu.h"
#include "exynos_drm_debugfs.h"

#define DRIVER_NAME	"exynos"
#define DRIVER_DESC	"Samsung SoC DRM"
#define DRIVER_DATE	"20110530"
#define DRIVER_MAJOR	1
#define DRIVER_MINOR	0

#define VBLANK_OFF_DELAY	132

static const struct input_device_id exynos_drm_input_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_ABS) },
	},
	{
		.flags = INPUT_DEVICE_ID_MATCH_RELBIT,
		.evbit = { BIT_MASK(EV_REL) },
		.keybit = { [BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) },
		.relbit = { BIT_MASK(REL_X) | BIT_MASK(REL_Y) },
	},
	{ },
};

static int exynos_drm_load(struct drm_device *dev, unsigned long flags)
{
	struct exynos_drm_private *private;
	struct input_handler *input_hdl;
	int ret;
	int nr;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	private = kzalloc(sizeof(struct exynos_drm_private), GFP_KERNEL);
	if (!private) {
		DRM_ERROR("failed to allocate private\n");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&private->pageflip_event_list);
	dev->dev_private = (void *)private;

	/*
	 * create mapping to manage iommu table and set a pointer to iommu
	 * mapping structure to iommu_mapping of private data.
	 * also this iommu_mapping can be used to check if iommu is supported
	 * or not.
	 */
	ret = drm_create_iommu_mapping(dev);
	if (ret < 0) {
		DRM_ERROR("failed to create iommu mapping.\n");
		goto err_crtc;
	}

	drm_mode_config_init(dev);

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(dev);

	exynos_drm_mode_config_init(dev);

	/*
	 * EXYNOS4 is enough to have two CRTCs and each crtc would be used
	 * without dependency of hardware.
	 */
	for (nr = 0; nr < MAX_CRTC; nr++) {
		ret = exynos_drm_crtc_create(dev, nr);
		if (ret)
			goto err_release_iommu_mapping;
	}

	for (nr = 0; nr < MAX_PLANE; nr++) {
		struct drm_plane *plane;
		unsigned int possible_crtcs = (1 << MAX_CRTC) - 1;

		plane = exynos_plane_init(dev, possible_crtcs, false);
		if (!plane)
			goto err_release_iommu_mapping;
	}

	ret = drm_vblank_init(dev, MAX_CRTC);
	if (ret)
		goto err_release_iommu_mapping;

	/*
	 * probe sub drivers such as display controller and hdmi driver,
	 * that were registered at probe() of platform driver
	 * to the sub driver and create encoder and connector for them.
	 */
	ret = exynos_drm_device_register(dev);
	if (ret)
		goto err_vblank;

	/* setup possible_clones. */
	exynos_drm_encoder_setup(dev);

	/*
	 * create and configure fb helper and also exynos specific
	 * fbdev object.
	 */
	ret = exynos_drm_fbdev_init(dev);
	if (ret) {
		DRM_ERROR("failed to initialize drm fbdev.\n");
		goto err_drm_device;
	}

	input_hdl = kzalloc(sizeof(*input_hdl), GFP_KERNEL);
	if (!input_hdl) {
		DRM_ERROR("failed to alloc input_hdl.\n");
		goto err_fbdev;
	}

	input_hdl->event = exynos_drm_crtc_input_event;
	input_hdl->match = exynos_drm_crtc_input_match;
	input_hdl->connect = exynos_drm_crtc_input_connect;
	input_hdl->disconnect = exynos_drm_crtc_input_disconnect;
	input_hdl->name = "exynos_drm";
	input_hdl->id_table = exynos_drm_input_ids;
	input_hdl->private = dev;

	if (input_register_handler(input_hdl)) {
		DRM_ERROR("unable to register the input handler\n");
		goto err_input;
	}

	private->input_hdl = input_hdl;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI_NOTIFY
	if (trustedui_nb_register(&private->nb_ctrl))
		pr_err("could not register trustedui notify callback\n");
	else
		private->nb_ctrl.notifier_call = exynos_drm_secure_notify;
#endif
	drm_vblank_offdelay = VBLANK_OFF_DELAY;

	return 0;

err_input:
	kfree(input_hdl);
err_fbdev:
	exynos_drm_fbdev_fini(dev);
err_drm_device:
	exynos_drm_device_unregister(dev);
err_vblank:
	drm_vblank_cleanup(dev);
err_release_iommu_mapping:
	drm_release_iommu_mapping(dev);
err_crtc:
	drm_mode_config_cleanup(dev);
	kfree(private);

	return ret;
}

static int exynos_drm_unload(struct drm_device *dev)
{
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI_NOTIFY
	struct exynos_drm_private *private = dev->dev_private;

	trustedui_nb_unregister(&private->nb_ctrl);
#endif

	exynos_drm_fbdev_fini(dev);
	exynos_drm_device_unregister(dev);
	drm_vblank_cleanup(dev);
	drm_kms_helper_poll_fini(dev);
	drm_mode_config_cleanup(dev);

	drm_release_iommu_mapping(dev);
	kfree(dev->dev_private);

	dev->dev_private = NULL;

	return 0;
}

static int exynos_drm_open(struct drm_device *dev, struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv;
	int ret;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	file_priv = kzalloc(sizeof(*file_priv), GFP_KERNEL);
	if (!file_priv)
		return -ENOMEM;

	file_priv->tgid = task_tgid_nr(current);

	drm_prime_init_file_private(&file->prime);
	file->driver_priv = file_priv;

	ret = exynos_drm_subdrv_open(dev, file);
	if (ret) {
		kfree(file_priv);
		file->driver_priv = NULL;
	}

	return ret;
}

static void exynos_drm_preclose(struct drm_device *dev,
					struct drm_file *file)
{
	struct exynos_drm_private *private = dev->dev_private;
	struct drm_pending_vblank_event *e, *t;
	unsigned long flags;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	/* release events of current file */
	spin_lock_irqsave(&dev->event_lock, flags);
	list_for_each_entry_safe(e, t, &private->pageflip_event_list,
			base.link) {
		if (e->base.file_priv == file) {
			list_del(&e->base.link);
			e->base.destroy(&e->base);
		}
	}
	drm_prime_destroy_file_private(&file->prime);
	spin_unlock_irqrestore(&dev->event_lock, flags);

	exynos_drm_subdrv_close(dev, file);
}

static void exynos_drm_postclose(struct drm_device *dev, struct drm_file *file)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	if (!file->driver_priv)
		return;

	kfree(file->driver_priv);
	file->driver_priv = NULL;
}

static void exynos_drm_lastclose(struct drm_device *dev)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	exynos_drm_fbdev_restore_mode(dev);
}

static struct vm_operations_struct exynos_drm_gem_vm_ops = {
	.fault = exynos_drm_gem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static struct drm_ioctl_desc exynos_ioctls[] = {
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_CREATE, exynos_drm_gem_create_ioctl,
			DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_MAP_OFFSET,
			exynos_drm_gem_map_offset_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_MMAP,
			exynos_drm_gem_mmap_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_GET,
			exynos_drm_gem_get_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_GEM_CACHE_OP,
			exynos_drm_gem_cache_op_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(EXYNOS_VIDI_CONNECTION,
			vidi_connection_ioctl, DRM_UNLOCKED | DRM_AUTH),

#ifdef CONFIG_DRM_EXYNOS_G2D
	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_GET_VER,
			exynos_g2d_get_ver_ioctl, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_SET_CMDLIST,
			exynos_g2d_set_cmdlist_ioctl, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_EXEC,
			exynos_g2d_exec_ioctl, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_G2D_LOCK_MODE,
			exynos_g2d_lock_mode_ioctl, DRM_UNLOCKED | DRM_AUTH),
#endif

	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_GET_PROPERTY,
			exynos_drm_ipp_get_property, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_SET_PROPERTY,
			exynos_drm_ipp_set_property, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_QUEUE_BUF,
			exynos_drm_ipp_queue_buf, DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(EXYNOS_IPP_CMD_CTRL,
			exynos_drm_ipp_cmd_ctrl, DRM_UNLOCKED | DRM_AUTH),

	DRM_IOCTL_DEF_DRV(EXYNOS_HDMI_AUDIO,
			exynos_drm_hdmi_audio, DRM_UNLOCKED | DRM_AUTH),

	DRM_IOCTL_DEF_DRV(EXYNOS_DPMS,
			exynos_drm_control_dpms, DRM_UNLOCKED | DRM_AUTH),
};

static const struct file_operations exynos_drm_driver_fops = {
	.owner		= THIS_MODULE,
	.open		= drm_open,
	.mmap		= exynos_drm_gem_mmap,
	.poll		= drm_poll,
	.read		= drm_read,
	.unlocked_ioctl	= drm_ioctl,
	.release	= drm_release,
};

static struct drm_driver exynos_drm_driver = {
	.driver_features	= DRIVER_HAVE_IRQ | DRIVER_MODESET |
					DRIVER_GEM | DRIVER_PRIME | DRIVER_USE_LPMODE,
	.load			= exynos_drm_load,
	.unload			= exynos_drm_unload,
	.open			= exynos_drm_open,
	.preclose		= exynos_drm_preclose,
	.lastclose		= exynos_drm_lastclose,
	.postclose		= exynos_drm_postclose,
	.get_vblank_counter	= drm_vblank_count,
	.prepare_vblank		= exynos_drm_crtc_prepare_vblank,
	.enable_vblank		= exynos_drm_crtc_enable_vblank,
	.disable_vblank		= exynos_drm_crtc_disable_vblank,
#if defined(CONFIG_DEBUG_FS)
	.debugfs_init		= exynos_drm_debugfs_init,
	.debugfs_cleanup	= exynos_drm_debugfs_cleanup,
#endif
	.set_lpmode		= exynos_drm_crtc_lp_mode_enable,
	.gem_init_object	= exynos_drm_gem_init_object,
	.gem_free_object	= exynos_drm_gem_free_object,
	.gem_vm_ops		= &exynos_drm_gem_vm_ops,
	.gem_close_object	= &exynos_drm_gem_close_object,
	.dumb_create		= exynos_drm_gem_dumb_create,
	.dumb_map_offset	= exynos_drm_gem_dumb_map_offset,
	.dumb_destroy		= exynos_drm_gem_dumb_destroy,
	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle,
	.gem_prime_export	= exynos_dmabuf_prime_export,
	.gem_prime_import	= exynos_dmabuf_prime_import,
	.ioctls			= exynos_ioctls,
	.fops			= &exynos_drm_driver_fops,
	.name	= DRIVER_NAME,
	.desc	= DRIVER_DESC,
	.date	= DRIVER_DATE,
	.major	= DRIVER_MAJOR,
	.minor	= DRIVER_MINOR,
};

static int exynos_drm_platform_probe(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	exynos_drm_driver.num_ioctls = DRM_ARRAY_SIZE(exynos_ioctls);

	return drm_platform_init(&exynos_drm_driver, pdev);
}

static int exynos_drm_platform_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	drm_platform_exit(&exynos_drm_driver, pdev);

	return 0;
}

static struct platform_driver exynos_drm_platform_driver = {
	.probe		= exynos_drm_platform_probe,
	.remove		= __devexit_p(exynos_drm_platform_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "exynos-drm",
	},
};

static int __init exynos_drm_init(void)
{
	int ret;

	DRM_DEBUG_DRIVER("%s\n", __FILE__);

#ifdef CONFIG_DRM_EXYNOS_FIMD
	ret = platform_driver_register(&fimd_driver);
	if (ret < 0)
		goto out_fimd;
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
#ifdef CONFIG_DRM_EXYNOS_HDCP
	ret = platform_driver_register(&hdcp_driver);
	if (ret < 0)
		goto out_hdcp;
#endif
	ret = platform_driver_register(&hdmi_driver);
	if (ret < 0)
		goto out_hdmi;
	ret = platform_driver_register(&mixer_driver);
	if (ret < 0)
		goto out_mixer;
	ret = platform_driver_register(&exynos_drm_common_hdmi_driver);
	if (ret < 0)
		goto out_common_hdmi;
#endif

#ifdef CONFIG_DRM_EXYNOS_VIDI
	ret = platform_driver_register(&vidi_driver);
	if (ret < 0)
		goto out_vidi;
#endif

#ifdef CONFIG_DRM_EXYNOS_G2D
	ret = platform_driver_register(&g2d_driver);
	if (ret < 0)
		goto out_g2d;
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMC
	ret = platform_driver_register(&fimc_driver);
	if (ret < 0)
		goto out_fimc;
#endif

#ifdef CONFIG_DRM_EXYNOS_GSC
	ret = platform_driver_register(&gsc_driver);
	if (ret < 0)
		goto out_gsc;
#endif

#ifdef CONFIG_DRM_EXYNOS_SC
	ret = platform_driver_register(&sc_driver);
	if (ret < 0)
		goto out_sc;
#endif

#ifdef CONFIG_DRM_EXYNOS_ROTATOR
	ret = platform_driver_register(&rotator_driver);
	if (ret < 0)
		goto out_rotator;
#endif

#ifdef CONFIG_DRM_EXYNOS_IPP
	ret = platform_driver_register(&ipp_driver);
	if (ret < 0)
		goto out_ipp;
#endif

#ifdef CONFIG_DRM_EXYNOS_DBG
	ret = platform_driver_register(&dbg_driver);
	if (ret < 0)
		goto out_dbg;
#endif

	ret = platform_driver_register(&exynos_drm_platform_driver);
	if (ret < 0)
		goto out;

	return 0;

out:
#ifdef CONFIG_DRM_EXYNOS_DBG
	platform_driver_unregister(&dbg_driver);
out_dbg:
#endif

#ifdef CONFIG_DRM_EXYNOS_IPP
	platform_driver_unregister(&ipp_driver);
out_ipp:
#endif

#ifdef CONFIG_DRM_EXYNOS_ROTATOR
	platform_driver_unregister(&rotator_driver);
out_rotator:
#endif

#ifdef CONFIG_DRM_EXYNOS_SC
	platform_driver_unregister(&sc_driver);
out_sc:
#endif

#ifdef CONFIG_DRM_EXYNOS_GSC
	platform_driver_unregister(&gsc_driver);
out_gsc:
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMC
	platform_driver_unregister(&fimc_driver);
out_fimc:
#endif

#ifdef CONFIG_DRM_EXYNOS_G2D
	platform_driver_unregister(&g2d_driver);
out_g2d:
#endif

#ifdef CONFIG_DRM_EXYNOS_VIDI
	platform_driver_unregister(&vidi_driver);
out_vidi:
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
	platform_driver_unregister(&exynos_drm_common_hdmi_driver);
out_common_hdmi:
	platform_driver_unregister(&mixer_driver);
out_mixer:
	platform_driver_unregister(&hdmi_driver);
out_hdmi:
#ifdef CONFIG_DRM_EXYNOS_HDCP
	platform_driver_unregister(&hdcp_driver);
out_hdcp:
#endif
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMD
	platform_driver_unregister(&fimd_driver);
out_fimd:
#endif
	return ret;
}

static void __exit exynos_drm_exit(void)
{
	DRM_DEBUG_DRIVER("%s\n", __FILE__);

	platform_driver_unregister(&exynos_drm_platform_driver);

#ifdef CONFIG_DRM_EXYNOS_DBG
	platform_driver_unregister(&dbg_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_IPP
	platform_driver_unregister(&ipp_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_ROTATOR
	platform_driver_unregister(&rotator_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_SC
	platform_driver_unregister(&sc_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_GSC
	platform_driver_unregister(&gsc_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMC
	platform_driver_unregister(&fimc_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_G2D
	platform_driver_unregister(&g2d_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_VIDI
	platform_driver_unregister(&vidi_driver);
#endif

#ifdef CONFIG_DRM_EXYNOS_HDMI
	platform_driver_unregister(&exynos_drm_common_hdmi_driver);
	platform_driver_unregister(&mixer_driver);
	platform_driver_unregister(&hdmi_driver);
#ifdef CONFIG_DRM_EXYNOS_HDCP
	platform_driver_unregister(&hdcp_driver);
#endif
#endif

#ifdef CONFIG_DRM_EXYNOS_FIMD
	platform_driver_unregister(&fimd_driver);
#endif
}

module_init(exynos_drm_init);
module_exit(exynos_drm_exit);

MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_AUTHOR("Seung-Woo Kim <sw0312.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC DRM Driver");
MODULE_LICENSE("GPL");
