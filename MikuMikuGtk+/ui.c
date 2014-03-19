// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <GL/glew.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include "application.h"
#include "mikumikugtk.h"
#include "spin_scale.h"
#include "memory.h"

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

typedef enum _eSET_VALUE_TYPE
{
	SET_VALUE_TYPE_X,
	SET_VALUE_TYPE_Y,
	SET_VALUE_TYPE_Z
} eSET_VALUE_TYPE;

static void ExecuteLoadModel(APPLICATION* application);
static void ExecuteLoadPose(APPLICATION* application);
static void FillParentModelComboBox(GtkWidget* combo, SCENE* scene, APPLICATION* application);
static void FillParentBoneComboBox(GtkWidget* combo, SCENE* scene, APPLICATION* application);
static void SetCameraPositionWidget(APPLICATION* application);
static void SetLightColorDirectionWidget(APPLICATION* application);

static void ToggleButtonSetValueCallback(GtkWidget* button, int* set_target)
{
	int value = (int)(g_object_get_data(G_OBJECT(button), "value"));
	void *data = g_object_get_data(G_OBJECT(button), "callback_data");
	void (*callback)(void*, int) = (void (*)(void*, int))g_object_get_data(G_OBJECT(button), "callback_function");
	int *disabled = (int*)(g_object_get_data(G_OBJECT(button), "disabled"));

	if(disabled == NULL || *disabled == 0)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
		{
			*set_target = value;
			if(callback != NULL)
			{
				callback(data, value);
			}
		}
	}
}

static void ToggleButtonSetValue(
	GtkWidget* button,
	int value,
	int* target,
	void* data,
	void (*callback)(void*, int),
	int* disabled
)
{
	(void)g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ToggleButtonSetValueCallback), (void*)target);
	g_object_set_data(G_OBJECT(button), "value", GINT_TO_POINTER(value));
	g_object_set_data(G_OBJECT(button), "callback_data", data);
	g_object_set_data(G_OBJECT(button), "callback_function", (void*)callback);
	g_object_set_data(G_OBJECT(button), "disabled", disabled);
}

static void ToggleButtonSetFlagCallback(GtkWidget* button, int* set_target)
{
	unsigned int value = (unsigned int)(g_object_get_data(G_OBJECT(button), "value"));
	void *data = g_object_get_data(G_OBJECT(button), "callback_data");
	void (*callback)(void*, int) = (void (*)(void*, int))g_object_get_data(G_OBJECT(button), "callback_function");
	int *disabled = (int*)(g_object_get_data(G_OBJECT(button), "disabled"));
	int active;

	if(disabled == NULL || *disabled == 0)
	{
		if((active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) != FALSE)
		{
			*set_target |= value;
		}
		else
		{
			*set_target &= ~(value);
		}

		if(callback != NULL)
		{
			callback(data, active);
		}
	}
}

static void ToggleButtonSetFlag(
	GtkWidget* button,
	unsigned int value,
	unsigned int* target,
	void* data,
	void (*callback)(void*, int),
	int* disabled
)
{
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ToggleButtonSetFlagCallback), target);
	g_object_set_data(G_OBJECT(button), "value", GUINT_TO_POINTER(value));
	g_object_set_data(G_OBJECT(button), "callback_data", data);
	g_object_set_data(G_OBJECT(button), "callback_function", (void*)callback);
	g_object_set_data(G_OBJECT(button), "disabled", disabled);
}

static void InitializeGL(PROJECT* project, int widget_width, int widget_height)
{
	SetRequiredOpengGLState();

	// �O���b�h
	InitializeGrid(&project->grid);
	LoadGrid(&project->grid, (void*)project->application_context);

	// �����G���W���f�[�^
	InitializeWorld(&project->world);

	// �V�[���Ǘ��f�[�^
	project->scene = (SCENE*)MEM_ALLOC_FUNC(sizeof(*project->scene));
	InitializeScene(project->scene, widget_width, widget_height, project->world.world, project);

	// �V���h�E�}�b�s���O�̏�����
	project->shadow_map = (SHADOW_MAP*)MEM_ALLOC_FUNC(sizeof(*project->shadow_map));
	InitializeShadowMap(project->shadow_map, widget_width, widget_height, (void*)project->scene);

	// ���쒆�̃{�[������\������f�[�^��������
	InitializeDebugDrawer(&project->debug_drawer, (void*)project);
	DebugDrawerLoad(&project->debug_drawer);

	InitializeControl(&project->control, widget_width, widget_height, project);
	LoadControlHandle(&project->control.handle, project->application_context);

	project->world.debug_drawer = &project->debug_drawer;
}

static void ShowChildMenuItem(GtkWidget* menu, void* dummy)
{
	gtk_menu_item_select(GTK_MENU_ITEM(menu));
}

GtkWidget* MakeMenuBar(void* application_context, GtkAccelGroup* hot_key)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = application->projects[application->active_project];
	GtkWidget *menu_bar;
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkWidget *sub_item;
	char buff[2048];

	if(hot_key == NULL)
	{
		hot_key = gtk_accel_group_new();
		gtk_window_add_accel_group(GTK_WINDOW(application->widgets.main_window), hot_key);
	}

	menu_bar = gtk_menu_bar_new();

	// �u�t�@�C���v���j���[
	(void)sprintf(buff, "_%s(_F)", application->label.menu.file);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", hot_key, 'F',
		GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(ShowChildMenuItem), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// �u���f���E�A�N�Z�T���[�̓ǂݍ��݁v
	(void)sprintf(buff, "%s", application->label.menu.add_model_accessory);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", hot_key,
		'O', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteLoadModel), application);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	// [�|�[�Y�̓ǂݍ��݁v
	(void)sprintf(buff, "%s", application->label.control.load_pose);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteLoadPose), application);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// �u�ҏW�v���j���[
	(void)sprintf(buff, "_%s(_E)", application->label.menu.edit);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", hot_key, 'E',
		GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(ShowChildMenuItem), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	// ���ɖ߂�
	(void)sprintf(buff, "%s", application->label.menu.undo);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", hot_key,
		'Z', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteControlUndo), application);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	// ��蒼��
	(void)sprintf(buff, "%s", application->label.menu.redo);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	gtk_widget_add_accelerator(menu_item, "activate", hot_key,
		'Y', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
		G_CALLBACK(ExecuteControlRedo), application);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	return menu_bar;
}

