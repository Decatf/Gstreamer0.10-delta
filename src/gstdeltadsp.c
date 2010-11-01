/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2010 Robert Y<<Decatf@gmail.com>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-delta
 *
 * FIXME:Describe delta here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! delta ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstdeltadsp.h"
#include "delta.h"

GST_DEBUG_CATEGORY_STATIC (gst_delta_debug);
#define GST_CAT_DEFAULT gst_delta_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_GAIN,
  PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
        GST_STATIC_CAPS (
            "audio/x-raw-int, "
            "rate = (int) [ 8000, 96000 ], "
            "channels = (int) { 1, 2 }, "
            "endianness = (int) { 1234, 4321 }, "
            "width = (int) { 8, 16, 32 }, "
            "depth = (int) { 8, 16, 32 }, "
            "signed = (boolean) { true, false }; "

            "audio/x-raw-float, "
            "rate = (int) [ 8000, 96000 ], "
            "channels = (int) 2, "
            "endianness = (int) { 1234, 4321 }, "
            "width = (int) {32, 64} "
            )
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
        GST_STATIC_CAPS (
            "audio/x-raw-int, "
            "rate = (int) [ 8000, 96000 ], "
            "channels = (int) { 1, 2 }, "
            "endianness = (int) { 1234, 4321 }, "
            "width = (int) { 8, 16, 32 }, "
            "depth = (int) { 8, 16, 32 }, "
            "signed = (boolean) { true, false }; "

            "audio/x-raw-float, "
            "rate = (int) [ 8000, 96000 ], "
            "channels = (int) 2, "
            "endianness = (int) { 1234, 4321 }, "
            "width = (int) {32, 64} "
            )
    );

GST_BOILERPLATE (Gstdelta, gst_delta, GstElement,
    GST_TYPE_ELEMENT);

static void gst_delta_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_delta_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_delta_set_caps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_delta_chain (GstPad * pad, GstBuffer * buf);

static void set_bs2b_filter_function (Gstdelta *filter);

/* GObject vmethod implementations */


static void
gst_delta_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "delta",
    "Effect/Audio",
    "Audio noise sharpening",
    "Robert Y<<Decatf@gmail.com>>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the delta's class */
static void
gst_delta_class_init (GstdeltaClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_delta_set_property;
  gobject_class->get_property = gst_delta_get_property;

  g_object_class_install_property (gobject_class, PROP_GAIN,
      g_param_spec_int ("gain", "Gain", "Delta gain to apply",
          0, 200, 100, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_delta_init (Gstdelta * filter,
    GstdeltaClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_setcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_delta_set_caps));
  gst_pad_set_getcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_delta_chain));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_getcaps_function (filter->srcpad,
                                GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->gain = 1.15;
  filter->silent = TRUE;
}

static void
gst_delta_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstdelta *filter = GST_DELTA_DSP (object);

  switch (prop_id) {
    case PROP_GAIN:
      filter->gain = (gfloat)(g_value_get_int (value)/100.f);
      break;
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_delta_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstdelta *filter = GST_DELTA_DSP (object);

  switch (prop_id) {
    case PROP_GAIN:
      g_value_set_int (value, (gint)(filter->gain*100));
      break;
    case PROP_SILENT:
      g_value_set_boolean (value, (gboolean)filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean
gst_delta_set_caps1 (GstPad * pad, GstCaps * caps)
{
  Gstdelta *filter;
  GstPad *otherpad;

  filter = GST_DELTA_DSP (gst_pad_get_parent (pad));
  otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;
  gst_object_unref (filter);

  return gst_pad_set_caps (otherpad, caps);
}

static gboolean
gst_delta_set_caps (GstPad  *pad, GstCaps *caps)
{

  Gstdelta *filter;
  GstPad *otherpad;

  filter = GST_DELTA_DSP (gst_pad_get_parent (pad));
  otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;

  GstStructure *structure = gst_caps_get_structure (caps, 0);
  const gchar *mime;

  /* Since we're an audio filter, we want to handle raw audio
   * and from that audio type, we need to get the samplerate and
   * number of channels. */
  mime = gst_structure_get_name (structure);
  if (strcmp (mime, "audio/x-raw-int") != 0 &&
        strcmp (mime, "audio/x-raw-float") != 0) {
    GST_WARNING ("Wrong mimetype %s provided, we only support %s",
		 mime, "audio/x-raw-int");
    return FALSE;
  }

  /* we're a filter and don't touch the properties of the data.
   * That means we can set the given caps unmodified on the next
   * element, and use that negotiation return value as ours. */
  if (!gst_pad_set_caps (otherpad, caps))
    return FALSE;

  /* Capsnego succeeded, get the stream properties for internal
   * usage and return success. */

  if (strcmp (mime, "audio/x-raw-int") == 0)
    filter->is_int = TRUE;
  else if (strcmp (mime, "audio/x-raw-float") == 0)
    filter->is_int = FALSE;

  gst_structure_get_int (structure, "channels", &filter->channels);
  filter->little_endian = g_value_get_int(gst_structure_get_value (structure, "endianness")) == 1234;
  gst_structure_get_boolean (structure, "signed", &filter->sign);
  gst_structure_get_int (structure, "width", &filter->width);
  gst_structure_get_int (structure, "depth", &filter->depth);

  //g_print ("Caps negotiation succeeded with %d is_int @ %d channels, endianness %d, width %d, depth %d, sign %d\n",
	   //filter->is_int, filter->channels, filter->little_endian, filter->width, filter->depth, filter->sign);

  set_bs2b_filter_function (filter);

  gst_object_unref (filter);

  return TRUE;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_delta_chain (GstPad * pad, GstBuffer * buf)
{
  Gstdelta *filter;

  filter = GST_DELTA_DSP (GST_OBJECT_PARENT (pad));

  if (filter->silent == FALSE)
    g_print ("[Delta] GStreamer Noise Sharpening. gain=%f\n", filter->gain);

  filter->process (GST_BUFFER_DATA (buf), (gint)GST_BUFFER_SIZE (buf), filter->channels, filter->gain);

  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, buf);
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
delta_init (GstPlugin * delta)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template delta' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_delta_debug, "delta",
      0, "Noise sharpening audio dsp");

  return gst_element_register (delta, "delta", GST_RANK_NONE,
      GST_TYPE_DELTA_DSP);
}

static void set_bs2b_filter_function (Gstdelta *filter) {

  if (filter->is_int && filter->little_endian) {
    if (filter->width == 8) {
      if (filter->sign)
        filter->process = (void*)process8;
      else
        filter->process = (void*)process8u;
    } else if (filter->width == 16) {
      if (filter->sign)
        filter->process = (void*)process16;
      else
        filter->process = (void*)process16u;
    } else if (filter->width == 32) {
      if (filter->sign)
        filter->process = (void*)process32;
      else
        filter->process = (void*)process32u;
    } else if (filter->width == 64) {
      if (filter->sign)
        filter->process = (void*)process64;
      else
        filter->process = (void*)process64u;
    }
  } else {
    filter->process = (void*)processf;
  }
}


/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "delta"
#endif

/* gstreamer looks for this structure to register deltas
 *
 * exchange the string 'Template delta' with your delta description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "delta",
    "Noise sharpening audio dsp",
    delta_init,
    VERSION,
    "GPL",
    "gstreamer0.10-delta",
    "http://gstreamer.net/"
)
