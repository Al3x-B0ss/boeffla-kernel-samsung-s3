/* drivers/media/video/db8131a.h
 *
 * Driver for db8131a (3MP Camera) from SEC(LSI), firmware EVT1.1
 *
 * Copyright (C) 2010, SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __DB8131A_H__
#define __DB8131A_H__
#include <linux/i2c.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/videodev2_exynos_camera.h>
#include <media/db8131m_platform.h>

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#define DB8131A_DRIVER_NAME	DB8131M_DEVICE_NAME
static const char driver_name[] = DB8131A_DRIVER_NAME;

/************************************
 * FEATURE DEFINITIONS
 ************************************/
#define CONFIG_SUPPORT_AF			false
#define CONFIG_SUPPORT_FLASH			false
#define CONFIG_SUPPORT_CAMSYSFS			true
#define CONFIG_SUPPORT_WAIT_STREAMOFF_V2	false

/* for tuning */
#define CONFIG_LOAD_FILE		false

/** Debuging Feature **/
#if defined(CONFIG_MACH_TAB3) || defined(CONFIG_MACH_ZEST)
#define CONFIG_CAM_DEBUG		true
#define CONFIG_CAM_TRACE		true /* Enable me with CONFIG_CAM_DEBUG */
#define CONFIG_CAM_AF_DEBUG 		false /* Enable me with CONFIG_CAM_DEBUG */
#define DEBUG_WRITE_REGS		true

#define CONFIG_DEBUG_STREAMOFF		false
#else
#define CONFIG_CAM_DEBUG		false
#define CONFIG_CAM_TRACE		false /* Enable me with CONFIG_CAM_DEBUG */
#define CONFIG_CAM_AF_DEBUG 		false /* Enable me with CONFIG_CAM_DEBUG */
#define DEBUG_WRITE_REGS		false

#define CONFIG_DEBUG_STREAMOFF		false
#endif

/***********************************/

#ifdef CONFIG_VIDEO_DB8131A_DEBUG
enum {
	DB8131A_DEBUG_I2C		= 1U << 0,
	DB8131A_DEBUG_I2C_BURSTS	= 1U << 1,
};
static uint32_t db8131a_debug_mask = DB8131A_DEBUG_I2C_BURSTS;
module_param_named(debug_mask, db8131a_debug_mask, uint, S_IWUSR | S_IRUGO);

#define db8131a_debug(mask, x...) \
	do { \
		if (db8131a_debug_mask & mask) \
			pr_info(x);	\
	} while (0)
#else
#define db8131a_debug(mask, x...)
#endif