static void MakeContextShadowMap(PROJECT* project, int width, int height)
{
	ResizeShadowMap(project->shadow_map, width, height);
	MakeShadowMap(project->shadow_map);
}

gboolean ConfigureEvent(
	GtkWidget* widget,
	void* event_info,
	void* project_context
)
{
#define MIN_SIZE 32
	PROJECT *project = (PROJECT*)project_context;
	GdkEventConfigure *info = (GdkEventConfigure*)event_info;
	GtkAllocation allocation;
	GdkGLContext *context = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(widget);

	if(gdk_gl_drawable_gl_begin(drawable, context) == FALSE)
	{
		return FALSE;
	}

#if GTK_MAJOR_VERSION <= 2
	allocation = widget->allocation;
#else
	gtk_widget_get_allocation(widget, &allocation);
#endif

	if((project->flags & PROJECT_FLAG_GL_INTIALIZED) == 0)
	{
		if(allocation.width < MIN_SIZE || allocation.height < MIN_SIZE)
		{
			return FALSE;
		}

		if(glewInit() != GLEW_OK)
		{
			gdk_gl_drawable_gl_end(drawable);
			return FALSE;
		}

		InitializeGL(project, allocation.width, allocation.height);

		MakeContextShadowMap(project, 2048, 2048);
		project->flags |= PROJECT_FLAG_GL_INTIALIZED;

		SetCameraPositionWidget(project->application_context);
		SetLightColorDirectionWidget(project->application_context);
	}

	project->scene->width = allocation.width;
	project->scene->height = allocation.height;
	ResizeHandle(&project->control.handle, allocation.width, allocation.height);

	glViewport(0, 0, allocation.width, allocation.height);

	gdk_gl_drawable_gl_end(drawable);

	return TRUE;
}

void InitializeProjectWidgets(PROJECT_WIDGETS* widgets, int widget_width, int widget_height, void* project)
{
	// OpenGL�̐ݒ�
	GdkGLConfig *config;

	// �L�����o�X�E�B�W�F�b�g���쐬
	widgets->drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(widgets->drawing_area, widget_width, widget_height);
	// ��������C�x���g��ݒ�
	gtk_widget_set_events(widgets->drawing_area,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_SCROLL_MASK
				| GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_RELEASE_MASK
#if GTK_MAJOR_VERSION >= 3
				| GDK_TOUCH_MASK
#endif
	);
	// �L�����o�X��OpenGL��ݒ�
	config = gdk_gl_config_new_by_mode(
		GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_STENCIL);
	if(config == NULL)
	{
		return;
	}
	if(gtk_widget_set_gl_capability(widgets->drawing_area, config,
		NULL, TRUE, GDK_GL_RGBA_TYPE) == FALSE)
	{
		return;
	}
}

gboolean MouseButtonPressEvent(GtkWidget* widget, GdkEventButton* event_info, void* project_context)
{
	MouseButtonPressCallback(project_context, event_info->x, event_info->y,
		(int)event_info->button, event_info->state);

	return TRUE;
}

gboolean MouseMotionEvent(GtkWidget* widget, GdkEventMotion* event_info, void* project_context)
{
	gdouble x0, y0;
	GdkModifierType state;

	if(event_info->is_hint != 0)
	{
		gint get_x, get_y;
		gdk_window_get_pointer(event_info->window, &get_x, &get_y, &state);
		x0 = get_x, y0 = get_y;
	}
	else
	{
		state = event_info->state;
		x0 = event_info->x;
		y0 = event_info->y;
	}

	MouseMotionCallback(project_context, event_info->x, event_info->y, (int)state);

	if((event_info->state & GDK_BUTTON2_MASK) || (event_info->state & GDK_BUTTON3_MASK) != 0)
	{
		SetCameraPositionWidget(((PROJECT*)project_context)->application_context);
	}

	return TRUE;
}

gboolean MouseButtonReleaseEvent(GtkWidget* widget, GdkEventButton* event_info, void* project_context)
{
	MouseButtonReleaseCallback(project_context, event_info->x, event_info->y,
		(int)event_info->button, event_info->state);

	return TRUE;
}

gboolean MouseWheelScrollEvent(GtkWidget* widget, GdkEventScroll* event_info, void* project_context)
{
	MouseScrollCallback(project_context, event_info->direction, event_info->state);
	SetCameraPositionWidget(((PROJECT*)project_context)->application_context);

	return TRUE;
}

gboolean ProjectDisplayEvent(GtkWidget* widget, GdkEventExpose* event_info, void* project_context)
{
	GtkAllocation allocation;
	GdkGLContext *context = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(widget);

	if(gdk_gl_drawable_gl_begin(drawable, context) == FALSE)
	{
		return FALSE;
	}

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(widget, &allocation);
#else
	allocation = widget->allocation;
#endif

	DisplayProject((PROJECT*)project_context, allocation.width, allocation.height);

	if(gdk_gl_drawable_is_double_buffered(drawable) != FALSE)
	{
		gdk_gl_drawable_swap_buffers(drawable);
	}
	else
	{
		glFlush();
	}

	//glPopMatrix ();

	gdk_gl_drawable_gl_end(drawable);

	return TRUE;
}

int ShowModelComment(MODEL_INTERFACE* model, PROJECT* project)
{
#define DIALOG_SIZE 480
	GtkWidget *dialog;
	GtkWidget *note_book;
	GtkWidget *scrolled_window;
	GtkWidget *label;
	int ok;

	dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(project->application_context->widgets.main_window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	note_book = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		note_book, TRUE, TRUE, 2);

	// ���{��^�u
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	label = gtk_label_new("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E");
	(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), scrolled_window, label);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	label = gtk_label_new(model->comment);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), label);
	// English�^�u
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	label = gtk_label_new("English");
	(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), scrolled_window, label);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	label = gtk_label_new(model->english_comment);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), label);

	gtk_widget_set_size_request(dialog, DIALOG_SIZE, DIALOG_SIZE);

	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		ok = TRUE;
	}
	else
	{
		ok = FALSE;
	}
	gtk_widget_destroy(dialog);
#undef DIALOG_SIZE
	return ok;
}

