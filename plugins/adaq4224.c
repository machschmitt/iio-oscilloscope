/**
 * Copyright (C) 2023 Analog Devices, Inc.
 *
 * Licensed under the GPL-2.
 *
 **/
// based on ad9739a.c
#include <stdio.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gtkdatabox.h>
#include <gtkdatabox_grid.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_lines.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <iio.h>

#include "../libini2.h"
#include "../osc.h"
#include "../iio_widget.h"
#include "../osc_plugin.h"
#include "../config.h"
//#include "dac_data_manager.h"

#define THIS_DRIVER "adaq4224"
#define IIO_CHANNEL0 "voltage0"

#define SYNC_RELOAD "SYNC_RELOAD"

#define SCALE_AVAIL_BUF_SIZE 128

#define ARRAY_SIZE(x) (!sizeof(x) ?: sizeof(x) / sizeof((x)[0]))

//static const char *DAC_DEVICE;

//#define LPC_DAC_DEVICE "axi-ad9739a-lpc"
//#define HPC_DAC_DEVICE "axi-ad9739a-hpc"
#define IIO_DEVICE "adaq4224"

//static struct dac_data_manager *dac_tx_manager;

static struct iio_context *ctx;
static struct iio_device *dev;

static struct iio_widget tx_widgets[20];
static unsigned int num_tx;

static bool can_update_widgets;

//static const char **adaq4224_sr_attribs;
//static int sr_attribs_array_size;

static char scale_avail_buf[SCALE_AVAIL_BUF_SIZE];
static char *scale_available[4];

static GtkWidget *pgia_write;
static GtkWidget *combobox_debug_scanel;
static GtkWidget *combobox_attr_type;
static GtkWidget *scanel_options;

static struct iio_channel *ch;

static const char *adaq4224_sr_attribs[] = {
	IIO_DEVICE".in_voltage_scale",
	IIO_DEVICE".in_voltage_scale_available",
	//IIO_DEVICE".operation_mode",
};

//static const char * adaq4224_driver_attribs[] = {
//	"dds_mode",
//	"tx_channel_0",
//	"dac_buf_filename",
//};

static int compare_gain(const char *a, const char *b) __attribute__((unused));
static int compare_gain(const char *a, const char *b)
{
	double val_a, val_b;
	sscanf(a, "%lf", &val_a);
	sscanf(b, "%lf", &val_b);

	if (val_a < val_b)
		return -1;
	else if(val_a > val_b)
		return 1;
	else
		return 0;
}

static void update_widgets(void)
{
	iio_update_widgets_of_device(tx_widgets, num_tx, dev);
}

static void reload_button_clicked(GtkButton *btn, gpointer data)
{
	update_widgets();
	//dac_data_manager_update_iio_widgets(dac_tx_manager);
}
static void save_widget_value(GtkWidget *widget, struct iio_widget *iio_w)
{
	iio_w->save(iio_w);
}

static void make_widget_update_signal_based(struct iio_widget *widgets,
	unsigned int num_widgets)
{
	//NULL was somewhere here
	char signal_name[25];
	unsigned int i;

	for (i = 0; i < num_widgets; i++) {
		if (GTK_IS_CHECK_BUTTON(widgets[i].widget))
			sprintf(signal_name, "%s", "toggled");
		else if (GTK_IS_TOGGLE_BUTTON(widgets[i].widget))
			sprintf(signal_name, "%s", "toggled");
		else if (GTK_IS_SPIN_BUTTON(widgets[i].widget))
			sprintf(signal_name, "%s", "value-changed");
		else if (GTK_IS_COMBO_BOX_TEXT(widgets[i].widget))
			sprintf(signal_name, "%s", "changed");
		else
			printf("unhandled widget type, attribute: %s\n", widgets[i].attr_name);

		if (GTK_IS_SPIN_BUTTON(widgets[i].widget) &&
			widgets[i].priv_progress != NULL) {
				iio_spin_button_progress_activate(&widgets[i]);
		} else {
			g_signal_connect(G_OBJECT(widgets[i].widget), signal_name, G_CALLBACK(save_widget_value), &widgets[i]);
		}
	}
}

