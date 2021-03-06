#ifndef _INCLUDED_LABELS_H_
#define _INCLUDED_LABELS_H_

#include <gtk/gtk.h>
#include "brush_core.h"
#include "layer.h"
#include "fractal_label.h"
#include "ini_file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SELECT_TARGET_LABELS
{
	gchar *target, *area;
	gchar *rgb, *rgba, *alpha;
	gchar *active_layer, *under_layer, *canvas;
	gchar *threshold;
	gchar *detect_area, *area_normal, *area_large;
} SELECT_TARGET_LABELS;

typedef struct _CONTROL_POINT_LABELS
{
	gchar *select, *move, *change_pressure, *delete_point, *move_stroke,
		*copy_stroke, *joint_stroke;
} CONTROL_POINT_LABELS;

typedef struct _SHAPE_LABELS
{
	gchar *circle, *eclipse, *triangle, *square, *rhombus, *hexagon,
		*star, *pattern, *image;
} SHAPE_LABELS;

typedef struct _APPLICATON_LABELS
{
	char *language_name;

	struct
	{
		gchar *ok, *apply, *cancel, *normal, *reverse, *edit_selection, *window,
			*close_window, *place_left, *place_right, *fullscreen, *reference,
			*move_top_left, *hot_key, *loading, *saving;
	} window;

	struct
	{
		gchar *pixel, *length, *angle, *bg, *repeat, *preview, *interval, *minute,
			*detail, *target, *clip_board, *name, *type, *resolution, *center,
			*straight, *extend, *mode, *red, *green, *blue, *cyan, *magenta, *yellow,
			*key_plate, *add, *_delete;
	} unit;

	struct
	{
		gchar *file, *make_new, *open, *open_as_layer, *save, *save_as, *close, *quit;
		gchar *edit, *undo, *redo, *copy, *copy_visible, *cut, *paste, *clip_board,
			*transform, *projection;
		gchar *canvas, *change_resolution, *change_canvas_size,
			*flip_canvas_horizontally, *flip_canvas_vertically,
			*switch_bg_color, *change_2nd_bg_color, *canvas_icc;
		gchar *layer, *new_color, *new_vector, *new_layer_set, *new_3d_modeling,
			*copy_layer, *delete_layer, *fill_layer_fg_color, *fill_layer_pattern,
			*rasterize_layer, *merge_down_layer, *merge_all_layer, *visible2layer,
			*visible_copy;
		gchar *select, *select_none, *select_invert, *select_all, *selection_extend,
			*selection_reduct;
		gchar *view, *zoom, *zoom_in, *zoom_out, *zoom_reset, *reverse_horizontally,
			*rotate, *reset_rotate, *display_filter, *no_filter, *gray_scale,
			*gray_scale_yiq;
		gchar *filters, *blur, *motion_blur, *gaussian_blur, *bright_contrast,
			*hue_saturtion, *color_levels, *tone_curve, *luminosity2opacity,
			*color2alpha, *colorize_with_under, *gradation_map, *detect_max,
			*tranparancy_as_white, *fill_with_vector, *render, *cloud, *fractal;
		gchar *plug_in;
		gchar *script;
		gchar *help, *version;
	} menu;

	struct
	{
		gchar *title, *name, *width, *height, *second_bg_color, *adopt_icc_profile,
			*preset, *add_preset, *swap_height_and_width;
	} make_new;

	struct
	{
		gchar *title, *initialize, *new_brush, *smooth, *smooth_quality,
			*smooth_rate, *smooth_gaussian, *smooth_average, *base_scale,
			*brush_scale, *scale, *flow, *pressure, *extend, *blur,
			*outline_hardness, *color_extend, *select_move_start, *anti_alias,
			*change_text_color, *text_horizonal, *text_vertical, *text_style,
			*text_normal, *text_bold, *text_italic, *text_oblique,
			*has_balloon, *balloon_has_edge, *line_color, *fill_color, *line_width,
			*change_line_width, *manually_set, *aspect_ratio,
			*centering_horizontally, *centering_vertically, *adjust_range_to_text,
			*num_edge, *edge_size, *random_edge_size, *random_edge_distance,
			*num_children, *start_child_size, *end_child_size, *reverse,
			*reverse_horizontally, *reverse_vertically, *blend_mode, *hue,
			*saturation, *brightness, *contrast, *distance, *rotate_start,
			*rotate_speed, *random_rotate, *rotate_to_brush_direction, *size_range,
			*rotate_range, *random_size, *clockwise, *counter_clockwise,
			*both_direction, *min_degree, *min_distance, *min_pressure, *enter,
			*out, *enter_out, *mix, *gradation_reverse, *devide_stroke,
			*delete_stroke, *target, *stroke, *prior_angle, *control_point,
			*transform_free, *transform_scale, *transform_free_shape,
			*transform_rotate, *preference, *name, *copy_brush, *change_brush,
			*delete_brush, *texture, *texture_strength, *no_texture, *pallete_add,
			*pallete_delete, *load_pallete, *load_pallete_after, *write_pallete,
			*clear_pallete, *pick_mode, *single_pixels, *average_color, *open_path,
			*close_path, *update, *frequency, *cloud_color, *persistence,
			*rand_seed, *use_random, *update_immediately, *num_octaves, *linear,
			*cosine, *cubic, *colorize, *start_edit_3d, *end_edit_3d, *scatter,
			*scatter_size, *scatter_range, *scatter_random_size,
			*scatter_random_flow, *bevel, *round, *miter, *normal_brush;
		gchar *brush_default_names[NUM_BRUSH_TYPE];
		SELECT_TARGET_LABELS select;
		CONTROL_POINT_LABELS control;
		SHAPE_LABELS shape;
	} tool_box;

	struct
	{
		gchar *title, *new_layer, *new_vector, *new_layer_set, *new_text,
			*new_3d_modeling, *add_normal, *add_vector, *add_layer_set,
			*add_3d_modeling, *rename, *reorder, *alpha_to_select,
			*alpha_add_select, *pasted_layer, *under_layer, *mixed_under_layer;
		gchar *blend_mode, *opacity, *mask_with_under, *lock_opacity;
		gchar *blend_modes[LAYER_BLEND_SLELECTABLE_NUM];
	} layer_window;

	struct
	{
		gchar *straight_random, *bidirection;
	} filter;

	struct
	{
		gchar *compress, *quality, *write_alpha, *write_profile,
			*close_with_unsaved_chage, *quit_with_unsaved_change, *recover_backup;
	} save;

	struct
	{
		gchar *title;
	} navigation;

	struct
	{
		gchar *title, *base_setting, *auto_save, *theme, *default_theme,
			*conflict_hot_key, *language, *backup_path, *show_preview_on_taskbar;
		gchar *draw_with_touch, *scale_and_move_with_touch, *set_back_ground;
	} preference;

	struct
	{
		gchar *auto_save;
	} status_bar;
} APPLICATION_LABELS;

// 関数のプロトタイプ宣言
extern void LoadLabels(
	APPLICATION_LABELS* labels,
	FRACTAL_LABEL* fractal_labels,
	const char* lang_file_path
);

extern void LoadFractalLabels(
	FRACTAL_LABEL* labels,
	APPLICATION_LABELS* app_label,
	INI_FILE* file,
	const char* code
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_LABELS_H_