static void ExecuteLoadModel(APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	GtkWidget *chooser;
	GtkFileFilter *filter;
	GtkWindow *main_window = NULL;

	main_window = GTK_WINDOW(application->widgets.main_window);

	chooser = gtk_file_chooser_dialog_new(
		application->label.control.load_model,
		main_window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Model File");
	gtk_file_filter_add_pattern(filter, "*.pmx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Accessory File");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
	{
		MODEL_INTERFACE *model;
		gchar *system_path;
		gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		gchar *file_type;
		const gchar *filter_name;
		int ok = TRUE;

		filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
		filter_name = gtk_file_filter_get_name(filter);
		if(strcmp(filter_name, "Model File") == 0)
		{
			file_type = ".pmx";
		}
		else
		{
			gchar *str = path;
			file_type = path;
			while(*str != '\0')
			{
				if((*str == '/' || *str == '\\')
					&& *(str+1) != '\0')
				{
					file_type = str+1;
				}
				else if(*str == '.')
				{
					file_type = str;
				}

				str = NextCharUTF8(str);
			}
		}

		gtk_widget_destroy(chooser);

		system_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		model = LoadModel(application, system_path, file_type);
		if(model->type == MODEL_TYPE_PMX_MODEL)
		{
			ok = ShowModelComment(model, application->projects[application->active_project]);
			if(ok == FALSE)
			{
				SceneRemoveModel(project->scene, model);
			}
		}

		if(ok != FALSE)
		{
#if GTK_MAJOR_VERSION <= 2
			gtk_combo_box_append_text(GTK_COMBO_BOX(application->widgets.model_combo_box), model->name);
#else
			gtk_combo_box_text_append_text(GTK_COMBO_BOX(application->widgets.model_combo_box), model->name);
#endif
			gtk_combo_box_set_active(GTK_COMBO_BOX(application->widgets.model_combo_box),
				(int)project->scene->models->num_data);
			FillParentModelComboBox(application->widgets.connect_model, project->scene, application);
		}

		g_free(system_path);
	}
	else
	{
		gtk_widget_destroy(chooser);
	}
}

static void ExecuteLoadPose(APPLICATION* application)
{
	PROJECT *project = NULL;
	GtkWidget *chooser;
	GtkFileFilter *filter;
	GtkWindow *main_window = NULL;
	GtkWidget *apply_center_position;

	if(application->num_projects > 0)
	{
		project = application->projects[application->active_project];
		if(project->scene->selected_model == NULL)
		{
			return;
		}
		main_window = GTK_WINDOW(
			application->widgets.main_window);
	}
	else
	{
		return;
	}

	chooser = gtk_file_chooser_dialog_new(
		application->label.control.load_pose,
		main_window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Pose File");
	gtk_file_filter_add_pattern(filter, "*.vpd");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	apply_center_position = gtk_check_button_new_with_label(application->label.control.apply_center_position);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(chooser))), apply_center_position, FALSE, FALSE, 2);

	gtk_widget_show_all(chooser);
	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
	{
		POSE *pose;
		FILE *fp;
		gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		char *system_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		char *pose_data;
		gboolean apply_center = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(apply_center_position));
		long data_size;

		fp = fopen(system_path, "rb");
		(void)fseek(fp, 0, SEEK_END);
		data_size = ftell(fp);
		rewind(fp);
		pose_data = (char*)MEM_ALLOC_FUNC(data_size+1);
		(void)fread(pose_data, 1, data_size, fp);
		pose_data[data_size] = '\0';
		g_free(system_path);

		gtk_widget_destroy(chooser);

		pose = LoadPoseData(pose_data);
		ApplyPose(pose, project->scene->selected_model, apply_center);
		DeletePose(&pose);

		SceneUpdateModel(project->scene, project->scene->selected_model, TRUE);
		if((project->flags & PROJECT_FLAG_ALWAYS_PHYSICS) == 0)
		{
			WorldStepsSimulation(&project->world, 20, 120);
		}

		MEM_FREE_FUNC(pose_data);
	}
	else
	{
		gtk_widget_destroy(chooser);
	}
}

static void OnChangeSelectedModel(GtkWidget* combo, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	if(application->num_projects <= 0)
	{
		return;
	}
	scene = project->scene;

	if(select <= 0)
	{
		scene->selected_model = NULL;
	}
	else
	{
		float set_value[4];
		application->widgets.ui_disabled = TRUE;
		scene->selected_model = (MODEL_INTERFACE*)scene->models->buffer[select-1];
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_scale), scene->selected_model->scale_factor * 100.0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_scale), scene->selected_model->opacity * 100.0);
		scene->selected_model->get_world_translation(scene->selected_model, set_value);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_position[0]), set_value[0]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_position[1]), set_value[1]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_position[2]), set_value[2]);
		scene->selected_model->get_world_orientation(scene->selected_model, set_value);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_rotation[0]), set_value[0]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_rotation[1]), set_value[1]);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.model_rotation[2]), set_value[2]);
		application->widgets.ui_disabled = FALSE;
	}
}

static void OnChangeModelScale(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	if(application->num_projects <= 0)
	{
		return;
	}

	if(project->scene->selected_model != NULL)
	{
		project->scene->selected_model->scale_factor =
			(float)(gtk_adjustment_get_value(adjustment) * 0.01);
		SceneUpdateModel(project->scene, project->scene->selected_model, FALSE);
	}
}

static void OnChangeModelOpacity(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	if(application->num_projects <= 0)
	{
		return;
	}

	if(project->scene->selected_model != NULL)
	{
		project->scene->selected_model->opacity =
			(float)(gtk_adjustment_get_value(adjustment) * 0.01);
		SceneUpdateModel(project->scene, project->scene->selected_model, FALSE);
	}
}

static void FillParentModelComboBox(GtkWidget* combo, SCENE* scene, APPLICATION* application)
{
	GtkTreeModel *tree_model;
	int i;

	tree_model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	gtk_list_store_clear(GTK_LIST_STORE(tree_model));
#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), application->label.control.no_select);
	if(scene != NULL)
	{
		for(i=0; i<(int)scene->models->num_data; i++)
		{
			if(scene->models->buffer[i] != (void*)scene->selected_model)
			{
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo), ((MODEL_INTERFACE*)scene->models->buffer[i])->name);
			}
			if(scene->selected_model != NULL)
			{
				if(scene->models->buffer[i] == (void*)scene->selected_model->parent_model)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i+1);
				}
			}
		}
	}