#define cam_err(fmt, ...)	\
	printk(KERN_ERR "[""%s""] " fmt, driver_name, ##__VA_ARGS__)
#define cam_warn(fmt, ...)	\
	printk(KERN_WARNING "[""%s""] " fmt, driver_name, ##__VA_ARGS__)
#define cam_info(fmt, ...)	\
	printk(KERN_INFO "[""%s""] " fmt, driver_name, ##__VA_ARGS__)

#if CONFIG_CAM_DEBUG
#define cam_dbg(fmt, ...)	\
	printk(KERN_DEBUG "[""%s""] " fmt, driver_name, ##__VA_ARGS__)

#else
#define cam_dbg(fmt, ...)	\
	do { \
		if (dbg_level & CAMDBG_LEVEL_DEBUG) \
			printk(KERN_DEBUG "[""%s""] " fmt, driver_name, ##__VA_ARGS__); \
	} while (0)
#endif

#if CONFIG_CAM_DEBUG && CONFIG_CAM_TRACE
#define cam_trace(fmt, ...)	cam_dbg("%s: " fmt, __func__, ##__VA_ARGS__);
#else
#define cam_trace(fmt, ...)	\
	do { \
		if (dbg_level & CAMDBG_LEVEL_TRACE) \
			printk(KERN_DEBUG "[""%s""] " fmt, \
				driver_name, ##__VA_ARGS__); \
	} while (0)
#endif

#if CONFIG_CAM_DEBUG && CONFIG_CAM_AF_DEBUG
#define af_dbg(fmt, ...)	cam_dbg(fmt, ##__VA_ARGS__);
#else
#define af_dbg(fmt, ...)
#endif

#define CHECK_ERR_COND(condition, ret)	\
	do { if (unlikely(condition)) return ret; } while (0)
#define CHECK_ERR_COND_MSG(condition, ret, fmt, ...) \
		if (unlikely(condition)) { \
			cam_err("%s: ERROR, " fmt, __func__, ##__VA_ARGS__); \
			return ret; \
		}

#define CHECK_ERR(x)	CHECK_ERR_COND(((x) < 0), (x))
#define CHECK_ERR_MSG(x, fmt, ...) \
	CHECK_ERR_COND_MSG(((x) < 0), (x), fmt, ##__VA_ARGS__)


#if CONFIG_LOAD_FILE
#define DB8131A_BURST_WRITE_REGS(sd, A) \
	({ \
		int ret; \
		cam_info("BURST_WRITE_REGS: reg_name=%s from setfile\n", #A); \
		ret = db8131a_write_regs_from_sd(sd, #A); \
		ret; \
	})
#else
#define DB8131A_BURST_WRITE_REGS(sd, A) \
	db8131a_burst_write_regs(sd, A, (sizeof(A) / sizeof(A[0])), #A)
#endif

/* result values returned to HAL */
enum af_result_status {
	AF_RESULT_NONE = 0x00,
	AF_RESULT_FAILED = 0x01,
	AF_RESULT_SUCCESS = 0x02,
	AF_RESULT_CANCELLED = 0x04,
	AF_RESULT_DOING = 0x08
};

enum af_operation_status {
	AF_NONE = 0,
	AF_START,
	AF_CANCEL,
};

enum preflash_status {
	PREFLASH_NONE = 0,
	PREFLASH_OFF,
	PREFLASH_ON,
};

enum db8131a_oprmode {
	DB8131A_OPRMODE_VIDEO = 0,
	DB8131A_OPRMODE_IMAGE = 1,
};

enum stream_cmd {
	STREAM_STOP,
	STREAM_START,
};

enum wide_req_cmd {
	WIDE_REQ_NONE,
	WIDE_REQ_CHANGE,
	WIDE_REQ_RESTORE,
};

enum db8131a_preview_frame_size {
	PREVIEW_SZ_QVGA,	/* 320x240 */
	PREVIEW_SZ_CIF,		/* 352x288 */
	PREVIEW_SZ_VGA,		/* 640x480 */
	PREVIEW_SZ_MAX,
};

/* Capture Size List: Capture size is defined as below.
 *
 *	CAPTURE_SZ_VGA:		640x480
 *	CAPTURE_SZ_WVGA:		800x480
 *	CAPTURE_SZ_SVGA:		800x600
 *	CAPTURE_SZ_WSVGA:		1024x600
 *	CAPTURE_SZ_1MP:		1280x960
 *	CAPTURE_SZ_W1MP:		1600x960
 *	CAPTURE_SZ_2MP:		UXGA - 1600x1200
 *	CAPTURE_SZ_W2MP:		35mm Academy Offset Standard 1.66
 *					2048x1232, 2.4MP
 *	CAPTURE_SZ_3MP:		QXGA  - 2048x1536
 *	CAPTURE_SZ_W4MP:		WQXGA - 2560x1536
 *	CAPTURE_SZ_5MP:		2560x1920
 */

enum db8131a_capture_frame_size {
	CAPTURE_SZ_VGA = 0,	/* 640x480 */
	CAPTURE_SZ_1MP,
	CAPTURE_SZ_MAX,
};

enum db8131a_fps_index {
	I_FPS_0,
	I_FPS_7,
	I_FPS_10,
	I_FPS_12,
	I_FPS_15,
	I_FPS_25,
	I_FPS_30,
	I_FPS_MAX,
};

enum ae_awb_lock {
	AEAWB_UNLOCK = 0,
	AEAWB_LOCK,
	AEAWB_LOCK_MAX,
};

struct db8131a_control {
	u32 id;
	s32 value;
	s32 default_value;
};

#define DB8131A_INIT_CONTROL(ctrl_id, default_val) \
	{					\
		.id = ctrl_id,			\
		.value = default_val,		\
		.default_value = default_val,	\
	}

struct db8131a_framesize {
	s32 index;
	u32 width;
	u32 height;
};

enum {
    FRMRATIO_QCIF   = 12,   /* 11 : 9 */
    FRMRATIO_VGA    = 13,   /* 4 : 3 */
    FRMRATIO_D1     = 15,   /* 3 : 2 */
    FRMRATIO_WVGA   = 16,   /* 5 : 3 */
    FRMRATIO_HD     = 17,   /* 16 : 9 */
    FRMRATIO_SQUARE     = 10,   /* 1 : 1 */
};

#define FRM_RATIO(w, h)	((w) * 10 / (h))

#define FRAMESIZE_RATIO(framesize) \
	FRM_RATIO((framesize)->width, (framesize)->height)

struct db8131a_stream_time {
	struct timeval curr_time;
	struct timeval before_time;
};

#define GET_ELAPSED_TIME(cur, before) \
		(((cur).tv_sec - (before).tv_sec) * USEC_PER_SEC \
		+ ((cur).tv_usec - (before).tv_usec))

struct db8131a_fps {
	u32 index;
	u32 fps;
};

struct db8131a_version {
	u32 major;
	u32 minor;
};

struct db8131a_date_info {
	u32 year;
	u32 month;
	u32 date;
};

enum runmode {
	RUNMODE_NOTREADY,
	RUNMODE_INIT,
	/*RUNMODE_IDLE,*/
	RUNMODE_RUNNING, /* previewing */
	RUNMODE_RUNNING_STOP,
	RUNMODE_CAPTURING,
	RUNMODE_CAPTURING_STOP,
	RUNMODE_RECORDING,	/* camcorder mode */
	RUNMODE_RECORDING_STOP,
};

struct db8131a_firmware {
	u32 addr;
	u32 size;
};

struct db8131a_jpeg_param {
	u32 enable;
	u32 quality;
	u32 main_size;		/* Main JPEG file size */
	u32 thumb_size;		/* Thumbnail file size */
	u32 main_offset;
	u32 thumb_offset;
	/* u32 postview_offset; */
};

struct db8131a_position {
	s32 x;
	s32 y;
};

struct db8131a_rect {
	s32 x;
	s32 y;
	u32 width;
	u32 height;
};

struct gps_info_common {
	u32 direction;
	u32 dgree;
	u32 minute;
	u32 second;
};

struct db8131a_gps_info {
	u8 gps_buf[8];
	u8 altitude_buf[4];
	s32 gps_timeStamp;
};

struct db8131a_mode {
	enum v4l2_sensor_mode sensor;
	enum runmode runmode;

	u32 hd_video:1;
} ;

struct db8131a_preview {
	const struct db8131a_framesize *frmsize;
	u32 update_frmsize:1;
	u32 fast_ae:1;
};

struct db8131a_capture {
	const struct db8131a_framesize *frmsize;
	u32 pre_req;	/* for fast capture */
	u32 ae_manual_mode:1;
	u32 lowlux_night:1;
	u32 ready:1;	/* for fast capture */
};

struct db8131a_focus {
	enum v4l2_focusmode mode;
	enum af_result_status status;
	struct db8131a_stream_time win_stable;

	u32 pos_x;
	u32 pos_y;

	u32 support:1;
	u32 start:1;	/* enum v4l2_auto_focus*/
	u32 touch:1;

	/* It means that cancel has been done and then each AF regs-table
	 * has been written. */
	u32 reset_done:1;
};

/* struct for sensor specific data */
struct db8131a_ae_gain_offset {
	u32	ae_auto;
	u32	ae_now;
	u32	ersc_auto;
	u32	ersc_now;

	u32	ae_ofsetval;
	u32	ae_maxdiff;
};

/* Flash struct */
struct db8131a_flash {
	struct db8131a_ae_gain_offset ae_offset;
	enum v4l2_flash_mode mode;
	enum preflash_status preflash;
	u32 awb_delay;
	u32 ae_scl;	/* for back-up */
	u32 on:1;	/* flash on/off */
	u32 ignore_flash:1;
	u32 ae_flash_lock:1;
	u32 support:1;	/* to support flash */
};

/* Exposure struct */
struct db8131a_exposure {
	s32 val;	/* exposure value */
	u32 ae_lock:1;
};

/* White Balance struct */
struct db8131a_whitebalance {
	enum v4l2_wb_mode mode; /* wb mode */
	u32 awb_lock:1;
};

struct db8131a_exif {
	u16 exp_time_den;
	u16 iso;
	u16 flash;

	/*int bv;*/		/* brightness */
	/*int ebv;*/		/* exposure bias */
};

/* EXIF - flash filed */
#define EXIF_FLASH_FIRED		(0x01)
#define EXIF_FLASH_MODE_FIRING		(0x01 << 3)
#define EXIF_FLASH_MODE_SUPPRESSION	(0x02 << 3)
#define EXIF_FLASH_MODE_AUTO		(0x03 << 3)

struct db8131a_regset {
	u32 size;
	u8 *data;
};


#if CONFIG_LOAD_FILE

#if !(DEBUG_WRITE_REGS)
#undef DEBUG_WRITE_REGS
#define DEBUG_WRITE_REGS	true
#endif

struct regset_table {
	const char	*const name;
};

#define REGSET(x, y)		\
	[(x)] = {			\
		.name		= #y,	\
}

#define REGSET_TABLE(y)	\
	{				\
		.name		= #y,	\
}

#else /* !CONFIG_LOAD_FILE */
struct regset_table {
	const u16	*const reg;
	const u32	array_size;
#if DEBUG_WRITE_REGS
	const char	*const name;
#endif
};

#if DEBUG_WRITE_REGS
#define REGSET(x, y)		\
	[(x)] = {					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
}

#define REGSET_TABLE(y)		\
	{					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
}
#else
#define REGSET(x, y)		\
	[(x)] = {					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
}

#define REGSET_TABLE(y)		\
	{					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
}
#endif /* DEBUG_WRITE_REGS */
#endif /* CONFIG_LOAD_FILE */

#define EV_MIN_VLAUE		EV_MINUS_4
#define GET_EV_INDEX(EV)	((EV) - (EV_MIN_VLAUE))

struct db8131a_regs {
	struct regset_table ev[GET_EV_INDEX(EV_MAX_V4L2)];
	struct regset_table metering[METERING_MAX];
	struct regset_table iso[ISO_MAX];
	struct regset_table effect[IMAGE_EFFECT_MAX];
	struct regset_table white_balance[WHITE_BALANCE_MAX];
	struct regset_table preview_size[PREVIEW_SZ_MAX];
	struct regset_table scene_mode[SCENE_MODE_MAX];
	struct regset_table saturation[SATURATION_MAX];
	struct regset_table contrast[CONTRAST_MAX];
	struct regset_table sharpness[SHARPNESS_MAX];
	struct regset_table fps[I_FPS_MAX];
	struct regset_table flash_start;
	struct regset_table flash_end;
	struct regset_table af_pre_flash_start;
	struct regset_table af_pre_flash_end;
	struct regset_table flash_ae_set;
	struct regset_table flash_ae_clear;
	struct regset_table ae_lock_on;
	struct regset_table ae_lock_off;
	struct regset_table awb_lock_on;
	struct regset_table awb_lock_off;
	struct regset_table restore_cap;
	struct regset_table change_wide_cap;
#ifdef CONFIG_MACH_P8
	struct regset_table set_lowlight_cap;
#endif

	/* AF */
	struct regset_table af_macro_mode;
	struct regset_table af_normal_mode;
#if !defined(CONFIG_MACH_P2)
	struct regset_table af_night_normal_mode;
#endif
	struct regset_table af_off;
	struct regset_table hd_af_start;
	struct regset_table hd_first_af_start;
	struct regset_table single_af_start;

	/* Init */
	struct regset_table init;
	struct regset_table init_vt;

	struct regset_table get_light_level;
	struct regset_table get_esd_status;
	struct regset_table get_iso;
	struct regset_table get_ae_stable;
	struct regset_table get_shutterspeed;

	/* Mode */
	struct regset_table preview_mode;
	struct regset_table preview_hd_mode;
	struct regset_table return_preview_mode;
	struct regset_table capture_mode[CAPTURE_SZ_MAX];
	struct regset_table camcorder_on;
	struct regset_table camcorder_off;
	struct regset_table stream_stop;
#ifdef CONFIG_MACH_P8
	struct regset_table antibanding;
#endif /* CONFIG_MACH_P8 */
};

struct db8131a_state {
	struct db8131m_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format req_fmt;
	struct db8131a_preview preview;
	struct db8131a_capture capture;
	struct db8131a_focus focus;
	struct db8131a_flash flash;
	struct db8131a_exposure exposure;
	struct db8131a_whitebalance wb;
	struct db8131a_exif exif;
	struct db8131a_stream_time stream_time;
	const struct db8131a_regs *regs;
	struct mutex ctrl_lock;
	struct mutex af_lock;
	struct work_struct af_work;
	struct work_struct af_win_work;
#if CONFIG_DEBUG_STREAMOFF
	struct work_struct streamoff_work;
#endif
	struct workqueue_struct *workqueue;
	enum runmode runmode;
	enum v4l2_sensor_mode sensor_mode;
	enum v4l2_pix_format_mode format_mode;
	enum v4l2_scene_mode scene_mode;

	/* To switch from nornal ratio to wide ratio.*/
	enum wide_req_cmd wide_cmd;

	s32 vt_mode;
	s32 req_fps;
	s32 fps;
	s32 freq;		/* MCLK in Hz */
	u32 one_frame_delay_ms;
	u32 light_level;	/* light level */
	/* u32 streamoff_delay;*/
	u32 *dbg_level;
#ifdef CONFIG_DEBUG_STREAMOFF
	atomic_t streamoff_check;
#endif
	pid_t af_pid;

	u32 recording:1;
	u32 hd_videomode:1;
	u32 need_wait_streamoff:1;
	u32 initialized:1;
};

#define TO_STATE(p, m)		(container_of(p, struct db8131a_state, m))
#define IS_FLASH_SUPPORTED()	(CONFIG_SUPPORT_FLASH)
#define IS_AF_SUPPORTED()	(CONFIG_SUPPORT_AF)

extern struct class *camera_class;

static inline struct  db8131a_state *to_state(struct v4l2_subdev *sd)
{
	return TO_STATE(sd, sd);
}

static inline int check_af_pid(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

	if (state->af_pid && (task_pid_nr(current) == state->af_pid))
		return -EPERM;
	else
		return 0;
}

static int db8131a_init(struct v4l2_subdev *, u32);
static int db8131a_reset(struct v4l2_subdev *, u32);
static int db8131a_s_ctrl(struct v4l2_subdev *, struct v4l2_control *);

/*********** Sensor specific ************/
#define DB8131A_DELAY		0xE700

#define DB8131A_CHIP_ID			0x6100
#define DB8131A_CHIP_REV		0x06
#define DB8131A_CHIP_REV_OLD		0x04

#define FORMAT_FLAGS_COMPRESSED		0x3

#define POLL_TIME_MS		10
#define CAPTURE_POLL_TIME_MS    1000

/* maximum time for one frame in norma light */
#define ONE_FRAME_DELAY_MS_NORMAL		66
/* maximum time for one frame in low light: minimum 10fps. */
#define ONE_FRAME_DELAY_MS_LOW			100
/* maximum time for one frame in night mode: 6fps */
#define ONE_FRAME_DELAY_MS_NIGHTMODE		166

/* level at or below which we need to enable flash when in auto mode */
#ifdef CONFIG_MACH_P2
#define FLASH_LOW_LIGHT_LEVEL		0x3A
#elif defined(CONFIG_MACH_P8)
#define FLASH_LOW_LIGHT_LEVEL		0x46 /* 70 */
#define CAPTURE_LOW_LIGHT_LEVEL		0x20
#else
#define FLASH_LOW_LIGHT_LEVEL		0x4A
#endif /* CONFIG_MACH_P2 */

#define FIRST_AF_SEARCH_COUNT		220
#define SECOND_AF_SEARCH_COUNT		220
#define AE_STABLE_SEARCH_COUNT		22
#define STREAMOFF_CHK_COUNT		150

#define AF_SEARCH_DELAY			33
#define AE_STABLE_SEARCH_DELAY		33

/* Sensor AF first,second window size.
 * we use constant values intead of reading sensor register */
#define FIRST_WINSIZE_X			512
#define FIRST_WINSIZE_Y			568
#define SCND_WINSIZE_X			230
#define SCND_WINSIZE_Y			306

/* The Path of Setfile */
#if CONFIG_LOAD_FILE
struct test {
	u8 data;
	struct test *nextBuf;
};
static struct test *testBuf;
static s32 large_file;

#define TEST_INIT	\
{			\
	.data = 0;	\
	.nextBuf = NULL;	\
}

#if defined(CONFIG_MACH_ZEST)
#define TUNING_FILE_PATH "/mnt/sdcard/db8131a_regs-zest.h"
#elif defined(CONFIG_MACH_TAB3)
#define TUNING_FILE_PATH "/mnt/sdcard/db8131a_regs-tab3.h"
#else
#define TUNING_FILE_PATH NULL
#endif
#endif /* CONFIG_LOAD_FILE*/

#if defined(CONFIG_MACH_ZEST)
#include "db8131a_regs_zest.h"
#else /* CONFIG_MACH_TAB3*/
#include "db8131a_regs.h"
#endif

#endif /* __DB8131A_H__ */