static int adaq4224_handle_driver(struct osc_plugin *plugin, const char *attrib, const char *value)
{
	printf("adaq4224_handle_driver ....\n");
	//if (MATCH_ATTRIB("dds_mode")) {
	//	dac_data_manager_set_dds_mode(dac_tx_manager,
	//			DAC_DEVICE, 1, atoi(value));
	//} else if (!strncmp(attrib, "tx_channel_", sizeof("tx_channel_") - 1)) {
	//	int tx = atoi(attrib + sizeof("tx_channel_") - 1);
	//	dac_data_manager_set_tx_channel_state(
	//			dac_tx_manager, tx, !!atoi(value));
	//} else if (MATCH_ATTRIB("dac_buf_filename")) {
	//	if (dac_data_manager_get_dds_mode(dac_tx_manager,
	//				DAC_DEVICE, 1) == DDS_BUFFER)
	//		dac_data_manager_set_buffer_chooser_filename(
	//				dac_tx_manager, value);
	//} else if (MATCH_ATTRIB("SYNC_RELOAD")) {
	//	if (can_update_widgets) {
	//		reload_button_clicked(NULL, NULL);
	//	}
	//} else {
	//	return -EINVAL;
	//}

	return 0;
}

static int adaq4224_handle(struct osc_plugin *plugin, int line, const char *attrib, const char *value)
{
	printf("adaq4224_handle \n");
	return osc_plugin_default_handle(ctx, line, attrib, value,
			adaq4224_handle_driver, NULL);
}

static void load_profile(struct osc_plugin *plugin, const char *ini_fn)
{
	//unsigned i;

	//for (i = 0; i < ARRAY_SIZE(adaq4224_driver_attribs); i++) {
	//	char *value = read_token_from_ini(ini_fn, THIS_DRIVER,
	//			adaq4224_driver_attribs[i]);
	//	if (value) {
	//		adaq4224_handle_driver(NULL, adaq4224_driver_attribs[i], value);
	//		free(value);
	//	}
	//}

	printf("load_profile \n");
	//TODO fix
	//update_from_ini(ini_fn, THIS_DRIVER, dev, adaq4224_sr_attribs,
	//		ARRAY_SIZE(adaq4224_sr_attribs));

	//if (can_update_widgets)
	//	reload_button_clicked(NULL, NULL);
	reload_button_clicked(NULL, NULL);
}

static void scanel_write_clicked(GtkButton *btn, gpointer data)
{
	char *attr_name;

	printf("Clicked!\n");
	attr_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox_attr_type));

	if (strcmp(attr_name, "0.33") == 0) {
		iio_channel_attr_write(ch, "scale", scale_available[0]);
	} else if (strcmp(attr_name, "0.56") == 0) {
		iio_channel_attr_write(ch, "scale", scale_available[1]);
	} else if (strcmp(attr_name, "2.22") == 0) {
		iio_channel_attr_write(ch, "scale", scale_available[2]);
	} else if (strcmp(attr_name, "6.67") == 0) {
		iio_channel_attr_write(ch, "scale", scale_available[3]);
	}

	printf("Clicked! %s\n", attr_name);

	//iio_channel_attr_write(ch, "scale", "0.001323223");

	g_free(attr_name);

	//scanel_read_clicked(GTK_BUTTON(scanel_read), data);
}