#else
	gtk_combo_box_text_append_text(GTK_COMBO_BOX(combo), application->label.control.no_select);
	if(scene != NULL)
	{
		for(i=0; i<(int)scene->models->num_data; i++)
		{
			if(scene->models->buffer[i] != (void*)scene->selected_model)
			{
				gtk_combo_box_text_append_text(GTK_COMBO_BOX(combo), ((MODEL_INTERFACE*)scene->models->buffer[i])->name);
			}
			if(scene->selected_model != NULL)
			{
				if(scene->models->buffer[i] == (void*)scene->selected_model->parent_model)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i+1);
				}
			}
		}
	}
#endif
}

static void FillParentBoneComboBox(GtkWidget* combo, SCENE* scene, APPLICATION* application)
{
	GtkTreeModel *tree_model;
	int i;

	tree_model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	gtk_list_store_clear(GTK_LIST_STORE(tree_model));
#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), application->label.control.no_select);
	if(scene != NULL)
	{
		if(scene->selected_model != NULL)
		{
			if(scene->selected_model->parent_model != NULL)
			{
				char **names = GetChildBoneNames(scene->selected_model->parent_model, application);
				for(i=0; names[i] != NULL; i++)
				{
					gtk_combo_box_append_text(GTK_COMBO_BOX(combo), names[i]);
					if(scene->selected_model->parent_bone != NULL)
					{
						if(strcmp(names[i], scene->selected_model->parent_bone->name) == 0)
						{
							gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i+1);
						}
					}
					MEM_FREE_FUNC(names[i]);
				}
				MEM_FREE_FUNC(names);
			}
		}
	}
#else
	gtk_combo_box_text_append_text(GTK_COMBO_BOX(combo), application->label.control.no_select);
	if(scene != NULL)
	{
		if(scene->selected_model != NULL)
		{
			if(scene->selected_model->parent_model != NULL)
			{
				char **names = GetChildBoneNames(scene->selected_model->parent_model, application);
				for(i=0; names[i] != NULL; i++)
				{
					gtk_combo_box_text_append_text(GTK_COMBO_BOX(combo), names[i]);
					if(scene->selected_model->parent_bone != NULL)
					{
						if(strcmp(names[i], scene->selected_model->parent_bone->name) == 0)
						{
							gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i+1);
						}
					}
				}
				MEM_FREE_FUNC(names);
			}
		}
	}
#endif
}

static void OnChangeSelectedParentModel(GtkWidget* combo, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	GtkWidget *bone_combo = application->widgets.connect_bone;
	GtkTreeModel *tree_model;
	gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	if(application->num_projects <= 0)
	{
		return;
	}
	scene = project->scene;

	tree_model = gtk_combo_box_get_model(GTK_COMBO_BOX(bone_combo));
	gtk_list_store_clear(GTK_LIST_STORE(tree_model));
	if(select <= 0)
	{
		scene->selected_model->parent_model = NULL;
	}
	else
	{
		scene->selected_model->parent_model = (MODEL_INTERFACE*)scene->models->buffer[select-1];
	}
	FillParentBoneComboBox(application->widgets.connect_bone, scene, application);
}

static void OnChangeSelectedParentBone(GtkWidget* combo, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	gchar *name;

	if(application->num_projects <= 0)
	{
		return;
	}
	scene = project->scene;

	if(scene->selected_model == NULL)
	{
		return;
	}

	if(select <= 0)
	{
		scene->selected_model->parent_bone = NULL;
	}
	else
	{
#if GTK_MAJOR_VERSION <= 2
		name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
#else
		name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX(combo));
#endif
		if(scene->selected_model->parent_model != NULL)
		{
			scene->selected_model->parent_bone =
				scene->selected_model->parent_model->find_bone(scene->selected_model->parent_model, name);
		}
		else
		{
			scene->selected_model->parent_bone = NULL;
		}
	}

	SceneUpdateModel(scene, scene->selected_model, FALSE);
}

static void OnChangeModelPosition(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	VECTOR3 position;
	eSET_VALUE_TYPE type = (eSET_VALUE_TYPE)g_object_get_data(G_OBJECT(adjustment), "set_type");
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	if(scene->selected_model != NULL)
	{
		scene->selected_model->get_world_translation(scene->selected_model, position);
		position[type] = value;
		scene->selected_model->set_world_position(scene->selected_model, position);
	}

	SceneUpdateModel(scene, scene->selected_model, TRUE);
}

static void OnChangeModelRotation(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	QUATERNION rotation;
	eSET_VALUE_TYPE type = (eSET_VALUE_TYPE)g_object_get_data(G_OBJECT(adjustment), "set_type");
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	if(scene->selected_model != NULL)
	{
		scene->selected_model->get_world_orientation(scene->selected_model, rotation);
		rotation[type] = value * (float)M_PI / 180.0f;
		scene->selected_model->set_world_orientation(scene->selected_model, rotation);
	}

	SceneUpdateModel(scene, scene->selected_model, TRUE);
}

GtkWidget* BoneTreeViewNew(void)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *view;

	view = gtk_tree_view_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 0);

	return view;
}

void OnChangedSelectedBone(GtkWidget* widget, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	GtkTreeIter iter;
	GtkTreeModel *model;
	BONE_INTERFACE *bone;

	if(application->num_projects <= 0)
	{
		return;
	}

	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter) != FALSE)
	{
		gtk_tree_model_get(model, &iter, 1, &bone, -1);
		ChangeSelectedBones(project, &bone, 1);
	}
}

typedef struct _SET_LABEL_DATA
{
	char *label;
	POINTER_ARRAY *child_names;
} SET_LABEL_DATA;

void BoneTreeViewSetBones(GtkWidget *tree_view, void* model_interface, void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = application->projects[application->active_project];
	MODEL_INTERFACE *model = (MODEL_INTERFACE*)model_interface;
	STRUCT_ARRAY *set_label_data = StructArrayNew(sizeof(SET_LABEL_DATA), DEFAULT_BUFFER_SIZE);
	SET_LABEL_DATA *parent;
	SET_LABEL_DATA *data;
	LABEL_INTERFACE **labels;
	LABEL_INTERFACE *label;
	BONE_INTERFACE *bone;
	POINTER_ARRAY *added_bones;
	GtkTreeStore *tree_store;
	GtkTreeStore *old_store = NULL;
	GtkTreeIter toplevel;
	GtkTreeIter child;
	int num_labels;
	int num_children;
	int num_added_bones = 0;
	int i, j;

	if(model == NULL)
	{
		return;
	}

	tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view)));
	if(tree_store == NULL)
	{
		tree_store = old_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(tree_store));
	}
	else
	{
		gtk_tree_store_clear(tree_store);
	}

	labels = (LABEL_INTERFACE**)model->get_labels(model, &num_labels);

	parent = (SET_LABEL_DATA*)StructArrayReserve(set_label_data);
	parent->child_names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	added_bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	for(i=0; i<num_labels; i++)
	{
		label = labels[i];
		num_children = label->get_count(label);
		if(label->special != FALSE)
		{
			if(num_children > 0 && strcmp(label->name, "Root") == 0)
			{
				bone = label->get_bone(label, 0);
				if(bone != NULL)
				{
					parent->label = bone->name;
					PointerArrayAppend(added_bones, bone);
				}
			}
			if(parent->label == NULL)
			{
				continue;
			}
		}
		else
		{
			parent->label = label->name;
			PointerArrayAppend(added_bones, bone);
		}

		for(j=0; j<num_children; j++)
		{
			bone = label->get_bone(label, j);
			if(bone != NULL)
			{
				PointerArrayAppend(parent->child_names, bone->name);
				PointerArrayAppend(added_bones, bone);
			}
		}

		if(parent->child_names->num_data > 0)
		{
			parent = (SET_LABEL_DATA*)StructArrayReserve(set_label_data);
			parent->child_names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
		}
	}

	data = (SET_LABEL_DATA*)set_label_data->buffer;
	for(i=0; i<(int)set_label_data->num_data; i++)
	{
		parent = &data[i];
		bone = (BONE_INTERFACE*)added_bones->buffer[num_added_bones];
		num_added_bones++;
		gtk_tree_store_append(tree_store, &toplevel, NULL);
		gtk_tree_store_set(tree_store, &toplevel, 0, parent->label, 1, bone, -1);

		for(j=0; j<(int)parent->child_names->num_data; j++)
		{
			bone = (BONE_INTERFACE*)added_bones->buffer[num_added_bones];
			num_added_bones++;
			gtk_tree_store_append(tree_store, &child, &toplevel);
			gtk_tree_store_set(tree_store, &child, 0,
				parent->child_names->buffer[j], 1, bone, -1);
		}
	}

	for(i=0; i<(int)set_label_data->num_data; i++)
	{
		parent = &data[i];
		PointerArrayDestroy(&parent->child_names, NULL);
	}
	StructArrayDestroy(&set_label_data, NULL);


	MEM_FREE_FUNC(labels);

	if(old_store != NULL)
	{
		g_object_unref(old_store);
	}
}

void* ModelControlWidgetNew(void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene = NULL;
	GtkAdjustment *adjustment;
	GtkWidget *vbox;
	GtkWidget *note_book_box;
	GtkWidget *child_note_book;
	GtkWidget *child_note_book_box;
	GtkWidget *layout_box;
	GtkWidget *frame_box;
	GtkWidget *table;
	GtkWidget *note_book;
	GtkWidget *label;
	GtkWidget *widget;
	GtkWidget *control[4];
	GtkTreeSelection *selection;
	float scalar_value[4];
	gchar *path;
	char str[4096];
	int i;

	if(project != NULL)
	{
		scene = project->scene;
	}

	vbox = gtk_vbox_new(FALSE, 0);

	layout_box = gtk_vbox_new(FALSE, 0);
	path = g_build_filename(application->paths.image_directory_path, "add_model.png", NULL);
	widget = gtk_image_new_from_file(path);
	g_free(path);
	gtk_box_pack_start(GTK_BOX(layout_box), widget, FALSE, FALSE, 0);
	label = gtk_label_new(application->label.control.load_model);
	gtk_box_pack_start(GTK_BOX(layout_box), label, FALSE, FALSE, 0);
	control[0] = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(control[0]), layout_box);
	(void)g_signal_connect_swapped(G_OBJECT(control[0]), "clicked", G_CALLBACK(ExecuteLoadModel), application);
	gtk_box_pack_start(GTK_BOX(vbox), control[0], FALSE, FALSE, 0);

	note_book = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), note_book, FALSE, FALSE, 0);
	note_book_box = gtk_vbox_new(FALSE, 0);
	label = gtk_label_new(application->label.control.model);

	child_note_book = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(note_book_box), child_note_book, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), note_book_box, label);

	// ���f���̑I��
	label = gtk_label_new(application->label.control.model);
	child_note_book_box = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(child_note_book), child_note_book_box, label);
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(child_note_book_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.control_model);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION <= 2
	application->widgets.model_combo_box = control[0] = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(control[0]), application->label.control.no_select);
	if(scene != NULL)
	{
		for(i=0; i<(int)scene->models->num_data; i++)
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(control[0]), ((MODEL_INTERFACE*)scene->models->buffer[i])->name);
			if(scene->models->buffer[i] == (void*)scene->selected_model)
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(control[0]), i+1);
			}
		}
	}
#else
	application->widgets.model_combo_box = control[0] = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX(control[0]), application->label.control.no_select);
	if(scene != NULL)
	{
		for(i=0; i<(int)scene->models->num_data; i++)
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX(control[0]), ((MODEL_INTERFACE*)scene->models->buffer[i])->name);
			if(scene->models->buffer[i] == (void*)scene->selected_model)
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(control[0]), i+1);
			}
		}
	}