static GtkWidget * adaq4224_init(struct osc_plugin *plugin, GtkWidget *notebook, const char *ini_fn)
{
	GtkBuilder *builder;
	GtkWidget *adaq4224_panel;
	//struct iio_channel *ch;
	char *buf;
	int i;
	int len;

	ctx = osc_create_context();
	if (!ctx)
		return NULL;

	dev = iio_context_find_device(ctx, IIO_DEVICE);
	ch = iio_device_find_channel(dev, IIO_CHANNEL0, false);
	if (!ch) {
		printf("Error: Could not find %s channel\n", IIO_CHANNEL0);
		goto init_abort;
	}

	// Debug/test prints {
	printf("Found channel attr count %d\n", iio_channel_get_attrs_count(ch));
	printf("scale_available: %s\n", iio_channel_find_attr(ch, "scale_available"));
	len = iio_channel_attr_read(ch, "scale_available", scale_avail_buf,
				SCALE_AVAIL_BUF_SIZE);
	printf("scale_available len: %d\n", len);
	for (i = 0; i < len; i++) {
		printf("%c", scale_avail_buf[i]);
	}
	printf("\n");
	//scale_available[0] = strtok(scale_avail_buf, " ");
	buf = strtok(scale_avail_buf, " ");
	i = 0;
	while (buf != NULL) {
		scale_available[i++] = buf;
		buf = strtok(NULL, " ");
	}
	printf("tokend \n");
	// } Debug/test prints

	builder = gtk_builder_new();
	//nbook = GTK_NOTEBOOK(notebook);

	if (osc_load_glade_file(builder, "adaq4224") < 0) {
		osc_destroy_context(ctx);
		return NULL;
	}

	adaq4224_panel = GTK_WIDGET(gtk_builder_get_object(builder, "adaq4224_panel"));
	//dds_container = GTK_WIDGET(gtk_builder_get_object(builder, "dds_transmit_block"));
	//gtk_container_add(GTK_CONTAINER(dds_container), dac_data_manager_get_gui_container(dac_tx_manager));
	//gtk_widget_show_all(dds_container);

	/* Bind the IIO device files to the GUI widgets */
	printf("Bind the IIO device files ....\n");

	//iio_combo_box_init_from_builder(&tx_widgets[num_tx++],
	//	dev, NULL, "operation_mode", "operation_modes_available",
	//	 builder, "operation_modes_combo", NULL);

	//iio_combo_box_init_from_builder(&tx_widgets[num_tx++],
	//	dev, NULL, "in_voltage_scale", "in_voltage_scale_available",
	//	 builder, "pgia_gain_combo", NULL);
	//printf("combo inited \n");

	combobox_attr_type = GTK_WIDGET(gtk_builder_get_object(builder, "pgia_gain_combo"));
	printf("combo inited \n");


	//Write button
	pgia_write = GTK_WIDGET(gtk_builder_get_object(builder, "pgia_gain_write"));
	g_signal_connect(G_OBJECT(pgia_write), "clicked",
			G_CALLBACK(scanel_write_clicked), NULL);


	//scanel_options = GTK_WIDGET(gtk_builder_get_object(builder, "debug_scanel_options"));

	if (ini_fn)
		load_profile(NULL, ini_fn);
	printf("profile loaded \n");

	/* Update all widgets with current values */
	update_widgets();
	printf("updated widgets \n");

	//make_widget_update_signal_based(tx_widgets, num_tx);
	//g_builder_connect_signal(builder, "adaq4224_settings_reload", "clicked",
	//	G_CALLBACK(reload_button_clicked), NULL);

	//dac_data_manager_freq_widgets_range_update(dac_tx_manager, 2E15 / 2);

	//dac_data_manager_update_iio_widgets(dac_tx_manager);

	//dac_data_manager_set_buffer_chooser_current_folder(dac_tx_manager, OSC_WAVEFORM_FILE_PATH);

	can_update_widgets = true;

	return adaq4224_panel;

init_abort:
	osc_destroy_context(ctx);

	return NULL;
}

static void save_widgets_to_ini(FILE *f)
{
	printf("save_widgets_to_ini \n");
	//fprintf(f, "dds_mode = %i\n"
	//		"dac_buf_filename = %s\n"
	//		"tx_channel_0 = %i\n",
	//		dac_data_manager_get_dds_mode(dac_tx_manager, DAC_DEVICE, 1),
	//		dac_data_manager_get_buffer_chooser_filename(dac_tx_manager),
	//		dac_data_manager_get_tx_channel_state(dac_tx_manager, 0));
}

static void save_profile(const struct osc_plugin *plugin, const char *ini_fn)
{
	printf("save_profile \n");
	FILE *f = fopen(ini_fn, "a");
	if (f) {
		/* Write the section header */
		save_to_ini(f, THIS_DRIVER, dev, adaq4224_sr_attribs,
				ARRAY_SIZE(adaq4224_sr_attribs));
		//save_widgets_to_ini(f);
		fclose(f);
	}
}

static void context_destroy(struct osc_plugin *plugin, const char *ini_fn)
{
	save_profile(NULL, ini_fn);
	//dac_data_manager_free(dac_tx_manager);
	//dac_tx_manager = NULL;
	osc_destroy_context(ctx);
}

static bool adaq4224_identify(const struct osc_plugin *plugin)
{

	/* Use the OSC's IIO context just to detect the devices */
	struct iio_context *osc_ctx = get_context_from_osc();

	return !!iio_context_find_device(osc_ctx, IIO_DEVICE);
}

//GSList* get_dac_dev_names(const struct osc_plugin *plugin) {
//	GSList *list = NULL;
//
//	list = g_slist_append (list, (gpointer) DAC_DEVICE);
//
//	return list;
//}

struct osc_plugin plugin = {
	.name = THIS_DRIVER,
	.identify = adaq4224_identify,
	.init = adaq4224_init,
	.handle_item = adaq4224_handle,
	.save_profile = save_profile,
	.load_profile = load_profile,
	.destroy = context_destroy,
	//.get_dac_dev_names = get_dac_dev_names,
};