#endif
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	(void)g_signal_connect(G_OBJECT(control[0]), "changed",
		G_CALLBACK(OnChangeSelectedModel), application);
	// �g�嗦
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(child_note_book_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.scale);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
	adjustment = NULL;
	if(scene != NULL)
	{
		if(scene->selected_model != NULL)
		{
			adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scene->selected_model->scale_factor * 100.0f,
				0, 10000, 1, 5, 0));
		}
	}
	if(adjustment == NULL)
	{
		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(100.0f, 0, 10000, 1, 5, 0));
	}
	application->widgets.model_scale =  control[0] = gtk_spin_button_new(adjustment, 1, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelScale), application);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	// �s�����x
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(child_note_book_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.opacity);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
	adjustment = NULL;
	if(scene != NULL)
	{
		if(scene->selected_model != NULL)
		{
			adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scene->selected_model->opacity * 100.0f,
				0, 100, 1, 5, 0));
		}
	}
	if(adjustment == NULL)
	{
		adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(100.0f, 0, 100, 1, 5, 0));
	}
	application->widgets.model_opacity =  control[0] = gtk_spin_button_new(adjustment, 1, 1);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelOpacity), application);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	// �ڑ���t���[��
	layout_box = gtk_frame_new(application->label.control.model_connect_to);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	gtk_box_pack_start(GTK_BOX(child_note_book_box), layout_box, FALSE, FALSE, 0);
	// �ڑ��惂�f��
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.model);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
	application->widgets.connect_model = control[0] =
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_new_text();
#else
		gtk_combo_box_text_new();
#endif
	FillParentModelComboBox(control[0], scene, application);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	(void)g_signal_connect(G_OBJECT(control[0]), "changed",
		G_CALLBACK(OnChangeSelectedParentModel), application);
	// �ڑ���{�[��
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.bone);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
	application->widgets.connect_bone = control[0] =
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_new_text();
#else
		gtk_combo_box_text_new();
#endif
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	(void)g_signal_connect(G_OBJECT(control[0]), "changed",
		G_CALLBACK(OnChangeSelectedParentBone), application);
	// �ʒu�Ɖ�]
	table = gtk_table_new(1, 2, TRUE);
	gtk_box_pack_start(GTK_BOX(child_note_book_box), table, FALSE, FALSE, 0);
	// �ʒu
	scalar_value[0] = scalar_value[1] = scalar_value[2] = scalar_value[3] = 0;
	if(scene != NULL)
	{
		if(scene->selected_model != NULL)
		{
			scene->selected_model->get_world_translation(scene->selected_model, scalar_value);
		}
	}
	layout_box = gtk_frame_new(application->label.control.position);
	gtk_table_attach_defaults(GTK_TABLE(table), layout_box, 0, 1, 0, 1);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	// X
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("X:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[0], -10000, 10000, 1, 5, 0));
	application->widgets.model_position[0] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_X));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelPosition), application);
	// Y
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Y:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[1], -10000, 10000, 1, 5, 0));
	application->widgets.model_position[1] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Y));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelPosition), application);
	// Z
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Z:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[2], -10000, 10000, 1, 5, 0));
	application->widgets.model_position[2] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Z));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelPosition), application);
	// ��]
	scalar_value[0] = scalar_value[1] = scalar_value[2] = scalar_value[3] = 0;
	if(scene != NULL)
	{
		if(scene->selected_model != NULL)
		{
			scene->selected_model->get_world_orientation(scene->selected_model, scalar_value);
		}
	}
	layout_box = gtk_frame_new(application->label.control.rotation);
	gtk_table_attach_defaults(GTK_TABLE(table), layout_box, 1, 2, 0, 1);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	// X
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("X:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[0], -180, 180, 1, 5, 0));
	application->widgets.model_rotation[0] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_X));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelRotation), application);
	// Y
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Y:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[1], -180, 180, 1, 5, 0));
	application->widgets.model_rotation[1] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Y));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelRotation), application);
	// Z
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Z:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[2], -180, 180, 1, 5, 0));
	application->widgets.model_rotation[2] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Z));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeModelRotation), application);

	label = gtk_label_new(application->label.control.bone);
	child_note_book_box = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(child_note_book), child_note_book_box, label);
	control[0] = gtk_radio_button_new_with_label(NULL, application->label.control.edit_mode.select);
	control[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(control[0])),
		application->label.control.edit_mode.move);
	control[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(control[0])),
		application->label.control.edit_mode.rotate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[project->control.edit_mode]), TRUE);
	for(i=0; i<NUM_EDIT_MODE; i++)
	{
		ToggleButtonSetValue(control[i], i, (int*)&project->control.edit_mode,
			&project->control, (void (*)(void*, int))ControlSetEditMode, &application->widgets.ui_disabled);
		gtk_box_pack_start(GTK_BOX(child_note_book_box), control[i], FALSE, FALSE, 0);
	}

	control[0] = gtk_scrolled_window_new(NULL, NULL);
	application->widgets.bone_tree_view = BoneTreeViewNew();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(control[0]), application->widgets.bone_tree_view);
	gtk_widget_set_size_request(control[0], 128, 360);
	gtk_box_pack_start(GTK_BOX(child_note_book_box), control[0], FALSE, TRUE, 0);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(application->widgets.bone_tree_view));
	(void)g_signal_connect(G_OBJECT(selection), "changed",
		G_CALLBACK(OnChangedSelectedBone), application);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	label = gtk_label_new(application->label.control.environment);
	note_book_box = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), note_book_box, label);

	control[0] = gtk_check_button_new_with_label(application->label.control.enable_physics);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[0]), project->flags & PROJECT_FLAG_ALWAYS_PHYSICS);
	ToggleButtonSetFlag(control[0], PROJECT_FLAG_ALWAYS_PHYSICS, &project->flags,
		project, (void (*)(void*, int))ProjectSetEnableAlwaysPhysics, &application->widgets.ui_disabled);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, FALSE, 0);

	control[0] = gtk_check_button_new_with_label(application->label.control.display_grid);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[0]),
		project->flags & PROJECT_FLAG_DRAW_GRID);
	ToggleButtonSetFlag(control[0], PROJECT_FLAG_DRAW_GRID, &project->flags,
		NULL, NULL, &application->widgets.ui_disabled);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, FALSE, 0);

	control[0] = gtk_check_button_new_with_label(application->label.control.render_edge_only);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[0]),
		project->flags & PROJECT_FLAG_RENDER_EDGE_ONLY);
	ToggleButtonSetFlag(control[0], PROJECT_FLAG_RENDER_EDGE_ONLY, &project->flags,
		NULL, NULL, &application->widgets.ui_disabled);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, FALSE, 0);

	return (void*)vbox;
}

static void OnChangeCameraPosition(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	eSET_VALUE_TYPE type = (eSET_VALUE_TYPE)g_object_get_data(G_OBJECT(adjustment), "set_type");
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	scene->camera.look_at[type] = value;

	CameraUpdateTransform(&scene->camera);
}

static void OnChangeCameraAngle(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	eSET_VALUE_TYPE type = (eSET_VALUE_TYPE)g_object_get_data(G_OBJECT(adjustment), "set_type");
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	scene->camera.angle[type] = (float)(value * M_PI / 180.0);

	CameraUpdateTransform(&scene->camera);
}

static void OnChangeCameraDistance(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	scene->camera.distance[2] = - value;

	CameraUpdateTransform(&scene->camera);
}

static void OnChangeCameraFieldOfView(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	scene->camera.fov = value;

	CameraUpdateTransform(&scene->camera);
}

static void ResetCameraPositionButtonClicked(GtkWidget* button, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	if(application->num_projects <= 0)
	{
		return;
	}

	ResetCameraDefault(&project->scene->camera);
	SetCameraPositionWidget(application);
}

static void OnChangeLightColor(GtkWidget* button, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	GdkColor color;
	if(application->num_projects <= 0)
	{
		return;
	}

	gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &color);
	project->scene->light.vertex.color[0] = color.red / 256;
	project->scene->light.vertex.color[1] = color.green / 256;
	project->scene->light.vertex.color[2] = color.blue / 256;
}

static void OnChangeLightDirection(GtkAdjustment* adjustment, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene;
	eSET_VALUE_TYPE type = (eSET_VALUE_TYPE)g_object_get_data(G_OBJECT(adjustment), "set_type");
	float value;

	if(application->num_projects <= 0 || application->widgets.ui_disabled != FALSE)
	{
		return;
	}
	scene = project->scene;
	value = (float)gtk_adjustment_get_value(adjustment);
	scene->light.vertex.position[type] = value;

	CameraUpdateTransform(&scene->camera);
}

static void ResetLightButtonClicked(GtkWidget* button, APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	if(application->num_projects <= 0)
	{
		return;
	}

	ResetLightDefault(&project->scene->light);
	SetLightColorDirectionWidget(application);
}

static void SetCameraPositionWidget(APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	if(application->num_projects <= 0)
	{
		return;
	}

	application->widgets.ui_disabled = TRUE;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_look_at[0]),
		project->scene->camera.look_at[0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_look_at[2]),
		project->scene->camera.look_at[1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_look_at[1]),
		project->scene->camera.look_at[2]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_angle[0]),
		project->scene->camera.angle[0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_angle[1]),
		project->scene->camera.angle[1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_angle[2]),
		project->scene->camera.angle[2]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_distance),
		- project->scene->camera.distance[2]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.camera_fov),
		project->scene->camera.fov);
	application->widgets.ui_disabled = FALSE;
}

static void SetLightColorDirectionWidget(APPLICATION* application)
{
	PROJECT *project = application->projects[application->active_project];
	GdkColor color;
	if(application->num_projects <= 0)
	{
		return;
	}

	application->widgets.ui_disabled = TRUE;
	color.red = project->scene->light.vertex.color[0] | (project->scene->light.vertex.color[0] << 8);
	color.green = project->scene->light.vertex.color[1] | (project->scene->light.vertex.color[1] << 8);
	color.blue = project->scene->light.vertex.color[2] | (project->scene->light.vertex.color[2] << 8);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(application->widgets.light_color), &color);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.light_direction[0]), project->scene->light.vertex.position[0]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.light_direction[1]), project->scene->light.vertex.position[1]);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(application->widgets.light_direction[2]), project->scene->light.vertex.position[2]);
	application->widgets.ui_disabled = FALSE;
}

void* CameraLightControlWidgetNew(void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene = NULL;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *note_book;
	GtkWidget *note_book_box;
	GtkWidget *layout_box;
	GtkWidget *frame_box;
	GtkWidget *label;
	GtkWidget *control[4];
	GtkWidget *table;
	GtkAdjustment *adjustment;
	char str[4096];
	float scalar_value[4];

	if(project != NULL)
	{
		scene = project->scene;
	}

	note_book = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), note_book, TRUE, TRUE, 0);

	// �J�����^�u
	label = gtk_label_new(application->label.control.camera);
	note_book_box = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), note_book_box, label);
	// �ʒu�Ɖ�]
	table = gtk_table_new(1, 2, TRUE);
	gtk_box_pack_start(GTK_BOX(note_book_box), table, FALSE, FALSE, 0);
	// �ʒu
	scalar_value[0] = scalar_value[1] = scalar_value[2] = scalar_value[3] = 0;
	if(scene != NULL)
	{
		COPY_VECTOR3(scalar_value, scene->camera.position);
	}
	layout_box = gtk_frame_new(application->label.control.position);
	gtk_table_attach_defaults(GTK_TABLE(table), layout_box, 0, 1, 0, 1);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	// X
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("X:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[0], -10000, 10000, 1, 5, 0));
	application->widgets.camera_look_at[0] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_X));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraPosition), application);
	// Y
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Y:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[1], -10000, 10000, 1, 5, 0));
	application->widgets.camera_look_at[1] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Y));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraPosition), application);
	// Z
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Z:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[2], -10000, 10000, 1, 5, 0));
	application->widgets.camera_look_at[2] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Z));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraPosition), application);
	// ��]
	scalar_value[0] = scalar_value[1] = scalar_value[2] = scalar_value[3] = 0;
	if(scene != NULL)
	{
		COPY_VECTOR4(scalar_value, scene->camera.angle);
	}
	layout_box = gtk_frame_new(application->label.control.rotation);
	gtk_table_attach_defaults(GTK_TABLE(table), layout_box, 1, 2, 0, 1);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	// X
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("X:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[0], -180, 180, 1, 5, 0));
	application->widgets.camera_angle[0] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_X));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraAngle), application);
	// Y
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Y:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[1], -180, 180, 1, 5, 0));
	application->widgets.camera_angle[1] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Y));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraAngle), application);
	// Z
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Z:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[2], -180, 180, 1, 5, 0));
	application->widgets.camera_angle[2] = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Z));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraAngle), application);
	// ����
	scalar_value[0] = scalar_value[1] = scalar_value[2] = scalar_value[3] = 0;
	if(scene != NULL)
	{
		COPY_VECTOR3(scalar_value, scene->camera.distance);
		SET_POSITION(scalar_value, scalar_value);
	}
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(note_book_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.distance);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[2], -10000, 10000, 1, 5, 0));
	application->widgets.camera_distance = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraDistance), application);
	// ����p
	scalar_value[0] = 0;
	if(scene != NULL)
	{
		scalar_value[0] = scene->camera.fov;
	}
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(note_book_box), layout_box, FALSE, FALSE, 0);
	(void)sprintf(str, "%s : ", application->label.control.field_of_view);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new(str), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[0], -10000, 10000, 1, 5, 0));
	application->widgets.camera_fov = control[0] = gtk_spin_button_new(adjustment, 1, 1);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeCameraFieldOfView), application);
	// �J�����̈ʒu��������
	control[0] = gtk_button_new_with_label(application->label.control.reset);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, FALSE, 0);
	(void)g_signal_connect(G_OBJECT(control[0]), "clicked",
		G_CALLBACK(ResetCameraPositionButtonClicked), application);

	// �Ɩ��^�u
	label = gtk_label_new(application->label.control.light);
	note_book_box = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), note_book_box, label);
	// �F�ƌ���
	table = gtk_table_new(1, 2, TRUE);
	gtk_box_pack_start(GTK_BOX(note_book_box), table, FALSE, FALSE, 0);
	// �F
	layout_box = gtk_frame_new(application->label.control.color);
	gtk_table_attach_defaults(GTK_TABLE(table), layout_box, 0, 1, 0, 1);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	application->widgets.light_color = control[0] = gtk_color_button_new();
	gtk_box_pack_start(GTK_BOX(frame_box), control[0], TRUE, TRUE, 2);
	(void)g_signal_connect(G_OBJECT(control[0]), "color-set",
		G_CALLBACK(OnChangeLightColor), application);
	// ����
	scalar_value[0] = scalar_value[1] = scalar_value[2] = scalar_value[3] = 0;
	if(scene != NULL)
	{
		COPY_VECTOR3(scalar_value, scene->light.vertex.position);
	}
	layout_box = gtk_frame_new(application->label.control.position);
	gtk_table_attach_defaults(GTK_TABLE(table), layout_box, 1, 2, 0, 1);
	frame_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(layout_box), frame_box);
	// X
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("X:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[0], -1, 1, 0.01, 0.05, 0));
	application->widgets.light_direction[0] = control[0] = gtk_spin_button_new(adjustment, 0.01, 4);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_X));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeLightDirection), application);
	// Y
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Y:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[1], -1, 1, 0.01, 0.05, 0));
	application->widgets.light_direction[1] = control[0] = gtk_spin_button_new(adjustment, 0.01, 4);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Y));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeLightDirection), application);
	// Z
	layout_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(frame_box), layout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(layout_box), gtk_label_new("Z:"), FALSE, FALSE, 0);
	adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(scalar_value[2], -1, 1, 0.01, 0.05, 0));
	application->widgets.light_direction[2] = control[0] = gtk_spin_button_new(adjustment, 0.01, 4);
	gtk_box_pack_start(GTK_BOX(layout_box), control[0], TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(adjustment), "set_type", GINT_TO_POINTER(SET_VALUE_TYPE_Z));
	(void)g_signal_connect(G_OBJECT(adjustment), "value_changed",
		G_CALLBACK(OnChangeLightDirection), application);

	return (void*)vbox;
}

typedef struct _RENDER_FOR_PIXEL_DATA
{
	uint8 *pixels;
	int width;
	int height;
	PROJECT *project;
	void *user_data;
	void (*finished)(void*, unsigned char*);
} RENDER_FOR_PIXEL_DATA;

gboolean RenderForPixelDataDrawing(
	GtkWidget* widget,
	GdkEventExpose* event_info,
	RENDER_FOR_PIXEL_DATA* data
)
{
	GtkAllocation allocation;
	GdkGLContext *context = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(widget);

	if(gdk_gl_drawable_gl_begin(drawable, context) == FALSE)
	{
		return TRUE;
	}

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(widget, &allocation);
#else
	allocation = widget->allocation;
#endif

	if(allocation.width != data->width || allocation.height != data->height)
	{
		return TRUE;
	}

	RenderEngines(data->project, allocation.width, allocation.height);

	if(gdk_gl_drawable_is_double_buffered(drawable) != FALSE)
	{
		gdk_gl_drawable_swap_buffers(drawable);
	}
	else
	{
		glFlush();
	}

	gdk_gl_drawable_gl_end(drawable);

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, data->width, data->height, GL_RGBA,
		GL_UNSIGNED_BYTE, data->pixels);

	(void)g_signal_handlers_disconnect_by_func(G_OBJECT(widget),
		G_CALLBACK(RenderForPixelDataDrawing), data);
	data->finished(data->user_data, data->pixels);

	MEM_FREE_FUNC(data);

	return FALSE;
}

void RenderForPixelData(
	void* project_context,
	int width,
	int height,
	unsigned char* pixels,
	void (*after_render)(void*, unsigned char*),
	void* user_data
)
{
	RENDER_FOR_PIXEL_DATA *data;
	PROJECT *project = (PROJECT*)project_context;

	data = (RENDER_FOR_PIXEL_DATA*)MEM_ALLOC_FUNC(sizeof(*data));
	data->pixels = pixels;
	data->width = width;
	data->height = height;
	data->project = project;
	data->user_data = user_data;
	data->finished = after_render;

	gtk_widget_set_size_request(project->widgets.drawing_area, width, height);
	(void)g_signal_handlers_disconnect_by_func(
		G_OBJECT(project->widgets.drawing_area), G_CALLBACK(ProjectDisplayEvent), project);
#if GTK_MAJOR_VERSION >= 3
	(void)g_signal_connect(G_OBJECT(project->widgets.drawing_area),
		"draw", G_CALLBACK(RenderForPixelDataDrawing), data);
#else
	(void)g_signal_connect(G_OBJECT(project->widgets.drawing_area),
		"expose-event", G_CALLBACK(RenderForPixelDataDrawing), data);
#endif
}